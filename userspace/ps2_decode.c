/* userspace/ps2_decode.c - see ps2_decode.h. Same US QWERTY tables and
 * rules as the Ring 0 path of kernel/keyboard.c (which stays the early
 * boot, stall and after-death fallback). */
#include "ps2_decode.h"

static const char base_map[128] = {
    /*00-0F*/  0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', '\b','\t',
    /*10-1F*/ 'q','w','e','r','t','y','u','i','o','p','[',']','\n',  0, 'a','s',
    /*20-2F*/ 'd','f','g','h','j','k','l',';','\'', '`',  0, '\\','z','x','c','v',
    /*30-3F*/ 'b','n','m',',','.','/',  0,  '*',  0, ' ',  0,   0,   0,   0,   0,   0,
    /*40-4F*/  0,  0,  0,  0,  0,  0,  0,  '7','8','9','-','4','5','6','+','1',
    /*50-5F*/ '2','3','0','.', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*60-6F*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*70-7F*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const char shift_map[128] = {
    /*00-0F*/  0,   0,  '!','@','#','$','%','^','&','*','(',')','_','+', '\b','\t',
    /*10-1F*/ 'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',  0, 'A','S',
    /*20-2F*/ 'D','F','G','H','J','K','L',':','"','~',  0,  '|', 'Z','X','C','V',
    /*30-3F*/ 'B','N','M','<','>','?',  0,  '*',  0, ' ',  0,   0,   0,   0,   0,   0,
    /*40-4F*/  0,  0,  0,  0,  0,  0,  0,  '7','8','9','-','4','5','6','+','1',
    /*50-5F*/ '2','3','0','.', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*60-6F*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    /*70-7F*/  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

void ps2_decoder_init(ps2_decoder_t* d) {
    uint32_t i;
    uint8_t* p = (uint8_t*)d;
    if (!d) return;
    for (i = 0; i < sizeof(*d); i++) p[i] = 0;
}

char ps2_map(const ps2_decoder_t* d, uint8_t sc) {
    char ch;
    if (!d || sc >= 128) return 0;
    ch = d->shift ? shift_map[sc] : base_map[sc];
    /* Letters follow Shift XOR Caps Lock; symbols stay on the shift map. */
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
        int upper = (d->shift ? 1 : 0) ^ (d->caps ? 1 : 0);
        if (upper && ch >= 'a' && ch <= 'z') return (char)(ch - 'a' + 'A');
        if (!upper && ch >= 'A' && ch <= 'Z') return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static int emit(os_input_event_t* out, uint32_t* n, uint32_t cap, uint8_t type, uint8_t code, int dx, int dy) {
    if (!out || !n || *n >= cap) return 0;
    out[*n].type = type;
    out[*n].code = code;
    out[*n].dx = (int8_t)dx;
    out[*n].dy = (int8_t)dy;
    (*n)++;
    return 1;
}

static int emit_mods(const ps2_decoder_t* d, os_input_event_t* out, uint32_t* n, uint32_t cap) {
    return emit(out, n, cap, OS_INPUT_EV_MODS,
                (uint8_t)((d->shift ? OS_INPUT_MOD_SHIFT : 0U) | (d->caps ? OS_INPUT_MOD_CAPS : 0U)), 0, 0);
}

void ps2_decoder_seed(ps2_decoder_t* d, uint32_t mods) {
    if (!d) return;
    d->shift = (mods & OS_INPUT_MOD_SHIFT) ? 1U : 0U;
    d->caps = (mods & OS_INPUT_MOD_CAPS) ? 1U : 0U;
}

int ps2_decode_byte(ps2_decoder_t* d, uint8_t b, int aux, os_input_event_t* out, uint32_t* n, uint32_t cap) {
    if (!d) return 0;
    d->bytes++;
    if (aux) {
        if (d->mouse_cycle == 0) {
            if ((b & 0x08U) == 0U) { d->mouse_dropped++; return 0; }   /* resync */
            d->mouse[0] = b;
            d->mouse_cycle = 1;
            return 0;
        }
        if (d->mouse_cycle == 1) { d->mouse[1] = b; d->mouse_cycle = 2; return 0; }
        d->mouse[2] = b;
        d->mouse_cycle = 0;
        d->mouse_packets++;
        return emit(out, n, cap, OS_INPUT_EV_MOUSE, (uint8_t)(d->mouse[0] & 0x07U),
                    (int)(int8_t)d->mouse[1], (int)(int8_t)d->mouse[2]);
    }
    if (b == 0xFAU || b == 0xFEU) { d->ignored++; return 0; }        /* ACK / resend */
    if (b == 0xE0U) { d->e0 = 1; return 0; }
    if (d->e0) {
        d->e0 = 0;
        if (b & 0x80U) return 0;                                        /* E0 break */
        return emit(out, n, cap, OS_INPUT_EV_EXT, b, 0, 0);
    }
    /* Lot 6: modifier changes are reported, so the kernel always holds the
     * live Shift/Caps state (Ring 0 fallback and respawned driver seed). */
    if (b == 0x2AU || b == 0x36U) { if (d->shift) return 0; d->shift = 1; return emit_mods(d, out, n, cap); }
    if (b == 0xAAU || b == 0xB6U) { if (!d->shift) return 0; d->shift = 0; return emit_mods(d, out, n, cap); }
    if (b == 0x3AU) { d->caps = (uint8_t)!d->caps; return emit_mods(d, out, n, cap); }
    if (b & 0x80U) return 0;                                            /* other breaks */
    if (b == 0x01U) return emit(out, n, cap, OS_INPUT_EV_ESC, 0, 0, 0);
    {
        char c = ps2_map(d, b);
        if (!c) { d->ignored++; return 0; }
        return emit(out, n, cap, OS_INPUT_EV_CHAR, (uint8_t)c, 0, 0);
    }
}
