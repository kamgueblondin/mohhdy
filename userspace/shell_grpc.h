#ifndef MOHHDY_SHELL_GRPC_H
#define MOHHDY_SHELL_GRPC_H
/* Lot 6 US-067: gRPC unary calls over a real guest TCP connection. */
int shell_grpc_is(const char* line);
int shell_grpc_line(const char* line);
#endif
