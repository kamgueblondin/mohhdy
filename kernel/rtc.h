#ifndef MOHHDY_RTC_H
#define MOHHDY_RTC_H

#include <stdint.h>

#define RTC_CMOS_INDEX_PORT 0x70U
#define RTC_CMOS_DATA_PORT 0x71U
#define RTC_UTC_TEXT_LENGTH 15U
#define RTC_UTC_BUFFER_LENGTH 16U

typedef uint8_t (*rtc_inb_fn)(void* context,uint16_t port);
typedef void (*rtc_outb_fn)(void* context,uint16_t port,uint8_t value);
typedef struct { void* context; rtc_inb_fn inb; rtc_outb_fn outb; } rtc_io_t;

/* Prépare les accès ports CMOS réels sur i386 ; indisponible dans les tests hôte. */
int rtc_i386_io(rtc_io_t* io);
/* Lit une photographie CMOS UTC stable et écrit YYYYMMDDHHMMSSZ + NUL dans un buffer caller-owned. */
int rtc_read_utc(const rtc_io_t* io,char* output,uint16_t output_capacity);

#endif
