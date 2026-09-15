#include "../../framework/unity.h"
#include "../../../kernel/ne2k.h"
#include "../../../kernel/net_socket.h"

typedef struct {
    uint8_t reset;
    uint8_t isr;
    uint8_t writes;
    uint8_t dcr;
    uint8_t prom[12];
    uint8_t prom_index;
    uint8_t tx_data[NE2K_ETHERNET_MAX_FRAME];
    uint16_t tx_count;
    uint16_t trace_ports[128];
    uint8_t trace_values[128];
    uint16_t trace_count;
    uint8_t command;
    uint8_t boundary;
    uint8_t current_page;
} fake_ne2k_t;
typedef struct { uint8_t registers[128]; uint8_t selected; } fake_rtc_t;

void setUp(void) {}
void tearDown(void) {}

static uint8_t fake_inb(void* context, uint16_t port) {
    fake_ne2k_t* fake = (fake_ne2k_t*)context;
    if ((port & 0x1fU) == NE2K_REG_RESET) return fake->reset;
    if ((port & 0x1fU) == NE2K_REG_ISR)
        return (fake->command & 0xc0U) == NE2K_COMMAND_PAGE1 ? fake->current_page : fake->isr;
    if ((port & 0x1fU) == NE2K_REG_BNRY) return fake->boundary;
    if ((port & 0x1fU) == NE2K_REG_DCR) return fake->dcr;
    if ((port & 0x1fU) == NE2K_REG_DATA && fake->prom_index < 12U)
        return fake->prom[fake->prom_index++];
    return 0U;
}

static uint8_t fake_rtc_inb(void* context,uint16_t port){fake_rtc_t* fake=(fake_rtc_t*)context;return port==RTC_CMOS_DATA_PORT?fake->registers[fake->selected]:0U;}
static void fake_rtc_outb(void* context,uint16_t port,uint8_t value){fake_rtc_t* fake=(fake_rtc_t*)context;if(port==RTC_CMOS_INDEX_PORT)fake->selected=(uint8_t)(value&0x7fU);}

static void fake_outb(void* context, uint16_t port, uint8_t value) {
    fake_ne2k_t* fake = (fake_ne2k_t*)context;
    if (fake->trace_count < 128U) {
        fake->trace_ports[fake->trace_count] = port;
        fake->trace_values[fake->trace_count++] = value;
    }
    fake->writes++;
    if ((port & 0x1fU) == NE2K_REG_COMMAND) fake->command = value;
    if ((port & 0x1fU) == NE2K_REG_RESET) fake->reset = value;
    if ((port & 0x1fU) == NE2K_REG_BNRY && (fake->command & 0xc0U) == NE2K_COMMAND_PAGE0)
        fake->boundary = value;
    if ((port & 0x1fU) == NE2K_REG_CURR && (fake->command & 0xc0U) == NE2K_COMMAND_PAGE1)
        fake->current_page = value;
    if ((port & 0x1fU) == NE2K_REG_DCR) fake->dcr = value;
    if ((port & 0x1fU) == NE2K_REG_DATA && fake->tx_count < NE2K_ETHERNET_MAX_FRAME)
        fake->tx_data[fake->tx_count++] = value;
}

void test_probe_and_prepare_use_injected_io(void) {
    fake_ne2k_t fake = {0x12, NE2K_ISR_RESET, 0, 0};
    ne2k_io_t io = {&fake, fake_inb, fake_outb};
    ne2k_device_t device;
    TEST_ASSERT_EQUAL(0, ne2k_probe(&device, 0x300, &io));
    TEST_ASSERT_EQUAL(0, ne2k_prepare(&device, &io));
    TEST_ASSERT_EQUAL(0, ne2k_configure_rings(&device, &io));
    TEST_ASSERT_EQUAL(1, device.initialized);
    TEST_ASSERT_GREATER_THAN(8, fake.writes);
    { uint8_t mac[6] = {0x02, 0, 0, 0, 0, 1}; TEST_ASSERT_EQUAL(0, ne2k_set_mac(&device, mac)); TEST_ASSERT_EQUAL(0x02, device.mac[0]); }
    { uint8_t zero[6] = {0, 0, 0, 0, 0, 0}; TEST_ASSERT_NOT_EQUAL(0, ne2k_set_mac(&device, zero)); }
    { uint8_t multicast[6] = {0x01, 0, 0, 0, 0, 1}; TEST_ASSERT_NOT_EQUAL(0, ne2k_set_mac(&device, multicast)); }
    fake.prom[0] = 0x02; fake.prom[1] = 0xaa; fake.prom[2] = 0x10;
    fake.prom[3] = 0xbb; fake.prom[4] = 0x20; fake.prom[5] = 0xcc;
    fake.prom[6] = 0x30; fake.prom[7] = 0xdd; fake.prom[8] = 0x40;
    fake.prom[9] = 0xee; fake.prom[10] = 0x50; fake.prom[11] = 0xff;
    fake.prom_index = 0U;
    TEST_ASSERT_EQUAL(0, ne2k_read_mac(&device, &io));
    TEST_ASSERT_EQUAL(1, device.mac_valid); TEST_ASSERT_EQUAL(0x02, device.mac[0]);
    TEST_ASSERT_EQUAL(0x10, device.mac[1]); TEST_ASSERT_EQUAL(0x20, device.mac[2]);
    TEST_ASSERT_EQUAL(0x30, device.mac[3]); TEST_ASSERT_EQUAL(0x40, device.mac[4]);
    TEST_ASSERT_EQUAL(0x50, device.mac[5]);
    { uint8_t frame[10] = {1,2,3,4,5,6,7,8,9,10};
      fake.isr = (uint8_t)(NE2K_ISR_RESET | NE2K_ISR_RDC); fake.tx_count = 0U;
      TEST_ASSERT_EQUAL(0, ne2k_tx_submit(&device, &io, frame, sizeof(frame)));
      TEST_ASSERT_EQUAL(NE2K_ETHERNET_MIN_FRAME, fake.tx_count);
      TEST_ASSERT_EQUAL(1, fake.tx_data[0]); TEST_ASSERT_EQUAL(10, fake.tx_data[9]);
      TEST_ASSERT_EQUAL(0, fake.tx_data[59]);
      TEST_ASSERT_NOT_EQUAL(0, ne2k_tx_submit(&device, &io, frame, NE2K_ETHERNET_MAX_FRAME + 1U)); }
    { uint8_t frame[128] = {0}; uint8_t destination_mac[6] = {0x52, 0x54, 0, 0, 0, 2};
      uint8_t source_ip[4] = {10, 0, 2, 15}; uint8_t destination_ip[4] = {10, 0, 2, 2};
      uint8_t payload[3] = {1, 2, 3}; fake.isr = NE2K_ISR_RDC;
      TEST_ASSERT_EQUAL(0, ne2k_tx_udp(&device, &io, frame, sizeof(frame), destination_mac,
                                       source_ip, destination_ip, 4000, 4001, payload, sizeof(payload)));
      TEST_ASSERT_EQUAL(0x52, frame[0]); TEST_ASSERT_EQUAL(0x08, frame[12]);
      TEST_ASSERT_EQUAL(0x00, frame[13]); }
    { uint8_t frame[300] = {0}; fake.isr = NE2K_ISR_RDC;
      TEST_ASSERT_EQUAL(0, ne2k_dhcp_discover(&device, &io, frame, sizeof(frame), 0x12345678U));
      TEST_ASSERT_EQUAL(0xff, frame[0]); TEST_ASSERT_EQUAL(0x08, frame[12]);
      TEST_ASSERT_EQUAL(0x63, frame[42U + 236U]); TEST_ASSERT_EQUAL(0x82, frame[43U + 236U]); }
    fake.isr = NE2K_ISR_RDC; TEST_ASSERT_EQUAL(0, ne2k_irq_attach(&device, &io));
    TEST_ASSERT_EQUAL(0, ne2k_irq_count()); ne2k_irq_service();
    TEST_ASSERT_EQUAL(1, ne2k_irq_count());
    { uint8_t rx[64] = {0}; uint16_t rx_len = 99U; fake.isr = 0U;
      TEST_ASSERT_EQUAL(1, ne2k_rx_poll(&device, &io, rx, sizeof(rx), &rx_len));
      TEST_ASSERT_EQUAL(0, rx_len); }
    { uint8_t rx_frame[128] = {0}; uint8_t tx_frame[128] = {0};
      uint8_t local_mac[6] = {0x02, 0, 0, 0, 0, 1};
      uint8_t local_ip[4] = {10, 0, 2, 15};
      TEST_ASSERT_EQUAL(1, ne2k_arp_service(&device, &io, rx_frame, sizeof(rx_frame),
                                             tx_frame, sizeof(tx_frame), local_mac, local_ip));
      { net_arp_cache_t cache; uint8_t target_ip[4] = {10, 0, 2, 2};
        uint8_t target_mac[6] = {0x52, 0x54, 0, 0, 0, 2};
        TEST_ASSERT_EQUAL(0, net_arp_cache_init(&cache));
        TEST_ASSERT_EQUAL(0, net_arp_cache_put(&cache, target_ip, target_mac));
        TEST_ASSERT_EQUAL(0, ne2k_arp_resolve(&device, &io, &cache, rx_frame, sizeof(rx_frame),
                                              tx_frame, sizeof(tx_frame), local_mac, local_ip,
                                              target_ip, 2)); }
    }
}

void test_ne2k_rom_read_and_tx_dma_order(void) {
    fake_ne2k_t fake = {0x12, NE2K_ISR_RESET | NE2K_ISR_RDC, 0, 0};
    ne2k_io_t io = {&fake, fake_inb, fake_outb};
    ne2k_device_t device;
    uint8_t frame[10] = {1,2,3,4,5,6,7,8,9,10};
    uint16_t i, read_command = 128U, rbcr = 128U, rsar = 128U;
    uint16_t rdc = 128U, write_command = 128U, first_data = 128U, transmit = 128U;
    uint8_t prom[12] = {0x02,0xaa,0x10,0xbb,0x20,0xcc,0x30,0xdd,0x40,0xee,0x50,0xff};
    for (i = 0U; i < 12U; ++i) fake.prom[i] = prom[i];
    TEST_ASSERT_EQUAL(0, ne2k_probe(&device, 0x300U, &io));
    TEST_ASSERT_EQUAL(0, ne2k_prepare(&device, &io));
    fake.trace_count = 0U; fake.prom_index = 0U;
    TEST_ASSERT_EQUAL(0, ne2k_read_mac(&device, &io));
    for (i = 0U; i < fake.trace_count; ++i) {
        if (fake.trace_ports[i] == 0x30aU && fake.trace_values[i] == 12U) rbcr = i;
        if (fake.trace_ports[i] == 0x308U && fake.trace_values[i] == 0U) rsar = i;
        if (fake.trace_ports[i] == 0x300U && fake.trace_values[i] == NE2K_COMMAND_REMOTE_READ) read_command = i;
    }
    TEST_ASSERT_TRUE(rbcr < rsar); TEST_ASSERT_TRUE(rsar < read_command);
    TEST_ASSERT_EQUAL(0x02, device.mac[0]); TEST_ASSERT_EQUAL(0x50, device.mac[5]);
    TEST_ASSERT_EQUAL(0, ne2k_configure_rings(&device, &io));
    fake.trace_count = 0U; fake.tx_count = 0U; fake.isr = NE2K_ISR_RDC;
    TEST_ASSERT_EQUAL(0, ne2k_tx_submit(&device, &io, frame, sizeof(frame)));
    for (i = 0U; i < fake.trace_count; ++i) {
        if (fake.trace_ports[i] == 0x307U && fake.trace_values[i] == NE2K_ISR_RDC) rdc = i;
        if (fake.trace_ports[i] == 0x300U && fake.trace_values[i] == NE2K_COMMAND_REMOTE_WRITE) write_command = i;
        if (first_data == 128U && fake.trace_ports[i] == 0x310U) first_data = i;
        if (fake.trace_ports[i] == 0x300U && fake.trace_values[i] == NE2K_COMMAND_TRANSMIT) transmit = i;
    }
    TEST_ASSERT_TRUE(rdc < write_command); TEST_ASSERT_TRUE(write_command < first_data);
    TEST_ASSERT_TRUE(first_data < transmit);
}

void test_ne2k_udp_via_gateway_preserves_ipv4_destination(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;uint8_t local_mac[6]={2,0,0,0,0,1},gateway_ip[4]={10,0,2,2},gateway_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},dns_ip[4]={1,1,1,1},request[128]={0},rx[128]={0},frame[128]={0},payload[2]={1,2};TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,gateway_ip,gateway_mac));TEST_ASSERT_EQUAL(0,ne2k_tx_udp_via(&device,&io,&cache,request,sizeof(request),rx,sizeof(rx),frame,sizeof(frame),local_ip,dns_ip,gateway_ip,49152U,53U,payload,sizeof(payload),1U));TEST_ASSERT_EQUAL(0x52,frame[0]);TEST_ASSERT_EQUAL(0x54,frame[1]);TEST_ASSERT_EQUAL(1,frame[30]);TEST_ASSERT_EQUAL(1,frame[31]);TEST_ASSERT_EQUAL(1,frame[32]);TEST_ASSERT_EQUAL(1,frame[33]);}

void test_ne2k_tcp_syn_via_gateway_preserves_ipv4_destination(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;uint8_t local_mac[6]={2,0,0,0,0,1},gateway_ip[4]={10,0,2,2},gateway_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={1,1,1,1},request[128]={0},rx[128]={0},frame[128]={0};TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,gateway_ip,gateway_mac));TEST_ASSERT_EQUAL(0,ne2k_tcp_syn_via(&device,&io,&cache,request,sizeof(request),rx,sizeof(rx),frame,sizeof(frame),local_ip,remote_ip,gateway_ip,49152U,443U,100U,1U));TEST_ASSERT_EQUAL(0x52,frame[0]);TEST_ASSERT_EQUAL(0x54,frame[1]);TEST_ASSERT_EQUAL(1,frame[30]);TEST_ASSERT_EQUAL(1,frame[31]);TEST_ASSERT_EQUAL(1,frame[32]);TEST_ASSERT_EQUAL(1,frame[33]);}

void test_ne2k_llm_network_context_lifecycle(void){ne2k_llm_network_context_t context;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_init(&context));TEST_ASSERT_EQUAL(0U,context.lease.valid);TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,context.session.phase);TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&context.connection,49152U,443U,100U));context.lease.valid=1U;context.lease.ipv4[0]=10U;context.session.phase=NE2K_LLM_CONNECTION_RESPONSE_READY;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_reset_for_request(&context));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,context.session.phase);TEST_ASSERT_EQUAL(1U,context.lease.valid);TEST_ASSERT_EQUAL(10U,context.lease.ipv4[0]);TEST_ASSERT_EQUAL(101U,context.connection.local_sequence);TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_network_context_init(0));}
void test_ne2k_llm_network_context_sse_resume_lifecycle(void){
    ne2k_llm_network_context_t context;
    net_llm_sse_response_t source={0},restored={0},before={0};
    uint8_t source_http[32]={0},source_sse[32]={0},restored_http[32]={0},restored_sse[32]={0};
    uint8_t provider=NE2K_LLM_PROVIDER_OLLAMA,retries=0U;
    TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_init(&context));
    TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&source,source_http,sizeof(source_http),source_sse,sizeof(source_sse)));
    TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&restored,restored_http,sizeof(restored_http),restored_sse,sizeof(restored_sse)));
    source.sse.event_id[0]='e'; source.sse.event_id[1]='v'; source.sse.event_id[2]='t';
    source.sse.event_id_length=3U; source.sse.event_id_valid=1U;
    TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_checkpoint(&context,NE2K_LLM_PROVIDER_OPENAI,2U,&source));
    TEST_ASSERT_EQUAL(1U,context.sse_resume_valid);
    context.session.phase=NE2K_LLM_CONNECTION_RESPONSE_READY;
    TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_reset_for_request(&context));
    TEST_ASSERT_EQUAL(1U,context.sse_resume_valid);
    TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_resume_load(&context,&provider,&retries,&restored));
    TEST_ASSERT_EQUAL(NE2K_LLM_PROVIDER_OPENAI,provider);
    TEST_ASSERT_EQUAL(2U,retries);
    TEST_ASSERT_EQUAL(1U,restored.sse.event_id_valid);
    TEST_ASSERT_EQUAL(3U,restored.sse.event_id_length);
    TEST_ASSERT_EQUAL_MEMORY("evt",restored.sse.event_id,3U);
    before=restored; context.sse_resume.checksum^=1U;
    provider=NE2K_LLM_PROVIDER_OLLAMA; retries=7U;
    TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_network_context_sse_resume_load(&context,&provider,&retries,&restored));
    TEST_ASSERT_EQUAL(NE2K_LLM_PROVIDER_OLLAMA,provider);
    TEST_ASSERT_EQUAL(7U,retries);
    TEST_ASSERT_EQUAL_MEMORY(&before,&restored,sizeof(restored));
    ne2k_llm_network_context_sse_resume_clear(&context);
    TEST_ASSERT_EQUAL(0U,context.sse_resume_valid);
    TEST_ASSERT_EQUAL(0U,context.sse_resume.magic);
    TEST_ASSERT_EQUAL(-2,ne2k_llm_network_context_sse_resume_load(&context,&provider,&retries,&restored));
}

void test_ne2k_llm_network_context_sse_event_tick_persists_retry(void){ne2k_llm_network_context_t context,before;net_llm_sse_response_t response={0},restored={0};net_llm_sse_reconnect_t reconnect={0};uint8_t http_buffer[32]={0},sse_buffer[32]={0},restored_http[32]={0},restored_sse[32]={0},text[1]={0},provider=NE2K_LLM_PROVIDER_OPENAI,retries=0U;uint16_t text_length=7U,consumed=7U;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_init(&context));context.session.phase=NE2K_LLM_CONNECTION_REQUEST_SENT;TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&response,http_buffer,sizeof(http_buffer),sse_buffer,sizeof(sse_buffer)));TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&restored,restored_http,sizeof(restored_http),restored_sse,sizeof(restored_sse)));response.sse.event_id[0]='e';response.sse.event_id[1]='v';response.sse.event_id[2]='t';response.sse.event_id_length=3U;response.sse.event_id_valid=1U;TEST_ASSERT_EQUAL(0,net_llm_sse_reconnect_init(&reconnect,2U));TEST_ASSERT_EQUAL(1,ne2k_llm_network_context_sse_event_tick(&context,0,0,0,0,0U,0,0U,0,0,NE2K_LLM_PROVIDER_OPENAI,0,0U,&response,text,sizeof(text),&text_length,&consumed,&reconnect,100U,0,0U,0,0,0,0U,1U,10U,25U));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,context.session.phase);TEST_ASSERT_EQUAL(1U,context.sse_resume_valid);TEST_ASSERT_EQUAL(1U,reconnect.retries_used);TEST_ASSERT_EQUAL(110U,reconnect.next_tick);TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_resume_load(&context,&provider,&retries,&restored));TEST_ASSERT_EQUAL(NE2K_LLM_PROVIDER_OPENAI,provider);TEST_ASSERT_EQUAL(1U,retries);TEST_ASSERT_EQUAL_MEMORY("evt",restored.sse.event_id,3U);TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_event_tick(&context,0,0,0,0,0U,0,0U,0,0,NE2K_LLM_PROVIDER_OPENAI,0,0U,&response,text,sizeof(text),&text_length,&consumed,&reconnect,109U,0,0U,0,0,0,0U,1U,10U,25U));before=context;TEST_ASSERT_EQUAL(-1,ne2k_llm_network_context_sse_event_tick(&context,0,0,0,0,0U,0,0U,0,0,NE2K_LLM_PROVIDER_OPENAI,0,0U,&response,text,sizeof(text),&text_length,&consumed,&reconnect,109U,0,0U,0,0,0,0U,1U,0U,25U));TEST_ASSERT_EQUAL_MEMORY(&before,&context,sizeof(context));}

void test_ne2k_llm_network_context_sse_rotate_provider(void){ne2k_llm_network_context_t context,before;net_llm_sse_response_t response={0},restored={0};net_llm_sse_reconnect_t reconnect,before_reconnect;uint8_t http_buffer[32]={0},sse_buffer[32]={0},restored_http[32]={0},restored_sse[32]={0},provider=NE2K_LLM_PROVIDER_OLLAMA,retries=9U,restored_provider=NE2K_LLM_PROVIDER_OLLAMA;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_init(&context));TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&response,http_buffer,sizeof(http_buffer),sse_buffer,sizeof(sse_buffer)));TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&restored,restored_http,sizeof(restored_http),restored_sse,sizeof(restored_sse)));response.sse.event_id[0]='e';response.sse.event_id[1]='v';response.sse.event_id[2]='t';response.sse.event_id_length=3U;response.sse.event_id_valid=1U;TEST_ASSERT_EQUAL(0,net_llm_sse_reconnect_init(&reconnect,2U));reconnect.retries_used=2U;TEST_ASSERT_EQUAL(1,ne2k_llm_network_context_sse_rotate_provider(&context,&provider,&reconnect,&response));TEST_ASSERT_EQUAL(NE2K_LLM_PROVIDER_OPENAI,provider);TEST_ASSERT_EQUAL(0U,reconnect.retries_used);TEST_ASSERT_EQUAL(0U,reconnect.scheduled);TEST_ASSERT_EQUAL(1U,context.sse_resume_valid);TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_resume_load(&context,&restored_provider,&retries,&restored));TEST_ASSERT_EQUAL(NE2K_LLM_PROVIDER_OPENAI,restored_provider);TEST_ASSERT_EQUAL(0U,retries);TEST_ASSERT_EQUAL(0U,restored.sse.event_id_valid);TEST_ASSERT_EQUAL(0U,restored.sse.event_id_length);before=context;before_reconnect=reconnect;reconnect.scheduled=1U;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_rotate_provider(&context,&provider,&reconnect,&response));TEST_ASSERT_EQUAL_MEMORY(&before,&context,sizeof(context));TEST_ASSERT_EQUAL(1U,reconnect.scheduled);reconnect=before_reconnect;provider=9U;TEST_ASSERT_EQUAL(-2,ne2k_llm_network_context_sse_rotate_provider(&context,&provider,&reconnect,&response));TEST_ASSERT_EQUAL(9U,provider);}

void test_ne2k_llm_network_context_sse_schedule_jittered(void){ne2k_llm_network_context_t context,before;net_llm_sse_response_t response={0},restored={0};net_llm_sse_reconnect_t reconnect,before_reconnect;uint8_t http_buffer[32]={0},sse_buffer[32]={0},restored_http[32]={0},restored_sse[32]={0},provider=NE2K_LLM_PROVIDER_OLLAMA,retries=0U;uint32_t seed=1U,before_seed;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_init(&context));context.session.phase=NE2K_LLM_CONNECTION_STREAMING;TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&response,http_buffer,sizeof(http_buffer),sse_buffer,sizeof(sse_buffer)));TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&restored,restored_http,sizeof(restored_http),restored_sse,sizeof(restored_sse)));response.sse.event_id[0]='e';response.sse.event_id[1]='v';response.sse.event_id[2]='t';response.sse.event_id_length=3U;response.sse.event_id_valid=1U;TEST_ASSERT_EQUAL(0,net_llm_sse_reconnect_init(&reconnect,2U));TEST_ASSERT_EQUAL(1,ne2k_llm_network_context_sse_schedule_jittered(&context,NE2K_LLM_PROVIDER_OLLAMA,&reconnect,&response,503U,10U,100U,100U,5U,&seed));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,context.session.phase);TEST_ASSERT_TRUE(reconnect.next_tick>=105U);TEST_ASSERT_TRUE(reconnect.next_tick<=115U);TEST_ASSERT_EQUAL(1U,reconnect.retries_used);TEST_ASSERT_NOT_EQUAL(1U,seed);TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_resume_load(&context,&provider,&retries,&restored));TEST_ASSERT_EQUAL(NE2K_LLM_PROVIDER_OLLAMA,provider);TEST_ASSERT_EQUAL(1U,retries);TEST_ASSERT_EQUAL_MEMORY("evt",restored.sse.event_id,3U);before=context;before_reconnect=reconnect;before_seed=seed;context.session.phase=NE2K_LLM_CONNECTION_STREAMING;TEST_ASSERT_EQUAL(-1,ne2k_llm_network_context_sse_schedule_jittered(&context,NE2K_LLM_PROVIDER_OLLAMA,&reconnect,&response,503U,0U,100U,100U,5U,&seed));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_STREAMING,context.session.phase);context=before;reconnect=before_reconnect;seed=before_seed;}

void test_ne2k_llm_network_context_dhcp_renew_if_due(void){fake_ne2k_t fake={0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device={0};ne2k_llm_network_context_t context;net_dhcp_lease_t preserved;uint8_t tx[512]={0},rx[512]={0};TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_init(&context));context.lease=(net_dhcp_lease_t){1U,1U,1U,1U,{10U,0U,2U,15U},{10U,0U,2U,2U},{255U,255U,255U,0U},{10U,0U,2U,2U},{1U,1U,1U,1U},0x12345678U,100U,1000U};TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_dhcp_renew_if_due(&context,&device,&io,tx,sizeof(tx),rx,sizeof(rx),0x87654321U,1U,1049U));preserved=context.lease;TEST_ASSERT_EQUAL(-3,ne2k_llm_network_context_dhcp_renew_if_due(&context,&device,&io,tx,sizeof(tx),rx,sizeof(rx),0x87654321U,1U,1050U));TEST_ASSERT_EQUAL_MEMORY(&preserved,&context.lease,sizeof(context.lease));TEST_ASSERT_EQUAL(-2,ne2k_llm_network_context_dhcp_renew_if_due(&context,&device,&io,tx,sizeof(tx),rx,sizeof(rx),0x87654321U,1U,1100U));TEST_ASSERT_EQUAL_MEMORY(&preserved,&context.lease,sizeof(context.lease));TEST_ASSERT_EQUAL(-1,ne2k_llm_network_context_dhcp_renew_if_due(0,&device,&io,tx,sizeof(tx),rx,sizeof(rx),0x87654321U,1U,1049U));}

void test_ne2k_llm_network_context_reconcile_lease(void){ne2k_llm_network_context_t context;net_dhcp_lease_t lease;net_llm_sse_response_t response={0},restored={0};uint8_t http[32]={0},sse[32]={0},restored_http[32]={0},restored_sse[32]={0},provider=0U,retries=0U;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_init(&context));lease=(net_dhcp_lease_t){1U,1U,1U,1U,{10U,0U,2U,15U},{10U,0U,2U,2U},{255U,255U,255U,0U},{10U,0U,2U,2U},{1U,1U,1U,1U},1U,100U,10U};context.lease=lease;context.session.phase=NE2K_LLM_CONNECTION_STREAMING;TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&context.connection,49152U,443U,100U));TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&response,http,sizeof(http),sse,sizeof(sse)));TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&restored,restored_http,sizeof(restored_http),restored_sse,sizeof(restored_sse)));response.sse.event_id[0]='e';response.sse.event_id_length=1U;response.sse.event_id_valid=1U;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_checkpoint(&context,NE2K_LLM_PROVIDER_OLLAMA,1U,&response));lease.acquired_tick=20U;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_reconcile_lease(&context,&lease));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_STREAMING,context.session.phase);lease.router_ipv4[3]=3U;TEST_ASSERT_EQUAL(1,ne2k_llm_network_context_reconcile_lease(&context,&lease));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,context.session.phase);TEST_ASSERT_EQUAL(0U,context.connection.local_port);TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_resume_load(&context,&provider,&retries,&restored));TEST_ASSERT_EQUAL(1U,restored.sse.event_id_length);TEST_ASSERT_EQUAL('e',restored.sse.event_id[0]);TEST_ASSERT_EQUAL(-1,ne2k_llm_network_context_reconcile_lease(0,&lease));}

void test_ne2k_llm_network_context_sse_resume_decide(void){ne2k_llm_network_context_t context;net_llm_sse_response_t response={0};uint8_t http[32]={0},sse[32]={0};TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_init(&context));TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&response,http,sizeof(http),sse,sizeof(sse)));context.session.phase=NE2K_LLM_CONNECTION_TLS_COMPLETE;TEST_ASSERT_EQUAL(0,ne2k_llm_network_context_sse_resume_decide(&context,&response));response.sse.event_id[0]='e';response.sse.event_id_length=1U;response.sse.event_id_valid=1U;TEST_ASSERT_EQUAL(1,ne2k_llm_network_context_sse_resume_decide(&context,&response));context.session.phase=NE2K_LLM_CONNECTION_STREAMING;TEST_ASSERT_EQUAL(-2,ne2k_llm_network_context_sse_resume_decide(&context,&response));TEST_ASSERT_EQUAL(-1,ne2k_llm_network_context_sse_resume_decide(0,&response));}

void test_ne2k_llm_connection_acquire_start_dhcp_guard_is_transactional(void){net_dhcp_lease_t lease={1U,1U,1U,1U,{10U,0U,2U,15U},{10U,0U,2U,2U},{255U,255U,255U,0U},{10U,0U,2U,2U},{1U,1U,1U,1U},0x12345678U};ne2k_llm_connection_state_t state;net_tcp_connection_t connection;TEST_ASSERT_EQUAL(0,ne2k_llm_connection_state_init(&state));state.phase=NE2K_LLM_CONNECTION_SYN_SENT;TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_acquire_start_dhcp(0,0,0,0,0U,0,0U,0U,0U,0,0U,0,0U,0,0U,0U,0,0U,0U,0U,0U,0U,&lease,&state,&connection));TEST_ASSERT_EQUAL(1U,lease.valid);TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_SYN_SENT,state.phase);TEST_ASSERT_EQUAL(101U,connection.local_sequence);}

void test_ne2k_llm_connection_start_dhcp_guard_is_transactional(void){ne2k_llm_connection_state_t state;net_tcp_connection_t connection;TEST_ASSERT_EQUAL(0,ne2k_llm_connection_state_init(&state));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_start_dhcp(0,0,0,0,0U,0,0U,0,0U,0,0U,0,0U,0U,0U,0U,0U,&state,&connection));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,state.phase);TEST_ASSERT_EQUAL(0U,state.remote_ip[0]);TEST_ASSERT_EQUAL(101U,connection.local_sequence);}

void test_tcp_ack_is_emitted_from_connection_state(void) {
    fake_ne2k_t fake = {0x12, 0, 0, 0}; ne2k_io_t io = {&fake, fake_inb, fake_outb}; ne2k_device_t device;
    net_arp_cache_t cache; net_tcp_connection_t connection; uint8_t frame[128] = {0};
    uint8_t local_ip[4] = {10, 0, 2, 15}; uint8_t remote_ip[4] = {10, 0, 2, 2};
    uint8_t remote_mac[6] = {0x52, 0x54, 0, 0, 0, 2}; uint8_t local_mac[6] = {0x02, 0, 0, 0, 0, 1};
    fake.isr = (uint8_t)(NE2K_ISR_RESET | NE2K_ISR_RDC);
    TEST_ASSERT_EQUAL(0, ne2k_probe(&device, 0x300, &io));
    TEST_ASSERT_EQUAL(0, ne2k_prepare(&device, &io));
    TEST_ASSERT_EQUAL(0, ne2k_configure_rings(&device, &io));
    TEST_ASSERT_EQUAL(0, ne2k_set_mac(&device, local_mac));
    TEST_ASSERT_EQUAL(0, net_arp_cache_init(&cache));
    TEST_ASSERT_EQUAL(0, net_arp_cache_put(&cache, remote_ip, remote_mac));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    { net_tcp_view_t syn_ack = {443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
      TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &syn_ack)); }
    TEST_ASSERT_EQUAL(0, ne2k_tcp_ack(&device, &io, &cache, frame, sizeof(frame), local_ip, remote_ip, &connection));
    TEST_ASSERT_EQUAL(0x52, frame[0]); TEST_ASSERT_EQUAL(0x02, frame[6]);
    TEST_ASSERT_EQUAL(0x08, frame[12]); TEST_ASSERT_EQUAL(0x00, frame[13]);
    TEST_ASSERT_EQUAL(0x45, frame[14]); TEST_ASSERT_EQUAL(NET_TCP_PROTOCOL, frame[23]);
    TEST_ASSERT_EQUAL(49152, ((uint16_t)frame[34] << 8) | frame[35]);
    TEST_ASSERT_EQUAL(443, ((uint16_t)frame[36] << 8) | frame[37]);
    TEST_ASSERT_EQUAL(101U, ((uint32_t)frame[38] << 24) | ((uint32_t)frame[39] << 16) | ((uint32_t)frame[40] << 8) | frame[41]);
    TEST_ASSERT_EQUAL(701U, ((uint32_t)frame[42] << 24) | ((uint32_t)frame[43] << 16) | ((uint32_t)frame[44] << 8) | frame[45]);
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_ACK, frame[47]);
    { uint8_t payload[4] = {'P','I','N','G'};
      TEST_ASSERT_EQUAL(0, ne2k_tcp_data(&device, &io, &cache, frame, sizeof(frame), local_ip, remote_ip, &connection, payload, sizeof(payload)));
      TEST_ASSERT_EQUAL(44, ((uint16_t)frame[16] << 8) | frame[17]);
      TEST_ASSERT_EQUAL('P', frame[54]); TEST_ASSERT_EQUAL('G', frame[57]); }
    { uint8_t payload[4] = {'P','I','N','G'}; uint32_t first_sequence;
      TEST_ASSERT_EQUAL(0, net_tcp_connection_track_send(&connection, payload, sizeof(payload), 1U));
      TEST_ASSERT_EQUAL(0, ne2k_tcp_data(&device, &io, &cache, frame, sizeof(frame), local_ip, remote_ip, &connection, payload, sizeof(payload)));
      first_sequence = ((uint32_t)frame[38] << 24) | ((uint32_t)frame[39] << 16) | ((uint32_t)frame[40] << 8) | frame[41];
      TEST_ASSERT_EQUAL(0, net_tcp_connection_commit_send(&connection, sizeof(payload)));
      TEST_ASSERT_EQUAL(0, ne2k_tcp_retransmit(&device, &io, &cache, frame, sizeof(frame), local_ip, remote_ip, &connection));
      TEST_ASSERT_EQUAL(first_sequence, ((uint32_t)frame[38] << 24) | ((uint32_t)frame[39] << 16) | ((uint32_t)frame[40] << 8) | frame[41]);
      TEST_ASSERT_EQUAL(0, net_tcp_connection_retransmit_allowed(&connection));
      TEST_ASSERT_NOT_EQUAL(0, ne2k_tcp_retransmit(&device, &io, &cache, frame, sizeof(frame), local_ip, remote_ip, &connection)); }
    { net_tcp_view_t syn_ack = {443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
      TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
      TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &syn_ack));
      TEST_ASSERT_EQUAL(0, ne2k_tcp_fin(&device, &io, &cache, frame, sizeof(frame), local_ip, remote_ip, &connection));
      TEST_ASSERT_EQUAL(NET_TCP_STATE_FIN_WAIT_1, connection.state);
      TEST_ASSERT_EQUAL((NET_TCP_FLAG_FIN | NET_TCP_FLAG_ACK), frame[47]); }
}

void test_tcp_receive_copies_bounded_payload(void) {
    uint8_t frame[128] = {0}, payload[4] = {0}; net_tcp_connection_t connection; uint16_t length = 0U, i;
    uint8_t tcp_payload[4] = {'P','O','N','G'}; uint8_t remote_ip[4] = {10,0,2,2}; uint8_t local_ip[4] = {10,0,2,15}; uint16_t checksum;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    { net_tcp_view_t syn_ack = {443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
      TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &syn_ack)); }
    for (i = 0; i < 6U; ++i) { frame[i] = 0x52U; frame[6U+i] = 0x02U; }
    frame[12] = 0x08U; frame[13] = 0x00U; frame[14] = 0x45U; frame[16] = 0U; frame[17] = 44U; frame[22] = 64U; frame[23] = NET_TCP_PROTOCOL;
    for (i = 0; i < 4U; ++i) { frame[26U+i] = remote_ip[i]; frame[30U+i] = local_ip[i]; }
    { uint16_t ip_checksum = net_ipv4_checksum(frame + 14U, 20U); frame[24] = (uint8_t)(ip_checksum >> 8); frame[25] = (uint8_t)ip_checksum; }
    TEST_ASSERT_EQUAL(24, net_tcp_build_data(frame + 34U, sizeof(frame) - 34U, 443, 49152, 701U, 101U, tcp_payload, sizeof(tcp_payload)));
    checksum = net_tcp_checksum_ipv4(remote_ip, local_ip, frame + 34U, 24U); frame[50] = (uint8_t)(checksum >> 8); frame[51] = (uint8_t)checksum;
    TEST_ASSERT_EQUAL(0, ne2k_tcp_receive(frame, 58U, &connection, payload, sizeof(payload), &length));
    TEST_ASSERT_EQUAL(4U, length); TEST_ASSERT_EQUAL('P', payload[0]); TEST_ASSERT_EQUAL('G', payload[3]);
    frame[50] ^= 0xffU; TEST_ASSERT_NOT_EQUAL(0, ne2k_tcp_receive(frame, 58U, &connection, payload, sizeof(payload), &length));
    frame[50] ^= 0xffU; frame[24] ^= 0xffU; TEST_ASSERT_NOT_EQUAL(0, ne2k_tcp_receive(frame, 58U, &connection, payload, sizeof(payload), &length));
    TEST_ASSERT_NOT_EQUAL(0, ne2k_tcp_receive(frame, 58U, &connection, payload, 3U, &length));
}

void test_tcp_poll_is_bounded_when_rx_empty(void) {
    fake_ne2k_t fake = {0x12, 0, 0, 0}; ne2k_io_t io = {&fake, fake_inb, fake_outb}; ne2k_device_t device;
    net_tcp_connection_t connection; uint8_t frame[64] = {0}; uint8_t payload[8] = {0}; uint16_t length = 99U;
    fake.isr = NE2K_ISR_RESET; TEST_ASSERT_EQUAL(0, ne2k_probe(&device, 0x300, &io));
    TEST_ASSERT_EQUAL(0, ne2k_prepare(&device, &io)); TEST_ASSERT_EQUAL(0, ne2k_configure_rings(&device, &io));
    TEST_ASSERT_EQUAL(1, ne2k_tcp_poll(&device, &io, frame, sizeof(frame), &connection, payload, sizeof(payload), &length));
    TEST_ASSERT_EQUAL(0U, length);
    { net_arp_cache_t cache; uint8_t tx[64] = {0}; uint8_t local_ip[4] = {10,0,2,15}; uint8_t remote_ip[4] = {10,0,2,2};
      TEST_ASSERT_EQUAL(0, net_arp_cache_init(&cache));
      fake.tx_count = 0U; TEST_ASSERT_EQUAL(1, ne2k_tcp_poll_ack(&device, &io, &cache, frame, sizeof(frame), tx, sizeof(tx), local_ip, remote_ip, &connection, payload, sizeof(payload), &length));
      TEST_ASSERT_EQUAL(0U, length); TEST_ASSERT_EQUAL(0U, fake.tx_count);
      fake.tx_count = 0U; TEST_ASSERT_EQUAL(1, ne2k_tcp_poll_fin_ack(&device, &io, &cache, frame, sizeof(frame), tx, sizeof(tx), local_ip, remote_ip, &connection));
      TEST_ASSERT_EQUAL(0U, fake.tx_count); }
}

void test_rx_extract_publishes_bounded_frame(void) {
    uint8_t storage[NET_NIC_QUEUE_CAPACITY * 64U] = {0};
    uint8_t dma[9] = {NE2K_RX_STATUS_OK, 0, 9, 0, 1, 2, 3, 4, 5};
    net_nic_queue_t queue; uint8_t* frame; uint16_t length;
    TEST_ASSERT_EQUAL(0, net_nic_queue_init(&queue, storage, sizeof(storage), 64));
    TEST_ASSERT_EQUAL(0, ne2k_rx_extract(dma, sizeof(dma), &queue));
    TEST_ASSERT_EQUAL(0, net_nic_queue_pop(&queue, &frame, &length));
    TEST_ASSERT_EQUAL(5, length); TEST_ASSERT_EQUAL(1, frame[0]); TEST_ASSERT_EQUAL(5, frame[4]);
    dma[0] = 0; TEST_ASSERT_NOT_EQUAL(0, ne2k_rx_extract(dma, sizeof(dma), &queue));
}


void test_probe_rejects_missing_reset_ack(void) {
    fake_ne2k_t fake = {0, 0, 0, 0};
    ne2k_io_t io = {&fake, fake_inb, fake_outb};
    ne2k_device_t device;
    TEST_ASSERT_NOT_EQUAL(0, ne2k_probe(&device, 0x300, &io));
}

void test_ne2k_tls_client_start_and_empty_poll(void) {
    fake_ne2k_t fake = {0x12, NE2K_ISR_RESET | NE2K_ISR_RDC, 0, 0};fake_rtc_t fake_rtc={0};ne2k_io_t io = {&fake, fake_inb, fake_outb};rtc_io_t rtc_io={&fake_rtc,fake_rtc_inb,fake_rtc_outb};ne2k_device_t device;net_arp_cache_t cache;net_tcp_connection_t connection;ne2k_tls_client_t client;x509_certificate_view_t anchor={0};
    uint8_t local_mac[6]={0x02,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={10,0,2,2},client_random[32]={0},client_private[32]={0};
    uint8_t record_buffer[256]={0},handshake_buffer[256]={0},transcript_buffer[512]={0},hello_record[256]={0},rx_frame[256]={0},tx_frame[512]={0},tcp_segment[256]={0},flight_records[128]={0},plaintext[128]={0};uint32_t rsa_workspace[224]={0},x25519_workspace[136]={0},flight_length=99U;uint8_t prf_workspace[256]={0};uint16_t consumed=99U;int length;
    TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));
    {net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&connection,&syn_ack));}
    fake_rtc.registers[0x00U]=0U;fake_rtc.registers[0x02U]=0U;fake_rtc.registers[0x04U]=0x12U;fake_rtc.registers[0x07U]=0x18U;fake_rtc.registers[0x08U]=0x08U;fake_rtc.registers[0x09U]=0x26U;fake_rtc.registers[0x0aU]=0U;fake_rtc.registers[0x0bU]=0U;TEST_ASSERT_EQUAL(0,ne2k_tls_client_init(&client,record_buffer,sizeof(record_buffer),handshake_buffer,sizeof(handshake_buffer),transcript_buffer,sizeof(transcript_buffer)));
    length=ne2k_tls_client_start(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,client_random,hello_record,sizeof(hello_record),1U);
    TEST_ASSERT_GREATER_THAN(0,length);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_CLIENT_HELLO_SENT,client.handshake.state);TEST_ASSERT_EQUAL((uint16_t)(length-5),client.transcript.length);TEST_ASSERT_EQUAL((uint32_t)(101U+length),connection.local_sequence);TEST_ASSERT_EQUAL(NET_TLS_CONTENT_HANDSHAKE,tx_frame[54]);
    fake.isr=NE2K_ISR_RESET;TEST_ASSERT_EQUAL(1,ne2k_tls_client_poll(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,client_random,client_private,&anchor,"api.example.test","20260818000000Z",rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));TEST_ASSERT_EQUAL(0U,consumed);TEST_ASSERT_EQUAL(0U,flight_length);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_CLIENT_HELLO_SENT,client.handshake.state);TEST_ASSERT_EQUAL(1,ne2k_tls_client_poll_chain_two(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,client_random,client_private,&anchor,&anchor,"api.example.test","20260818000000Z",rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));TEST_ASSERT_NOT_EQUAL(0,ne2k_tls_client_poll_chain_two(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,client_random,client_private,0,&anchor,"api.example.test","20260818000000Z",rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));TEST_ASSERT_EQUAL(1,ne2k_tls_client_poll_received_chain(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,client_random,client_private,&anchor,"api.example.test","20260818000000Z",rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));TEST_ASSERT_NOT_EQUAL(0,ne2k_tls_client_poll_received_chain(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,0,client_random,client_private,&anchor,"api.example.test","20260818000000Z",rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));fake.isr=NE2K_ISR_RESET;flight_length=99U;consumed=99U;TEST_ASSERT_EQUAL(1,ne2k_tls_client_poll_received_chain_rtc(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,client_random,client_private,&anchor,"api.example.test",&rtc_io,rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));TEST_ASSERT_EQUAL(0U,flight_length);TEST_ASSERT_EQUAL(0U,consumed);fake_rtc.registers[0x00U]=0x6aU;flight_length=77U;consumed=55U;TEST_ASSERT_NOT_EQUAL(0,ne2k_tls_client_poll_received_chain_rtc(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,client_random,client_private,&anchor,"api.example.test",&rtc_io,rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));TEST_ASSERT_EQUAL(77U,flight_length);TEST_ASSERT_EQUAL(55U,consumed);{net_tcp_connection_retry_t retry;client.peer_identity_validated=1U;client.complete=1U;client.transcript.length=7U;client.stream.record_accumulator.length=3U;client.stream.handshake_accumulator.length=2U;client.master_secret[0]=1U;client.x25519.ready=1U;client.session.write_sequence=4U;TEST_ASSERT_EQUAL(0,net_tcp_connection_retry_init(&retry,1U));TEST_ASSERT_EQUAL(1,ne2k_tls_client_retry_reset(&connection,&client,&retry,900U));TEST_ASSERT_EQUAL(1U,retry.retries_used);TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT,connection.state);TEST_ASSERT_EQUAL(901U,connection.local_sequence);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_IDLE,client.handshake.state);TEST_ASSERT_EQUAL(0U,client.transcript.length);TEST_ASSERT_EQUAL(0U,client.stream.record_accumulator.length);TEST_ASSERT_EQUAL(0U,client.stream.handshake_accumulator.length);TEST_ASSERT_EQUAL(0U,client.master_secret[0]);TEST_ASSERT_EQUAL(0U,client.x25519.ready);TEST_ASSERT_EQUAL(0U,client.peer_identity_validated);TEST_ASSERT_EQUAL(0U,client.complete);TEST_ASSERT_EQUAL(0,ne2k_tls_client_retry_reset(&connection,&client,&retry,1000U));TEST_ASSERT_EQUAL(901U,connection.local_sequence);}
}

void test_ne2k_llm_dns_syn_bootstrap_failure_is_transactional(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;net_tcp_connection_t connection;uint8_t local_mac[6]={2,0,0,0,0,1},dns_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},dns_ip[4]={10,0,2,3},remote_ip[4]={1,2,3,4},arp_request[128]={0},arp_rx[128]={0},frame[512]={0};TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,dns_ip,dns_mac));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_dns_syn_bootstrap(&device,&io,&cache,arp_request,sizeof(arp_request),arp_rx,sizeof(arp_rx),frame,sizeof(frame),local_ip,dns_ip,0x1234U,"api.example.test",0U,1U,49152U,443U,900U,remote_ip,&connection));TEST_ASSERT_EQUAL(1U,remote_ip[0]);TEST_ASSERT_EQUAL(101U,connection.local_sequence);TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT,connection.state);TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_dns_syn_bootstrap(&device,&io,&cache,arp_request,sizeof(arp_request),arp_rx,sizeof(arp_rx),frame,sizeof(frame),local_ip,dns_ip,0x1234U,"api.example.test",1U,1U,49152U,443U,900U,remote_ip,&connection));TEST_ASSERT_EQUAL(1U,remote_ip[0]);TEST_ASSERT_EQUAL(101U,connection.local_sequence);}

void test_ne2k_socket_syn_ack_tls_start_is_transactional(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;ne2k_tls_client_t client;net_tcp_connection_t connection;net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0},bad_ack={443U,49152U,700U,99U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};uint8_t local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={10,0,2,2},client_random[32]={0},record_buffer[256]={0},handshake_buffer[256]={0},transcript_buffer[512]={0},hello_record[256]={0},tcp_segment[512]={0},tx_frame[512]={0};int socket_id,length;net_socket_reset_all();socket_id=net_socket_open(49152U,443U,100U);TEST_ASSERT_TRUE(socket_id>=0);TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));TEST_ASSERT_EQUAL(0,ne2k_tls_client_init(&client,record_buffer,sizeof(record_buffer),handshake_buffer,sizeof(handshake_buffer),transcript_buffer,sizeof(transcript_buffer)));TEST_ASSERT_NOT_EQUAL(0,ne2k_socket_tls_accept_syn_ack_start(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,&bad_ack,&client,client_random,hello_record,sizeof(hello_record),tcp_segment,sizeof(tcp_segment),2U));TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_IDLE,client.handshake.state);TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&connection));TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT,connection.state);length=ne2k_socket_tls_accept_syn_ack_start(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,&syn_ack,&client,client_random,hello_record,sizeof(hello_record),tcp_segment,sizeof(tcp_segment),2U);TEST_ASSERT_GREATER_THAN(0,length);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_CLIENT_HELLO_SENT,client.handshake.state);TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&connection));TEST_ASSERT_EQUAL(NET_TCP_STATE_ESTABLISHED,connection.state);TEST_ASSERT_EQUAL((uint32_t)(101U+length),connection.local_sequence);TEST_ASSERT_EQUAL(NET_TLS_CONTENT_HANDSHAKE,tx_frame[54]);TEST_ASSERT_EQUAL(0,net_socket_close(socket_id));}

void test_ne2k_syn_ack_tls_start_is_transactional(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;net_tcp_connection_t connection;ne2k_tls_client_t client;net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0},bad_ack={443U,49152U,700U,99U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};uint8_t local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={10,0,2,2},client_random[32]={0},record_buffer[256]={0},handshake_buffer[256]={0},transcript_buffer[512]={0},hello_record[256]={0},rx_frame[128]={0},tx_frame[512]={0};int length;TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));TEST_ASSERT_EQUAL(0,ne2k_tls_client_init(&client,record_buffer,sizeof(record_buffer),handshake_buffer,sizeof(handshake_buffer),transcript_buffer,sizeof(transcript_buffer)));TEST_ASSERT_NOT_EQUAL(0,ne2k_tls_client_accept_syn_ack_start(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,&bad_ack,&connection,&client,client_random,hello_record,sizeof(hello_record),1U));TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT,connection.state);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_IDLE,client.handshake.state);length=ne2k_tls_client_accept_syn_ack_start(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,&syn_ack,&connection,&client,client_random,hello_record,sizeof(hello_record),1U);TEST_ASSERT_GREATER_THAN(0,length);TEST_ASSERT_EQUAL(NET_TCP_STATE_ESTABLISHED,connection.state);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_CLIENT_HELLO_SENT,client.handshake.state);TEST_ASSERT_EQUAL((uint32_t)(101U+length),connection.local_sequence);TEST_ASSERT_EQUAL(1,ne2k_llm_syn_ack_tls_start(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,client_random,hello_record,sizeof(hello_record),1U));}

void test_ne2k_dhcp_renew_if_due_guards_transactionally(void){fake_ne2k_t fake={0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device={0};net_dhcp_lease_t lease={1U,1U,1U,1U,{10U,0U,2U,15U},{10U,0U,2U,2U},{255U,255U,255U,0U},{10U,0U,2U,2U},{1U,1U,1U,1U},0x12345678U,100U,1000U},preserved;uint8_t tx[512]={0},rx[512]={0};TEST_ASSERT_EQUAL(0,ne2k_dhcp_renew_if_due(&device,&io,tx,sizeof(tx),rx,sizeof(rx),0x87654321U,1U,1049U,&lease));preserved=lease;TEST_ASSERT_EQUAL(-3,ne2k_dhcp_renew_if_due(&device,&io,tx,sizeof(tx),rx,sizeof(rx),0x87654321U,1U,1050U,&lease));TEST_ASSERT_EQUAL_MEMORY(&preserved,&lease,sizeof(lease));TEST_ASSERT_EQUAL(-2,ne2k_dhcp_renew_if_due(&device,&io,tx,sizeof(tx),rx,sizeof(rx),0x87654321U,1U,1100U,&lease));}

void test_ne2k_dhcp_acquire_guard_is_transactional(void){net_dhcp_lease_t lease={1U,1U,1U,1U,{10U,0U,2U,15U},{10U,0U,2U,2U},{255U,255U,255U,0U},{10U,0U,2U,2U},{1U,1U,1U,1U},0x12345678U};uint8_t tx[1]={0},rx[1]={0};TEST_ASSERT_NOT_EQUAL(0,ne2k_dhcp_acquire(0,0,tx,sizeof(tx),rx,sizeof(rx),0x12345678U,1U,&lease));TEST_ASSERT_EQUAL(1U,lease.valid);TEST_ASSERT_EQUAL(10U,lease.ipv4[0]);TEST_ASSERT_EQUAL(15U,lease.ipv4[3]);TEST_ASSERT_EQUAL(1U,lease.subnet_valid);TEST_ASSERT_EQUAL(1U,lease.router_valid);TEST_ASSERT_EQUAL(1U,lease.dns_valid);TEST_ASSERT_NOT_EQUAL(0,ne2k_dhcp_acquire(0,0,tx,sizeof(tx),rx,sizeof(rx),0x12345678U,0U,&lease));TEST_ASSERT_EQUAL(1U,lease.valid);TEST_ASSERT_NOT_EQUAL(0,ne2k_dhcp_poll_ack(0,0,rx,sizeof(rx),0x12345678U,1U,&lease));TEST_ASSERT_EQUAL(1U,lease.valid);}

void test_ne2k_llm_connection_state_phase_guards(void){ne2k_llm_connection_state_t state;net_tcp_connection_t connection;ne2k_tls_client_t client={0};TEST_ASSERT_EQUAL(0,ne2k_llm_connection_state_init(&state));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,state.phase);TEST_ASSERT_EQUAL(0U,state.remote_ip[0]);TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_start(0,0,0,0,0U,0,0U,0,0U,0,0,0U,0,0U,0U,0U,0U,0U,&state,&connection));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,state.phase);TEST_ASSERT_EQUAL(101U,connection.local_sequence);TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_poll_tls_start(0,0,0,0,0U,0,0U,0,&state,&connection,&client,0,0,0U,0U));state.phase=NE2K_LLM_CONNECTION_SYN_SENT;TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_start(0,0,0,0,0U,0,0U,0,0U,0,0,0U,0,0U,0U,0U,0U,0U,&state,&connection));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_SYN_SENT,state.phase);TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_state_init(0));}

void test_ne2k_llm_connection_tls_phase_guard(void){ne2k_llm_connection_state_t state;net_tcp_connection_t connection;ne2k_tls_client_t client={0};TEST_ASSERT_EQUAL(0,ne2k_llm_connection_state_init(&state));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_poll_tls(0,0,0,0,0U,0,0U,0,&state,&connection,&client,0,0,0,0,0,0,0,0U,0,0U,0,0U,0,0U,0,0U,0,0U,0U,0U));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,state.phase);TEST_ASSERT_EQUAL(101U,connection.local_sequence);TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_request(0,0,0,0,0U,0,&state,&connection,&client,0U,0,0U,0,0U,0,0,0,0,0,0U,0,0U,0U));}

void test_ne2k_llm_connection_response_phase_guard(void){ne2k_llm_connection_state_t state;net_tcp_connection_t connection;ne2k_tls_client_t client={0};net_http_response_accumulator_t accumulator={0};net_http_response_view_t response={0};uint8_t plaintext[1]={0},text[1]={0};uint16_t text_length=9U,consumed=9U;TEST_ASSERT_EQUAL(0,ne2k_llm_connection_state_init(&state));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_poll_text(0,0,0,0,0U,0,0U,0,&state,&connection,&client,NE2K_LLM_PROVIDER_OLLAMA,plaintext,sizeof(plaintext),&accumulator,&response,text,sizeof(text),&text_length,&consumed));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,state.phase);TEST_ASSERT_EQUAL(9U,text_length);TEST_ASSERT_EQUAL(9U,consumed);}

void test_ne2k_llm_connection_reset_for_request(void){ne2k_llm_connection_state_t state;TEST_ASSERT_EQUAL(0,ne2k_llm_connection_state_init(&state));state.remote_ip[0]=10U;state.remote_ip[1]=0U;state.remote_ip[2]=2U;state.remote_ip[3]=2U;TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_reset_for_request(&state));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,state.phase);TEST_ASSERT_EQUAL(10U,state.remote_ip[0]);state.phase=NE2K_LLM_CONNECTION_RESPONSE_READY;TEST_ASSERT_EQUAL(0,ne2k_llm_connection_reset_for_request(&state));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,state.phase);TEST_ASSERT_EQUAL(10U,state.remote_ip[0]);TEST_ASSERT_EQUAL(2U,state.remote_ip[3]);TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_reset_for_request(&state));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,state.phase);TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_reset_for_request(0));}

void test_ne2k_llm_connection_sse_phase_guard_and_nonblocking_progress(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;net_tcp_connection_t connection;ne2k_tls_client_t client={0};ne2k_llm_connection_state_t state;net_llm_sse_response_t response={0};uint8_t local_mac[6]={2,0,0,0,0,1},local_ip[4]={10,0,2,15},plaintext[1]={0},text[1]={0};uint16_t text_length=9U,consumed=9U;TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,ne2k_llm_connection_state_init(&state));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_poll_sse(&device,&io,&cache,plaintext,sizeof(plaintext),plaintext,sizeof(plaintext),local_ip,&state,&connection,&client,NE2K_LLM_PROVIDER_OLLAMA,plaintext,sizeof(plaintext),&response,text,sizeof(text),&text_length,&consumed));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,state.phase);TEST_ASSERT_EQUAL(9U,text_length);TEST_ASSERT_EQUAL(9U,consumed);state.phase=NE2K_LLM_CONNECTION_REQUEST_SENT;client.complete=1U;client.handshake.state=NET_TLS_HANDSHAKE_SERVER_FINISHED_RECEIVED;text_length=9U;consumed=9U;TEST_ASSERT_EQUAL(1,ne2k_llm_connection_poll_sse(&device,&io,&cache,plaintext,sizeof(plaintext),plaintext,sizeof(plaintext),local_ip,&state,&connection,&client,NE2K_LLM_PROVIDER_OLLAMA,plaintext,sizeof(plaintext),&response,text,sizeof(text),&text_length,&consumed));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_STREAMING,state.phase);TEST_ASSERT_EQUAL(0U,text_length);TEST_ASSERT_EQUAL(0U,consumed);state.phase=NE2K_LLM_CONNECTION_RESPONSE_READY;text_length=7U;consumed=7U;TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_connection_poll_sse(&device,&io,&cache,plaintext,sizeof(plaintext),plaintext,sizeof(plaintext),local_ip,&state,&connection,&client,NE2K_LLM_PROVIDER_OLLAMA,plaintext,sizeof(plaintext),&response,text,sizeof(text),&text_length,&consumed));TEST_ASSERT_EQUAL(7U,text_length);TEST_ASSERT_EQUAL(7U,consumed);}

void test_ne2k_https_llm_request_composes_provider_json_and_bearer(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;net_tcp_connection_t connection;ne2k_tls_client_t client={0};net_tls_aes128_gcm_key_block_t block;uint8_t key_material[40]={0},local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={10,0,2,2},tx_frame[512]={0},json[192]={0},request[384]={0},tls_record[448]={0},prompt[]={'h','i'};uint8_t i;int length;for(i=0U;i<sizeof(key_material);i++)key_material[i]=(uint8_t)(i+1U);block=(net_tls_aes128_gcm_key_block_t){key_material,key_material+16U,key_material+32U,key_material+36U};TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));{net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&connection,&syn_ack));}TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&client.session,&block,1U));client.complete=1U;client.handshake.state=NET_TLS_HANDSHAKE_SERVER_FINISHED_RECEIVED;length=ne2k_https_llm_request(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,NE2K_LLM_PROVIDER_OPENAI,json,sizeof(json),request,sizeof(request),"api.example.test","/v1/chat/completions","sk-test-123","gpt-4o-mini",prompt,sizeof(prompt),tls_record,sizeof(tls_record),1U);TEST_ASSERT_GREATER_THAN(0,length);TEST_ASSERT_EQUAL_MEMORY("{\"model\":\"gpt-4o-mini\",\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}],\"stream\":false}",json,82U);TEST_ASSERT_NOT_NULL(strstr((const char*)request,"Authorization: Bearer sk-test-123\r\n"));TEST_ASSERT_NOT_NULL(strstr((const char*)request,"POST /v1/chat/completions HTTP/1.1\r\n"));TEST_ASSERT_EQUAL(1U,client.session.write_sequence);TEST_ASSERT_EQUAL((uint32_t)(101U+length),connection.local_sequence);TEST_ASSERT_NOT_EQUAL(0,ne2k_https_llm_request(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,NE2K_LLM_PROVIDER_OPENAI,json,sizeof(json),request,sizeof(request),"api.example.test","/v1/chat/completions",0,"gpt-4o-mini",prompt,sizeof(prompt),tls_record,sizeof(tls_record),1U));TEST_ASSERT_NOT_EQUAL(0,ne2k_https_llm_request(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,9U,json,sizeof(json),request,sizeof(request),"api.example.test","/",0,"gpt-4o-mini",prompt,sizeof(prompt),tls_record,sizeof(tls_record),1U));}

void test_ne2k_https_llm_close_notify_guards(void){ne2k_device_t device={0};ne2k_io_t io={0};net_arp_cache_t cache={0};net_tcp_connection_t connection={0},before;ne2k_tls_client_t client={0};uint8_t tx[8]={0},local_ip[4]={0},remote_ip[4]={0},tls_record[32]={0};before=connection;TEST_ASSERT_EQUAL(-2,ne2k_https_llm_close_notify(&device,&io,&cache,tx,sizeof(tx),local_ip,remote_ip,&connection,&client,tls_record,sizeof(tls_record),1U));TEST_ASSERT_EQUAL_MEMORY(&before,&connection,sizeof(connection));client.complete=1U;TEST_ASSERT_EQUAL(-2,ne2k_https_llm_close_notify(&device,&io,&cache,tx,sizeof(tx),local_ip,remote_ip,&connection,&client,tls_record,sizeof(tls_record),1U));TEST_ASSERT_EQUAL(0U,client.session.write_sequence);TEST_ASSERT_EQUAL(-1,ne2k_https_llm_close_notify(0,&io,&cache,tx,sizeof(tx),local_ip,remote_ip,&connection,&client,tls_record,sizeof(tls_record),1U));}
void test_ne2k_sse_peer_close_notify_transitions_response_ready(void){ne2k_llm_connection_state_t state={{10U,0U,2U,2U},NE2K_LLM_CONNECTION_STREAMING};TEST_ASSERT_EQUAL(0,ne2k_llm_connection_sse_peer_close_notify(&state));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_RESPONSE_READY,state.phase);TEST_ASSERT_EQUAL(10U,state.remote_ip[0]);TEST_ASSERT_EQUAL(2U,state.remote_ip[3]);TEST_ASSERT_EQUAL(-1,ne2k_llm_connection_sse_peer_close_notify(0));}
void test_ne2k_https_llm_close_notify_emits_tls_alert(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;net_tcp_connection_t connection;ne2k_tls_client_t client={0};net_tls_aes_gcm_session_t server_session;net_tls_aes128_gcm_key_block_t block;net_tcp_view_t view;net_tls_record_view_t record;uint8_t key_material[40]={0},local_mac[6]={2U,0U,0U,0U,0U,1U},remote_mac[6]={0x52U,0x54U,0U,0U,0U,2U},local_ip[4]={10U,0U,2U,15U},remote_ip[4]={10U,0U,2U,2U},tx_frame[128]={0},tls_record[48]={0},plaintext[4]={0};uint8_t index;int length;for(index=0U;index<40U;index++)key_material[index]=(uint8_t)(index+1U);block=(net_tls_aes128_gcm_key_block_t){key_material,key_material+16U,key_material+32U,key_material+36U};TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));{net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&connection,&syn_ack));}TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&client.session,&block,1U));TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&server_session,&block,0U));client.complete=1U;client.handshake.state=NET_TLS_HANDSHAKE_SERVER_FINISHED_RECEIVED;fake.tx_count=0U;length=ne2k_https_llm_close_notify(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,&connection,&client,tls_record,sizeof(tls_record),1U);TEST_ASSERT_EQUAL(31,length);TEST_ASSERT_EQUAL(1U,client.session.write_sequence);TEST_ASSERT_GREATER_THAN(54U,fake.tx_count);TEST_ASSERT_EQUAL(NET_TLS_CONTENT_ALERT,fake.tx_data[54]);TEST_ASSERT_EQUAL(0,net_tcp_parse(fake.tx_data+34U,(uint16_t)(fake.tx_count-34U),&view));TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_open(&server_session,view.payload,view.payload_length,plaintext,sizeof(plaintext),&record));TEST_ASSERT_EQUAL(NET_TLS_CONTENT_ALERT,record.content_type);TEST_ASSERT_EQUAL(1U,record.payload[0]);TEST_ASSERT_EQUAL(0U,record.payload[1]);}

void test_ne2k_sse_resume_request_guards(void){ne2k_device_t device={0};ne2k_io_t io={0};net_arp_cache_t cache={0};net_tcp_connection_t connection={0};ne2k_tls_client_t client={0};net_llm_sse_response_t response={0};uint8_t tx[8]={0},local_ip[4]={0},remote_ip[4]={0},request[128]={0},tls_record[128]={0};TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&response,request,sizeof(request),request,sizeof(request)));TEST_ASSERT_EQUAL(-2,ne2k_https_llm_sse_resume_request(&device,&io,&cache,tx,sizeof(tx),local_ip,remote_ip,&connection,&client,&response,request,sizeof(request),"api.example.test","/v1/chat",tls_record,sizeof(tls_record),1U));response.sse.event_id_valid=1U;TEST_ASSERT_EQUAL(-3,ne2k_https_llm_sse_resume_request(&device,&io,&cache,tx,sizeof(tx),local_ip,remote_ip,&connection,&client,&response,request,sizeof(request),"api.example.test","/v1/chat",tls_record,sizeof(tls_record),1U));}
void test_ne2k_sse_retry_scheduler_adapter(void){ne2k_llm_connection_state_t state={{10U,0U,2U,2U},NE2K_LLM_CONNECTION_STREAMING};net_llm_sse_reconnect_t reconnect={0};net_llm_sse_response_t response={0};uint8_t http_buffer[64]={0},sse_buffer[64]={0};TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&response,http_buffer,sizeof(http_buffer),sse_buffer,sizeof(sse_buffer)));response.sse.retry_valid=1U;response.sse.retry_delay_ms=1500U;TEST_ASSERT_EQUAL(0,net_llm_sse_reconnect_init(&reconnect,2U));TEST_ASSERT_EQUAL(1,ne2k_llm_connection_schedule_sse_retry(&state,&reconnect,&response,503U,10U,2500U,100U));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,state.phase);TEST_ASSERT_EQUAL(1600U,reconnect.next_tick);TEST_ASSERT_EQUAL(-2,ne2k_llm_connection_sse_retry_ready(&(ne2k_llm_connection_state_t){{0U,0U,0U,0U},NE2K_LLM_CONNECTION_STREAMING},&reconnect,100U));TEST_ASSERT_EQUAL(0,ne2k_llm_connection_sse_retry_ready(&state,&reconnect,1599U));TEST_ASSERT_EQUAL(1,ne2k_llm_connection_sse_retry_ready(&state,&reconnect,1600U));state.phase=NE2K_LLM_CONNECTION_IDLE;TEST_ASSERT_EQUAL(-2,ne2k_llm_connection_schedule_sse_retry(&state,&reconnect,&response,503U,10U,25U,1600U));}
void test_ne2k_sse_terminal_classification(void){TEST_ASSERT_EQUAL(NE2K_LLM_SSE_RESULT_PROGRESS,ne2k_llm_connection_classify_sse_result(1,200U));TEST_ASSERT_EQUAL(NE2K_LLM_SSE_RESULT_COMPLETED,ne2k_llm_connection_classify_sse_result(0,200U));TEST_ASSERT_EQUAL(NE2K_LLM_SSE_RESULT_RETRYABLE,ne2k_llm_connection_classify_sse_result(-3,503U));TEST_ASSERT_EQUAL(NE2K_LLM_SSE_RESULT_TRANSPORT,ne2k_llm_connection_classify_sse_result(-1,0U));TEST_ASSERT_EQUAL(NE2K_LLM_SSE_RESULT_TERMINAL,ne2k_llm_connection_classify_sse_result(-3,401U));}
void test_ne2k_sse_event_tick_schedules_retry_and_waits(void){ne2k_llm_connection_state_t state={{10U,0U,2U,2U},NE2K_LLM_CONNECTION_REQUEST_SENT};net_llm_sse_reconnect_t reconnect={0};net_llm_sse_response_t response={0};uint8_t http_buffer[32]={0},sse_buffer[32]={0},text[1]={0};uint16_t text_length=9U,consumed=9U;TEST_ASSERT_EQUAL(0,net_llm_sse_response_init(&response,http_buffer,sizeof(http_buffer),sse_buffer,sizeof(sse_buffer)));TEST_ASSERT_EQUAL(0,net_llm_sse_reconnect_init(&reconnect,2U));TEST_ASSERT_EQUAL(1,ne2k_llm_connection_sse_event_tick(0,0,0,0,0U,0,0U,0,&state,0,0,NE2K_LLM_PROVIDER_OLLAMA,0,0U,&response,text,sizeof(text),&text_length,&consumed,&reconnect,100U,0,0U,0,0,0,0U,1U,10U,25U));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,state.phase);TEST_ASSERT_EQUAL(1U,reconnect.scheduled);TEST_ASSERT_EQUAL(110U,reconnect.next_tick);TEST_ASSERT_EQUAL(0,ne2k_llm_connection_sse_event_tick(0,0,0,0,0U,0,0U,0,&state,0,0,NE2K_LLM_PROVIDER_OLLAMA,0,0U,&response,text,sizeof(text),&text_length,&consumed,&reconnect,109U,0,0U,0,0,0,0U,1U,10U,25U));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,state.phase);TEST_ASSERT_EQUAL(110U,reconnect.next_tick);TEST_ASSERT_EQUAL(-1,ne2k_llm_connection_sse_event_tick(0,0,0,0,0U,0,0U,0,&state,0,0,NE2K_LLM_PROVIDER_OLLAMA,0,0U,&response,text,sizeof(text),&text_length,&consumed,&reconnect,109U,0,0U,0,0,0,0U,1U,0U,25U));}
void test_ne2k_provider_rotation_budget(void){uint8_t provider=NE2K_LLM_PROVIDER_OLLAMA;TEST_ASSERT_EQUAL(0,ne2k_llm_connection_rotate_provider(&provider,2U,1U));TEST_ASSERT_EQUAL(NE2K_LLM_PROVIDER_OLLAMA,provider);TEST_ASSERT_EQUAL(1,ne2k_llm_connection_rotate_provider(&provider,2U,2U));TEST_ASSERT_EQUAL(NE2K_LLM_PROVIDER_OPENAI,provider);TEST_ASSERT_EQUAL(0xffU,ne2k_llm_provider_next(9U));}
void test_ne2k_tcp_segment_bridge(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;uint8_t local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={1,1,1,1},segment[64]={0},frame[128]={0},payload[4]={'T','L','S','!'};uint16_t segment_length;segment_length=(uint16_t)net_tcp_build_data(segment,sizeof(segment),49152U,443U,101U,701U,payload,sizeof(payload));TEST_ASSERT_EQUAL(24U,segment_length);TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));TEST_ASSERT_EQUAL(0,ne2k_tcp_segment(&device,&io,&cache,frame,sizeof(frame),local_ip,remote_ip,segment,segment_length));TEST_ASSERT_EQUAL(0x52,frame[0]);TEST_ASSERT_EQUAL(44U,((uint16_t)frame[16]<<8)|frame[17]);TEST_ASSERT_EQUAL(49152U,((uint16_t)frame[34]<<8)|frame[35]);TEST_ASSERT_EQUAL(443U,((uint16_t)frame[36]<<8)|frame[37]);TEST_ASSERT_EQUAL('T',frame[54]);TEST_ASSERT_EQUAL('!',frame[57]);TEST_ASSERT_EQUAL(0U,net_tcp_checksum_ipv4(local_ip,remote_ip,frame+34U,segment_length));}

void test_ne2k_socket_syn_bridge(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;uint8_t local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={1,1,1,1},frame[128]={0},segment[32]={0};int socket_id;net_socket_reset_all();socket_id=net_socket_open(49152U,443U,100U);TEST_ASSERT_TRUE(socket_id>=0);TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));TEST_ASSERT_EQUAL(0,ne2k_socket_syn(&device,&io,&cache,frame,sizeof(frame),local_ip,remote_ip,socket_id,segment,sizeof(segment)));TEST_ASSERT_EQUAL(0x52,frame[0]);TEST_ASSERT_EQUAL(0x54,frame[1]);TEST_ASSERT_EQUAL(49152U,((uint16_t)frame[34]<<8)|frame[35]);TEST_ASSERT_EQUAL(443U,((uint16_t)frame[36]<<8)|frame[37]);TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN,frame[47]);TEST_ASSERT_EQUAL(100U,((uint32_t)frame[38]<<24)|((uint32_t)frame[39]<<16)|((uint32_t)frame[40]<<8)|frame[41]);TEST_ASSERT_EQUAL(0,net_socket_close(socket_id));}

void test_ne2k_tcp_syn_ack_via_gateway(void){fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;uint8_t local_mac[6]={2,0,0,0,0,1},gateway_ip[4]={10,0,2,2},gateway_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={1,1,1,1},request[128]={0},rx[128]={0},frame[128]={0};TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,gateway_ip,gateway_mac));TEST_ASSERT_EQUAL(0,ne2k_tcp_syn_ack_via(&device,&io,&cache,request,sizeof(request),rx,sizeof(rx),frame,sizeof(frame),local_ip,remote_ip,gateway_ip,443U,49152U,700U,901U,1U));TEST_ASSERT_EQUAL(0x52,frame[0]);TEST_ASSERT_EQUAL(0x54,frame[1]);TEST_ASSERT_EQUAL(0x08,frame[12]);TEST_ASSERT_EQUAL(0x00,frame[13]);TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,frame[47]);}

void test_ne2k_socket_poll_tcp_guards(void){uint8_t frame[64]={0};TEST_ASSERT_EQUAL(-1,ne2k_socket_poll_tcp(0,0,frame,sizeof(frame),0));}

void test_ne2k_socket_tls_poll_empty_is_nonblocking_and_transactional(void){
    fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;
    net_arp_cache_t cache;ne2k_tls_client_t client;net_tcp_connection_t before,after;x509_certificate_view_t anchor={0};
    uint8_t local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={10,0,2,2};
    uint8_t record_buffer[256]={0},handshake_buffer[256]={0},transcript_buffer[512]={0},rx_frame[256]={0},tx_frame[512]={0},tcp_segment[256]={0},flight_records[128]={0},plaintext[128]={0},client_random[32]={0},client_private[32]={0},prf_workspace[256]={0};
    uint32_t rsa_workspace[224]={0},x25519_workspace[136]={0},flight_length=99U;uint16_t consumed=99U;int socket_id;
    net_socket_reset_all();socket_id=net_socket_open(49152U,443U,100U);TEST_ASSERT_TRUE(socket_id>=0);
    {net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_socket_accept_syn_ack(socket_id,&syn_ack));}
    TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));
    TEST_ASSERT_EQUAL(0,ne2k_tls_client_init(&client,record_buffer,sizeof(record_buffer),handshake_buffer,sizeof(handshake_buffer),transcript_buffer,sizeof(transcript_buffer)));
    TEST_ASSERT_EQUAL(0,ne2k_socket_ack(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id));TEST_ASSERT_EQUAL(49152U,((uint16_t)tx_frame[34]<<8)|tx_frame[35]);TEST_ASSERT_EQUAL(NET_TCP_FLAG_ACK,tx_frame[47]);
    TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&before));fake.isr=NE2K_ISR_RESET;
    TEST_ASSERT_EQUAL(1,ne2k_socket_tls_poll(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,&client,client_random,client_private,&anchor,"api.example.test","20260818000000Z",rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));
    TEST_ASSERT_EQUAL(0U,consumed);TEST_ASSERT_EQUAL(0U,flight_length);TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&after));TEST_ASSERT_EQUAL(before.local_sequence,after.local_sequence);TEST_ASSERT_EQUAL(before.remote_sequence,after.remote_sequence);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_IDLE,client.handshake.state);
    TEST_ASSERT_NOT_EQUAL(0,ne2k_socket_tls_poll(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,0,client_random,client_private,&anchor,"api.example.test","20260818000000Z",rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));
    TEST_ASSERT_EQUAL(0,net_socket_close(socket_id));
}

void test_ne2k_socket_tls_poll_retries_flight_without_rx_after_hello_done(void){
    fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;
    net_arp_cache_t cache;ne2k_tls_client_t client;net_tcp_connection_t before,after;x509_certificate_view_t anchor={0};
    uint8_t local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={10,0,2,2};
    uint8_t record_buffer[256]={0},handshake_buffer[256]={0},transcript_buffer[512]={0},rx_frame[256]={0},tx_frame[512]={0},tcp_segment[256]={0},flight_records[128]={0},plaintext[128]={0},client_random[32]={0},client_private[32]={0},prf_workspace[256]={0};
    uint32_t rsa_workspace[224]={0},x25519_workspace[136]={0},flight_length=99U;uint16_t consumed=99U;int socket_id;
    net_socket_reset_all();socket_id=net_socket_open(49152U,443U,100U);TEST_ASSERT_TRUE(socket_id>=0);
    {net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_socket_accept_syn_ack(socket_id,&syn_ack));}
    TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));
    TEST_ASSERT_EQUAL(0,ne2k_tls_client_init(&client,record_buffer,sizeof(record_buffer),handshake_buffer,sizeof(handshake_buffer),transcript_buffer,sizeof(transcript_buffer)));
    client.handshake.state=NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED;client.peer_identity_validated=1U;
    TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&before));fake.isr=NE2K_ISR_RESET;
    TEST_ASSERT_NOT_EQUAL(1,ne2k_socket_tls_poll(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,&client,client_random,client_private,&anchor,"example.com","20260818000000Z",rsa_workspace,224U,x25519_workspace,136U,prf_workspace,sizeof(prf_workspace),tcp_segment,sizeof(tcp_segment),flight_records,sizeof(flight_records),&flight_length,plaintext,sizeof(plaintext),1U,&consumed));
    TEST_ASSERT_EQUAL(0U,consumed);TEST_ASSERT_EQUAL(0U,flight_length);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED,client.handshake.state);
    TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&after));TEST_ASSERT_EQUAL(before.local_sequence,after.local_sequence);
    TEST_ASSERT_EQUAL(0,net_socket_close(socket_id));
}

void test_ne2k_socket_llm_orchestrator_is_transactional(void){
    fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;
    net_tls_aes_gcm_session_t session,before_session;net_tcp_connection_t before,after;net_http_response_accumulator_t accumulator={0};net_http_response_view_t http_response={0};net_llm_sse_response_t sse_response={0};
    uint8_t local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={10,0,2,2},key_material[40]={0},json[256]={0},request[512]={0},record[640]={0},segment[640]={0},tx_frame[1024]={0},rx_frame[256]={0},plaintext[256]={0},prompt[]={'h','i'},text[32]={0};
    net_tls_aes128_gcm_key_block_t block;uint16_t consumed=99U,text_length=99U;uint8_t i;int socket_id,length;
    for(i=0U;i<sizeof(key_material);i++)key_material[i]=(uint8_t)(i+1U);block=(net_tls_aes128_gcm_key_block_t){key_material,key_material+16U,key_material+32U,key_material+36U};
    TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));
    net_socket_reset_all();socket_id=net_socket_open(49152U,443U,100U);TEST_ASSERT_TRUE(socket_id>=0);{net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_socket_accept_syn_ack(socket_id,&syn_ack));}TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&session,&block,1U));
    length=ne2k_socket_llm_request(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,&session,NE2K_LLM_PROVIDER_OLLAMA,0U,json,sizeof(json),request,sizeof(request),"api.example.test","/api/generate",0,"tiny",prompt,sizeof(prompt),record,sizeof(record),segment,sizeof(segment),1U);
    TEST_ASSERT_GREATER_THAN(0,length);TEST_ASSERT_EQUAL(1U,session.write_sequence);TEST_ASSERT_EQUAL(NET_TLS_CONTENT_APPLICATION_DATA,tx_frame[54]);TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&after));TEST_ASSERT_GREATER_THAN(101U,after.local_sequence);
    TEST_ASSERT_EQUAL(0,net_socket_close(socket_id));net_socket_reset_all();socket_id=net_socket_open(49152U,443U,100U);TEST_ASSERT_TRUE(socket_id>=0);{net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_socket_accept_syn_ack(socket_id,&syn_ack));}TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&session,&block,1U));TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&before));before_session=session;fake.isr=NE2K_ISR_RESET;
    TEST_ASSERT_NOT_EQUAL(0,ne2k_socket_llm_request(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,&session,NE2K_LLM_PROVIDER_OLLAMA,0U,json,sizeof(json),request,sizeof(request),"api.example.test","/api/generate",0,"tiny",prompt,sizeof(prompt),record,sizeof(record),segment,sizeof(segment),1U));TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&after));TEST_ASSERT_EQUAL(before.local_sequence,after.local_sequence);TEST_ASSERT_EQUAL(before_session.write_sequence,session.write_sequence);
    consumed=99U;TEST_ASSERT_EQUAL(1,ne2k_socket_llm_poll_response(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,&session,plaintext,sizeof(plaintext),&accumulator,&http_response,&consumed));TEST_ASSERT_EQUAL(0U,consumed);TEST_ASSERT_EQUAL(0,net_socket_connection_snapshot(socket_id,&after));TEST_ASSERT_EQUAL(before.local_sequence,after.local_sequence);
    text_length=99U;consumed=99U;TEST_ASSERT_EQUAL(1,ne2k_socket_llm_poll_sse(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,remote_ip,socket_id,&session,plaintext,sizeof(plaintext),&sse_response,NE2K_LLM_PROVIDER_OLLAMA,text,sizeof(text),&text_length,&consumed));TEST_ASSERT_EQUAL(0U,text_length);TEST_ASSERT_EQUAL(0U,consumed);TEST_ASSERT_EQUAL(0,net_socket_close(socket_id));
}

void test_ne2k_llm_socket_session_phases_are_transactional(void){
    fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;ne2k_llm_socket_session_t socket_session;
    net_tls_aes_gcm_session_t tls_session;net_tls_aes128_gcm_key_block_t block;net_http_response_accumulator_t accumulator={0};net_http_response_view_t http_response={0};net_llm_sse_response_t sse_response={0};
    uint8_t local_mac[6]={2,0,0,0,0,1},remote_mac[6]={0x52,0x54,0,0,0,2},local_ip[4]={10,0,2,15},remote_ip[4]={10,0,2,2},key_material[40]={0},json[256]={0},request[512]={0},record[640]={0},segment[640]={0},tx_frame[1024]={0},rx_frame[256]={0},plaintext[256]={0},prompt[]={'o','k'},text[32]={0};
    uint8_t i;uint16_t consumed=99U,text_length=99U;int socket_id;
    for(i=0U;i<sizeof(key_material);i++)key_material[i]=(uint8_t)(i+1U);block=(net_tls_aes128_gcm_key_block_t){key_material,key_material+16U,key_material+32U,key_material+36U};
    TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));TEST_ASSERT_EQUAL(0,net_arp_cache_put(&cache,remote_ip,remote_mac));
    TEST_ASSERT_EQUAL(0,ne2k_llm_socket_session_init(&socket_session));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,socket_session.state.phase);TEST_ASSERT_EQUAL(-1,socket_session.socket_id);TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_socket_session_attach(&socket_session,0,remote_ip));
    net_socket_reset_all();socket_id=net_socket_open(49152U,443U,100U);TEST_ASSERT_TRUE(socket_id>=0);TEST_ASSERT_EQUAL(0,ne2k_llm_socket_session_attach(&socket_session,socket_id,remote_ip));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_SYN_SENT,socket_session.state.phase);TEST_ASSERT_EQUAL(2U,socket_session.state.remote_ip[3]);
    TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_socket_session_request(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,&socket_session,&tls_session,NE2K_LLM_PROVIDER_OLLAMA,0U,json,sizeof(json),request,sizeof(request),"api.example.test","/api/generate",0,"tiny",prompt,sizeof(prompt),record,sizeof(record),segment,sizeof(segment),1U));
    {net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_socket_accept_syn_ack(socket_id,&syn_ack));}TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&tls_session,&block,1U));socket_session.state.phase=NE2K_LLM_CONNECTION_TLS_COMPLETE;
    TEST_ASSERT_GREATER_THAN(0,ne2k_llm_socket_session_request(&device,&io,&cache,tx_frame,sizeof(tx_frame),local_ip,&socket_session,&tls_session,NE2K_LLM_PROVIDER_OLLAMA,0U,json,sizeof(json),request,sizeof(request),"api.example.test","/api/generate",0,"tiny",prompt,sizeof(prompt),record,sizeof(record),segment,sizeof(segment),1U));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_REQUEST_SENT,socket_session.state.phase);
    fake.isr=NE2K_ISR_RESET;TEST_ASSERT_EQUAL(1,ne2k_llm_socket_session_poll_response(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,&socket_session,&tls_session,plaintext,sizeof(plaintext),&accumulator,&http_response,&consumed));TEST_ASSERT_EQUAL(0U,consumed);TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_REQUEST_SENT,socket_session.state.phase);
    text_length=99U;consumed=99U;TEST_ASSERT_EQUAL(1,ne2k_llm_socket_session_poll_sse(&device,&io,&cache,rx_frame,sizeof(rx_frame),tx_frame,sizeof(tx_frame),local_ip,&socket_session,&tls_session,plaintext,sizeof(plaintext),&sse_response,NE2K_LLM_PROVIDER_OLLAMA,text,sizeof(text),&text_length,&consumed));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_STREAMING,socket_session.state.phase);TEST_ASSERT_EQUAL(0U,text_length);TEST_ASSERT_EQUAL(0U,consumed);
    socket_session.state.phase=NE2K_LLM_CONNECTION_RESPONSE_READY;TEST_ASSERT_EQUAL(0,ne2k_llm_socket_session_reset_for_request(&socket_session));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_TLS_COMPLETE,socket_session.state.phase);TEST_ASSERT_EQUAL(0,net_socket_close(socket_id));
}

void test_ne2k_llm_socket_bootstrap_failure_releases_slot(void){
    fake_ne2k_t fake={0x12,NE2K_ISR_RESET|NE2K_ISR_RDC,0,0};ne2k_io_t io={&fake,fake_inb,fake_outb};ne2k_device_t device;net_arp_cache_t cache;ne2k_llm_socket_session_t session;net_dhcp_lease_t lease={0};
    uint8_t local_mac[6]={2,0,0,0,0,1},local_ip[4]={10,0,2,15},dns_ip[4]={10,0,2,3},arp_request[256]={0},arp_rx[256]={0},frame[1024]={0};int slots[4],i;
    TEST_ASSERT_EQUAL(0,ne2k_probe(&device,0x300U,&io));TEST_ASSERT_EQUAL(0,ne2k_prepare(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_configure_rings(&device,&io));TEST_ASSERT_EQUAL(0,ne2k_set_mac(&device,local_mac));TEST_ASSERT_EQUAL(0,net_arp_cache_init(&cache));net_socket_reset_all();
    TEST_ASSERT_EQUAL(0,ne2k_llm_socket_session_init(&session));TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_socket_session_start(&device,&io,&cache,arp_request,sizeof(arp_request),arp_rx,sizeof(arp_rx),frame,sizeof(frame),local_ip,dns_ip,0x1234U,"api.example.test",1U,1U,49152U,443U,100U,&session));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,session.state.phase);TEST_ASSERT_EQUAL(-1,session.socket_id);
    for(i=0;i<4;i++){slots[i]=net_socket_open((uint16_t)(49152U+i),443U,(uint32_t)(100U+i));TEST_ASSERT_TRUE(slots[i]>=0);}for(i=0;i<4;i++)TEST_ASSERT_EQUAL(0,net_socket_close(slots[i]));
    TEST_ASSERT_NOT_EQUAL(0,ne2k_llm_socket_session_start_dhcp(&device,&io,&cache,arp_request,sizeof(arp_request),arp_rx,sizeof(arp_rx),frame,sizeof(frame),&lease,0x1234U,"api.example.test",1U,1U,49152U,443U,100U,&session));TEST_ASSERT_EQUAL(NE2K_LLM_CONNECTION_IDLE,session.state.phase);TEST_ASSERT_EQUAL(-1,session.socket_id);
    for(i=0;i<4;i++){slots[i]=net_socket_open((uint16_t)(49200U+i),443U,(uint32_t)(200U+i));TEST_ASSERT_TRUE(slots[i]>=0);}for(i=0;i<4;i++)TEST_ASSERT_EQUAL(0,net_socket_close(slots[i]));
}

int main(void) {
    unity_init();
    RUN_TEST(test_probe_and_prepare_use_injected_io);RUN_TEST(test_ne2k_rom_read_and_tx_dma_order);RUN_TEST(test_ne2k_udp_via_gateway_preserves_ipv4_destination);    RUN_TEST(test_ne2k_tcp_syn_via_gateway_preserves_ipv4_destination); RUN_TEST(test_ne2k_tcp_segment_bridge); RUN_TEST(test_ne2k_tcp_syn_ack_via_gateway);RUN_TEST(test_ne2k_socket_syn_bridge);
RUN_TEST(test_ne2k_llm_network_context_lifecycle);RUN_TEST(test_ne2k_llm_network_context_sse_resume_lifecycle);RUN_TEST(test_ne2k_llm_network_context_sse_event_tick_persists_retry);RUN_TEST(test_ne2k_llm_network_context_sse_rotate_provider);RUN_TEST(test_ne2k_llm_network_context_sse_schedule_jittered);RUN_TEST(test_ne2k_llm_network_context_dhcp_renew_if_due);RUN_TEST(test_ne2k_llm_network_context_reconcile_lease);RUN_TEST(test_ne2k_llm_network_context_sse_resume_decide);RUN_TEST(test_ne2k_socket_poll_tcp_guards);RUN_TEST(test_ne2k_dhcp_renew_if_due_guards_transactionally);RUN_TEST(test_ne2k_llm_connection_acquire_start_dhcp_guard_is_transactional);RUN_TEST(test_ne2k_llm_connection_start_dhcp_guard_is_transactional);
    RUN_TEST(test_tcp_receive_copies_bounded_payload);
    RUN_TEST(test_tcp_poll_is_bounded_when_rx_empty);
    RUN_TEST(test_tcp_ack_is_emitted_from_connection_state);
    RUN_TEST(test_rx_extract_publishes_bounded_frame);
    RUN_TEST(test_probe_rejects_missing_reset_ack);
    RUN_TEST(test_ne2k_tls_client_start_and_empty_poll);RUN_TEST(test_ne2k_llm_dns_syn_bootstrap_failure_is_transactional);RUN_TEST(test_ne2k_dhcp_acquire_guard_is_transactional);RUN_TEST(test_ne2k_syn_ack_tls_start_is_transactional);RUN_TEST(test_ne2k_socket_syn_ack_tls_start_is_transactional);RUN_TEST(test_ne2k_socket_tls_poll_empty_is_nonblocking_and_transactional);RUN_TEST(test_ne2k_socket_tls_poll_retries_flight_without_rx_after_hello_done);RUN_TEST(test_ne2k_socket_llm_orchestrator_is_transactional);RUN_TEST(test_ne2k_llm_socket_session_phases_are_transactional);RUN_TEST(test_ne2k_llm_socket_bootstrap_failure_releases_slot);RUN_TEST(test_ne2k_llm_connection_state_phase_guards);RUN_TEST(test_ne2k_llm_connection_tls_phase_guard);RUN_TEST(test_ne2k_llm_connection_response_phase_guard);RUN_TEST(test_ne2k_llm_connection_reset_for_request);RUN_TEST(test_ne2k_llm_connection_sse_phase_guard_and_nonblocking_progress);RUN_TEST(test_ne2k_https_llm_request_composes_provider_json_and_bearer);RUN_TEST(test_ne2k_https_llm_close_notify_guards);RUN_TEST(test_ne2k_sse_peer_close_notify_transitions_response_ready);RUN_TEST(test_ne2k_https_llm_close_notify_emits_tls_alert);RUN_TEST(test_ne2k_sse_resume_request_guards);RUN_TEST(test_ne2k_sse_retry_scheduler_adapter);RUN_TEST(test_ne2k_sse_terminal_classification);RUN_TEST(test_ne2k_sse_event_tick_schedules_retry_and_waits);RUN_TEST(test_ne2k_provider_rotation_budget);
    unity_print_results();
    unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}
