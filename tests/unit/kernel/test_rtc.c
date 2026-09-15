#include "../../framework/unity.h"
#include "../../../kernel/rtc.h"

typedef struct { uint8_t registers[128]; uint8_t selected; } fake_rtc_t;
void setUp(void) {}
void tearDown(void) {}
static uint8_t fake_inb(void* context,uint16_t port){fake_rtc_t* fake=(fake_rtc_t*)context;return port==RTC_CMOS_DATA_PORT?fake->registers[fake->selected]:0U;}
static void fake_outb(void* context,uint16_t port,uint8_t value){fake_rtc_t* fake=(fake_rtc_t*)context;if(port==RTC_CMOS_INDEX_PORT)fake->selected=(uint8_t)(value&0x7fU);}

void test_rtc_reads_bcd_12_hour_utc(void){fake_rtc_t fake={0};rtc_io_t io={&fake,fake_inb,fake_outb};char utc[RTC_UTC_BUFFER_LENGTH]={0};fake.registers[0x00U]=0x56U;fake.registers[0x02U]=0x34U;fake.registers[0x04U]=0x89U;fake.registers[0x07U]=0x18U;fake.registers[0x08U]=0x08U;fake.registers[0x09U]=0x26U;fake.registers[0x0aU]=0U;fake.registers[0x0bU]=0U;TEST_ASSERT_EQUAL(0,rtc_read_utc(&io,utc,sizeof(utc)));TEST_ASSERT_EQUAL_MEMORY("20260818213456Z",utc,RTC_UTC_TEXT_LENGTH);TEST_ASSERT_EQUAL(0,utc[15]);}
void test_rtc_reads_binary_24_hour_leap_day(void){fake_rtc_t fake={0};rtc_io_t io={&fake,fake_inb,fake_outb};char utc[RTC_UTC_BUFFER_LENGTH]={0};fake.registers[0x00U]=1U;fake.registers[0x02U]=2U;fake.registers[0x04U]=3U;fake.registers[0x07U]=29U;fake.registers[0x08U]=2U;fake.registers[0x09U]=24U;fake.registers[0x0aU]=0U;fake.registers[0x0bU]=0x06U;TEST_ASSERT_EQUAL(0,rtc_read_utc(&io,utc,sizeof(utc)));TEST_ASSERT_EQUAL_MEMORY("20240229030201Z",utc,RTC_UTC_TEXT_LENGTH);}
void test_rtc_rejects_invalid_or_unstable_source(void){fake_rtc_t fake={0};rtc_io_t io={&fake,fake_inb,fake_outb};char utc[RTC_UTC_BUFFER_LENGTH]={0};fake.registers[0x00U]=0x6aU;fake.registers[0x02U]=0U;fake.registers[0x04U]=0x01U;fake.registers[0x07U]=0x01U;fake.registers[0x08U]=0x01U;fake.registers[0x09U]=0x26U;fake.registers[0x0aU]=0U;fake.registers[0x0bU]=0U;TEST_ASSERT_NOT_EQUAL(0,rtc_read_utc(&io,utc,sizeof(utc)));TEST_ASSERT_NOT_EQUAL(0,rtc_read_utc(&io,utc,RTC_UTC_TEXT_LENGTH));fake.registers[0x0aU]=0x80U;TEST_ASSERT_NOT_EQUAL(0,rtc_read_utc(&io,utc,sizeof(utc)));TEST_ASSERT_NOT_EQUAL(0,rtc_i386_io(0));}

int main(void){unity_init();RUN_TEST(test_rtc_reads_bcd_12_hour_utc);RUN_TEST(test_rtc_reads_binary_24_hour_leap_day);RUN_TEST(test_rtc_rejects_invalid_or_unstable_source);unity_print_results();unity_cleanup();return unity_stats.tests_failed==0?0:1;}
