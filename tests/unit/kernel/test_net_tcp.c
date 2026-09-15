#include "../../framework/unity.h"
#include "../../../kernel/net_tcp.h"
#include "../../../kernel/net_http_tls.h"
static const uint8_t stream_modulus_der[65]={0U,0xa6U,0x91U,0x82U,0xbbU,0x02U,0x1eU,0x48U,0x2bU,0x19U,0x6cU,0x27U,0xc8U,0x72U,0x10U,0x1cU,0xb1U,0xc4U,0x1bU,0x39U,0xfbU,0x64U,0x21U,0xc1U,0x29U,0xd8U,0x9aU,0x66U,0xd6U,0xe8U,0x44U,0x76U,0x2eU,0x04U,0xc6U,0x7aU,0x50U,0x86U,0xadU,0x4fU,0x82U,0x9cU,0x27U,0x1dU,0xa6U,0xb7U,0x3aU,0xf9U,0x28U,0x93U,0xfeU,0x6dU,0xb9U,0x4cU,0x93U,0x49U,0xcfU,0x2dU,0x7bU,0xb5U,0x2aU,0xefU,0x95U,0x65U,0xefU};
static const uint8_t stream_exponent[3]={1U,0U,1U};
static const uint8_t stream_rsa_oid[13]={0x06U,0x09U,0x2aU,0x86U,0x48U,0x86U,0xf7U,0x0dU,0x01U,0x01U,0x01U,0x05U,0x00U};
static const uint8_t stream_server_random[32]={0};
static const uint8_t stream_signature[64]={0x45U,0xb9U,0x24U,0xa7U,0x7dU,0xe2U,0x2cU,0x2fU,0xd4U,0x6eU,0xfaU,0x5cU,0x98U,0xc8U,0x18U,0xa3U,0x08U,0x85U,0x71U,0xc8U,0x6aU,0x9bU,0xe1U,0x7dU,0x23U,0x99U,0xbdU,0x18U,0x06U,0x0aU,0x90U,0xdfU,0x58U,0x24U,0x4bU,0xfeU,0xf2U,0x8fU,0x5aU,0xc8U,0xb9U,0x4dU,0x51U,0xbdU,0x15U,0x5cU,0xd0U,0xd8U,0x66U,0x01U,0x55U,0xfdU,0x13U,0xb2U,0x09U,0xf9U,0xb8U,0x10U,0x68U,0x8eU,0xf6U,0xcaU,0x29U,0xa6U};
static void init_stream_authenticated_handshake(net_tls_handshake_t* handshake){TEST_ASSERT_EQUAL(0,net_tls_handshake_init(handshake));handshake->state=NET_TLS_HANDSHAKE_CERTIFICATE_RECEIVED;handshake->cipher_suite=NET_TLS_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256;handshake->server_x509_valid=1U;handshake->server_random=stream_server_random;handshake->server_x509.subject_public_key_algorithm=stream_rsa_oid;handshake->server_x509.subject_public_key_algorithm_length=sizeof(stream_rsa_oid);handshake->server_x509.rsa_modulus=stream_modulus_der;handshake->server_x509.rsa_modulus_length=65U;handshake->server_x509.rsa_exponent=stream_exponent;handshake->server_x509.rsa_exponent_length=3U;}
static void build_stream_signed_exchange(uint8_t exchange[108]){uint16_t i;exchange[0]=12U;exchange[3]=104U;exchange[4]=3U;exchange[6]=NET_TLS_NAMED_CURVE_X25519;exchange[7]=NET_TLS_X25519_KEY_LENGTH;for(i=0U;i<NET_TLS_X25519_KEY_LENGTH;i++)exchange[8U+i]=(uint8_t)(i+1U);exchange[40]=NET_TLS_HASH_SHA256;exchange[41]=NET_TLS_SIGNATURE_RSA;exchange[43]=64U;for(i=0U;i<64U;i++)exchange[44U+i]=stream_signature[i];}

void setUp(void) {}
void tearDown(void) {}
void test_passive_listen_syn_ack(void);
void test_passive_listen_rejects_ack_syn(void);

void test_build_and_parse_syn_ack(void) {
    uint8_t segment[32] = {0}; net_tcp_view_t view;
    TEST_ASSERT_EQUAL(20, net_tcp_build_syn(segment, sizeof(segment), 49152, 443, 0x10203040U));
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, 20, &view));
    TEST_ASSERT_EQUAL(49152, view.source_port); TEST_ASSERT_EQUAL(443, view.destination_port);
    TEST_ASSERT_EQUAL(0x10203040U, view.sequence); TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN, view.flags);
    segment[13] = NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK; segment[8]=0x10; segment[9]=0x20; segment[10]=0x30; segment[11]=0x41;
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, 20, &view));
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, view.flags);
    { uint8_t source[4] = {10,0,2,15}; uint8_t destination[4] = {10,0,2,2}; uint16_t checksum = net_tcp_checksum_ipv4(source, destination, segment, 20); TEST_ASSERT_NOT_EQUAL(0, checksum); TEST_ASSERT_EQUAL(checksum, net_tcp_checksum_ipv4(source, destination, segment, 20)); }
    TEST_ASSERT_EQUAL(0x10203041U, view.acknowledgment);
    { uint32_t remote_sequence = 0U;
      view.source_port = 443; view.destination_port = 49152;
      TEST_ASSERT_EQUAL(0, net_tcp_is_syn_ack_for(&view, 49152, 443, 0x10203040U, &remote_sequence));
      TEST_ASSERT_EQUAL(0x10203040U, remote_sequence);
      view.acknowledgment = 7U; TEST_ASSERT_NOT_EQUAL(0, net_tcp_is_syn_ack_for(&view, 49152, 443, 0x10203040U, &remote_sequence)); }
    segment[0] = 0; segment[1] = 0; TEST_ASSERT_NOT_EQUAL(0, net_tcp_parse(segment, 20, &view));
    { uint8_t packet[48] = {0}; uint8_t src[4] = {10,0,2,15}; uint8_t dst[4] = {10,0,2,2};
      TEST_ASSERT_EQUAL(40, net_tcp_build_syn_ipv4(packet, sizeof(packet), src, dst, 49152, 443, 0x10203040U));
      TEST_ASSERT_EQUAL(0x45, packet[0]); TEST_ASSERT_EQUAL(NET_TCP_PROTOCOL, packet[9]);
      TEST_ASSERT_EQUAL(49152, (packet[20] << 8) | packet[21]); TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN, packet[33] & 0x3f); }
    { uint8_t packet[48] = {0}; uint8_t src[4] = {10,0,2,2}; uint8_t dst[4] = {10,0,2,15}; net_tcp_view_t syn_ack;
      TEST_ASSERT_EQUAL(40, net_tcp_build_syn_ack_ipv4(packet, sizeof(packet), src, dst, 443, 49152, 700U, 901U));
      TEST_ASSERT_EQUAL(0, net_tcp_parse(packet + 20U, 20U, &syn_ack));
      TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, syn_ack.flags);
      TEST_ASSERT_EQUAL(700U, syn_ack.sequence); TEST_ASSERT_EQUAL(901U, syn_ack.acknowledgment); }
}

void test_connection_builds_first_ack(void) {
    uint8_t segment[20] = {0}; net_tcp_view_t view; net_tcp_connection_t connection;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT, connection.state);
    view.source_port = 443; view.destination_port = 49152; view.sequence = 700U;
    view.acknowledgment = 101U; view.flags = NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &view));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_ESTABLISHED, connection.state);
    TEST_ASSERT_EQUAL(701U, connection.remote_sequence);
    TEST_ASSERT_EQUAL(20, net_tcp_connection_build_ack(&connection, segment, sizeof(segment)));
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, 20, &view));
    TEST_ASSERT_EQUAL(49152, view.source_port); TEST_ASSERT_EQUAL(443, view.destination_port);
    TEST_ASSERT_EQUAL(101U, view.sequence); TEST_ASSERT_EQUAL(701U, view.acknowledgment);
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_ACK, view.flags);
    view.acknowledgment = 102U;
    TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &view));
}

void test_build_and_parse_ack_payload(void) {
    uint8_t segment[32] = {0}; uint8_t payload[4] = {'P','I','N','G'}; net_tcp_view_t view;
    TEST_ASSERT_EQUAL(24, net_tcp_build_data(segment, sizeof(segment), 49152, 443, 101U, 701U, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, 24, &view));
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_ACK, view.flags); TEST_ASSERT_EQUAL(101U, view.sequence);
    TEST_ASSERT_EQUAL(701U, view.acknowledgment); TEST_ASSERT_EQUAL(4, view.payload_length);
    TEST_ASSERT_EQUAL('P', view.payload[0]); TEST_ASSERT_EQUAL('G', view.payload[3]);
    TEST_ASSERT_NOT_EQUAL(0, net_tcp_build_data(segment, 23, 49152, 443, 101U, 701U, payload, sizeof(payload)));
}

void test_connection_advances_sequences_and_accepts_data(void) {
    net_tcp_connection_t connection; net_tcp_view_t view; uint16_t accepted = 0U;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    view.source_port = 443; view.destination_port = 49152; view.sequence = 700U; view.acknowledgment = 101U;
    view.flags = NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &view));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_commit_send(&connection, 4U));
    TEST_ASSERT_EQUAL(105U, connection.local_sequence);
    view.flags = NET_TCP_FLAG_ACK; view.sequence = 701U; view.acknowledgment = 105U; view.payload_length = 3U;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_data(&connection, &view, &accepted));
    TEST_ASSERT_EQUAL(3U, accepted); TEST_ASSERT_EQUAL(704U, connection.remote_sequence);
    view.sequence = 703U; TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_accept_data(&connection, &view, &accepted));
    TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_commit_send(&connection, 0U));
}

void test_tls_record_is_composed_into_tcp(void) {
    net_tcp_connection_t connection; uint8_t segment[64], record[32]; uint8_t hello[3] = {'H','I','!'}; net_tls_record_view_t view;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    { net_tcp_view_t syn_ack = {443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
      TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &syn_ack)); }
    TEST_ASSERT_EQUAL(28, net_tcp_connection_build_tls_record(&connection, segment, sizeof(segment), record, sizeof(record), NET_TLS_CONTENT_HANDSHAKE, hello, sizeof(hello), 2U));
    TEST_ASSERT_EQUAL(8, connection.pending_length); TEST_ASSERT_EQUAL(record, connection.pending_payload);
    TEST_ASSERT_EQUAL(0, net_tls_record_parse(record, 8, &view)); TEST_ASSERT_EQUAL(3U, view.payload_length);
}

void test_accept_tls_record_on_tcp_view(void) {
    net_tcp_connection_t connection; uint8_t record[16], payload[3] = {'T','L','S'}; net_tcp_view_t view; net_tls_record_view_t parsed; uint16_t consumed;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    view = (net_tcp_view_t){443,49152,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0}; TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection,&view));
    TEST_ASSERT_EQUAL(8, net_tls_record_build(record,sizeof(record),NET_TLS_CONTENT_HANDSHAKE,payload,3));
    view = (net_tcp_view_t){443,49152,701U,101U,NET_TCP_FLAG_ACK,record,8};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_tls_record(&connection,&view,&parsed,&consumed)); TEST_ASSERT_EQUAL(8,consumed); TEST_ASSERT_EQUAL(3,parsed.payload_length); TEST_ASSERT_EQUAL(709U,connection.remote_sequence);
    view.payload_length = 7U; TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_accept_tls_record(&connection,&view,&parsed,&consumed));
}

void test_accept_tls_handshake_transactional(void) {
    net_tcp_connection_t connection; net_tcp_view_t view; net_tls_handshake_t handshake; net_tls_transcript_t transcript; uint8_t transcript_buffer[64]={0},hello[42]={0},record[64]={0},other[16]={0}; uint16_t consumed=0U;
    hello[0]=NET_TLS_HANDSHAKE_SERVER_HELLO; hello[3]=38; hello[4]=3; hello[5]=3; hello[38]=0; hello[39]=0; hello[40]=0x9c; hello[41]=0;
    TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152,443,100U));
    view=(net_tcp_view_t){443,49152,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0}; TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&connection,&view));
    TEST_ASSERT_EQUAL(0,net_tls_handshake_init(&handshake)); TEST_ASSERT_EQUAL(0,net_tls_handshake_note_client_hello(&handshake)); TEST_ASSERT_EQUAL(0,net_tls_transcript_init(&transcript,transcript_buffer,sizeof(transcript_buffer)));
    TEST_ASSERT_EQUAL(47,net_tls_record_build(record,sizeof(record),NET_TLS_CONTENT_HANDSHAKE,hello,sizeof(hello)));
    view=(net_tcp_view_t){443,49152,701U,101U,NET_TCP_FLAG_ACK,record,47};
    TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_tls_handshake(&connection,&view,&handshake,&transcript,&consumed)); TEST_ASSERT_EQUAL(47,consumed); TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_SERVER_HELLO_RECEIVED,handshake.state); TEST_ASSERT_EQUAL(42,transcript.length); TEST_ASSERT_EQUAL(748U,connection.remote_sequence);
    TEST_ASSERT_EQUAL(9,net_tls_record_build(other,sizeof(other),23U,(uint8_t[]){14,0,0,0},4)); view=(net_tcp_view_t){443,49152,748U,101U,NET_TCP_FLAG_ACK,other,9};
    TEST_ASSERT_NOT_EQUAL(0,net_tcp_connection_accept_tls_handshake(&connection,&view,&handshake,&transcript,&consumed)); TEST_ASSERT_EQUAL(748U,connection.remote_sequence);
}

void test_accept_tls_postflight_transactional(void) {
    net_tcp_connection_t connection; net_tcp_view_t view; net_tls_handshake_t handshake; net_tls_transcript_t transcript; uint8_t transcript_buffer[32]={0},record[32]={0},verify[12]={0},bad[12]={0}; uint16_t consumed=0U; uint8_t i;
    for(i=0U;i<12U;i++)verify[i]=i;
    TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152,443,100U)); view=(net_tcp_view_t){443,49152,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0}; TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&connection,&view));
    TEST_ASSERT_EQUAL(0,net_tls_handshake_init(&handshake)); handshake.state=NET_TLS_HANDSHAKE_FINISHED_SENT; TEST_ASSERT_EQUAL(0,net_tls_transcript_init(&transcript,transcript_buffer,sizeof(transcript_buffer)));
    TEST_ASSERT_EQUAL(6,net_tls_change_cipher_spec_build(record,sizeof(record))); view=(net_tcp_view_t){443,49152,701U,101U,NET_TCP_FLAG_ACK,record,6}; TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_tls_postflight(&connection,&view,&handshake,&transcript,verify,&consumed)); TEST_ASSERT_EQUAL(707U,connection.remote_sequence); TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED,handshake.state);
    TEST_ASSERT_EQUAL(21,net_tls_finished_build(record,sizeof(record),verify)); view=(net_tcp_view_t){443,49152,707U,101U,NET_TCP_FLAG_ACK,record,21}; TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_tls_postflight(&connection,&view,&handshake,&transcript,verify,&consumed)); TEST_ASSERT_EQUAL(728U,connection.remote_sequence); TEST_ASSERT_EQUAL(16,transcript.length); TEST_ASSERT_EQUAL(1,net_tls_handshake_is_complete(&handshake));
    TEST_ASSERT_EQUAL(0,net_tls_handshake_init(&handshake)); handshake.state=NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED; view.sequence=728U; TEST_ASSERT_NOT_EQUAL(0,net_tcp_connection_accept_tls_postflight(&connection,&view,&handshake,&transcript,bad,&consumed)); TEST_ASSERT_EQUAL(728U,connection.remote_sequence);
}

void test_tcp_tls_aes_gcm_transport(void) {
    net_tcp_connection_t client_connection,server_connection; net_tcp_view_t syn_ack,view; net_tls_aes128_gcm_key_block_t key_block; net_tls_aes_gcm_session_t client_session,server_session; net_tls_record_view_t record_view; uint8_t key_material[40]={0},segment[64]={0},record[48]={0},plaintext[4]={'p','i','n','g'},decoded[8]={0},i; uint16_t consumed=0U;
    for(i=0U;i<40U;i++)key_material[i]=(uint8_t)(i+1U);key_block=(net_tls_aes128_gcm_key_block_t){key_material,key_material+16U,key_material+32U,key_material+36U};
    TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&client_session,&key_block,1));TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&server_session,&key_block,0));
    TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&client_connection,49152,443,100U));syn_ack=(net_tcp_view_t){443,49152,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&client_connection,&syn_ack));
    TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&server_connection,443,49152,700U));syn_ack=(net_tcp_view_t){49152,443,100U,701U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&server_connection,&syn_ack));
    TEST_ASSERT_EQUAL(53,net_tcp_connection_build_tls_aes_gcm(&client_connection,&client_session,segment,sizeof(segment),record,sizeof(record),NET_TLS_CONTENT_APPLICATION_DATA,plaintext,sizeof(plaintext),1));TEST_ASSERT_EQUAL(1,client_session.write_sequence);TEST_ASSERT_EQUAL(0,net_tcp_parse(segment,53,&view));TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_tls_aes_gcm(&server_connection,&server_session,&view,decoded,sizeof(decoded),&record_view,&consumed));TEST_ASSERT_EQUAL(33,consumed);TEST_ASSERT_EQUAL(1,server_session.read_sequence);TEST_ASSERT_EQUAL('p',decoded[0]);TEST_ASSERT_EQUAL('g',decoded[3]);
}

void test_tcp_tls_x25519_flight_postflight_and_application(void) {net_tcp_connection_t client,server;net_tcp_view_t view,syn_ack;net_tls_handshake_t handshake;net_tls_x25519_context_t context={0};net_tls_transcript_t transcript;net_tls_aes_gcm_session_t client_session,server_session;net_tls_aes128_gcm_key_block_t block;net_tls_record_view_t opened;uint8_t client_private[32]={5U},server_private[32]={11U},server_public[32]={0},client_random[32]={0},server_random[32]={0},transcript_buffer[160]={0},master[48]={0},key_block[40]={0},tcp_segment[160]={0},flight[128]={0},ccs[8]={0},server_record[48]={0},bad_record[48]={0},server_finished[16]={0},server_verify[12]={0},plaintext[32]={0},app_record[48]={0},app_segment[64]={0},app_plain[8]={0},done[4]={14U,0U,0U,0U};uint32_t x_workspace[136]={0},flight_length=99U;uint8_t prf_workspace[256]={0},hash[32]={0},i;uint16_t consumed=0U,accepted=0U;for(i=0U;i<32U;i++){client_random[i]=i;server_random[i]=(uint8_t)(i+32U);}TEST_ASSERT_EQUAL(0,x25519_public_key(server_public,server_private,x_workspace,136U));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&client,49152U,443U,100U));syn_ack=(net_tcp_view_t){443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&client,&syn_ack));TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&server,443U,49152U,700U));syn_ack=(net_tcp_view_t){49152U,443U,100U,701U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&server,&syn_ack));TEST_ASSERT_EQUAL(0,net_tls_handshake_init(&handshake));handshake.state=NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED;handshake.cipher_suite=NET_TLS_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256;handshake.server_named_curve=NET_TLS_NAMED_CURVE_X25519;handshake.server_public_key=server_public;handshake.server_public_key_length=32U;handshake.server_random=server_random;TEST_ASSERT_EQUAL(0,net_tls_transcript_init(&transcript,transcript_buffer,sizeof(transcript_buffer)));TEST_ASSERT_EQUAL(0,net_tls_transcript_append(&transcript,done,sizeof(done)));TEST_ASSERT_NOT_EQUAL(0,net_tcp_connection_build_tls_x25519_flight(&client,&handshake,&context,client_private,client_random,&transcript,master,key_block,&client_session,tcp_segment,112U,flight,sizeof(flight),&flight_length,x_workspace,136U,prf_workspace,sizeof(prf_workspace),1U));TEST_ASSERT_EQUAL(0,flight_length);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED,handshake.state);TEST_ASSERT_EQUAL(4,transcript.length);TEST_ASSERT_EQUAL(113,net_tcp_connection_build_tls_x25519_flight(&client,&handshake,&context,client_private,client_random,&transcript,master,key_block,&client_session,tcp_segment,sizeof(tcp_segment),flight,sizeof(flight),&flight_length,x_workspace,136U,prf_workspace,sizeof(prf_workspace),1U));TEST_ASSERT_EQUAL(93,flight_length);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_FINISHED_SENT,handshake.state);TEST_ASSERT_EQUAL(0,net_tcp_parse(tcp_segment,113U,&view));TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_data(&server,&view,&accepted));TEST_ASSERT_EQUAL(93,accepted);TEST_ASSERT_EQUAL(0,net_tcp_connection_commit_send(&client,flight_length));TEST_ASSERT_EQUAL(194,client.local_sequence);block=(net_tls_aes128_gcm_key_block_t){key_block,key_block+16U,key_block+32U,key_block+36U};TEST_ASSERT_EQUAL(0,net_tls_aes_gcm_session_init(&server_session,&block,0U));TEST_ASSERT_EQUAL(6,net_tls_change_cipher_spec_build(ccs,sizeof(ccs)));view=(net_tcp_view_t){443U,49152U,701U,194U,NET_TCP_FLAG_ACK,ccs,6U};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_tls_x25519_postflight(&client,&handshake,&transcript,master,&client_session,&view,plaintext,sizeof(plaintext),prf_workspace,sizeof(prf_workspace),&consumed));TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED,handshake.state);TEST_ASSERT_EQUAL(0,net_tls_server_finished_verify_data(server_verify,master,&transcript,hash,prf_workspace,sizeof(prf_workspace)));server_finished[0]=NET_TLS_HANDSHAKE_FINISHED;server_finished[3]=12U;for(i=0U;i<12U;i++)server_finished[4U+i]=server_verify[i];TEST_ASSERT_EQUAL(45,net_tls_aes_gcm_session_build(&server_session,server_record,sizeof(server_record),NET_TLS_CONTENT_HANDSHAKE,server_finished,sizeof(server_finished)));for(i=0U;i<45U;i++)bad_record[i]=server_record[i];bad_record[44]^=1U;view=(net_tcp_view_t){443U,49152U,707U,194U,NET_TCP_FLAG_ACK,bad_record,45U};TEST_ASSERT_NOT_EQUAL(0,net_tcp_connection_accept_tls_x25519_postflight(&client,&handshake,&transcript,master,&client_session,&view,plaintext,sizeof(plaintext),prf_workspace,sizeof(prf_workspace),&consumed));TEST_ASSERT_EQUAL(707,client.remote_sequence);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED,handshake.state);view.payload=server_record;TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_tls_x25519_postflight(&client,&handshake,&transcript,master,&client_session,&view,plaintext,sizeof(plaintext),prf_workspace,sizeof(prf_workspace),&consumed));TEST_ASSERT_EQUAL(1,net_tls_handshake_is_complete(&handshake));TEST_ASSERT_EQUAL(752,client.remote_sequence);TEST_ASSERT_EQUAL(1,client_session.read_sequence);TEST_ASSERT_EQUAL(0,net_tcp_connection_commit_send(&server,6U));TEST_ASSERT_EQUAL(0,net_tcp_connection_commit_send(&server,45U));server_session.read_sequence=1U;TEST_ASSERT_EQUAL(52,net_tcp_connection_build_tls_aes_gcm(&client,&client_session,app_segment,sizeof(app_segment),app_record,sizeof(app_record),NET_TLS_CONTENT_APPLICATION_DATA,(uint8_t[]){'o','k','!'},3U,1U));TEST_ASSERT_EQUAL(0,net_tcp_parse(app_segment,52U,&view));TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_tls_aes_gcm(&server,&server_session,&view,app_plain,sizeof(app_plain),&opened,&consumed));TEST_ASSERT_EQUAL(3,opened.payload_length);TEST_ASSERT_EQUAL('o',app_plain[0]);TEST_ASSERT_EQUAL('!',app_plain[2]);}
void test_tcp_tls_authenticated_fragment_stream(void){net_tcp_connection_t connection,bad_connection;net_tcp_view_t view,syn_ack;net_tcp_tls_stream_t stream,bad_stream;net_tls_handshake_t handshake,bad_handshake;net_tls_transcript_t transcript,bad_transcript;uint8_t record[160]={0},bad_record[160]={0},exchange[108]={0},transcript_buffer[192]={0},bad_transcript_buffer[192]={0},record_buffer[160]={0},handshake_buffer[160]={0},bad_record_buffer[160]={0},bad_handshake_buffer[160]={0},client_random[32]={0};uint32_t rsa_workspace[112]={0};uint16_t record_length,consumed=0U;build_stream_signed_exchange(exchange);record_length=(uint16_t)net_tls_record_build(record,sizeof(record),NET_TLS_CONTENT_HANDSHAKE,exchange,sizeof(exchange));TEST_ASSERT_EQUAL(113,record_length);TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));syn_ack=(net_tcp_view_t){443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&connection,&syn_ack));init_stream_authenticated_handshake(&handshake);TEST_ASSERT_EQUAL(0,net_tls_transcript_init(&transcript,transcript_buffer,sizeof(transcript_buffer)));TEST_ASSERT_EQUAL(0,net_tcp_tls_stream_init(&stream,record_buffer,sizeof(record_buffer),handshake_buffer,sizeof(handshake_buffer)));view=(net_tcp_view_t){443U,49152U,701U,101U,NET_TCP_FLAG_ACK,record,73U};TEST_ASSERT_EQUAL(1,net_tcp_connection_accept_tls_authenticated_fragment(&connection,&view,&stream,&handshake,client_random,&transcript,rsa_workspace,112U,&consumed));TEST_ASSERT_EQUAL(73,consumed);TEST_ASSERT_EQUAL(774,connection.remote_sequence);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_CERTIFICATE_RECEIVED,handshake.state);view.sequence=774U;view.payload=record+73U;view.payload_length=(uint16_t)(record_length-73U);TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_tls_authenticated_fragment(&connection,&view,&stream,&handshake,client_random,&transcript,rsa_workspace,112U,&consumed));TEST_ASSERT_EQUAL(40,consumed);TEST_ASSERT_EQUAL(814,connection.remote_sequence);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_SERVER_KEY_EXCHANGE_RECEIVED,handshake.state);TEST_ASSERT_EQUAL(108,transcript.length);for(uint16_t i=0U;i<record_length;i++)bad_record[i]=record[i];bad_record[record_length-1U]^=1U;TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&bad_connection,49152U,443U,100U));TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&bad_connection,&syn_ack));init_stream_authenticated_handshake(&bad_handshake);TEST_ASSERT_EQUAL(0,net_tls_transcript_init(&bad_transcript,bad_transcript_buffer,sizeof(bad_transcript_buffer)));TEST_ASSERT_EQUAL(0,net_tcp_tls_stream_init(&bad_stream,bad_record_buffer,sizeof(bad_record_buffer),bad_handshake_buffer,sizeof(bad_handshake_buffer)));view=(net_tcp_view_t){443U,49152U,701U,101U,NET_TCP_FLAG_ACK,bad_record,record_length};TEST_ASSERT_NOT_EQUAL(0,net_tcp_connection_accept_tls_authenticated_fragment(&bad_connection,&view,&bad_stream,&bad_handshake,client_random,&bad_transcript,rsa_workspace,112U,&consumed));TEST_ASSERT_EQUAL(0,consumed);TEST_ASSERT_EQUAL(701,bad_connection.remote_sequence);TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_CERTIFICATE_RECEIVED,bad_handshake.state);TEST_ASSERT_EQUAL(0,bad_transcript.length);}
void test_receive_window_is_bounded(void) {
    net_tcp_connection_t connection; net_tcp_view_t view; uint16_t accepted = 0U;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    view = (net_tcp_view_t){443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &view));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_set_receive_window(&connection, 3U));
    view.flags = NET_TCP_FLAG_ACK; view.sequence = 701U; view.acknowledgment = 101U; view.payload_length = 2U;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_data(&connection, &view, &accepted));
    TEST_ASSERT_EQUAL(1U, connection.receive_window);
    view.sequence = 703U; view.payload_length = 2U;
    TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_accept_data(&connection, &view, &accepted));
}

void test_build_data_tracks_until_commit(void) {
    net_tcp_connection_t connection; uint8_t segment[24]; uint8_t payload[4] = {'P','I','N','G'};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    { net_tcp_view_t syn_ack = {443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
      TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &syn_ack)); }
    TEST_ASSERT_EQUAL(24, net_tcp_connection_build_data(&connection, segment, sizeof(segment), payload, sizeof(payload), 3U));
    TEST_ASSERT_EQUAL(101U, connection.local_sequence); TEST_ASSERT_EQUAL(payload, connection.pending_payload);
    TEST_ASSERT_EQUAL(0, net_tcp_connection_commit_send(&connection, sizeof(payload)));
    TEST_ASSERT_EQUAL(105U, connection.local_sequence); TEST_ASSERT_EQUAL(1, net_tcp_connection_retransmit_allowed(&connection));
}

void test_ack_confirms_pending_payload(void) {
    net_tcp_connection_t connection; uint8_t payload[2] = {'O','K'}; net_tcp_view_t view;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    view = (net_tcp_view_t){443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &view));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_track_send(&connection, payload, sizeof(payload), 2U));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_commit_send(&connection, sizeof(payload)));
    view = (net_tcp_view_t){443, 49152, 701U, 103U, NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_ack(&connection, &view));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_retransmit_allowed(&connection));
    view.acknowledgment = 104U; TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_accept_ack(&connection, &view));
}

void test_fin_close_transitions(void) {
    net_tcp_connection_t connection; net_tcp_view_t view; uint8_t segment[20] = {0};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    view = (net_tcp_view_t){443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &view));
    TEST_ASSERT_EQUAL(20, net_tcp_connection_begin_close(&connection, segment, sizeof(segment)));
    TEST_ASSERT_EQUAL((NET_TCP_FLAG_FIN | NET_TCP_FLAG_ACK), segment[13]);
    TEST_ASSERT_EQUAL(NET_TCP_STATE_FIN_WAIT_1, connection.state); TEST_ASSERT_EQUAL(102U, connection.local_sequence);
    view.flags = NET_TCP_FLAG_ACK; view.acknowledgment = 102U; view.sequence = 700U;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_ack(&connection, &view));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_FIN_WAIT_2, connection.state);
    view.flags = NET_TCP_FLAG_FIN | NET_TCP_FLAG_ACK; view.sequence = 701U;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_fin(&connection, &view));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_CLOSED, connection.state);
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    view.flags = NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK; view.sequence = 700U; view.acknowledgment = 101U;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &view));
    view.flags = NET_TCP_FLAG_FIN | NET_TCP_FLAG_ACK; view.sequence = 701U; view.acknowledgment = 101U;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_fin(&connection, &view));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_CLOSE_WAIT, connection.state); TEST_ASSERT_EQUAL(702U, connection.remote_sequence);
    TEST_ASSERT_EQUAL(20, net_tcp_connection_build_ack(&connection, segment, sizeof(segment)));
    view.sequence = 704U; TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_accept_fin(&connection, &view));
}

void test_bounded_retransmission_metadata(void) {
    net_tcp_connection_t connection; uint8_t payload[2] = {'O','K'};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&connection, 49152, 443, 100U));
    { net_tcp_view_t syn_ack = {443, 49152, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
      TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&connection, &syn_ack)); }
    TEST_ASSERT_EQUAL(0, net_tcp_connection_track_send(&connection, payload, sizeof(payload), 2U));
    TEST_ASSERT_EQUAL(1, net_tcp_connection_retransmit_allowed(&connection));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_note_retransmit(&connection));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_note_retransmit(&connection));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_retransmit_allowed(&connection));
    TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_note_retransmit(&connection));
}

void test_connection_retry_reopens_bounded_syn(void){net_tcp_connection_t connection;net_tcp_connection_retry_t retry;uint8_t payload[]={'O','K'};TEST_ASSERT_EQUAL(0,net_tcp_connection_open(&connection,49152U,443U,100U));{net_tcp_view_t syn_ack={443U,49152U,700U,101U,NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK,0,0};TEST_ASSERT_EQUAL(0,net_tcp_connection_accept_syn_ack(&connection,&syn_ack));}TEST_ASSERT_EQUAL(0,net_tcp_connection_track_send(&connection,payload,sizeof(payload),2U));TEST_ASSERT_EQUAL(0,net_tcp_connection_retry_init(&retry,1U));TEST_ASSERT_EQUAL(1,net_tcp_connection_retry_reopen(&connection,&retry,900U));TEST_ASSERT_EQUAL(1U,retry.retries_used);TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT,connection.state);TEST_ASSERT_EQUAL(901U,connection.local_sequence);TEST_ASSERT_EQUAL(0U,connection.remote_sequence);TEST_ASSERT_EQUAL(0U,connection.pending_length);TEST_ASSERT_EQUAL(0,net_tcp_connection_retry_reopen(&connection,&retry,1000U));TEST_ASSERT_EQUAL(901U,connection.local_sequence);TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_SENT,connection.state);}

void test_tcp_tls_ecdhe_ecdsa_postflight_and_application(void) {
    uint8_t client_private[32] = {5U}, server_private[32] = {11U}, server_public[32] = {0}, client_random[32] = {0}, server_random[32] = {0};
    uint8_t transcript_buffer[128] = {0}, master[48] = {0}, key_block[40] = {0}, tcp_segment[128] = {0}, flight[128] = {0}, plaintext[32] = {0}, prf_workspace[256] = {0}, hash[32] = {0};
    uint8_t ccs[6] = {0}, server_verify[12] = {0}, server_finished[16] = {0}, server_record[64] = {0}, app_segment[80] = {0}, app_record[64] = {0}, app_plain[8] = {0}, json[96] = {0}, http_request[256] = {0}, http_segment[512] = {0}, http_record[512] = {0}, http_plain[256] = {0};
    uint32_t x_workspace[136] = {0}, flight_length = 0U;
    uint16_t consumed = 0U, accepted = 0U;
    int json_length, request_length, http_status;
    uint8_t i;
    net_tcp_connection_t client, server;
    net_tcp_view_t view, syn_ack;
    net_tls_handshake_t handshake;
    net_tls_transcript_t transcript;
    net_tls_x25519_context_t context = {0};
    net_tls_aes_gcm_session_t client_session, server_session;
    net_tls_aes128_gcm_key_block_t block;
    net_tls_record_view_t opened;
    for (i = 0U; i < 32U; i++) { client_random[i] = i; server_random[i] = (uint8_t)(i + 32U); }
    TEST_ASSERT_EQUAL(0, x25519_public_key(server_public, server_private, x_workspace, 136U));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&client, 49152U, 443U, 100U));
    syn_ack = (net_tcp_view_t){443U, 49152U, 700U, 101U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&client, &syn_ack));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_open(&server, 443U, 49152U, 700U));
    syn_ack = (net_tcp_view_t){49152U, 443U, 100U, 701U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn_ack(&server, &syn_ack));
    TEST_ASSERT_EQUAL(0, net_tls_handshake_init(&handshake));
    handshake.state = NET_TLS_HANDSHAKE_SERVER_HELLO_DONE_RECEIVED;
    handshake.cipher_suite = NET_TLS_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
    handshake.server_named_curve = NET_TLS_NAMED_CURVE_X25519;
    handshake.server_public_key = server_public;
    handshake.server_public_key_length = NET_TLS_X25519_KEY_LENGTH;
    handshake.server_random = server_random;
    TEST_ASSERT_EQUAL(0, net_tls_transcript_init(&transcript, transcript_buffer, sizeof(transcript_buffer)));
    TEST_ASSERT_EQUAL(0, net_tls_transcript_append(&transcript, (uint8_t[]){14U, 0U, 0U, 0U}, 4U));
    TEST_ASSERT_EQUAL(113, net_tcp_connection_build_tls_x25519_flight(&client, &handshake, &context, client_private, client_random, &transcript, master, key_block, &client_session, tcp_segment, sizeof(tcp_segment), flight, sizeof(flight), &flight_length, x_workspace, 136U, prf_workspace, sizeof(prf_workspace), 1U));
    TEST_ASSERT_EQUAL(93U, flight_length);
    TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_FINISHED_SENT, handshake.state);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(tcp_segment, 113U, &view));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_data(&server, &view, &accepted));
    TEST_ASSERT_EQUAL(93U, accepted);
    TEST_ASSERT_EQUAL(0, net_tcp_connection_commit_send(&client, flight_length));
    block = (net_tls_aes128_gcm_key_block_t){key_block, key_block + 16U, key_block + 32U, key_block + 36U};
    TEST_ASSERT_EQUAL(0, net_tls_aes_gcm_session_init(&server_session, &block, 0U));
    TEST_ASSERT_EQUAL(6, net_tls_change_cipher_spec_build(ccs, sizeof(ccs)));
    view = (net_tcp_view_t){443U, 49152U, 701U, client.local_sequence, NET_TCP_FLAG_ACK, ccs, 6U};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_tls_x25519_postflight(&client, &handshake, &transcript, master, &client_session, &view, plaintext, sizeof(plaintext), prf_workspace, sizeof(prf_workspace), &consumed));
    TEST_ASSERT_EQUAL(NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED, handshake.state);
    TEST_ASSERT_EQUAL(0, net_tls_server_finished_verify_data(server_verify, master, &transcript, hash, prf_workspace, sizeof(prf_workspace)));
    server_finished[0] = NET_TLS_HANDSHAKE_FINISHED; server_finished[3] = 12U;
    for (i = 0U; i < 12U; i++) server_finished[4U + i] = server_verify[i];
    TEST_ASSERT_EQUAL(45, net_tls_aes_gcm_session_build(&server_session, server_record, sizeof(server_record), NET_TLS_CONTENT_HANDSHAKE, server_finished, sizeof(server_finished)));
    view = (net_tcp_view_t){443U, 49152U, 707U, client.local_sequence, NET_TCP_FLAG_ACK, server_record, 45U};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_tls_x25519_postflight(&client, &handshake, &transcript, master, &client_session, &view, plaintext, sizeof(plaintext), prf_workspace, sizeof(prf_workspace), &consumed));
    TEST_ASSERT_EQUAL(1, net_tls_handshake_is_complete(&handshake));
    TEST_ASSERT_EQUAL(1U, client_session.read_sequence);
    TEST_ASSERT_EQUAL(0, net_tcp_connection_commit_send(&server, 6U));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_commit_send(&server, 45U));
    server_session.read_sequence = 1U;
    server.local_sequence = client.remote_sequence;
    TEST_ASSERT_EQUAL(52, net_tcp_connection_build_tls_aes_gcm(&client, &client_session, app_segment, sizeof(app_segment), app_record, sizeof(app_record), NET_TLS_CONTENT_APPLICATION_DATA, (uint8_t[]){'o', 'k', '!'}, 3U, 1U));
    TEST_ASSERT_EQUAL(0, net_tcp_parse(app_segment, 52U, &view));
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_tls_aes_gcm(&server, &server_session, &view, app_plain, sizeof(app_plain), &opened, &consumed));
    TEST_ASSERT_EQUAL(3U, opened.payload_length);
    TEST_ASSERT_EQUAL('o', app_plain[0]);
    TEST_ASSERT_EQUAL('!', app_plain[2]);
    TEST_ASSERT_EQUAL(0, net_tcp_connection_commit_send(&client, 32U));
    json_length = net_llm_build_ollama_generate_json(json, sizeof(json), "tinyllama", (const uint8_t*)"hi", 2U);
    TEST_ASSERT_TRUE(json_length > 0);
    request_length = net_http_build_post_json(http_request, sizeof(http_request), "api.example.test", "/api/generate", json, (uint16_t)json_length);
    TEST_ASSERT_TRUE(request_length > 0);
    TEST_ASSERT_TRUE(net_tcp_connection_build_tls_aes_gcm(&client, &client_session, http_segment, sizeof(http_segment), http_record, sizeof(http_record), NET_TLS_CONTENT_APPLICATION_DATA, http_request, (uint16_t)request_length, 1U) > 0);
    TEST_ASSERT_EQUAL(0, net_tcp_parse(http_segment, (uint16_t)(request_length + 49U), &view));
    http_status = net_tcp_connection_accept_tls_aes_gcm(&server, &server_session, &view, http_plain, sizeof(http_plain), &opened, &consumed);
    TEST_ASSERT_EQUAL(0, http_status);
    TEST_ASSERT_EQUAL((uint16_t)request_length, opened.payload_length);
    TEST_ASSERT_EQUAL('P', http_plain[0]);
    TEST_ASSERT_EQUAL('O', http_plain[1]);
    TEST_ASSERT_EQUAL('S', http_plain[2]);
    TEST_ASSERT_EQUAL('T', http_plain[3]);
}


void test_tcp_rto_bounded_timer(void){net_tcp_rto_timer_t timer;net_tcp_connection_t connection;static const uint8_t payload[]={'x'};connection=(net_tcp_connection_t){0};connection.state=NET_TCP_STATE_ESTABLISHED;connection.pending_payload=payload;connection.pending_length=1U;connection.retransmit_limit=3U;TEST_ASSERT_EQUAL(0,net_tcp_rto_init(&timer,10U));TEST_ASSERT_EQUAL(0,net_tcp_rto_arm(&timer,100U,40U));TEST_ASSERT_EQUAL(0,net_tcp_rto_ready(&timer,109U));TEST_ASSERT_EQUAL(1,net_tcp_rto_ready(&timer,110U));TEST_ASSERT_EQUAL(0,net_tcp_rto_consume(&timer,&connection,109U,40U));TEST_ASSERT_EQUAL(1,net_tcp_rto_consume(&timer,&connection,110U,40U));TEST_ASSERT_EQUAL(1U,connection.retransmit_count);TEST_ASSERT_EQUAL(20U,timer.delay);TEST_ASSERT_EQUAL(0,net_tcp_rto_ready(&timer,129U));TEST_ASSERT_EQUAL(1,net_tcp_rto_ready(&timer,130U));TEST_ASSERT_EQUAL(1,net_tcp_rto_consume(&timer,&connection,130U,40U));TEST_ASSERT_EQUAL(2U,connection.retransmit_count);TEST_ASSERT_EQUAL(40U,timer.delay);TEST_ASSERT_EQUAL(-1,net_tcp_rto_init(&timer,0U));TEST_ASSERT_EQUAL(-1,net_tcp_rto_arm(&timer,0U,5U));}
int main(void) {
    unity_init(); RUN_TEST(test_build_and_parse_syn_ack); RUN_TEST(test_connection_builds_first_ack); RUN_TEST(test_build_and_parse_ack_payload); RUN_TEST(test_connection_advances_sequences_and_accepts_data); RUN_TEST(test_tls_record_is_composed_into_tcp); RUN_TEST(test_accept_tls_record_on_tcp_view); RUN_TEST(test_accept_tls_handshake_transactional); RUN_TEST(test_accept_tls_postflight_transactional); RUN_TEST(test_tcp_tls_aes_gcm_transport); RUN_TEST(test_tcp_tls_x25519_flight_postflight_and_application);
    RUN_TEST(test_tcp_tls_ecdhe_ecdsa_postflight_and_application); RUN_TEST(test_tcp_tls_authenticated_fragment_stream); RUN_TEST(test_receive_window_is_bounded); RUN_TEST(test_build_data_tracks_until_commit); RUN_TEST(test_ack_confirms_pending_payload); RUN_TEST(test_fin_close_transitions); RUN_TEST(test_bounded_retransmission_metadata); RUN_TEST(test_connection_retry_reopens_bounded_syn); RUN_TEST(test_tcp_rto_bounded_timer); RUN_TEST(test_passive_listen_syn_ack); RUN_TEST(test_passive_listen_rejects_ack_syn); unity_print_results(); unity_cleanup();
    return (unity_stats.tests_failed == 0) ? 0 : 1;
}

void test_passive_listen_syn_ack(void) {
    net_tcp_connection_t connection = {0};
    net_tcp_view_t syn = {40000U, 8080U, 900U, 0U, NET_TCP_FLAG_SYN, 0, 0};
    net_tcp_view_t ack = {40000U, 8080U, 901U, 1235U, NET_TCP_FLAG_ACK, 0, 0};
    uint8_t segment[NET_TCP_HEADER_SIZE] = {0};
    net_tcp_view_t parsed;
    TEST_ASSERT_EQUAL(0, net_tcp_connection_listen(&connection, 8080U, 1234U));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_LISTEN, connection.state);
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_syn(&connection, &syn));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_SYN_RECEIVED, connection.state);
    TEST_ASSERT_EQUAL(NET_TCP_HEADER_SIZE, net_tcp_connection_build_syn_ack(&connection, segment, sizeof(segment)));
    TEST_ASSERT_EQUAL(0, net_tcp_parse(segment, sizeof(segment), &parsed));
    TEST_ASSERT_EQUAL(NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, parsed.flags);
    TEST_ASSERT_EQUAL(0, net_tcp_connection_accept_ack(&connection, &ack));
    TEST_ASSERT_EQUAL(NET_TCP_STATE_ESTABLISHED, connection.state);
}

void test_passive_listen_rejects_ack_syn(void) {
    net_tcp_connection_t connection = {0};
    TEST_ASSERT_EQUAL(0, net_tcp_connection_listen(&connection, 8080U, 1234U));
    TEST_ASSERT_NOT_EQUAL(0, net_tcp_connection_accept_syn(&connection,
        &(net_tcp_view_t){40000U, 8080U, 900U, 1U, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK, 0, 0}));
}
