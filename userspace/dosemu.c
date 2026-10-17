/* Lot 7 (US-086): 8086 interpreter for DOS .COM programs. See dosemu.h. */
#include "dosemu.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

enum { AX, CX, DX, BX, SP, BP, SI, DI };

static u8 M[65536];
static u16 R[8];
static u16 IP;
static int CF, ZF, SF, OF, DF;
static char* g_out;
static int g_cap, g_n;
static int g_stop, g_code;
static dos_result_t* g_res;

static u16 rw(u16 a) { return (u16)(M[a] | (M[(u16)(a + 1U)] << 8)); }
static void ww(u16 a, u16 v) { M[a] = (u8)v; M[(u16)(a + 1U)] = (u8)(v >> 8); }
static u8 f8(void) { return M[IP++]; }
static u16 f16(void) { u16 v = rw(IP); IP = (u16)(IP + 2U); return v; }
static u8 get8(int r) { return r < 4 ? (u8)(R[r] & 0xFFU) : (u8)(R[r - 4] >> 8); }
static void set8(int r, u8 v) {
    if (r < 4) R[r] = (u16)((R[r] & 0xFF00U) | v);
    else R[r - 4] = (u16)((R[r - 4] & 0x00FFU) | ((u16)v << 8));
}
static void push(u16 v) { R[SP] = (u16)(R[SP] - 2U); ww(R[SP], v); }
static u16 pop(void) { u16 v = rw(R[SP]); R[SP] = (u16)(R[SP] + 2U); return v; }

static void out_c(char c) {
    if (c == '\r') return;                    /* the shell console wants \n only */
    if (g_n < g_cap - 1) { g_out[g_n++] = c; g_out[g_n] = 0; }
}

static void stop(int status, int op) {
    if (g_stop) return;
    g_stop = 1;
    g_res->status = status;
    g_res->fault_op = op;
}

/* ModRM */
typedef struct { int mod, reg, rm; u16 ea; } modrm_t;
static modrm_t decode(void) {
    modrm_t m;
    u8 b = f8();
    m.mod = b >> 6; m.reg = (b >> 3) & 7; m.rm = b & 7; m.ea = 0;
    if (m.mod == 3) return m;
    switch (m.rm) {
    case 0: m.ea = (u16)(R[BX] + R[SI]); break;
    case 1: m.ea = (u16)(R[BX] + R[DI]); break;
    case 2: m.ea = (u16)(R[BP] + R[SI]); break;
    case 3: m.ea = (u16)(R[BP] + R[DI]); break;
    case 4: m.ea = R[SI]; break;
    case 5: m.ea = R[DI]; break;
    case 6: m.ea = m.mod == 0 ? f16() : R[BP]; break;
    default: m.ea = R[BX]; break;
    }
    if (m.mod == 1) m.ea = (u16)(m.ea + (u16)(signed char)f8());
    else if (m.mod == 2) m.ea = (u16)(m.ea + f16());
    return m;
}
static u32 rd(const modrm_t* m, int w) {
    if (m->mod == 3) return w ? R[m->rm] : get8(m->rm);
    return w ? rw(m->ea) : M[m->ea];
}
static void wr(const modrm_t* m, int w, u32 v) {
    if (m->mod == 3) { if (w) R[m->rm] = (u16)v; else set8(m->rm, (u8)v); return; }
    if (w) ww(m->ea, (u16)v); else M[m->ea] = (u8)v;
}
static u32 rreg(int r, int w) { return w ? R[r] : get8(r); }
static void wreg(int r, int w, u32 v) { if (w) R[r] = (u16)v; else set8(r, (u8)v); }

static void szf(u32 v, int w) {
    u32 mask = w ? 0xFFFFU : 0xFFU;
    ZF = (v & mask) == 0;
    SF = (int)((v >> (w ? 15 : 7)) & 1U);
}

/* op: 0 ADD 1 OR 2 ADC 3 SBB 4 AND 5 SUB 6 XOR 7 CMP */
static u32 alu(int op, u32 a, u32 b, int w) {
    u32 mask = w ? 0xFFFFU : 0xFFU, sign = w ? 0x8000U : 0x80U, r, c;
    a &= mask; b &= mask;
    switch (op) {
    case 0: case 2:
        c = op == 2 ? (u32)CF : 0U;
        r = a + b + c;
        CF = r > mask;
        OF = ((a ^ r) & (b ^ r) & sign) != 0;
        break;
    case 3: case 5: case 7:
        c = op == 3 ? (u32)CF : 0U;
        r = a - b - c;
        CF = a < b + c;
        OF = ((a ^ b) & (a ^ r) & sign) != 0;
        break;
    case 1: r = a | b; CF = OF = 0; break;
    case 4: r = a & b; CF = OF = 0; break;
    default: r = a ^ b; CF = OF = 0; break;
    }
    r &= mask;
    szf(r, w);
    return op == 7 ? a : r;
}

static u32 incdec(u32 v, int dec, int w) {
    u32 mask = w ? 0xFFFFU : 0xFFU, sign = w ? 0x8000U : 0x80U, r;
    r = (dec ? v - 1U : v + 1U) & mask;
    OF = dec ? (v & mask) == sign : r == sign;
    szf(r, w);
    return r;
}

static u32 shift(int kind, u32 v, int cnt, int w) {
    u32 mask = w ? 0xFFFFU : 0xFFU, sign = w ? 0x8000U : 0x80U, orig = v & mask;
    int i, nc;
    v &= mask;
    if (cnt == 0) return v;
    for (i = 0; i < cnt; i++) {
        switch (kind) {
        case 0: CF = (v & sign) != 0; v = ((v << 1) | (u32)CF) & mask; break;           /* ROL */
        case 1: CF = (int)(v & 1U); v = (v >> 1) | (CF ? sign : 0U); break;              /* ROR */
        case 2: nc = (v & sign) != 0; v = ((v << 1) | (u32)CF) & mask; CF = nc; break;   /* RCL */
        case 3: nc = (int)(v & 1U); v = (v >> 1) | (CF ? sign : 0U); CF = nc; break;     /* RCR */
        case 4: case 6: CF = (v & sign) != 0; v = (v << 1) & mask; break;                /* SHL */
        case 5: CF = (int)(v & 1U); v >>= 1; break;                                      /* SHR */
        default: CF = (int)(v & 1U); v = (v >> 1) | (v & sign); break;                   /* SAR */
        }
    }
    if (kind == 0 || kind == 2 || kind == 4 || kind == 6) OF = (((v & sign) != 0) ^ CF);
    else if (kind == 5) OF = (orig & sign) != 0;
    else if (kind == 7) OF = 0;
    else OF = (((v >> (w ? 15 : 7)) ^ (v >> (w ? 14 : 6))) & 1U) != 0;
    if (kind >= 4) szf(v, w);
    return v;
}

static int cond(int c) {
    switch (c) {
    case 0x0: return OF;
    case 0x1: return !OF;
    case 0x2: return CF;
    case 0x3: return !CF;
    case 0x4: return ZF;
    case 0x5: return !ZF;
    case 0x6: return CF || ZF;
    case 0x7: return !CF && !ZF;
    case 0x8: return SF;
    case 0x9: return !SF;
    case 0xC: return SF != OF;
    case 0xD: return SF == OF;
    case 0xE: return ZF || SF != OF;
    case 0xF: return !ZF && SF == OF;
    default: stop(DOS_ERR_OPCODE, 0x70 + c); return 0;   /* JP/JNP: PF not computed */
    }
}

static void interrupt(int n) {
    u8 ah = get8(4);
    g_res->ints++;
    if (n == 0x20) { g_code = 0; stop(DOS_EXITED, 0); return; }
    if (n == 0x21) {
        if (ah == 0x02) { out_c((char)get8(2)); set8(0, get8(2)); return; }
        if (ah == 0x09) {
            u16 a = R[DX];
            int k;
            for (k = 0; k < 65536 && M[a] != '$'; k++, a++) out_c((char)M[a]);
            set8(0, '$');
            return;
        }
        if (ah == 0x30) { R[AX] = 0x0005U; R[BX] = 0; R[CX] = 0; return; }
        if (ah == 0x4C) { g_code = get8(0); stop(DOS_EXITED, 0); return; }
        if (ah == 0x00) { g_code = 0; stop(DOS_EXITED, 0); return; }
        stop(DOS_ERR_INT, (n << 8) | ah);
        return;
    }
    if (n == 0x10 && ah == 0x0E) { out_c((char)get8(0)); return; }
    stop(DOS_ERR_INT, (n << 8) | ah);
}

static void group3(int w) {
    modrm_t m = decode();
    u32 v = rd(&m, w), mask = w ? 0xFFFFU : 0xFFU, sign = w ? 0x8000U : 0x80U;
    switch (m.reg) {
    case 0: case 1: (void)alu(4, v, w ? f16() : f8(), w); break;           /* TEST imm */
    case 2: wr(&m, w, ~v & mask); break;                                    /* NOT */
    case 3: {                                                               /* NEG */
        u32 r = (0U - v) & mask;
        CF = v != 0; OF = v == sign;
        szf(r, w); wr(&m, w, r);
        break;
    }
    case 4:                                                                 /* MUL */
        if (!w) { u32 r = get8(0) * v; R[AX] = (u16)r; CF = OF = (r >> 8) != 0; }
        else { u32 r = R[AX] * v; R[AX] = (u16)r; R[DX] = (u16)(r >> 16); CF = OF = R[DX] != 0; }
        break;
    case 5:                                                                 /* IMUL */
        if (!w) { int r = (int)(signed char)get8(0) * (int)(signed char)v; R[AX] = (u16)r; CF = OF = r < -128 || r > 127; }
        else {
            int r = (int)(short)R[AX] * (int)(short)v;
            R[AX] = (u16)r; R[DX] = (u16)((u32)r >> 16);
            CF = OF = r < -32768 || r > 32767;
        }
        break;
    case 6:                                                                 /* DIV */
        if (v == 0) { stop(DOS_ERR_DIV, 0xF6); return; }
        if (!w) {
            u32 q = R[AX] / v;
            if (q > 0xFFU) { stop(DOS_ERR_DIV, 0xF6); return; }
            set8(4, (u8)(R[AX] % v)); set8(0, (u8)q);
        } else {
            u32 d = ((u32)R[DX] << 16) | R[AX], q = d / v;
            if (q > 0xFFFFU) { stop(DOS_ERR_DIV, 0xF7); return; }
            R[DX] = (u16)(d % v); R[AX] = (u16)q;
        }
        break;
    default:                                                                /* IDIV */
        if (v == 0) { stop(DOS_ERR_DIV, 0xF6); return; }
        if (!w) {
            int d = (short)R[AX], s = (signed char)v, q = d / s;
            if (q < -128 || q > 127) { stop(DOS_ERR_DIV, 0xF6); return; }
            set8(4, (u8)(d % s)); set8(0, (u8)q);
        } else {
            int d = (int)(((u32)R[DX] << 16) | R[AX]), s = (short)v, q;
            if (d == (int)0x80000000U && s == -1) { stop(DOS_ERR_DIV, 0xF7); return; }
            q = d / s;
            if (q < -32768 || q > 32767) { stop(DOS_ERR_DIV, 0xF7); return; }
            R[DX] = (u16)(d % s); R[AX] = (u16)q;
        }
        break;
    }
}

/* one string instruction iteration; returns 1 when it compares */
static int string_op(u8 op) {
    int w = op & 1, d = DF ? -1 : 1, step = w ? 2 : 1;
    switch (op & 0xFE) {
    case 0xA4:
        if (w) ww(R[DI], rw(R[SI])); else M[R[DI]] = M[R[SI]];
        R[SI] = (u16)(R[SI] + d * step); R[DI] = (u16)(R[DI] + d * step);
        return 0;
    case 0xA6:
        (void)alu(7, w ? rw(R[SI]) : M[R[SI]], w ? rw(R[DI]) : M[R[DI]], w);
        R[SI] = (u16)(R[SI] + d * step); R[DI] = (u16)(R[DI] + d * step);
        return 1;
    case 0xAA:
        if (w) ww(R[DI], R[AX]); else M[R[DI]] = get8(0);
        R[DI] = (u16)(R[DI] + d * step);
        return 0;
    case 0xAC:
        if (w) R[AX] = rw(R[SI]); else set8(0, M[R[SI]]);
        R[SI] = (u16)(R[SI] + d * step);
        return 0;
    default:                                                                /* SCAS */
        (void)alu(7, w ? R[AX] : get8(0), w ? rw(R[DI]) : M[R[DI]], w);
        R[DI] = (u16)(R[DI] + d * step);
        return 1;
    }
}

static void step(unsigned int max_steps) {
    int rep = 0, w;
    u16 at = IP;
    u8 op;
    for (;;) {
        op = f8();
        if (op == 0x26 || op == 0x2E || op == 0x36 || op == 0x3E) continue;   /* one segment */
        if (op == 0xF2 || op == 0xF3) { rep = op; continue; }
        break;
    }
    g_res->fault_ip = at;
    if (op < 0x40 && (op & 7) < 6) {
        int a = op >> 3;
        w = op & 1;
        switch (op & 7) {
        case 0: case 1: { modrm_t m = decode(); u32 r = alu(a, rd(&m, w), rreg(m.reg, w), w); if (a != 7) wr(&m, w, r); break; }
        case 2: case 3: { modrm_t m = decode(); u32 r = alu(a, rreg(m.reg, w), rd(&m, w), w); if (a != 7) wreg(m.reg, w, r); break; }
        case 4: { u32 r = alu(a, get8(0), f8(), 0); if (a != 7) set8(0, (u8)r); break; }
        default: { u32 r = alu(a, R[AX], f16(), 1); if (a != 7) R[AX] = (u16)r; break; }
        }
        return;
    }
    if (op >= 0x40 && op <= 0x4F) { R[op & 7] = (u16)incdec(R[op & 7], op >= 0x48, 1); return; }
    if (op >= 0x50 && op <= 0x57) { u16 v = R[op & 7]; push(op == 0x54 ? (u16)(v - 2U) : v); return; }
    if (op >= 0x58 && op <= 0x5F) { R[op & 7] = pop(); return; }
    if (op >= 0x70 && op <= 0x7F) { signed char d = (signed char)f8(); if (cond(op & 0xF)) IP = (u16)(IP + d); return; }
    if (op >= 0x91 && op <= 0x97) { u16 t = R[AX]; R[AX] = R[op & 7]; R[op & 7] = t; return; }
    if (op >= 0xB0 && op <= 0xB7) { set8(op & 7, f8()); return; }
    if (op >= 0xB8 && op <= 0xBF) { R[op & 7] = f16(); return; }
    if (op >= 0xA4 && op <= 0xAF && op != 0xA8 && op != 0xA9) {
        if (!rep) { (void)string_op(op); return; }
        while (R[CX] != 0 && !g_stop) {
            int cmp = string_op(op);
            R[CX]--;
            if (++g_res->steps >= max_steps) { stop(DOS_ERR_STEPS, op); return; }
            if (cmp && ((rep == 0xF3 && !ZF) || (rep == 0xF2 && ZF))) break;
        }
        return;
    }
    switch (op) {
    case 0x06: case 0x0E: case 0x16: case 0x1E: push(0); return;            /* PUSH seg */
    case 0x07: case 0x17: case 0x1F: (void)pop(); return;                   /* POP seg */
    case 0x80: case 0x81: case 0x83: {
        modrm_t m = decode();
        u32 imm;
        w = op != 0x80;
        imm = op == 0x81 ? f16() : op == 0x83 ? (u32)(u16)(signed char)f8() : f8();
        imm = alu(m.reg, rd(&m, w), imm, w);
        if (m.reg != 7) wr(&m, w, imm);
        return;
    }
    case 0x84: case 0x85: { modrm_t m = decode(); w = op & 1; (void)alu(4, rd(&m, w), rreg(m.reg, w), w); return; }
    case 0x86: case 0x87: {
        modrm_t m = decode(); u32 t;
        w = op & 1;
        t = rd(&m, w); wr(&m, w, rreg(m.reg, w)); wreg(m.reg, w, t);
        return;
    }
    case 0x88: case 0x89: { modrm_t m = decode(); w = op & 1; wr(&m, w, rreg(m.reg, w)); return; }
    case 0x8A: case 0x8B: { modrm_t m = decode(); w = op & 1; wreg(m.reg, w, rd(&m, w)); return; }
    case 0x8C: { modrm_t m = decode(); wr(&m, 1, 0); return; }               /* MOV r/m, seg: 0 */
    case 0x8E: { modrm_t m = decode(); (void)rd(&m, 1); return; }           /* MOV seg: ignored */
    case 0x8D: { modrm_t m = decode(); if (m.mod == 3) { stop(DOS_ERR_OPCODE, op); return; } R[m.reg] = m.ea; return; }
    case 0x8F: { modrm_t m = decode(); wr(&m, 1, pop()); return; }
    case 0x90: return;
    case 0x98: R[AX] = (u16)(signed char)get8(0); return;
    case 0x99: R[DX] = (R[AX] & 0x8000U) ? 0xFFFFU : 0U; return;
    case 0x9C: push((u16)(0xF002U | (u16)CF | (u16)(ZF << 6) | (u16)(SF << 7) | (u16)(DF << 10) | (u16)(OF << 11))); return;
    case 0x9D: { u16 f = pop(); CF = f & 1; ZF = (f >> 6) & 1; SF = (f >> 7) & 1; DF = (f >> 10) & 1; OF = (f >> 11) & 1; return; }
    case 0xA0: set8(0, M[f16()]); return;
    case 0xA1: R[AX] = rw(f16()); return;
    case 0xA2: M[f16()] = get8(0); return;
    case 0xA3: ww(f16(), R[AX]); return;
    case 0xA8: (void)alu(4, get8(0), f8(), 0); return;
    case 0xA9: (void)alu(4, R[AX], f16(), 1); return;
    case 0xC2: { u16 n = f16(); IP = pop(); R[SP] = (u16)(R[SP] + n); return; }
    case 0xC3: IP = pop(); return;
    case 0xC6: { modrm_t m = decode(); wr(&m, 0, f8()); return; }
    case 0xC7: { modrm_t m = decode(); wr(&m, 1, f16()); return; }
    case 0xCD: interrupt(f8()); return;
    case 0xD0: case 0xD1: case 0xD2: case 0xD3: {
        modrm_t m = decode();
        int cnt = op >= 0xD2 ? (get8(1) & 0x1F) : 1;
        w = op & 1;
        wr(&m, w, shift(m.reg, rd(&m, w), cnt, w));
        return;
    }
    case 0xE0: case 0xE1: case 0xE2: {
        signed char d = (signed char)f8();
        R[CX]--;
        if (R[CX] != 0 && (op == 0xE2 || (op == 0xE1 && ZF) || (op == 0xE0 && !ZF))) IP = (u16)(IP + d);
        return;
    }
    case 0xE3: { signed char d = (signed char)f8(); if (R[CX] == 0) IP = (u16)(IP + d); return; }
    case 0xE8: { u16 d = f16(); push(IP); IP = (u16)(IP + d); return; }
    case 0xE9: { u16 d = f16(); IP = (u16)(IP + d); return; }
    case 0xEB: { signed char d = (signed char)f8(); IP = (u16)(IP + d); return; }
    case 0xF5: CF = !CF; return;
    case 0xF6: group3(0); return;
    case 0xF7: group3(1); return;
    case 0xF8: CF = 0; return;
    case 0xF9: CF = 1; return;
    case 0xFA: case 0xFB: return;                                           /* CLI/STI: no IRQ here */
    case 0xFC: DF = 0; return;
    case 0xFD: DF = 1; return;
    case 0xFE: {
        modrm_t m = decode();
        if (m.reg > 1) { stop(DOS_ERR_OPCODE, op); return; }
        wr(&m, 0, incdec(rd(&m, 0), m.reg, 0));
        return;
    }
    case 0xFF: {
        modrm_t m = decode();
        switch (m.reg) {
        case 0: case 1: wr(&m, 1, incdec(rd(&m, 1), m.reg, 1)); return;
        case 2: { u16 t = (u16)rd(&m, 1); push(IP); IP = t; return; }
        case 4: IP = (u16)rd(&m, 1); return;
        case 6: push((u16)rd(&m, 1)); return;
        default: stop(DOS_ERR_OPCODE, op); return;
        }
    }
    default:
        stop(DOS_ERR_OPCODE, op);                /* 286+ opcodes, far calls, I/O, x87 */
        return;
    }
}

int dos_run_com(const unsigned char* image, int len, char* out, int cap, unsigned int max_steps,
                dos_result_t* res) {
    int i;
    res->status = DOS_EXITED; res->exit_code = 0; res->steps = 0; res->ints = 0;
    res->fault_ip = 0; res->fault_op = 0; res->out_len = 0;
    g_out = out; g_cap = cap; g_n = 0; g_stop = 0; g_code = 0; g_res = res;
    if (cap > 0) out[0] = 0;
    if (!image || len <= 0 || len > 0xFE00) { res->status = DOS_ERR_SIZE; return res->status; }
    for (i = 0; i < 65536; i++) M[i] = 0;
    M[0] = 0xCD; M[1] = 0x20;                    /* PSP: INT 20h, so RET exits */
    M[0x80] = 0; M[0x81] = 0x0D;                 /* empty command tail */
    for (i = 0; i < len; i++) M[0x100 + i] = image[i];
    for (i = 0; i < 8; i++) R[i] = 0;
    R[SP] = 0xFFFEU;
    IP = 0x100U;
    CF = ZF = SF = OF = DF = 0;
    while (!g_stop) {
        if (res->steps >= max_steps) { stop(DOS_ERR_STEPS, M[IP]); break; }
        res->steps++;
        step(max_steps);
    }
    res->exit_code = g_code;
    res->out_len = g_n;
    return res->status;
}

const char* dos_status_name(int status) {
    switch (status) {
    case DOS_EXITED: return "exited";
    case DOS_ERR_OPCODE: return "unsupported_opcode";
    case DOS_ERR_INT: return "unsupported_service";
    case DOS_ERR_STEPS: return "step_limit";
    case DOS_ERR_DIV: return "divide_error";
    default: return "bad_image";
    }
}
