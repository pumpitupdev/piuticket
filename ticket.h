#pragma once

#define OLD_TICKET_ENDPOINT "usb/013/038"
#define FAKE_TICKET_FD 0x1338
#define FAKE_TICKET_DEVICE_PATH "./fake_devices"
#define REAL_DEVICES_PATH "/sys/kernel/debug/usb/devices"



void parse_ticketcmd(void* data);
void TicketControllerInfo();
void make_faketicketdevices(char* devices_path);