/* out_capture.h - Lot 46: bounded capture of the output of the programs a
 * desktop window launches (spawn, exec), for the GUI tool windows.
 *
 * The shell task opens a capture (owner), runs the command line, then reads
 * what its children printed and closes the capture. While a capture is
 * open, a task created by the owner (or by a task already in the capture)
 * joins it: its SYS_PUTC / SYS_PUTS bytes go to this buffer (and the serial
 * line) instead of the VGA console. One capture at a time, a static buffer
 * of OS_OUTCAP_MAX bytes and OS_OUTCAP_PIDS tracked tasks; bytes past the
 * bound are counted, never written. Only the owner may read or close it, and
 * it only ever sees its own descendants: the kernel rights are unchanged.
 * Pure state (no task table access) so that it is host testable. */
#ifndef OUT_CAPTURE_H
#define OUT_CAPTURE_H
#include <stdint.h>
#include "../include/os_syscalls.h"

void out_cap_reset(void);
/* 0, OS_OUTCAP_BUSY when another owner holds the capture, OS_OUTCAP_BAD */
int out_cap_begin(int32_t owner);
/* closes the capture of owner; returns the bytes dropped (>= 0) or < 0 */
int out_cap_end(int32_t owner);
/* the current owner, 0 when no capture is open */
int32_t out_cap_owner(void);
/* a task was created by parent: it joins the capture when parent is the
 * owner or already tracked. Returns 1 when it joined, 0 otherwise (a full
 * pid table counts the child in out_cap_untracked). */
int out_cap_child(int32_t parent, int32_t child);
/* 1 when pid's output is captured */
int out_cap_tracked(int32_t pid);
/* one output byte of pid: 1 when captured (kept or counted as dropped),
 * 0 when pid is not in the capture (the caller prints it as before) */
int out_cap_put(int32_t pid, char c);
/* copies up to max - 1 captured bytes (oldest first, NUL added) and removes
 * them from the buffer; returns the count or < 0 when caller is not the owner */
int out_cap_read(int32_t caller, char *dst, int max);
uint32_t out_cap_dropped(void);
uint32_t out_cap_untracked(void);
uint32_t out_cap_total(void);     /* bytes captured since begin (kept + dropped) */
/* tracked pid number k (0 .. count-1), for the "still alive" count */
int out_cap_pid_count(void);
int32_t out_cap_pid_at(int k);
#endif
