#include "net_http_tls.h"

static int net_http_append_char(uint8_t* output,uint16_t capacity,uint16_t* length,uint8_t value){
    if(!output||!length||*length>=capacity)return -1;
    output[*length]=value;*length=(uint16_t)(*length+1U);return 0;
}

static int net_http_append_text(uint8_t* output,uint16_t capacity,uint16_t* length,const char* text){
    uint16_t index=0U;
    if(!text)return -1;
    while(text[index]){
        uint8_t value=(uint8_t)text[index++];
        if(value<0x21U||value>0x7eU)return -2;
        if(net_http_append_char(output,capacity,length,value)!=0)return -3;
    }
    return 0;
}

static int net_http_append_bytes(uint8_t* output,uint16_t capacity,uint16_t* length,const uint8_t* input,uint16_t input_length){
    uint16_t index;
    if(!output||!length||(!input&&input_length)||(uint32_t)*length+input_length>capacity)return -1;
    for(index=0U;index<input_length;index++)output[*length+index]=input[index];
    *length=(uint16_t)(*length+input_length);
    return 0;
}

static int net_http_append_uint16(uint8_t* output,uint16_t capacity,uint16_t* length,uint16_t value){
    uint16_t divisor=10000U;uint8_t emitted=0U;
    while(divisor>0U){
        uint8_t digit=(uint8_t)(value/divisor);
        if(digit||emitted||divisor==1U){if(net_http_append_char(output,capacity,length,(uint8_t)('0'+digit))!=0)return -1;emitted=1U;}
        value=(uint16_t)(value%divisor);divisor=(uint16_t)(divisor/10U);
    }
    return 0;
}

static int net_http_prefix(const uint8_t* input,uint16_t length,uint16_t offset,const char* text){
    uint16_t index=0U;
    while(text[index]){if((uint16_t)(offset+index)>=length||input[offset+index]!=(uint8_t)text[index])return 0;index++;}
    return 1;
}

static int net_http_parse_headers(const uint8_t* input,uint16_t length,uint16_t* status,uint16_t* header_length,uint16_t* content_length,uint8_t* has_content_length){
    uint16_t index,line_start,header_end=0U;uint32_t value;uint8_t seen=0U;
    if(!input||!status||!header_length||!content_length||!has_content_length||length<15U)return -1;
    if(input[0]!='H'||input[1]!='T'||input[2]!='T'||input[3]!='P'||input[4]!='/'||input[5]!='1'||input[6]!='.'||input[7]!='1'||input[8]!=' '||input[9]<'0'||input[9]>'9'||input[10]<'0'||input[10]>'9'||input[11]<'0'||input[11]>'9'||input[12]!=' ')return -2;
    for(index=13U;(uint16_t)(index+3U)<length;index++)if(input[index]=='\r'&&input[index+1U]=='\n'&&input[index+2U]=='\r'&&input[index+3U]=='\n'){header_end=(uint16_t)(index+4U);break;}
    if(header_end==0U)return 1;
    *status=(uint16_t)((uint16_t)(input[9]-'0')*100U+(uint16_t)(input[10]-'0')*10U+(uint16_t)(input[11]-'0'));
    *header_length=header_end;*content_length=0U;*has_content_length=0U;line_start=13U;
    while(line_start<(uint16_t)(header_end-2U)){
        index=line_start;while((uint16_t)(index+1U)<header_end&&!(input[index]=='\r'&&input[index+1U]=='\n'))index++;
        if((uint16_t)(index+1U)>=header_end)return -3;
        if(net_http_prefix(input,index,line_start,"Content-Length:")){
            uint16_t cursor=(uint16_t)(line_start+15U);if(seen)return -4;seen=1U;while(cursor<index&&input[cursor]==' ')cursor++;if(cursor==index)return -5;value=0U;
            while(cursor<index){if(input[cursor]<'0'||input[cursor]>'9')return -6;value=value*10U+(uint32_t)(input[cursor]-'0');if(value>65535U)return -7;cursor++;}
            *content_length=(uint16_t)value;*has_content_length=1U;
        }
        line_start=(uint16_t)(index+2U);
    }
    return 0;
}

int net_http_build_get(uint8_t* request,uint16_t capacity,const char* host,const char* path){
    uint16_t length=0U;
    if(!request||!host||!path||path[0]!='/')return -1;
    if(net_http_append_text(request,capacity,&length,"GET")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,path)!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"HTTP/1.1")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Host:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,host)!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Connection:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"close")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0)return -2;
    return (int)length;
}
int net_http_build_sse_resume_get(uint8_t* request,uint16_t capacity,const char* host,const char* path,const uint8_t* last_event_id,uint8_t last_event_id_length){uint16_t length=0U,index;if(!request||!host||!path||path[0]!='/'||(!last_event_id&&last_event_id_length)||last_event_id_length>NET_LLM_SSE_EVENT_ID_MAX)return -1;for(index=0U;index<last_event_id_length;index++)if(last_event_id[index]<0x21U||last_event_id[index]>0x7eU)return -2;if(net_http_append_text(request,capacity,&length,"GET")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,path)!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"HTTP/1.1")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Host:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,host)!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0)return -3;if(last_event_id_length&&(net_http_append_text(request,capacity,&length,"Last-Event-ID:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_bytes(request,capacity,&length,last_event_id,last_event_id_length)!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0))return -4;if(net_http_append_text(request,capacity,&length,"Connection:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"close")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0)return -5;return (int)length;}

int net_http_tls_build_get(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,uint8_t* tcp_segment,uint32_t tcp_capacity,uint8_t* tls_record,uint32_t tls_capacity,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,uint8_t retransmit_limit){
    int request_length=net_http_build_get(request,request_capacity,host,path);
    if(request_length<0)return -1;
    return net_tcp_connection_build_tls_aes_gcm(connection,session,tcp_segment,tcp_capacity,tls_record,tls_capacity,NET_TLS_CONTENT_APPLICATION_DATA,request,(uint16_t)request_length,retransmit_limit);
}

int net_http_build_post_json(uint8_t* request,uint16_t capacity,const char* host,const char* path,const uint8_t* json,uint16_t json_length){
    uint16_t length=0U;
    if(!request||!host||!path||path[0]!='/'||(!json&&json_length))return -1;
    if(net_http_append_text(request,capacity,&length,"POST")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,path)!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"HTTP/1.1")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Host:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,host)!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Content-Type:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"application/json")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Content-Length:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_uint16(request,capacity,&length,json_length)!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Connection:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"close")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_bytes(request,capacity,&length,json,json_length)!=0)return -2;
    return (int)length;
}

int net_http_build_post_json_bearer(uint8_t* request,uint16_t capacity,const char* host,const char* path,const char* bearer_token,const uint8_t* json,uint16_t json_length){
    uint16_t length=0U;
    if(!request||!host||!path||path[0]!='/'||!bearer_token||!bearer_token[0]||(!json&&json_length))return -1;
    if(net_http_append_text(request,capacity,&length,"POST")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,path)!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"HTTP/1.1")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Host:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,host)!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Authorization:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"Bearer")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,bearer_token)!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Content-Type:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"application/json")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Content-Length:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_uint16(request,capacity,&length,json_length)!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_text(request,capacity,&length,"Connection:")!=0||net_http_append_char(request,capacity,&length,' ')!=0||net_http_append_text(request,capacity,&length,"close")!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_char(request,capacity,&length,'\r')!=0||net_http_append_char(request,capacity,&length,'\n')!=0||net_http_append_bytes(request,capacity,&length,json,json_length)!=0)return -2;
    return (int)length;
}

int net_http_tls_build_post_json(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,uint8_t* tcp_segment,uint32_t tcp_capacity,uint8_t* tls_record,uint32_t tls_capacity,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,const uint8_t* json,uint16_t json_length,uint8_t retransmit_limit){
    int request_length=net_http_build_post_json(request,request_capacity,host,path,json,json_length);
    if(request_length<0)return -1;
    return net_tcp_connection_build_tls_aes_gcm(connection,session,tcp_segment,tcp_capacity,tls_record,tls_capacity,NET_TLS_CONTENT_APPLICATION_DATA,request,(uint16_t)request_length,retransmit_limit);
}
int net_http_tls_build_post_json_bearer_store(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,uint8_t* tcp_segment,uint32_t tcp_capacity,uint8_t* tls_record,uint32_t tls_capacity,uint8_t* request,uint16_t request_capacity,const char* host,const char* path,const net_llm_bearer_store_t* store,const uint8_t* json,uint16_t json_length,uint8_t retransmit_limit){int request_length;if(!store||!store->provisioned)return -1;request_length=net_http_build_post_json_bearer_store(request,request_capacity,host,path,store,json,json_length);if(request_length<0)return -2;return net_tcp_connection_build_tls_aes_gcm(connection,session,tcp_segment,tcp_capacity,tls_record,tls_capacity,NET_TLS_CONTENT_APPLICATION_DATA,request,(uint16_t)request_length,retransmit_limit);}

int net_http_response_parse(const uint8_t* plaintext,uint16_t plaintext_length,net_http_response_view_t* out){
    uint16_t content_length;uint8_t has_content_length;
    if(!out)return -1;
    if(net_http_parse_headers(plaintext,plaintext_length,&out->status_code,&out->header_length,&content_length,&has_content_length)!=0)return -2;
    out->body=plaintext+out->header_length;out->body_length=(uint16_t)(plaintext_length-out->header_length);return 0;
}

int net_http_response_accumulator_init(net_http_response_accumulator_t* accumulator,uint8_t* buffer,uint16_t capacity){
    if(!accumulator||!buffer||capacity==0U)return -1;
    accumulator->buffer=buffer;accumulator->capacity=capacity;accumulator->length=0U;accumulator->header_length=0U;accumulator->expected_body_length=0U;accumulator->status_code=0U;accumulator->headers_complete=0U;return 0;
}

int net_http_response_accumulator_feed(net_http_response_accumulator_t* accumulator,const uint8_t* fragment,uint16_t fragment_length,net_http_response_view_t* out){
    uint16_t index,content_length;uint8_t has_content_length;int status;
    if(!accumulator||!accumulator->buffer||(!fragment&&fragment_length)||!out)return -1;
    if((uint32_t)accumulator->length+fragment_length>accumulator->capacity)return -2;
    for(index=0U;index<fragment_length;index++){
        accumulator->buffer[accumulator->length+index]=fragment[index];
    }
    accumulator->length=(uint16_t)(accumulator->length+fragment_length);
    if(!accumulator->headers_complete){
        status=net_http_parse_headers(accumulator->buffer,accumulator->length,&accumulator->status_code,&accumulator->header_length,&content_length,&has_content_length);
        if(status==1)return 1;
        if(status!=0||!has_content_length)return -3;
        accumulator->expected_body_length=content_length;accumulator->headers_complete=1U;
    }
    if(accumulator->length<(uint16_t)(accumulator->header_length+accumulator->expected_body_length))return 1;
    if(accumulator->length!=(uint16_t)(accumulator->header_length+accumulator->expected_body_length))return -4;
    out->status_code=accumulator->status_code;out->header_length=accumulator->header_length;out->body=accumulator->buffer+accumulator->header_length;out->body_length=accumulator->expected_body_length;return 0;
}

int net_http_tls_open_response(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,net_http_response_view_t* response,uint16_t* consumed){
    net_tcp_connection_t previous_connection;net_tls_aes_gcm_session_t previous_session;net_tls_record_view_t record;int status;
    if(!connection||!session||!view||!plaintext||!response||!consumed)return -1;
    previous_connection=*connection;previous_session=*session;
    status=net_tcp_connection_accept_tls_aes_gcm(connection,session,view,plaintext,plaintext_capacity,&record,consumed);
    if(status!=0||record.content_type!=NET_TLS_CONTENT_APPLICATION_DATA||net_http_response_parse(record.payload,record.payload_length,response)!=0){*connection=previous_connection;*session=previous_session;*consumed=0U;return -2;}
    return 0;
}

int net_http_tls_open_response_stream(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,net_http_response_accumulator_t* accumulator,net_http_response_view_t* response,uint16_t* consumed){
    net_tcp_connection_t previous_connection;net_tls_aes_gcm_session_t previous_session;net_http_response_accumulator_t previous_accumulator;net_tls_record_view_t record;int status;
    if(!connection||!session||!view||!plaintext||!accumulator||!response||!consumed)return -1;
    previous_connection=*connection;previous_session=*session;previous_accumulator=*accumulator;
    status=net_tcp_connection_accept_tls_aes_gcm(connection,session,view,plaintext,plaintext_capacity,&record,consumed);
    if(status!=0)goto rollback;
    if(record.content_type==NET_TLS_CONTENT_ALERT){if(net_tls_close_notify_parse(&record)==0)return NET_HTTP_TLS_STATUS_CLOSE_NOTIFY;goto rollback;}
    if(record.content_type!=NET_TLS_CONTENT_APPLICATION_DATA)goto rollback;
    status=net_http_response_accumulator_feed(accumulator,record.payload,record.payload_length,response);
    if(status>=0)return status;
rollback:
    *connection=previous_connection;*session=previous_session;*accumulator=previous_accumulator;*consumed=0U;return -2;
}

int net_http_tls_open_sse_stream(net_tcp_connection_t* connection,net_tls_aes_gcm_session_t* session,const net_tcp_view_t* view,uint8_t* plaintext,uint16_t plaintext_capacity,net_llm_sse_response_t* response,uint8_t provider,uint8_t* text,uint16_t text_capacity,uint16_t* text_length,uint16_t* consumed){net_tcp_connection_t previous_connection;net_tls_aes_gcm_session_t previous_session;net_llm_sse_response_t previous_response;net_tls_record_view_t record;int status;if(!connection||!session||!view||!plaintext||!response||!text||!text_length||!consumed)return -1;previous_connection=*connection;previous_session=*session;previous_response=*response;status=net_tcp_connection_accept_tls_aes_gcm(connection,session,view,plaintext,plaintext_capacity,&record,consumed);if(status!=0)goto rollback;if(record.content_type==NET_TLS_CONTENT_ALERT){if(net_tls_close_notify_parse(&record)==0)return NET_HTTP_TLS_STATUS_CLOSE_NOTIFY;goto rollback;}if(record.content_type!=NET_TLS_CONTENT_APPLICATION_DATA)goto rollback;status=net_llm_sse_response_feed(response,provider,record.payload,record.payload_length,text,text_capacity,text_length);if(status>=0)return status;rollback:*connection=previous_connection;*session=previous_session;*response=previous_response;*consumed=0U;return -2;}

int net_http_chunked_accumulator_init(net_http_chunked_accumulator_t* accumulator,uint8_t* buffer,uint16_t capacity){
    uint8_t index;
    if(!accumulator||!buffer||capacity==0U)return -1;
    accumulator->buffer=buffer;accumulator->capacity=capacity;accumulator->length=0U;accumulator->raw_length=0U;accumulator->header_length=0U;accumulator->status_code=0U;accumulator->chunk_remaining=0U;accumulator->line_length=0U;accumulator->state=0U;
    for(index=0U;index<sizeof(accumulator->line);index++)accumulator->line[index]=0U;
    return 0;
}

static int net_http_chunked_hex(uint8_t value,uint8_t* digit){
    if(value>='0'&&value<='9'){*digit=(uint8_t)(value-'0');return 0;}
    if(value>='a'&&value<='f'){*digit=(uint8_t)(value-'a'+10U);return 0;}
    if(value>='A'&&value<='F'){*digit=(uint8_t)(value-'A'+10U);return 0;}
    return -1;
}

static int net_http_chunked_has_transfer_encoding(const uint8_t* input,uint16_t header_length){
    uint16_t start=13U,index,cursor;
    while(start<(uint16_t)(header_length-2U)){
        index=start;while((uint16_t)(index+1U)<header_length&&!(input[index]=='\r'&&input[index+1U]=='\n'))index++;
        if(net_http_prefix(input,index,start,"Transfer-Encoding:")){
            cursor=(uint16_t)(start+18U);while(cursor<index&&input[cursor]==' ')cursor++;
            if((uint16_t)(index-cursor)==7U&&input[cursor]=='c'&&input[cursor+1U]=='h'&&input[cursor+2U]=='u'&&input[cursor+3U]=='n'&&input[cursor+4U]=='k'&&input[cursor+5U]=='e'&&input[cursor+6U]=='d')return 1;
        }
        start=(uint16_t)(index+2U);
    }
    return 0;
}

static int net_http_chunked_byte(net_http_chunked_accumulator_t* accumulator,uint8_t value){
    uint8_t digit;uint32_t next;
    if(accumulator->state==1U){
        if(value=='\r'){if(accumulator->line_length==0U)return -1;accumulator->chunk_remaining=0U;for(digit=0U;digit<accumulator->line_length;digit++){uint8_t hex;if(net_http_chunked_hex(accumulator->line[digit],&hex)!=0)return -2;next=(accumulator->chunk_remaining<<4U)|hex;if(next>65535U)return -3;accumulator->chunk_remaining=next;}accumulator->line_length=0U;accumulator->state=2U;return 1;}
        if(accumulator->line_length>=sizeof(accumulator->line)||net_http_chunked_hex(value,&digit)!=0)return -4;
        accumulator->line[accumulator->line_length++]=value;
        return 1;
    }
    if(accumulator->state==2U){if(value!='\n')return -5;accumulator->state=accumulator->chunk_remaining==0U?6U:3U;return 1;}
    if(accumulator->state==3U){if(accumulator->length>=accumulator->capacity)return -6;accumulator->buffer[accumulator->length++]=value;if(--accumulator->chunk_remaining==0U)accumulator->state=4U;return 1;}
    if(accumulator->state==4U){if(value!='\r')return -7;accumulator->state=5U;return 1;}
    if(accumulator->state==5U){if(value!='\n')return -8;accumulator->state=1U;return 1;}
    if(accumulator->state==6U){if(value!='\r')return -9;accumulator->state=7U;return 1;}
    if(accumulator->state==7U){if(value!='\n')return -10;accumulator->state=8U;return 0;}
    return -11;
}

int net_http_chunked_accumulator_feed(net_http_chunked_accumulator_t* accumulator,const uint8_t* fragment,uint16_t fragment_length,net_http_response_view_t* out){
    uint16_t index,header_length,content_length;uint8_t has_content_length;int status;
    if(!accumulator||!accumulator->buffer||(!fragment&&fragment_length)||!out)return -1;
    if(accumulator->state==8U)return -2;
    if(accumulator->state==0U){
        if((uint32_t)accumulator->raw_length+fragment_length>accumulator->capacity)return -3;
        for(index=0U;index<fragment_length;index++)accumulator->buffer[accumulator->raw_length+index]=fragment[index];
        accumulator->raw_length=(uint16_t)(accumulator->raw_length+fragment_length);
        status=net_http_parse_headers(accumulator->buffer,accumulator->raw_length,&accumulator->status_code,&header_length,&content_length,&has_content_length);
        if(status==1)return 1;
        if(status!=0||has_content_length||!net_http_chunked_has_transfer_encoding(accumulator->buffer,header_length))return -4;
        accumulator->header_length=header_length;accumulator->length=0U;accumulator->state=1U;
        for(index=header_length;index<accumulator->raw_length;index++){status=net_http_chunked_byte(accumulator,accumulator->buffer[index]);if(status<0)return -5;if(status==0)break;}
        accumulator->raw_length=0U;
    }else{
        for(index=0U;index<fragment_length;index++){status=net_http_chunked_byte(accumulator,fragment[index]);if(status<0)return -6;if(status==0)break;}
    }
    if(accumulator->state!=8U)return 1;
    out->status_code=accumulator->status_code;out->header_length=accumulator->header_length;out->body=accumulator->buffer;out->body_length=accumulator->length;return 0;
}

static void net_llm_sse_discard(net_llm_sse_accumulator_t* accumulator,uint16_t count){uint16_t index,remaining=(uint16_t)(accumulator->length-count);for(index=0U;index<remaining;index++)accumulator->buffer[index]=accumulator->buffer[count+index];accumulator->length=remaining;}
static int net_llm_sse_event_end(const net_llm_sse_accumulator_t* accumulator,uint16_t* event_length,uint16_t* consumed){uint16_t index;if(!accumulator||!event_length||!consumed)return -1;for(index=0U;index<accumulator->length;index++){if((uint16_t)(index+1U)<accumulator->length&&accumulator->buffer[index]=='\n'&&accumulator->buffer[index+1U]=='\n'){*event_length=index;*consumed=(uint16_t)(index+2U);return 0;}if((uint16_t)(index+3U)<accumulator->length&&accumulator->buffer[index]=='\r'&&accumulator->buffer[index+1U]=='\n'&&accumulator->buffer[index+2U]=='\r'&&accumulator->buffer[index+3U]=='\n'){*event_length=index;*consumed=(uint16_t)(index+4U);return 0;}}return 1;}
int net_llm_sse_accumulator_init(net_llm_sse_accumulator_t* accumulator,uint8_t* buffer,uint16_t capacity){uint8_t index;if(!accumulator||!buffer||capacity==0U)return -1;accumulator->buffer=buffer;accumulator->capacity=capacity;accumulator->length=0U;accumulator->done=0U;accumulator->event_id_length=0U;accumulator->event_id_valid=0U;accumulator->retry_delay_ms=0U;accumulator->retry_valid=0U;for(index=0U;index<NET_LLM_SSE_EVENT_ID_MAX;index++)accumulator->event_id[index]=0U;return 0;}
int net_llm_sse_accumulator_feed(net_llm_sse_accumulator_t* accumulator,uint8_t provider,const uint8_t* fragment,uint16_t fragment_length,uint8_t* text,uint16_t text_capacity,uint16_t* text_length){uint16_t event_length,consumed,start,index,delta_length,produced=0U;int status;if(!accumulator||!accumulator->buffer||(!fragment&&fragment_length)||!text||!text_length)return -1;if(provider>1U)return -2;if(accumulator->done){*text_length=0U;return 0;}if((uint32_t)accumulator->length+fragment_length>accumulator->capacity)return -3;for(index=0U;index<fragment_length;index++)accumulator->buffer[accumulator->length+index]=fragment[index];accumulator->length=(uint16_t)(accumulator->length+fragment_length);while((status=net_llm_sse_event_end(accumulator,&event_length,&consumed))==0){while(event_length>0U&&accumulator->buffer[event_length-1U]=='\r')event_length--;{uint16_t line_start=0U,line_end,payload=5U;while(line_start<event_length){line_end=line_start;while(line_end<event_length&&accumulator->buffer[line_end]!='\n')line_end++;if(line_end>line_start&&accumulator->buffer[line_end-1U]=='\r')line_end--;if((uint16_t)(line_end-line_start)>=6U&&accumulator->buffer[line_start]=='r'&&accumulator->buffer[line_start+1U]=='e'&&accumulator->buffer[line_start+2U]=='t'&&accumulator->buffer[line_start+3U]=='r'&&accumulator->buffer[line_start+4U]=='y'&&accumulator->buffer[line_start+5U]==':'){uint16_t retry_start=(uint16_t)(line_start+6U),digit_count=0U;uint32_t retry_value=0U;while(retry_start<line_end&&accumulator->buffer[retry_start]==' ')retry_start++;while(retry_start<line_end){uint8_t digit=accumulator->buffer[retry_start];if(digit<'0'||digit>'9'||retry_value>(NET_LLM_SSE_RETRY_MAX_MS-(uint32_t)(digit-'0'))/10U)return -9;retry_value=retry_value*10U+(uint32_t)(digit-'0');retry_start++;digit_count++;}if(digit_count==0U)return -9;accumulator->retry_delay_ms=retry_value;accumulator->retry_valid=1U;line_start=(uint16_t)(line_end<event_length?line_end+1U:line_end);continue;}if((uint16_t)(line_end-line_start)>=3U&&accumulator->buffer[line_start]=='i'&&accumulator->buffer[line_start+1U]=='d'&&accumulator->buffer[line_start+2U]==':'){uint16_t id_start=(uint16_t)(line_start+3U),id_length;while(id_start<line_end&&accumulator->buffer[id_start]==' ')id_start++;id_length=(uint16_t)(line_end-id_start);if(id_length>NET_LLM_SSE_EVENT_ID_MAX)return -8;for(index=0U;index<id_length;index++){if(accumulator->buffer[id_start+index]<0x21U||accumulator->buffer[id_start+index]>0x7eU)return -8;accumulator->event_id[index]=accumulator->buffer[id_start+index];}for(index=(uint8_t)id_length;index<NET_LLM_SSE_EVENT_ID_MAX;index++)accumulator->event_id[index]=0U;accumulator->event_id_length=(uint8_t)id_length;accumulator->event_id_valid=1U;line_start=(uint16_t)(line_end<event_length?line_end+1U:line_end);continue;}if((uint16_t)(line_end-line_start)<5U||accumulator->buffer[line_start]!='d'||accumulator->buffer[line_start+1U]!='a'||accumulator->buffer[line_start+2U]!='t'||accumulator->buffer[line_start+3U]!='a'||accumulator->buffer[line_start+4U]!=':')return -4;start=(uint16_t)(line_start+5U);while(start<line_end&&accumulator->buffer[start]==' ')start++;while(start<line_end)accumulator->buffer[payload++]=accumulator->buffer[start++];line_start=(uint16_t)(line_end<event_length?line_end+1U:line_end);}event_length=payload;accumulator->buffer[0]='d';accumulator->buffer[1]='a';accumulator->buffer[2]='t';accumulator->buffer[3]='a';accumulator->buffer[4]=':';}if(event_length<5U||accumulator->buffer[0]!='d'||accumulator->buffer[1]!='a'||accumulator->buffer[2]!='t'||accumulator->buffer[3]!='a'||accumulator->buffer[4]!=':')return -4;start=5U;while(start<event_length&&accumulator->buffer[start]==' ')start++;if((uint16_t)(event_length-start)==6U&&accumulator->buffer[start]=='['&&accumulator->buffer[start+1U]=='D'&&accumulator->buffer[start+2U]=='O'&&accumulator->buffer[start+3U]=='N'&&accumulator->buffer[start+4U]=='E'&&accumulator->buffer[start+5U]==']'){accumulator->done=1U;net_llm_sse_discard(accumulator,consumed);continue;}delta_length=0U;status=provider==0U?net_llm_ollama_response_extract(accumulator->buffer+start,(uint16_t)(event_length-start),text+produced,(uint16_t)(text_capacity-produced),&delta_length):net_llm_openai_response_extract(accumulator->buffer+start,(uint16_t)(event_length-start),text+produced,(uint16_t)(text_capacity-produced),&delta_length);if(status==-9&&provider==1U)delta_length=0U;else if(status!=0)return -5;if((uint32_t)produced+delta_length>text_capacity)return -6;produced=(uint16_t)(produced+delta_length);net_llm_sse_discard(accumulator,consumed);}if(status<0)return -7;*text_length=produced;return produced>0U||accumulator->done?0:1;}
int net_llm_sse_response_init(net_llm_sse_response_t* response,uint8_t* http_buffer,uint16_t http_capacity,uint8_t* sse_buffer,uint16_t sse_capacity){if(!response)return -1;if(net_http_chunked_accumulator_init(&response->http,http_buffer,http_capacity)!=0)return -2;if(net_llm_sse_accumulator_init(&response->sse,sse_buffer,sse_capacity)!=0)return -3;response->decoded_consumed=0U;return 0;}
int net_llm_sse_response_feed(net_llm_sse_response_t* response,uint8_t provider,const uint8_t* fragment,uint16_t fragment_length,uint8_t* text,uint16_t text_capacity,uint16_t* text_length){net_http_response_view_t http_response;uint16_t previous_length,decoded_length;int status,sse_status;if(!response||(!fragment&&fragment_length)||!text||!text_length)return -1;previous_length=response->http.length;status=net_http_chunked_accumulator_feed(&response->http,fragment,fragment_length,&http_response);if(status<0)return -2;if(response->http.state!=0U&&(response->http.status_code<200U||response->http.status_code>=300U))return -3;if(response->http.length<previous_length)return -4;decoded_length=(uint16_t)(response->http.length-previous_length);if(decoded_length==0U){*text_length=0U;return (response->sse.done||status==0)?0:1;}sse_status=net_llm_sse_accumulator_feed(&response->sse,provider,response->http.buffer+previous_length,decoded_length,text,text_capacity,text_length);if(sse_status<0)return -5;response->decoded_consumed=response->http.length;return (response->sse.done||response->http.state==8U)?0:1;}
int net_llm_sse_response_feed_transactional(net_llm_sse_response_t* response,uint8_t provider,const uint8_t* fragment,uint16_t fragment_length,uint8_t* text,uint16_t text_capacity,uint16_t* text_length){net_llm_sse_response_t before;uint16_t before_text_length;int status;if(!response||!text_length)return -1;before=*response;before_text_length=*text_length;status=net_llm_sse_response_feed(response,provider,fragment,fragment_length,text,text_capacity,text_length);if(status<0){*response=before;*text_length=before_text_length;}return status;}
int net_llm_sse_response_reset(net_llm_sse_response_t* response){uint8_t* hb,*sb,saved_id[NET_LLM_SSE_EVENT_ID_MAX];uint16_t hc,sc;uint8_t saved_length,saved_valid,index;if(!response)return -1;hb=response->http.buffer;hc=response->http.capacity;sb=response->sse.buffer;sc=response->sse.capacity;saved_length=response->sse.event_id_length;saved_valid=response->sse.event_id_valid;for(index=0U;index<NET_LLM_SSE_EVENT_ID_MAX;index++)saved_id[index]=response->sse.event_id[index];if(net_http_chunked_accumulator_init(&response->http,hb,hc)!=0)return -2;if(net_llm_sse_accumulator_init(&response->sse,sb,sc)!=0)return -3;for(index=0U;index<NET_LLM_SSE_EVENT_ID_MAX;index++)response->sse.event_id[index]=saved_id[index];response->sse.event_id_length=saved_length;response->sse.event_id_valid=saved_valid;response->decoded_consumed=0U;return 0;}
int net_llm_sse_build_resume_get(uint8_t* request,uint16_t capacity,const char* host,const char* path,const net_llm_sse_response_t* response){if(!response||!response->sse.event_id_valid)return -1;return net_http_build_sse_resume_get(request,capacity,host,path,response->sse.event_id,response->sse.event_id_length);}
int net_llm_sse_reconnect_init(net_llm_sse_reconnect_t* reconnect,uint8_t retry_limit){if(!reconnect)return -1;reconnect->retries_used=0U;reconnect->retry_limit=retry_limit;reconnect->next_tick=0U;reconnect->scheduled=0U;return 0;}
int net_llm_sse_reconnect_schedule(net_llm_sse_reconnect_t* reconnect,net_llm_sse_response_t* response,uint16_t status_code,uint32_t base_delay,uint32_t max_delay,uint32_t now){int status;uint32_t effective_delay;if(!reconnect||!response)return -1;effective_delay=(response->sse.retry_valid&&response->sse.retry_delay_ms>0U)?response->sse.retry_delay_ms:base_delay;if(response->sse.retry_valid&&effective_delay>max_delay)effective_delay=max_delay;status=net_llm_http_retry_schedule(status_code,reconnect->retry_limit,effective_delay,max_delay,now,&reconnect->retries_used,&reconnect->next_tick);if(status<=0)return status;if(net_llm_sse_response_reset(response)!=0)return -2;reconnect->scheduled=1U;return 1;}
int net_llm_sse_reconnect_ready(const net_llm_sse_reconnect_t* reconnect,uint32_t now){if(!reconnect)return -1;if(!reconnect->scheduled)return 0;return now>=reconnect->next_tick?1:0;}

static int net_json_key_match(const uint8_t* input,uint16_t start,uint16_t end,const char* key){uint16_t index=0U;if(!key)return 0;while(key[index]){if((uint16_t)(start+index)>=end||input[start+index]!=(uint8_t)key[index])return 0;index++;}return (uint16_t)(start+index)==end;}
static uint16_t net_json_skip_space(const uint8_t* input,uint16_t length,uint16_t index){while(index<length&&(input[index]==' '||input[index]=='\t'||input[index]=='\r'||input[index]=='\n'))index++;return index;}
static int net_json_utf8_decode(const uint8_t* input,uint16_t length,uint32_t* codepoint,uint8_t* width){uint8_t a,b,c,d;if(!input||!codepoint||!width||length==0U)return -1;a=input[0];if(a<0x80U){*codepoint=a;*width=1U;return 0;}if(a>=0xc2U&&a<=0xdfU){if(length<2U)return -2;b=input[1];if(b<0x80U||b>0xbfU)return -3;*codepoint=((uint32_t)(a&0x1fU)<<6U)|(uint32_t)(b&0x3fU);*width=2U;return 0;}if(a>=0xe0U&&a<=0xefU){if(length<3U)return -2;b=input[1];c=input[2];if(b<0x80U||b>0xbfU||c<0x80U||c>0xbfU||(a==0xe0U&&b<0xa0U)||(a==0xedU&&b>0x9fU))return -3;*codepoint=((uint32_t)(a&0x0fU)<<12U)|((uint32_t)(b&0x3fU)<<6U)|(uint32_t)(c&0x3fU);*width=3U;return 0;}if(a>=0xf0U&&a<=0xf4U){if(length<4U)return -2;b=input[1];c=input[2];d=input[3];if(b<0x80U||b>0xbfU||c<0x80U||c>0xbfU||d<0x80U||d>0xbfU||(a==0xf0U&&b<0x90U)||(a==0xf4U&&b>0x8fU))return -3;*codepoint=((uint32_t)(a&0x07U)<<18U)|((uint32_t)(b&0x3fU)<<12U)|((uint32_t)(c&0x3fU)<<6U)|(uint32_t)(d&0x3fU);*width=4U;return 0;}return -3;}
static int net_json_append_codepoint(uint8_t* output,uint16_t capacity,uint16_t* length,uint32_t codepoint){uint8_t bytes[4];uint8_t count,index;if(!output||!length||codepoint>0x10ffffU||(codepoint>=0xd800U&&codepoint<=0xdfffU))return -1;if(codepoint<0x80U){bytes[0]=(uint8_t)codepoint;count=1U;}else if(codepoint<0x800U){bytes[0]=(uint8_t)(0xc0U|(codepoint>>6U));bytes[1]=(uint8_t)(0x80U|(codepoint&0x3fU));count=2U;}else if(codepoint<0x10000U){bytes[0]=(uint8_t)(0xe0U|(codepoint>>12U));bytes[1]=(uint8_t)(0x80U|((codepoint>>6U)&0x3fU));bytes[2]=(uint8_t)(0x80U|(codepoint&0x3fU));count=3U;}else{bytes[0]=(uint8_t)(0xf0U|(codepoint>>18U));bytes[1]=(uint8_t)(0x80U|((codepoint>>12U)&0x3fU));bytes[2]=(uint8_t)(0x80U|((codepoint>>6U)&0x3fU));bytes[3]=(uint8_t)(0x80U|(codepoint&0x3fU));count=4U;}if((uint32_t)*length+count>capacity)return -2;for(index=0U;index<count;index++)output[(*length)++]=bytes[index];return 0;}
static int net_json_hex(uint8_t value){if(value>='0'&&value<='9')return value-'0';if(value>='a'&&value<='f')return value-'a'+10;if(value>='A'&&value<='F')return value-'A'+10;return -1;}
int net_json_extract_string(const uint8_t* json,uint16_t json_length,const char* key,uint8_t* output,uint16_t output_capacity,uint16_t* output_length){uint16_t index,start,end,cursor,out=0U;uint8_t escaped,value,width;uint32_t codepoint,high,low;int digit;
    if(!json||!key||!key[0]||!output||!output_length)return -1;
    for(index=0U;index<json_length;index++){
        if(json[index]!='"')continue;start=(uint16_t)(index+1U);escaped=0U;for(index=start;index<json_length;index++){if(escaped){escaped=0U;continue;}if(json[index]=='\\'){escaped=1U;continue;}if(json[index]=='"')break;}if(index>=json_length)return -2;end=index;
        if(!net_json_key_match(json,start,end,key))continue;cursor=net_json_skip_space(json,json_length,(uint16_t)(end+1U));if(cursor>=json_length||json[cursor]!=':')continue;cursor=net_json_skip_space(json,json_length,(uint16_t)(cursor+1U));if(cursor>=json_length||json[cursor]!='"')return -3;cursor++;
        for(;cursor<json_length;cursor++){value=json[cursor];if(value=='"'){*output_length=out;return 0;}if(value=='\\'){if(++cursor>=json_length)return -4;value=json[cursor];if(value=='"'||value=='\\'||value=='/')codepoint=value;else if(value=='b')codepoint='\b';else if(value=='f')codepoint='\f';else if(value=='n')codepoint='\n';else if(value=='r')codepoint='\r';else if(value=='t')codepoint='\t';else if(value=='u'){if((uint32_t)cursor+4U>=json_length)return -5;high=0U;for(index=1U;index<=4U;index++){digit=net_json_hex(json[cursor+index]);if(digit<0)return -5;high=(high<<4U)|(uint32_t)digit;}cursor=(uint16_t)(cursor+4U);if(high>=0xd800U&&high<=0xdbffU){if((uint32_t)cursor+6U>=json_length||json[cursor+1U]!='\\'||json[cursor+2U]!='u')return -5;low=0U;for(index=3U;index<=6U;index++){digit=net_json_hex(json[cursor+index]);if(digit<0)return -5;low=(low<<4U)|(uint32_t)digit;}if(low<0xdc00U||low>0xdfffU)return -5;cursor=(uint16_t)(cursor+6U);codepoint=0x10000U+((high-0xd800U)<<10U)+(low-0xdc00U);}else if(high>=0xdc00U&&high<=0xdfffU)return -5;else codepoint=high;}else return -5;}else if(value<0x20U)return -6;else{if(value<0x80U){codepoint=value;}else{if(net_json_utf8_decode(json+cursor,(uint16_t)(json_length-cursor),&codepoint,&width)!=0)return -6;cursor=(uint16_t)(cursor+width-1U);}}if(net_json_append_codepoint(output,output_capacity,&out,codepoint)!=0)return -7;}
        return -8;
    }
    return -9;
}

int net_llm_ollama_response_extract(const uint8_t* json,uint16_t json_length,uint8_t* output,uint16_t output_capacity,uint16_t* output_length){
    return net_json_extract_string(json,json_length,"response",output,output_capacity,output_length);
}
int net_llm_openai_response_extract(const uint8_t* json,uint16_t json_length,uint8_t* output,uint16_t output_capacity,uint16_t* output_length){
    return net_json_extract_string(json,json_length,"content",output,output_capacity,output_length);
}

static int net_json_append_escaped(uint8_t* output,uint16_t capacity,uint16_t* length,const uint8_t* input,uint16_t input_length){uint16_t index;uint8_t value,width;uint32_t codepoint;if(!input&&input_length)return -1;for(index=0U;index<input_length;index++){value=input[index];if(value=='"'||value=='\\'){if(net_http_append_char(output,capacity,length,'\\')!=0||net_http_append_char(output,capacity,length,value)!=0)return -2;}else if(value=='\n'||value=='\r'||value=='\t'){uint8_t escaped=value=='\n'?'n':(value=='\r'?'r':'t');if(net_http_append_char(output,capacity,length,'\\')!=0||net_http_append_char(output,capacity,length,escaped)!=0)return -3;}else if(value<0x20U)return -4;else if(value<0x80U){if(net_http_append_char(output,capacity,length,value)!=0)return -5;}else{if(net_json_utf8_decode(input+index,(uint16_t)(input_length-index),&codepoint,&width)!=0)return -6;if((uint32_t)*length+width>capacity)return -7;while(width--)output[(*length)++]=input[index++];index--;} }return 0;}
static int net_llm_build_json(uint8_t* output,uint16_t capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length,uint8_t openai,uint8_t stream){uint16_t length=0U,model_length=0U;if(!output||!model||!model[0]||(!prompt&&prompt_length))return -1;while(model[model_length]){if(model_length==65535U)return -2;model_length++;}if(net_http_append_text(output,capacity,&length,"{\"model\":\"")!=0||net_json_append_escaped(output,capacity,&length,(const uint8_t*)model,model_length)!=0)return -3;if(openai){if(net_http_append_text(output,capacity,&length,"\",\"messages\":[{\"role\":\"user\",\"content\":\"")!=0)return -4;}else if(net_http_append_text(output,capacity,&length,"\",\"prompt\":\"")!=0)return -5;if(net_json_append_escaped(output,capacity,&length,prompt,prompt_length)!=0)return -6;if(net_http_append_text(output,capacity,&length,openai?(stream?"\"}],\"stream\":true}":"\"}],\"stream\":false}"):(stream?"\",\"stream\":true}":"\",\"stream\":false}"))!=0)return -7;return (int)length;}
int net_llm_build_ollama_generate_json(uint8_t* output,uint16_t output_capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length){return net_llm_build_json(output,output_capacity,model,prompt,prompt_length,0U,0U);}
int net_llm_build_openai_chat_json(uint8_t* output,uint16_t output_capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length){return net_llm_build_json(output,output_capacity,model,prompt,prompt_length,1U,0U);}
int net_llm_build_ollama_generate_stream_json(uint8_t* output,uint16_t output_capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length){return net_llm_build_json(output,output_capacity,model,prompt,prompt_length,0U,1U);}
int net_llm_build_openai_chat_stream_json(uint8_t* output,uint16_t output_capacity,const char* model,const uint8_t* prompt,uint16_t prompt_length){return net_llm_build_json(output,output_capacity,model,prompt,prompt_length,1U,1U);}
int net_llm_http_status_classify(uint16_t status_code){if(status_code>=200U&&status_code<300U)return NET_LLM_HTTP_STATUS_SUCCESS;if(status_code==401U||status_code==403U)return NET_LLM_HTTP_STATUS_AUTH;if(status_code==408U||status_code==425U||status_code==429U||(status_code>=500U&&status_code<600U))return NET_LLM_HTTP_STATUS_RETRYABLE;if(status_code>=300U&&status_code<500U)return NET_LLM_HTTP_STATUS_PERMANENT;return NET_LLM_HTTP_STATUS_PROTOCOL;}
int net_llm_http_retry_consume(uint16_t status_code,uint8_t retry_limit,uint8_t* retries_used){if(!retries_used)return -1;if(net_llm_http_status_classify(status_code)!=NET_LLM_HTTP_STATUS_RETRYABLE)return 0;if(*retries_used>=retry_limit)return 0;(*retries_used)++;return 1;}
int net_llm_http_retry_schedule(uint16_t status_code,uint8_t retry_limit,uint32_t base_delay,uint32_t max_delay,uint32_t now,uint8_t* retries_used,uint32_t* retry_at){uint8_t attempt,index;uint32_t delay,previous;if(!retries_used||!retry_at||base_delay==0U||max_delay<base_delay)return -1;if(net_llm_http_status_classify(status_code)!=NET_LLM_HTTP_STATUS_RETRYABLE)return 0;if(*retries_used>=retry_limit)return 0;attempt=*retries_used;delay=base_delay;for(index=0U;index<attempt&&delay<max_delay;index++){previous=delay;if(delay>max_delay/2U)delay=max_delay;else delay<<=1U;if(delay<previous)delay=max_delay;}if(delay>max_delay)delay=max_delay;*retries_used=(uint8_t)(attempt+1U);*retry_at=(now>0xffffffffU-delay)?0xffffffffU:now+delay;return 1;}
int net_llm_bearer_store_provision(net_llm_bearer_store_t* store,const uint8_t* token,uint16_t token_length){uint16_t index;if(!store||!token||token_length==0U||token_length>=NET_LLM_BEARER_MAX)return -1;for(index=0U;index<token_length;index++)if(token[index]<33U||token[index]>126U)return -2;for(index=0U;index<NET_LLM_BEARER_MAX;index++)store->token[index]=0U;for(index=0U;index<token_length;index++)store->token[index]=token[index];store->length=token_length;store->provisioned=1U;return 0;}
void net_llm_bearer_store_clear(net_llm_bearer_store_t* store){uint16_t index;if(!store)return;for(index=0U;index<NET_LLM_BEARER_MAX;index++)store->token[index]=0U;store->length=0U;store->provisioned=0U;}
int net_http_build_post_json_bearer_store(uint8_t* request,uint16_t capacity,const char* host,const char* path,const net_llm_bearer_store_t* store,const uint8_t* json,uint16_t json_length){char token[NET_LLM_BEARER_MAX];uint16_t index;int status;if(!store||!store->provisioned||store->length==0U||store->length>=NET_LLM_BEARER_MAX)return -1;for(index=0U;index<store->length;index++)token[index]=(char)store->token[index];token[store->length]='\0';status=net_http_build_post_json_bearer(request,capacity,host,path,token,json,json_length);for(index=0U;index<NET_LLM_BEARER_MAX;index++)token[index]='\0';return status;}

int net_llm_sse_reconnect_schedule_jittered(net_llm_sse_reconnect_t* reconnect,net_llm_sse_response_t* response,uint16_t status_code,uint32_t base_delay,uint32_t max_delay,uint32_t now,uint32_t jitter_window,uint32_t* jitter_seed){uint32_t span,random_value,delay;int32_t delta;if(!reconnect||!response||!jitter_seed||base_delay==0U||max_delay<base_delay)return -1;if(jitter_window>0x3fffffffU)jitter_window=0x3fffffffU;span=(jitter_window*2U)+1U;*jitter_seed=(*jitter_seed*1664525U)+1013904223U;random_value=(*jitter_seed)%span;delta=(int32_t)random_value-(int32_t)jitter_window;if(delta<0){uint32_t magnitude=(uint32_t)(-delta);delay=base_delay>magnitude?base_delay-magnitude:1U;}else{delay=base_delay>(max_delay-(uint32_t)delta)?max_delay:base_delay+(uint32_t)delta;}if(delay>max_delay)delay=max_delay;return net_llm_http_retry_schedule(status_code,reconnect->retry_limit,delay,max_delay,now,&reconnect->retries_used,&reconnect->next_tick)==1?(reconnect->scheduled=1U,net_llm_sse_response_reset(response)==0?1:-2):0;}

static uint32_t net_llm_sse_persist_checksum(const net_llm_sse_persisted_state_t* persisted){const uint8_t* bytes=(const uint8_t*)persisted;uint32_t hash=2166136261U;uint16_t index;for(index=0U;index<(uint16_t)(sizeof(*persisted)-sizeof(persisted->checksum));index++){hash^=bytes[index];hash*=16777619U;}return hash;}
int net_llm_sse_persist_save(net_llm_sse_persisted_state_t* persisted,uint8_t provider,uint8_t retries_used,const net_llm_sse_response_t* response){uint8_t index;if(!persisted||!response||response->sse.event_id_length>NET_LLM_SSE_EVENT_ID_MAX||(!response->sse.event_id_valid&&response->sse.event_id_length!=0U))return -1;if(provider>1U)return -2;persisted->magic=NET_LLM_SSE_PERSIST_MAGIC;persisted->version=NET_LLM_SSE_PERSIST_VERSION;persisted->provider=provider;persisted->retries_used=retries_used;persisted->event_id_length=response->sse.event_id_length;for(index=0U;index<NET_LLM_SSE_EVENT_ID_MAX;index++)persisted->event_id[index]=response->sse.event_id[index];persisted->checksum=net_llm_sse_persist_checksum(persisted);return 0;}
int net_llm_sse_persist_load(const net_llm_sse_persisted_state_t* persisted,uint8_t* provider,uint8_t* retries_used,net_llm_sse_response_t* response){uint8_t index;if(!persisted||!provider||!retries_used||!response)return -1;if(persisted->magic!=NET_LLM_SSE_PERSIST_MAGIC||persisted->version!=NET_LLM_SSE_PERSIST_VERSION||persisted->event_id_length>NET_LLM_SSE_EVENT_ID_MAX||persisted->checksum!=net_llm_sse_persist_checksum(persisted))return -2;if(persisted->provider>1U)return -3;for(index=0U;index<NET_LLM_SSE_EVENT_ID_MAX;index++)response->sse.event_id[index]=persisted->event_id[index];response->sse.event_id_length=persisted->event_id_length;response->sse.event_id_valid=persisted->event_id_length>0U?1U:0U;*provider=persisted->provider;*retries_used=persisted->retries_used;return 0;}

int net_llm_model_policy_validate(const net_llm_model_policy_t* policy){uint8_t index;if(!policy||!policy->model||policy->model_length==0U||policy->model_length>NET_LLM_MODEL_NAME_MAX)return -1;if(policy->provider>1U)return -2;if(policy->allow_rotation>1U)return -3;for(index=0U;index<policy->model_length;index++){if((uint8_t)policy->model[index]<33U||(uint8_t)policy->model[index]>126U)return -4;}return 0;}

int net_https_application_ready(const net_tls_handshake_t* handshake,const net_tls_aes_gcm_session_t* session){if(!handshake||!session||!net_tls_handshake_is_complete(handshake))return 0;if(!session->write_key||!session->write_fixed_iv||!session->read_key||!session->read_fixed_iv)return 0;return 1;}

int net_https_build_application_record_if_ready(const net_tls_handshake_t* handshake,net_tls_aes_gcm_session_t* session,uint8_t* record,uint32_t capacity,uint8_t content_type,const uint8_t* plaintext,uint16_t plaintext_length){if(!net_https_application_ready(handshake,session))return -1;return net_tls_aes_gcm_session_build(session,record,capacity,content_type,plaintext,plaintext_length);}

int net_https_open_application_record_if_ready(const net_tls_handshake_t* handshake,net_tls_aes_gcm_session_t* session,const uint8_t* record,uint32_t length,uint8_t* plaintext,uint16_t plaintext_capacity,net_tls_record_view_t* out){if(!net_https_application_ready(handshake,session))return -1;return net_tls_aes_gcm_session_open(session,record,length,plaintext,plaintext_capacity,out);}
