#ifndef MOHHDY_LFN_UTF8_H
#define MOHHDY_LFN_UTF8_H

#include <stdint.h>

/* Conversion LFN bornée UTF-8/UTF-16LE, y compris paires de surrogates. */
static int lfn_utf8_to_utf16_bmp(const char* input, uint16_t* units,
                                 uint32_t capacity, uint32_t* unit_count) {
    uint32_t in = 0U, out = 0U;
    uint32_t max_bytes;
    if (!input || !units || !unit_count || capacity == 0U || capacity > 255U) return -1;
    max_bytes = capacity * 4U;
    while (in < max_bytes && input[in] != '\0') {
        uint8_t a = (uint8_t)input[in++];
        uint32_t codepoint;
        if (a < 0x80U) {
            codepoint = a;
        } else if (a >= 0xC2U && a <= 0xDFU) {
            uint8_t b;
            if (in >= max_bytes || input[in] == '\0') return -1;
            b = (uint8_t)input[in++];
            if ((b & 0xC0U) != 0x80U) return -1;
            codepoint = ((uint32_t)(a & 0x1FU) << 6U) | (uint32_t)(b & 0x3FU);
        } else if (a >= 0xE0U && a <= 0xEFU) {
            uint8_t b, c;
            if (in + 1U >= max_bytes || input[in] == '\0' || input[in + 1U] == '\0') return -1;
            b = (uint8_t)input[in++]; c = (uint8_t)input[in++];
            if ((b & 0xC0U) != 0x80U || (c & 0xC0U) != 0x80U ||
                (a == 0xE0U && b < 0xA0U) || (a == 0xEDU && b >= 0xA0U)) return -1;
            codepoint = ((uint32_t)(a & 0x0FU) << 12U) |
                        ((uint32_t)(b & 0x3FU) << 6U) | (uint32_t)(c & 0x3FU);
        } else if (a >= 0xF0U && a <= 0xF4U) {
            uint8_t b, c, d;
            if (in + 2U >= max_bytes || input[in] == '\0' || input[in + 1U] == '\0' || input[in + 2U] == '\0') return -1;
            b = (uint8_t)input[in++]; c = (uint8_t)input[in++]; d = (uint8_t)input[in++];
            if ((b & 0xC0U) != 0x80U || (c & 0xC0U) != 0x80U || (d & 0xC0U) != 0x80U ||
                (a == 0xF0U && b < 0x90U) || (a == 0xF4U && b > 0x8FU)) return -1;
            codepoint = ((uint32_t)(a & 0x07U) << 18U) | ((uint32_t)(b & 0x3FU) << 12U) |
                        ((uint32_t)(c & 0x3FU) << 6U) | (uint32_t)(d & 0x3FU);
        } else return -1;
        if (codepoint < 0x20U || codepoint == '/' || codepoint == '\\' || codepoint == 0x7FU) return -1;
        if (codepoint >= 0x10000U) {
            uint32_t value = codepoint - 0x10000U;
            if (out + 1U >= capacity) return -1;
            units[out++] = (uint16_t)(0xD800U | (value >> 10U));
            units[out++] = (uint16_t)(0xDC00U | (value & 0x3FFU));
        } else {
            if (out >= capacity) return -1;
            units[out++] = (uint16_t)codepoint;
        }
    }
    if (in == max_bytes && input[in] != '\0') return -1;
    if (out == 0U) return -1;
    *unit_count = out;
    return 0;
}

static int lfn_utf16_bmp_to_utf8(const uint16_t* units, uint32_t unit_count,
                                  char* output, uint32_t capacity) {
    uint32_t in, out = 0U;
    if (!units || !output || capacity == 0U) return -1;
    for (in = 0U; in < unit_count && units[in] != 0U && units[in] != 0xFFFFU; in++) {
        uint16_t unit = units[in];
        if (unit >= 0xD800U && unit <= 0xDBFFU) {
            uint16_t low;
            uint32_t codepoint;
            if (in + 1U >= unit_count || units[in + 1U] < 0xDC00U || units[in + 1U] > 0xDFFFU) return -1;
            low = units[++in];
            codepoint = 0x10000U + (((uint32_t)(unit - 0xD800U) << 10U) | (uint32_t)(low - 0xDC00U));
            if (out + 4U >= capacity) return -1;
            output[out++] = (char)(0xF0U | (codepoint >> 18U));
            output[out++] = (char)(0x80U | ((codepoint >> 12U) & 0x3FU));
            output[out++] = (char)(0x80U | ((codepoint >> 6U) & 0x3FU));
            output[out++] = (char)(0x80U | (codepoint & 0x3FU));
        } else if (unit >= 0xDC00U && unit <= 0xDFFFU) return -1;
        else if (unit < 0x80U) {
            if (out + 1U >= capacity) return -1;
            output[out++] = (char)unit;
        } else if (unit < 0x800U) {
            if (out + 2U >= capacity) return -1;
            output[out++] = (char)(0xC0U | (unit >> 6U));
            output[out++] = (char)(0x80U | (unit & 0x3FU));
        } else {
            if (out + 3U >= capacity) return -1;
            output[out++] = (char)(0xE0U | (unit >> 12U));
            output[out++] = (char)(0x80U | ((unit >> 6U) & 0x3FU));
            output[out++] = (char)(0x80U | (unit & 0x3FU));
        }
    }
    output[out] = '\0';
    return (int)out;
}

#endif
