//
// Created by rfx on 2/12/17.
//

#ifndef ZERO_PATCH_HOOKS_H
#define ZERO_PATCH_HOOKS_H
#define ATA_IDENTIFY_DEVICE 0x30D
#define MICRODOG_XACT 0x6B00
#define USBDEVFS_CONTROL32 0xC0105500
#define TICKET_XACT USBDEVFS_CONTROL32 

void resolve_hooks(void);
#endif //ZERO_PATCH_HOOKS_H
