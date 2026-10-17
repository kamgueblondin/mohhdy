#include "rtc.h"

#define RTC_REGISTER_SECONDS 0x00U
#define RTC_REGISTER_MINUTES 0x02U
#define RTC_REGISTER_HOURS 0x04U
#define RTC_REGISTER_DAY 0x07U
#define RTC_REGISTER_MONTH 0x08U
#define RTC_REGISTER_YEAR 0x09U
#define RTC_REGISTER_STATUS_A 0x0aU
#define RTC_REGISTER_STATUS_B 0x0bU
#define RTC_STATUS_A_UPDATE_IN_PROGRESS 0x80U
#define RTC_STATUS_B_24_HOUR 0x02U
#define RTC_STATUS_B_BINARY 0x04U

static uint8_t rtc_read_register(const rtc_io_t* io,uint8_t reg){io->outb(io->context,RTC_CMOS_INDEX_PORT,(uint8_t)(reg&0x7fU));return io->inb(io->context,RTC_CMOS_DATA_PORT);}
static int rtc_bcd(uint8_t value,uint8_t* output){if(!output||(value&0x0fU)>9U||((value>>4)&0x0fU)>9U)return -1;*output=(uint8_t)(((value>>4)*10U)+(value&0x0fU));return 0;}
static uint8_t rtc_leap(uint16_t year){return (uint8_t)((year%4U==0U&&(year%100U!=0U||year%400U==0U))?1U:0U);}
static uint8_t rtc_days_in_month(uint16_t year,uint8_t month){static const uint8_t days[12]={31U,28U,31U,30U,31U,30U,31U,31U,30U,31U,30U,31U};if(month<1U||month>12U)return 0U;return (uint8_t)(month==2U?days[1]+rtc_leap(year):days[month-1U]);}
static void rtc_two_digits(char* output,uint16_t offset,uint8_t value){output[offset]=(char)('0'+value/10U);output[offset+1U]=(char)('0'+value%10U);}

#ifdef __i386__
static uint8_t rtc_i386_inb(void* context,uint16_t port){uint8_t value;(void)context;__asm__ volatile("inb %1,%0":"=a"(value):"Nd"(port));return value;}
static void rtc_i386_outb(void* context,uint16_t port,uint8_t value){(void)context;__asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port));}
#endif

int rtc_i386_io(rtc_io_t* io){
    if(!io)return -1;
    io->context=(void*)0;
#ifdef __i386__
    io->inb=rtc_i386_inb;io->outb=rtc_i386_outb;return 0;
#else
    io->inb=(rtc_inb_fn)0;io->outb=(rtc_outb_fn)0;return -1;
#endif
}

/* A CMOS snapshot is only trusted when two full reads, each started with
 * UIP clear, agree byte for byte. The old reader gave up after 3 back-to-back
 * attempts: under QEMU TCG every read inside the update window (about 0.8 ms
 * per second measured, 5052 failures in 6e6 reads) failed, so a TLS poll that
 * needed the time failed at random (OS_LLM_TLS_FAILED, the NE2000 TLS flake).
 * The wait is bounded: a stuck UIP or a register file that never settles
 * still fails (-5). */
#define RTC_UIP_SPIN_LIMIT 200000U
#define RTC_STABLE_ATTEMPTS 16U
typedef struct { uint8_t second, minute, hour, day, month, year, status_b; } rtc_snapshot_t;
static int rtc_wait_uip_clear(const rtc_io_t* io){uint32_t spin;for(spin=0U;spin<RTC_UIP_SPIN_LIMIT;spin++)if((rtc_read_register(io,RTC_REGISTER_STATUS_A)&RTC_STATUS_A_UPDATE_IN_PROGRESS)==0U)return 0;return -1;}
static void rtc_snapshot(const rtc_io_t* io,rtc_snapshot_t* snap){snap->second=rtc_read_register(io,RTC_REGISTER_SECONDS);snap->minute=rtc_read_register(io,RTC_REGISTER_MINUTES);snap->hour=rtc_read_register(io,RTC_REGISTER_HOURS);snap->day=rtc_read_register(io,RTC_REGISTER_DAY);snap->month=rtc_read_register(io,RTC_REGISTER_MONTH);snap->year=rtc_read_register(io,RTC_REGISTER_YEAR);snap->status_b=rtc_read_register(io,RTC_REGISTER_STATUS_B);}
static int rtc_snapshot_equal(const rtc_snapshot_t* a,const rtc_snapshot_t* b){return a->second==b->second&&a->minute==b->minute&&a->hour==b->hour&&a->day==b->day&&a->month==b->month&&a->year==b->year&&a->status_b==b->status_b;}
int rtc_read_stable_attempts;
int rtc_read_utc(const rtc_io_t* io,char* output,uint16_t output_capacity){
    rtc_snapshot_t first,second;uint8_t attempt,second_v,minute,hour,day,month,year,pm;uint16_t full_year;
    if(!io||!io->inb||!io->outb||!output||output_capacity<RTC_UTC_BUFFER_LENGTH)return -1;
    for(attempt=0U;attempt<RTC_STABLE_ATTEMPTS;attempt++){
        if(rtc_wait_uip_clear(io)!=0)return -5;
        rtc_snapshot(io,&first);
        if(rtc_wait_uip_clear(io)!=0)return -5;
        rtc_snapshot(io,&second);
        if(rtc_snapshot_equal(&first,&second))break;
    }
    rtc_read_stable_attempts=(int)attempt+1;
    if(attempt==RTC_STABLE_ATTEMPTS)return -5;
    second_v=first.second;minute=first.minute;hour=first.hour;day=first.day;month=first.month;year=first.year;
    pm=(uint8_t)(hour&0x80U);hour=(uint8_t)(hour&0x7fU);
    if((first.status_b&RTC_STATUS_B_BINARY)==0U){if(rtc_bcd(second_v,&second_v)!=0||rtc_bcd(minute,&minute)!=0||rtc_bcd(hour,&hour)!=0||rtc_bcd(day,&day)!=0||rtc_bcd(month,&month)!=0||rtc_bcd(year,&year)!=0)return -2;}
    if((first.status_b&RTC_STATUS_B_24_HOUR)==0U){if(hour<1U||hour>12U)return -3;if(pm){if(hour!=12U)hour=(uint8_t)(hour+12U);}else if(hour==12U)hour=0U;}
    full_year=(uint16_t)(2000U+year);
    if(second_v>59U||minute>59U||hour>23U||month<1U||month>12U||day<1U||day>rtc_days_in_month(full_year,month))return -4;
    output[0]=(char)('0'+(full_year/1000U)%10U);output[1]=(char)('0'+(full_year/100U)%10U);output[2]=(char)('0'+(full_year/10U)%10U);output[3]=(char)('0'+full_year%10U);
    rtc_two_digits(output,4U,month);rtc_two_digits(output,6U,day);rtc_two_digits(output,8U,hour);rtc_two_digits(output,10U,minute);rtc_two_digits(output,12U,second_v);
    output[14]='Z';output[15]='\0';return 0;
}
