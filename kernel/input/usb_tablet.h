#ifndef USB_TABLET_H
#define USB_TABLET_H

#include <stdint.h>

void usb_tablet_init(void);
void usb_tablet_poll(void);
int usb_tablet_present(void);

#endif
