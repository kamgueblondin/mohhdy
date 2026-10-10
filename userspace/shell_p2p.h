/* userspace/shell_p2p.h - p2p-* shell commands (Phase 5, see docs/p2p.md). */
#ifndef MOHHDY_SHELL_P2P_H
#define MOHHDY_SHELL_P2P_H
/* Runs a p2p-* line; returns the exit code (0 ok). */
int shell_p2p_line(const char* line);
int shell_p2p_active(void);
/* One background pump (called while the console waits for keys). */
void shell_p2p_poll(void);
#endif
