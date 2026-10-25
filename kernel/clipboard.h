/* clipboard.h - the one desktop clipboard (UX4), shared by the VBE
 * terminal (mouse selection / right-click paste), the desktop text editor
 * and the file explorer (SYS_CLIPBOARD). Text only, OS_CLIP_MAX bytes. */
#ifndef CLIPBOARD_H
#define CLIPBOARD_H
#include "../include/os_syscalls.h"
int clipboard_set(const char *text, int len);   /* bytes kept */
int clipboard_get(char *out, int max);          /* bytes copied, NUL added */
int clipboard_len(void);
unsigned clipboard_seq(void);                   /* bumps on every set */
#endif
