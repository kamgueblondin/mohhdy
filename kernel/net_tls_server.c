#include "net_tls_server.h"
#include "rsa_verify.h"
#include "sha256.h"

static void put16(uint8_t* out, uint16_t value) {
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)value;
}

static void put24(uint8_t* out, uint32_t value) {
    out[0] = (uint8_t)(value >> 16);
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)value;
}

static uint16_t read16(const uint8_t* data) {
    return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

int net_tls_client_hello_parse(const uint8_t* handshake, uint16_t length,
                               net_tls_client_hello_view_t* out) {
    uint16_t body_length, session_id_length, cipher_length, position, i;
    uint16_t suite;
    uint8_t found_c02f = 0U;
    if (!handshake || !out || length < 39U || handshake[0] != NET_TLS_HANDSHAKE_CLIENT_HELLO)
        return -1;
    body_length = (uint16_t)(((uint32_t)handshake[1] << 16) | ((uint32_t)handshake[2] << 8) | handshake[3]);
    if ((uint32_t)4U + body_length != (uint32_t)length) return -2;
    if (handshake[4] != NET_TLS_VERSION_1_2_MAJOR || handshake[5] != NET_TLS_VERSION_1_2_MINOR)
        return -3;
    out->random = handshake + 6;
    session_id_length = handshake[38];
    position = (uint16_t)(39U + session_id_length);
    if (position + 2U > length) return -4;
    cipher_length = read16(handshake + position);
    position = (uint16_t)(position + 2U);
    if ((cipher_length & 1U) != 0U || position + cipher_length + 1U > length) return -5;
    for (i = 0U; i + 1U < cipher_length; i = (uint16_t)(i + 2U)) {
        suite = read16(handshake + position + i);
        if (suite == NET_TLS_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256) found_c02f = 1U;
    }
    if (!found_c02f) return -6;
    out->cipher_suite = NET_TLS_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256;
    return 0;
}

int net_tls_server_hello_build(uint8_t* record, uint32_t capacity,
                               const uint8_t server_random[32], uint16_t cipher_suite) {
    uint8_t hello[42];
    uint16_t i;
    if (!record || !server_random) return -1;
    hello[0] = NET_TLS_HANDSHAKE_SERVER_HELLO;
    put24(hello + 1, 38U);
    hello[4] = NET_TLS_VERSION_1_2_MAJOR;
    hello[5] = NET_TLS_VERSION_1_2_MINOR;
    for (i = 0U; i < 32U; i++) hello[6U + i] = server_random[i];
    hello[38] = 0U;
    put16(hello + 39, cipher_suite);
    hello[41] = 0U;
    return net_tls_record_build(record, capacity, NET_TLS_CONTENT_HANDSHAKE, hello, 42U);
}

int net_tls_server_certificate_build(uint8_t* record, uint32_t capacity,
                                     const uint8_t* leaf_der, uint16_t leaf_length) {
    uint8_t message[900];
    uint32_t body_length;
    uint16_t i;
    if (!record || !leaf_der || leaf_length == 0U) return -1;
    body_length = 3U + 3U + leaf_length;
    if (4U + body_length > sizeof(message)) return -2;
    message[0] = NET_TLS_HANDSHAKE_CERTIFICATE;
    put24(message + 1, body_length);
    put24(message + 4, 3U + leaf_length);
    put24(message + 7, leaf_length);
    for (i = 0U; i < leaf_length; i++) message[10U + i] = leaf_der[i];
    return net_tls_record_build(record, capacity, NET_TLS_CONTENT_HANDSHAKE, message,
                                (uint16_t)(4U + body_length));
}

int net_tls_server_key_exchange_ecdhe_rsa_build(
    uint8_t* record, uint32_t capacity,
    const uint8_t client_random[32], const uint8_t server_random[32],
    const uint8_t server_public[NET_TLS_X25519_KEY_LENGTH],
    const uint8_t* modulus, uint16_t modulus_length,
    const uint8_t* private_exponent, uint16_t private_exponent_length,
    uint32_t* rsa_workspace, uint16_t rsa_workspace_length) {
    uint8_t params[36];
    uint8_t signed_input[32 + 32 + 36];
    uint8_t digest[32];
    uint8_t signature[256];
    uint8_t message[320];
    sha256_ctx_t ctx;
    uint16_t i;
    int sig_length;
    uint32_t body_length;
    if (!record || !client_random || !server_random || !server_public || !modulus ||
        !private_exponent || !rsa_workspace)
        return -1;
    params[0] = 3U;
    put16(params + 1, NET_TLS_NAMED_CURVE_X25519);
    params[3] = NET_TLS_X25519_KEY_LENGTH;
    for (i = 0U; i < NET_TLS_X25519_KEY_LENGTH; i++) params[4U + i] = server_public[i];
    for (i = 0U; i < 32U; i++) {
        signed_input[i] = client_random[i];
        signed_input[32U + i] = server_random[i];
    }
    for (i = 0U; i < 36U; i++) signed_input[64U + i] = params[i];
    sha256_init(&ctx);
    sha256_update(&ctx, signed_input, sizeof(signed_input));
    sha256_final(&ctx, digest);
    sig_length = rsa_pkcs1_v15_sha256_sign(modulus, modulus_length, private_exponent,
                                           private_exponent_length, digest, signature,
                                           sizeof(signature), rsa_workspace,
                                           rsa_workspace_length);
    if (sig_length != (int)modulus_length) return -2;
    body_length = 36U + 2U + 2U + (uint32_t)sig_length;
    if (4U + body_length > sizeof(message)) return -3;
    message[0] = NET_TLS_HANDSHAKE_SERVER_KEY_EXCHANGE;
    put24(message + 1, body_length);
    for (i = 0U; i < 36U; i++) message[4U + i] = params[i];
    message[40] = NET_TLS_HASH_SHA256;
    message[41] = NET_TLS_SIGNATURE_RSA;
    put16(message + 42, (uint16_t)sig_length);
    for (i = 0U; i < (uint16_t)sig_length; i++) message[44U + i] = signature[i];
    return net_tls_record_build(record, capacity, NET_TLS_CONTENT_HANDSHAKE, message,
                                (uint16_t)(4U + body_length));
}

int net_tls_server_hello_done_build(uint8_t* record, uint32_t capacity) {
    uint8_t message[4] = {NET_TLS_HANDSHAKE_SERVER_HELLO_DONE, 0, 0, 0};
    return net_tls_record_build(record, capacity, NET_TLS_CONTENT_HANDSHAKE, message, 4U);
}

int net_tls_server_init(net_tls_server_t* server, const uint8_t server_random[32],
                        const uint8_t server_private[NET_TLS_X25519_KEY_LENGTH],
                        uint32_t* x25519_workspace, uint16_t x25519_workspace_length) {
    uint16_t i;
    if (!server || !server_random || !server_private || !x25519_workspace) return -1;
    for (i = 0U; i < sizeof(*server); i++) ((uint8_t*)server)[i] = 0U;
    for (i = 0U; i < 32U; i++) {
        server->server_random[i] = server_random[i];
        server->server_private[i] = server_private[i];
    }
    if (x25519_public_key(server->server_public, server->server_private, x25519_workspace,
                          x25519_workspace_length) != 0)
        return -2;
    if (net_tls_transcript_init(&server->transcript, server->transcript_storage,
                                sizeof(server->transcript_storage)) != 0)
        return -3;
    server->phase = NET_TLS_SERVER_PHASE_IDLE;
    return 0;
}

int net_tls_server_accept_client_hello(net_tls_server_t* server, const uint8_t* record,
                                       uint16_t record_length) {
    net_tls_record_view_t view;
    net_tls_client_hello_view_t hello;
    uint16_t i;
    if (!server || !record || server->phase != NET_TLS_SERVER_PHASE_IDLE) return -1;
    if (net_tls_record_parse(record, record_length, &view) != 0) return -2;
    if (view.content_type != NET_TLS_CONTENT_HANDSHAKE) return -3;
    if (net_tls_client_hello_parse(view.payload, view.payload_length, &hello) != 0) return -4;
    for (i = 0U; i < 32U; i++) server->client_random[i] = hello.random[i];
    if (net_tls_transcript_append(&server->transcript, view.payload, view.payload_length) != 0)
        return -5;
    return 0;
}

int net_tls_server_note_handshake_message(net_tls_server_t* server, const uint8_t* handshake,
                                          uint16_t length) {
    net_tls_record_view_t view;
    if (!server || !handshake) return -1;
    if (net_tls_record_parse(handshake, length, &view) != 0) return -2;
    if (view.content_type != NET_TLS_CONTENT_HANDSHAKE) return -3;
    return net_tls_transcript_append(&server->transcript, view.payload, view.payload_length);
}

int net_tls_server_accept_client_flight(
    net_tls_server_t* server, const uint8_t* flight, uint16_t flight_length,
    uint32_t* x25519_workspace, uint16_t x25519_workspace_length, uint8_t* prf_workspace,
    uint32_t prf_workspace_capacity, uint8_t* plaintext, uint16_t plaintext_capacity) {
    net_tls_record_view_t records[3];
    uint16_t offsets[3];
    uint16_t consumed = 0U;
    uint16_t count = 0U;
    uint16_t i;
    const uint8_t* cke;
    uint8_t peer_public[NET_TLS_X25519_KEY_LENGTH];
    uint8_t shared[NET_TLS_X25519_KEY_LENGTH];
    uint8_t expected[12];
    uint8_t transcript_hash[32];
    net_tls_record_view_t finished_view;
    int status;
    if (!server || !flight || !x25519_workspace || !prf_workspace || !plaintext) return -1;
    if (server->phase != NET_TLS_SERVER_PHASE_WAIT_CLIENT_FLIGHT) return -2;
    while (consumed < flight_length && count < 3U) {
        if (net_tls_record_parse(flight + consumed, (uint32_t)(flight_length - consumed),
                                 &records[count]) != 0)
            return -3;
        offsets[count] = consumed;
        consumed = (uint16_t)(consumed + NET_TLS_RECORD_HEADER + records[count].payload_length);
        count++;
    }
    if (count != 3U || consumed != flight_length) return -4;
    if (records[0].content_type != NET_TLS_CONTENT_HANDSHAKE ||
        records[0].payload_length < 37U || records[0].payload[0] != NET_TLS_HANDSHAKE_CLIENT_KEY_EXCHANGE)
        return -5;
    cke = records[0].payload;
    if (cke[4] != NET_TLS_X25519_KEY_LENGTH) return -6;
    for (i = 0U; i < NET_TLS_X25519_KEY_LENGTH; i++) peer_public[i] = cke[5U + i];
    if (net_tls_transcript_append(&server->transcript, cke, 37U) != 0) return -7;
    if (x25519_shared_secret(shared, server->server_private, peer_public, x25519_workspace,
                             x25519_workspace_length) != 0)
        return -8;
    if (net_tls_derive_master_secret(server->master_secret, shared, NET_TLS_X25519_KEY_LENGTH,
                                     server->client_random, server->server_random, prf_workspace,
                                     prf_workspace_capacity) != 0)
        return -9;
    {
        net_tls_aes128_gcm_key_block_t block;
        if (net_tls_derive_aes128_gcm_key_block(server->key_block, sizeof(server->key_block),
                                                server->master_secret, server->client_random,
                                                server->server_random, &block, prf_workspace,
                                                prf_workspace_capacity) != 0)
            return -10;
        /* is_client=0 : write=server, read=client */
        if (net_tls_aes_gcm_session_init(&server->session, &block, 0U) != 0) return -11;
    }
    if (records[1].content_type != NET_TLS_CONTENT_CHANGE_CIPHER_SPEC) return -12;
    if (net_tls_change_cipher_spec_parse(records[1].payload, records[1].payload_length) != 0)
        return -12;
    status = net_tls_aes_gcm_session_open(&server->session, flight + offsets[2],
                                          (uint16_t)(NET_TLS_RECORD_HEADER + records[2].payload_length),
                                          plaintext, plaintext_capacity, &finished_view);
    if (status != 0) return -13;
    if (finished_view.content_type != NET_TLS_CONTENT_HANDSHAKE) return -14;
    if (net_tls_finished_verify_data(expected, server->master_secret, &server->transcript,
                                     transcript_hash, prf_workspace, prf_workspace_capacity) != 0)
        return -16;
    if (net_tls_finished_parse(finished_view.payload, finished_view.payload_length, expected) != 0)
        return -17;
    if (net_tls_transcript_append(&server->transcript, finished_view.payload,
                                  finished_view.payload_length) != 0)
        return -18;
    server->complete = 1U;
    server->phase = NET_TLS_SERVER_PHASE_COMPLETE;
    return 0;
}

int net_tls_server_finished_record_build(net_tls_server_t* server, uint8_t* record,
                                         uint32_t capacity, uint8_t* prf_workspace,
                                         uint32_t prf_workspace_capacity) {
    uint8_t verify[12];
    uint8_t transcript_hash[32];
    uint8_t finished[16];
    if (!server || !record || !prf_workspace || !server->complete) return -1;
    if (net_tls_server_finished_verify_data(verify, server->master_secret, &server->transcript,
                                            transcript_hash, prf_workspace,
                                            prf_workspace_capacity) != 0)
        return -3;
    finished[0] = NET_TLS_HANDSHAKE_FINISHED;
    put24(finished + 1, 12U);
    for (uint16_t i = 0U; i < 12U; i++) finished[4U + i] = verify[i];
    if (net_tls_transcript_append(&server->transcript, finished, 16U) != 0) return -4;
    return net_tls_aes_gcm_session_build(&server->session, record, capacity,
                                         NET_TLS_CONTENT_HANDSHAKE, finished, 16U);
}
