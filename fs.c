//
// Created by rfx on 2/11/17.
//

#include <string.h>
#include <stdio.h>
#include "fs.h"
#include "ticket.h"
#include "util.h"

// Resolves /proc/bus/usb/* to /dev/bus/usb/*
void fs_resolve_procbus(const char* filename,char* procbus_path){
    sprintf(procbus_path,"/dev/bus/usb/%s",filename+strlen(OLD_PROC_BUS_USB));
    printf("%s\n",procbus_path);
}

// Resolves direct HDD access to file
// TODO: Make this configurable.
void fs_resolve_hdd(const char* filename,char* hdd_path){
    sprintf(hdd_path,"%s/%s",root_path,hdd_filename);
    //printf("%s\n",hdd_path);
}


// Resolves /proc/bus/usb/devices
void fs_resolve_procbususbdev(const char* filename,char* devices_path){
    make_faketicketdevices(devices_path);
  }

// Resolves SETTINGS Directory
// TODO: Make Configurable
void fs_resolve_settings(const char* filename,char* settings_path){
    sprintf(settings_path,"%s%s",root_path,filename);
    //printf("%s\n",settings_path);
}

// Resolves SCRIPT Directory
// TODO: Make Configurable

// Resolves SCRIPT Directory for LUA files.
void fs_resolve_script(const char* filename,char* script_path){
    sprintf(script_path,"%s/EXT/%d%s",root_path,game_version,filename);
    //printf("%s\n",script_path);
}