//
// Created by rfx on 2/11/17.
//

#ifndef ZERO_PATCH_FS_H
#define ZERO_PATCH_FS_H

#define OLD_SETTINGS_DIR "/SETTINGS/"
#define OLD_SCRIPT_DIR "/SCRIPT/"
#define OLD_HDD_ENDPOINT "/dev/hdd"
#define OLD_PROC_BUS_USB "/proc/bus/usb/"
#define OLD_PROC_BUS_USB_DEV "/proc/bus/usb/devices"


void fs_resolve_procbus(const char* filename,char* procbus_path);
void fs_resolve_hdd(const char* filename,char* hdd_path);

void fs_resolve_procbususbdev(const char* filename,char* devices_path);
void fs_resolve_settings(const char * filename,char* settings_path);

void fs_resolve_script(const char* filename,char* script_path);
#endif //ZERO_PATCH_FS_H
