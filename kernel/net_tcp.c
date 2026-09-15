#include "net_tcp.h"

static uint16_t get16(const uint8_t* p) { return (uint16_t)(((uint16_t)p[0] << 8) | p[1]); }
static uint32_t get32(const uint8_t* p) { return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static void put16(uint8_t* p, uint16_t v) { p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static void put32(uint8_t* p, uint32_t v) { p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v; }

static int build_control(uint8_t* segment, uint32_t capacity, uint16_t source_port,
                         uint16_t destination_port, uint32_t sequence,
                         uint32_t acknowledgment, uint8_t flags) {
    uint16_t i;
    if (!segment || capacity < NET_TCP_HEADER_SIZE || source_port == 0U || destination_port == 0U) return -1;
    for (i=0; i<NET_TCP_HEADER_SIZE; ++i) segment[i]=0U;
    put16(segment, source_port); put16(segment+2, destination_port);
    put32(segment+4, sequence); put32(segment+8, acknowledgment);
    segment[12]=0x50U; segment[13]=flags; put16(segment+14, 65535U);
    return NET_TCP_HEADER_SIZE;
}

int net_tcp_build_syn(uint8_t* segment, uint32_t capacity, uint16_t source_port,
                      uint16_t destination_port, uint32_t sequence) {
    return build_control(segment, capacity, source_port, destination_port, sequence, 0U, NET_TCP_FLAG_SYN);
}

int net_tcp_build_syn_ack(uint8_t* segment, uint32_t capacity, uint16_t source_port,
                          uint16_t destination_port, uint32_t sequence, uint32_t acknowledgment) {
    return build_control(segment, capacity, source_port, destination_port, sequence,
                         acknowledgment, NET_TCP_FLAG_SYN | NET_TCP_FLAG_ACK);
}

int net_tcp_build_ack(uint8_t* segment, uint32_t capacity, uint16_t source_port,
                      uint16_t destination_port, uint32_t sequence, uint32_t acknowledgment) {
    return build_control(segment, capacity, source_port, destination_port, sequence, acknowledgment, NET_TCP_FLAG_ACK);
}

int net_tcp_build_fin_ack(uint8_t* segment, uint32_t capacity, uint16_t source_port,
                          uint16_t destination_port, uint32_t sequence, uint32_t acknowledgment) {
    return build_control(segment, capacity, source_port, destination_port, sequence, acknowledgment,
                         NET_TCP_FLAG_FIN | NET_TCP_FLAG_ACK);
}

int net_tcp_build_data(uint8_t* segment, uint32_t capacity, uint16_t source_port,
                       uint16_t destination_port, uint32_t sequence, uint32_t acknowledgment,
                       const uint8_t* payload, uint16_t payload_length) {
    uint16_t i; uint32_t total = NET_TCP_HEADER_SIZE + payload_length;
    if (!segment || (!payload && payload_length != 0U) || total > capacity) return -1;
    if (build_control(segment, capacity, source_port, destination_port, sequence, acknowledgment, NET_TCP_FLAG_ACK) < 0) return -2;
    for (i = 0; i < payload_length; ++i) segment[NET_TCP_HEADER_SIZE + i] = payload[i];
    return (int)total;
}

uint16_t net_tcp_checksum_ipv4(const uint8_t source_ip[4], const uint8_t destination_ip[4],
                               const uint8_t* segment, uint16_t length) {
    uint32_t sum = 0U; uint16_t i;
    if (!source_ip || !destination_ip || !segment || length == 0U) return 0U;
    sum += ((uint16_t)source_ip[0] << 8) | source_ip[1]; sum += ((uint16_t)source_ip[2] << 8) | source_ip[3];
    sum += ((uint16_t)destination_ip[0] << 8) | destination_ip[1]; sum += ((uint16_t)destination_ip[2] << 8) | destination_ip[3];
    sum += NET_TCP_PROTOCOL; sum += length;
    for (i=0; i+1U<length; i+=2U) { sum += get16(segment+i); while (sum>>16) sum=(sum&0xffffU)+(sum>>16); }
    if (length&1U) sum += (uint16_t)segment[length-1U]<<8;
    while (sum>>16) sum=(sum&0xffffU)+(sum>>16);
    return (uint16_t)~sum;
}

int net_tcp_build_syn_ipv4(uint8_t* packet, uint32_t capacity, const uint8_t source_ip[4],
                           const uint8_t destination_ip[4], uint16_t source_port,
                           uint16_t destination_port, uint32_t sequence) {
    uint16_t i, checksum; uint32_t sum=0U;
    if (!packet || !source_ip || !destination_ip || capacity < 40U) return -1;
    for (i=0;i<40U;++i) packet[i]=0U;
    packet[0]=0x45U; put16(packet+2,40U); packet[8]=64U; packet[9]=NET_TCP_PROTOCOL;
    for (i=0;i<4U;++i) { packet[12U+i]=source_ip[i]; packet[16U+i]=destination_ip[i]; }
    if (net_tcp_build_syn(packet+20U,capacity-20U,source_port,destination_port,sequence)<0) return -2;
    checksum=net_tcp_checksum_ipv4(source_ip,destination_ip,packet+20U,20U); put16(packet+36U,checksum);
    for (i=0;i<20U;i+=2U) { sum+=get16(packet+i); while(sum>>16) sum=(sum&0xffffU)+(sum>>16); }
    put16(packet+10U,(uint16_t)~sum); return 40;
}

int net_tcp_build_syn_ack_ipv4(uint8_t* packet, uint32_t capacity, const uint8_t source_ip[4],
                               const uint8_t destination_ip[4], uint16_t source_port,
                               uint16_t destination_port, uint32_t sequence, uint32_t acknowledgment) {
    uint16_t i, checksum; uint32_t sum=0U;
    if (!packet || !source_ip || !destination_ip || capacity < 40U) return -1;
    for (i=0U;i<40U;++i) packet[i]=0U;
    packet[0]=0x45U; put16(packet+2,40U); packet[8]=64U; packet[9]=NET_TCP_PROTOCOL;
    for (i=0U;i<4U;++i) { packet[12U+i]=source_ip[i]; packet[16U+i]=destination_ip[i]; }
    if (net_tcp_build_syn_ack(packet+20U, capacity-20U, source_port, destination_port,
                              sequence, acknowledgment) < 0) return -2;
    checksum=net_tcp_checksum_ipv4(source_ip, destination_ip, packet+20U, 20U); put16(packet+36U, checksum);
    for (i=0U;i<20U;i+=2U) { sum+=get16(packet+i); while (sum>>16) sum=(sum&0xffffU)+(sum>>16); }
    put16(packet+10U, (uint16_t)~sum); return 40;
}

int net_tcp_is_syn_ack_for(const net_tcp_view_t* view,uint16_t local_port,uint16_t remote_port,
                           uint32_t local_sequence,uint32_t* remote_sequence) {
    if (!view || !remote_sequence || local_port==0U || remote_port==0U) return -1;
    if (view->source_port!=remote_port || view->destination_port!=local_port ||
        (view->flags&(NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK))!=(NET_TCP_FLAG_SYN|NET_TCP_FLAG_ACK) ||
        view->acknowledgment!=local_sequence+1U) return -2;
    *remote_sequence=view->sequence; return 0;
}

int net_tcp_connection_open(net_tcp_connection_t* connection,uint16_t local_port,uint16_t remote_port,uint32_t local_sequence) {
    if (!connection || local_port==0U || remote_port==0U) return -1;
    connection->local_port=local_port; connection->remote_port=remote_port;
    connection->local_sequence=local_sequence+1U; connection->remote_sequence=0U;
    connection->pending_payload=0; connection->pending_length=0U; connection->retransmit_count=0U;
    connection->retransmit_limit=0U; connection->receive_window=0xffffU;
    connection->state=NET_TCP_STATE_SYN_SENT; return 0;
}

int net_tcp_connection_listen(net_tcp_connection_t* connection, uint16_t local_port,
                              uint32_t local_sequence) {
    if (!connection || local_port == 0U) return -1;
    connection->local_port = local_port; connection->remote_port = 0U;
    connection->local_sequence = local_sequence; connection->remote_sequence = 0U;
    connection->pending_payload = 0; connection->pending_length = 0U;
    connection->retransmit_count = 0U; connection->retransmit_limit = 0U;
    connection->receive_window = 0xffffU; connection->state = NET_TCP_STATE_LISTEN;
    return 0;
}

int net_tcp_connection_accept_syn(net_tcp_connection_t* connection, const net_tcp_view_t* view) {
    if (!connection || !view || connection->state != NET_TCP_STATE_LISTEN ||
        view->source_port == 0U || view->destination_port != connection->local_port ||
        (view->flags & NET_TCP_FLAG_SYN) == 0U || (view->flags & NET_TCP_FLAG_ACK) != 0U) return -1;
    connection->remote_port = view->source_port; connection->remote_sequence = view->sequence + 1U;
    connection->state = NET_TCP_STATE_SYN_RECEIVED; return 0;
}

int net_tcp_connection_build_syn_ack(const net_tcp_connection_t* connection,
                                     uint8_t* segment, uint32_t capacity) {
    if (!connection || !segment || connection->state != NET_TCP_STATE_SYN_RECEIVED) return -1;
    return net_tcp_build_syn_ack(segment, capacity, connection->local_port,
                                 connection->remote_port, connection->local_sequence,
                                 connection->remote_sequence);
}

int net_tcp_connection_retry_init(net_tcp_connection_retry_t* retry,uint8_t retry_limit){if(!retry)return -1;retry->retry_limit=retry_limit;retry->retries_used=0U;return 0;}
int net_tcp_connection_retry_consume(net_tcp_connection_retry_t* retry){if(!retry)return -1;if(retry->retries_used>=retry->retry_limit)return 0;retry->retries_used++;return 1;}
int net_tcp_connection_retry_reopen(net_tcp_connection_t* connection,net_tcp_connection_retry_t* retry,uint32_t local_sequence){uint16_t local_port,remote_port;int status;if(!connection||!retry)return -1;local_port=connection->local_port;remote_port=connection->remote_port;status=net_tcp_connection_retry_consume(retry);if(status<=0)return status;return net_tcp_connection_open(connection,local_port,remote_port,local_sequence)==0?1:-2;}

int net_tcp_connection_accept_syn_ack(net_tcp_connection_t* connection,const net_tcp_view_t* view) {
    uint32_t remote_sequence;
    if (!connection || !view || connection->state!=NET_TCP_STATE_SYN_SENT) return -1;
    if (net_tcp_is_syn_ack_for(view,connection->local_port,connection->remote_port,
                               connection->local_sequence-1U,&remote_sequence)!=0) return -2;
    connection->remote_sequence=remote_sequence+1U; connection->state=NET_TCP_STATE_ESTABLISHED; return 0;
}

int net_tcp_connection_build_ack(const net_tcp_connection_t* connection,uint8_t* segment,uint32_t capacity) {
    if (!connection || (connection->state != NET_TCP_STATE_ESTABLISHED &&
                        connection->state != NET_TCP_STATE_FIN_WAIT_2 &&
                        connection->state != NET_TCP_STATE_CLOSE_WAIT)) return -1;
    return net_tcp_build_ack(segment,capacity,connection->local_port,connection->remote_port,
                             connection->local_sequence,connection->remote_sequence);
}

int net_tcp_connection_build_data(net_tcp_connection_t* connection,uint8_t* segment,uint32_t capacity,
                                  const uint8_t* payload,uint16_t payload_length,uint8_t retransmit_limit) {
    int length;
    if (!connection || !segment || !payload || payload_length == 0U || connection->state != NET_TCP_STATE_ESTABLISHED) return -1;
    length = net_tcp_build_data(segment, capacity, connection->local_port, connection->remote_port,
                                connection->local_sequence, connection->remote_sequence,
                                payload, payload_length);
    if (length < 0) return -2;
    if (net_tcp_connection_track_send(connection, payload, payload_length, retransmit_limit) != 0) return -3;
    return length;
}

int net_tcp_connection_build_tls_record(net_tcp_connection_t* connection,uint8_t* segment,uint32_t capacity,
                                        uint8_t* record,uint32_t record_capacity,uint8_t content_type,
                                        const uint8_t* payload,uint16_t payload_length,uint8_t retransmit_limit) {
    int record_length;
    if (!record) return -1;
    record_length = net_tls_record_build(record, record_capacity, content_type, payload, payload_length);
    if (record_length < 0 || record_length > 0xffff) return -2;
    return net_tcp_connection_build_data(connection, segment, capacity, record, (uint16_t)record_length, retransmit_limit);
}

int net_tcp_connection_commit_send(net_tcp_connection_t* connection,uint16_t payload_length) {
    if (!connection || connection->state!=NET_TCP_STATE_ESTABLISHED || payload_length == 0U) return -1;
    connection->local_sequence += payload_length; return 0;
}

int net_tcp_connection_accept_data(net_tcp_connection_t* connection,const net_tcp_view_t* view,
                                   uint16_t* accepted_length) {
    if (!connection || !view || !accepted_length || connection->state != NET_TCP_STATE_ESTABLISHED) return -1;
    if (view->source_port != connection->remote_port || view->destination_port != connection->local_port ||
        (view->flags & NET_TCP_FLAG_ACK) == 0U || view->sequence != connection->remote_sequence ||
        view->acknowledgment > connection->local_sequence || view->payload_length > connection->receive_window) return -2;
    *accepted_length = view->payload_length;
    connection->remote_sequence += view->payload_length;
    connection->receive_window = (uint16_t)(connection->receive_window - view->payload_length);
    return 0;
}

int net_tcp_connection_accept_tls_record(net_tcp_connection_t* connection,const net_tcp_view_t* view,
                                         net_tls_record_view_t* record,uint16_t* consumed) {
    uint16_t accepted;
    if (!connection || !view || !record || !consumed) return -1;
    if (net_tls_record_parse_stream(view->payload, view->payload_length, record, consumed) != 0) return -2;
    if (*consumed != view->payload_length) return -3;
    if (net_tcp_connection_accept_data(connection, view, &accepted) != 0) return -4;
    return accepted == *consumed ? 0 : -5;
}
int net_tcp_tls_stream_init(net_tcp_tls_stream_t* stream,uint8_t* record_buffer,uint16_t record_capacity,uint8_t* handshake_buffer,uint16_t handshake_capacity){
    if(!stream)return -1;
    if(net_tls_record_accumulator_init(&stream->record_accumulator,record_buffer,record_capacity)!=0)return -2;
    if(net_tls_handshake_accumulator_init(&stream->handshake_accumulator,handshake_buffer,handshake_capacity)!=0)return -3;
    return 0;
}
int net_tcp_connection_accept_tls_authenticated_fragment(net_tcp_connection_t* connection,const net_tcp_view_t* view,net_tcp_tls_stream_t* stream,net_tls_handshake_t* handshake,const uint8_t client_random[32],net_tls_transcript_t* transcript,uint32_t* rsa_workspace,uint16_t rsa_workspace_length,uint16_t* consumed){
    static net_tcp_connection_t previous_connection;static net_tls_handshake_t previous_handshake;net_tls_record_view_t record;net_tls_handshake_view_t message;uint16_t accepted,previous_transcript_length,previous_record_length,previous_handshake_length;int status;
    if(!connection||!view||!stream||!handshake||!client_random||!transcript||!rsa_workspace||!consumed)return -1;
    previous_connection=*connection;previous_handshake=*handshake;previous_transcript_length=transcript->length;previous_record_length=stream->record_accumulator.length;previous_handshake_length=stream->handshake_accumulator.length;*consumed=0U;
    if(net_tcp_connection_accept_data(connection,view,&accepted)!=0||accepted!=view->payload_length)return -2;
    *consumed=accepted;
    status=net_tls_record_accumulator_feed(&stream->record_accumulator,view->payload,view->payload_length,&record);
    if(status==1)return 1;
    if(status!=0||record.content_type!=NET_TLS_CONTENT_HANDSHAKE)goto rollback;
    status=net_tls_handshake_accumulator_feed(&stream->handshake_accumulator,record.payload,record.payload_length,&message);
    if(status==1){
        if(net_tls_record_accumulator_consume(&stream->record_accumulator,(uint16_t)(NET_TLS_RECORD_HEADER+record.payload_length))!=0)goto rollback;
        return 1;
    }
    if(status!=0)goto rollback;
    status=net_tls_handshake_accept_server_message_authenticated(handshake,client_random,stream->handshake_accumulator.buffer,stream->handshake_accumulator.length,transcript,rsa_workspace,rsa_workspace_length);
    if(status!=0)goto rollback;
    if(net_tls_record_accumulator_consume(&stream->record_accumulator,(uint16_t)(NET_TLS_RECORD_HEADER+record.payload_length))!=0)goto rollback;
    stream->handshake_accumulator.length=0U;
    return 0;
rollback:
    *connection=previous_connection;*handshake=previous_handshake;transcript->length=previous_transcript_length;stream->record_accumulator.length=previous_record_length;stream->handshake_accumulator.length=previous_handshake_length;*consumed=0U;return -3;
}
int net_tcp_tls_stream_accept_pending(net_tcp_tls_stream_t* stream,net_tls_handshake_t* handshake,
                                      const uint8_t client_random[32],net_tls_transcript_t* transcript,
                                      uint32_t* rsa_workspace,uint16_t rsa_workspace_length){
    static net_tls_handshake_t previous_handshake;
    net_tls_record_view_t record;net_tls_handshake_view_t message;
    uint16_t previous_transcript_length,previous_record_length,previous_handshake_length,consumed=0U;int status;
    if(!stream||!handshake||!client_random||!transcript||!rsa_workspace)return -1;
    if(net_tls_record_parse_stream(stream->record_accumulator.buffer,stream->record_accumulator.length,&record,&consumed)!=0)return 1;
    if(record.content_type!=NET_TLS_CONTENT_HANDSHAKE)return -2;
    previous_handshake=*handshake;previous_transcript_length=transcript->length;
    previous_record_length=stream->record_accumulator.length;previous_handshake_length=stream->handshake_accumulator.length;
    status=net_tls_handshake_accumulator_feed(&stream->handshake_accumulator,record.payload,record.payload_length,&message);
    if(status==1){
        if(net_tls_record_accumulator_consume(&stream->record_accumulator,consumed)!=0)goto rollback;
        return 1;
    }
    if(status!=0)goto rollback;
    status=net_tls_handshake_accept_server_message_authenticated(handshake,client_random,stream->handshake_accumulator.buffer,stream->handshake_accumulator.length,transcript,rsa_workspace,rsa_workspace_length);
    if(status!=0)goto rollback;
    if(net_tls_record_accumulator_consume(&stream->record_accumulator,consumed)!=0)goto rollback;
    stream->handshake_accumulator.length=0U;
    return 0;
rollback:
    *handshake=previous_handshake;transcript->length=previous_transcript_length;
    stream->record_accumulator.length=previous_record_length;stream->handshake_accumulator.length=previous_handshake_length;
    return -3;
}
int net_tcp_connection_accept_tls_handshake(net_tcp_connection_t* connection,const net_tcp_view_t* view,
                                            net_tls_handshake_t* handshake,net_tls_transcript_t* transcript,
                                            uint16_t* consumed) {
    net_tcp_connection_t previous; net_tls_record_view_t record; int status;
    if(!connection||!view||!handshake||!consumed)return -1;
    previous=*connection;
    status=net_tcp_connection_accept_tls_record(connection,view,&record,consumed);
    if(status!=0)return -2;
    if(record.content_type!=NET_TLS_CONTENT_HANDSHAKE){*connection=previous;return -3;}
    status=net_tls_handshake_accept_server_message(handshake,record.payload,record.payload_length,transcript);
    if(status!=0){*connection=previous;return -4;}
    return 0;
}
int net_tcp_connection_build_tls_aes_gcm(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,uint8_t* segment,uint32_t segment_capacity,uint8_t* record,uint32_t record_capacity,uint8_t content_type,const uint8_t* plaintext,uint16_t plaintext_length,uint8_t retransmit_limit){
    uint64_t previous_sequence; int record_length,status;
    if(!connection||!session||!record)return -1; previous_sequence=session->write_sequence;
    record_length=net_tls_aes_gcm_session_build(session,record,record_capacity,content_type,plaintext,plaintext_length);
    if(record_length<0)return -2;
    status=net_tcp_connection_build_data(connection,segment,segment_capacity,record,(uint16_t)record_length,retransmit_limit);
    if(status<0){session->write_sequence=previous_sequence;return -3;} return status;
}
int net_tcp_connection_accept_tls_aes_gcm(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,net_tls_record_view_t* out,uint16_t* consumed){
    net_tcp_connection_t previous_connection; uint64_t previous_sequence; net_tls_record_view_t encrypted; uint16_t accepted;
    if(!connection||!session||!view||!plaintext||!out||!consumed)return -1; previous_connection=*connection; previous_sequence=session->read_sequence;
    if(net_tls_record_parse_stream(view->payload,view->payload_length,&encrypted,consumed)!=0||*consumed!=view->payload_length)return -2;
    if(net_tcp_connection_accept_data(connection,view,&accepted)!=0||accepted!=*consumed){*connection=previous_connection;return -3;}
    if(net_tls_aes_gcm_session_open(session,view->payload,view->payload_length,plaintext,plaintext_capacity,out)!=0){*connection=previous_connection;session->read_sequence=previous_sequence;return -4;} return 0;
}

int net_tcp_connection_build_tls_x25519_flight(net_tcp_connection_t* connection,net_tls_handshake_t* handshake,net_tls_x25519_context_t* context,const uint8_t client_private[NET_TLS_X25519_KEY_LENGTH],const uint8_t client_random[32],net_tls_transcript_t* transcript,uint8_t master_secret[48],uint8_t key_block[NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH],net_tls_aes_gcm_session_t* session,uint8_t* segment,uint32_t segment_capacity,uint8_t* records,uint32_t records_capacity,uint32_t* records_length,uint32_t* x25519_workspace,uint16_t x25519_workspace_length,uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint8_t retransmit_limit){
    net_tcp_connection_t previous_connection;net_tls_handshake_t previous_handshake;net_tls_x25519_context_t previous_context;net_tls_aes_gcm_session_t previous_session;uint8_t previous_master[48],previous_key_block[NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH],i;uint16_t previous_transcript_length;uint32_t local_records_length=0U;int status;
    if(!connection||!handshake||!context||!client_private||!client_random||!transcript||!master_secret||!key_block||!session||!segment||!records||!records_length||!x25519_workspace||!prf_workspace)return -1;
    previous_connection=*connection;previous_handshake=*handshake;previous_context=*context;previous_session=*session;previous_transcript_length=transcript->length;for(i=0U;i<48U;i++)previous_master[i]=master_secret[i];for(i=0U;i<NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH;i++)previous_key_block[i]=key_block[i];*records_length=0U;
    status=net_tls_x25519_client_flight_build(handshake,context,client_private,client_random,transcript,master_secret,key_block,session,records,records_capacity,&local_records_length,x25519_workspace,x25519_workspace_length,prf_workspace,prf_workspace_capacity);
    if(status!=0)goto rollback;
    status=net_tcp_connection_build_data(connection,segment,segment_capacity,records,(uint16_t)local_records_length,retransmit_limit);if(status<0)goto rollback;
    *records_length=local_records_length;return status;
rollback:
    *connection=previous_connection;*handshake=previous_handshake;*context=previous_context;*session=previous_session;transcript->length=previous_transcript_length;for(i=0U;i<48U;i++)master_secret[i]=previous_master[i];for(i=0U;i<NET_TLS_AES_128_GCM_KEY_BLOCK_LENGTH;i++)key_block[i]=previous_key_block[i];*records_length=0U;return -2;
}
int net_tcp_connection_accept_tls_postflight(net_tcp_connection_t* connection,const net_tcp_view_t* view,
                                             net_tls_handshake_t* handshake,net_tls_transcript_t* transcript,
                                             const uint8_t expected_verify_data[12],uint16_t* consumed){
    net_tcp_connection_t previous_connection; net_tls_handshake_t previous_handshake; net_tls_record_view_t record; uint16_t previous_transcript_length=0U; int status;
    if(!connection||!view||!handshake||!consumed)return -1;
    previous_connection=*connection; previous_handshake=*handshake; if(transcript)previous_transcript_length=transcript->length;
    status=net_tcp_connection_accept_tls_record(connection,view,&record,consumed);
    if(status!=0)return -2;
    if(record.content_type==NET_TLS_CONTENT_CHANGE_CIPHER_SPEC)status=net_tls_handshake_accept_server_change_cipher_spec(handshake,record.payload,record.payload_length);
    else if(record.content_type==NET_TLS_CONTENT_HANDSHAKE){
        status=net_tls_handshake_accept_server_finished(handshake,record.payload,record.payload_length,expected_verify_data);
        if(status==0&&transcript&&net_tls_transcript_append(transcript,record.payload,record.payload_length)!=0)status=-3;
    } else status=-4;
    if(status!=0){*connection=previous_connection;*handshake=previous_handshake;if(transcript)transcript->length=previous_transcript_length;return -5;}
    return 0;
}

int net_tcp_connection_accept_tls_x25519_postflight(net_tcp_connection_t* connection,net_tls_handshake_t* handshake,net_tls_transcript_t* transcript,const uint8_t master_secret[48],net_tls_aes_gcm_session_t* session,const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,uint8_t* prf_workspace,uint32_t prf_workspace_capacity,uint16_t* consumed){
    net_tcp_connection_t previous_connection;net_tls_handshake_t previous_handshake;net_tls_aes_gcm_session_t previous_session;net_tls_record_view_t record;uint8_t expected[12]={0},transcript_hash[32]={0};uint16_t previous_transcript_length;int status;
    if(!connection||!handshake||!transcript||!master_secret||!session||!view||!plaintext||!prf_workspace||!consumed)return -1;
    previous_connection=*connection;previous_handshake=*handshake;previous_session=*session;previous_transcript_length=transcript->length;
    if(handshake->state==NET_TLS_HANDSHAKE_FINISHED_SENT){
        status=net_tcp_connection_accept_tls_record(connection,view,&record,consumed);
        if(status!=0||record.content_type!=NET_TLS_CONTENT_CHANGE_CIPHER_SPEC||net_tls_handshake_accept_server_change_cipher_spec(handshake,record.payload,record.payload_length)!=0)goto rollback;
        return 0;
    }
    if(handshake->state!=NET_TLS_HANDSHAKE_SERVER_CHANGE_CIPHER_SPEC_RECEIVED)goto rollback;
    status=net_tcp_connection_accept_tls_aes_gcm(connection,session,view,plaintext,plaintext_capacity,&record,consumed);
    if(status!=0||record.content_type!=NET_TLS_CONTENT_HANDSHAKE)goto rollback;
    if(net_tls_server_finished_verify_data(expected,master_secret,transcript,transcript_hash,prf_workspace,prf_workspace_capacity)!=0)goto rollback;
    if(net_tls_handshake_accept_server_finished(handshake,record.payload,record.payload_length,expected)!=0)goto rollback;
    if(net_tls_transcript_append(transcript,record.payload,record.payload_length)!=0)goto rollback;
    return 0;
rollback:
    *connection=previous_connection;*handshake=previous_handshake;*session=previous_session;transcript->length=previous_transcript_length;return -2;
}
int net_tcp_connection_set_receive_window(net_tcp_connection_t* connection,uint16_t receive_window) {
    if (!connection) return -1;
    connection->receive_window = receive_window; return 0;
}

int net_tcp_connection_track_send(net_tcp_connection_t* connection,const uint8_t* payload,
                                  uint16_t payload_length,uint8_t retransmit_limit) {
    if (!connection || !payload || payload_length == 0U || connection->state != NET_TCP_STATE_ESTABLISHED) return -1;
    connection->pending_payload=payload; connection->pending_length=payload_length;
    connection->retransmit_count=0U; connection->retransmit_limit=retransmit_limit; return 0;
}

int net_tcp_connection_retransmit_allowed(const net_tcp_connection_t* connection) {
    if (!connection || !connection->pending_payload || connection->pending_length == 0U) return 0;
    return connection->retransmit_count < connection->retransmit_limit;
}

int net_tcp_connection_note_retransmit(net_tcp_connection_t* connection) {
    if (!net_tcp_connection_retransmit_allowed(connection)) return -1;
    connection->retransmit_count++; return 0;
}
int net_tcp_rto_init(net_tcp_rto_timer_t* timer,uint32_t initial_delay){if(!timer||initial_delay==0U)return -1;timer->deadline=0U;timer->delay=initial_delay;timer->armed=0U;return 0;}
int net_tcp_rto_arm(net_tcp_rto_timer_t* timer,uint32_t now,uint32_t max_delay){uint32_t effective;if(!timer||timer->delay==0U||max_delay<timer->delay)return -1;effective=timer->delay;if(now>0xffffffffU-effective)timer->deadline=0xffffffffU;else timer->deadline=now+effective;timer->armed=1U;return 0;}
int net_tcp_rto_ready(const net_tcp_rto_timer_t* timer,uint32_t now){if(!timer||!timer->armed)return 0;return now>=timer->deadline?1:0;}
int net_tcp_rto_consume(net_tcp_rto_timer_t* timer,net_tcp_connection_t* connection,uint32_t now,uint32_t max_delay){net_tcp_rto_timer_t previous_timer;net_tcp_connection_t previous_connection;uint32_t next_delay;if(!timer||!connection||!net_tcp_rto_ready(timer,now))return 0;if(!net_tcp_connection_retransmit_allowed(connection))return -1;previous_timer=*timer;previous_connection=*connection;if(net_tcp_connection_note_retransmit(connection)!=0)return -1;next_delay=timer->delay>max_delay/2U?max_delay:timer->delay*2U;timer->delay=next_delay;if(net_tcp_rto_arm(timer,now,max_delay)!=0){*timer=previous_timer;*connection=previous_connection;return -2;}return 1;}

int net_tcp_connection_begin_close(net_tcp_connection_t* connection,uint8_t* segment,uint32_t capacity) {
    int length;
    if (!connection || connection->state != NET_TCP_STATE_ESTABLISHED) return -1;
    length = net_tcp_build_fin_ack(segment, capacity, connection->local_port, connection->remote_port,
                                   connection->local_sequence, connection->remote_sequence);
    if (length < 0) return -2;
    connection->local_sequence++; connection->state = NET_TCP_STATE_FIN_WAIT_1;
    return length;
}

int net_tcp_connection_accept_ack(net_tcp_connection_t* connection,const net_tcp_view_t* view) {
    if (!connection || !view || (connection->state != NET_TCP_STATE_ESTABLISHED &&
        connection->state != NET_TCP_STATE_FIN_WAIT_1 && connection->state != NET_TCP_STATE_SYN_RECEIVED)) return -1;
    if (view->source_port != connection->remote_port || view->destination_port != connection->local_port ||
        (view->flags & NET_TCP_FLAG_ACK) == 0U) return -2;
    if (connection->state == NET_TCP_STATE_SYN_RECEIVED) {
        if (view->sequence != connection->remote_sequence || view->acknowledgment != connection->local_sequence + 1U) return -3;
        connection->local_sequence++; connection->state = NET_TCP_STATE_ESTABLISHED; return 0;
    }
    if (view->acknowledgment > connection->local_sequence) return -2;
    if (connection->pending_length != 0U && view->acknowledgment < connection->local_sequence) return -3;
    if (connection->pending_length != 0U) {
        connection->pending_payload = 0; connection->pending_length = 0U;
        connection->retransmit_count = 0U; connection->retransmit_limit = 0U;
    }
    if (connection->state == NET_TCP_STATE_FIN_WAIT_1 && view->acknowledgment == connection->local_sequence)
        connection->state = NET_TCP_STATE_FIN_WAIT_2;
    return 0;
}

int net_tcp_connection_accept_fin(net_tcp_connection_t* connection,const net_tcp_view_t* view) {
    if (!connection || !view || (connection->state != NET_TCP_STATE_ESTABLISHED && connection->state != NET_TCP_STATE_FIN_WAIT_2)) return -1;
    if (view->source_port != connection->remote_port || view->destination_port != connection->local_port ||
        (view->flags & NET_TCP_FLAG_FIN) == 0U || view->sequence != connection->remote_sequence) return -2;
    connection->remote_sequence++;
    connection->state = (connection->state == NET_TCP_STATE_FIN_WAIT_2) ? NET_TCP_STATE_CLOSED : NET_TCP_STATE_CLOSE_WAIT;
    return 0;
}

int net_tcp_parse(const uint8_t* segment,uint32_t length,net_tcp_view_t* out) {
    uint8_t header_words; uint16_t header_size;
    if (!segment || !out || length<NET_TCP_HEADER_SIZE) return -1;
    header_words=(uint8_t)(segment[12]>>4); header_size=(uint16_t)header_words*4U;
    if (header_words<5U || header_size>length) return -2;
    out->source_port=get16(segment); out->destination_port=get16(segment+2);
    if (out->source_port==0U || out->destination_port==0U) return -3;
    out->sequence=get32(segment+4); out->acknowledgment=get32(segment+8); out->flags=(uint8_t)(segment[13]&0x3fU);
    out->payload=segment+header_size; out->payload_length=(uint16_t)(length-header_size); return 0;
}
