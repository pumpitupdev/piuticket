//
// Created by rfx on 2/12/17.
//

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"
#include "gfx.h"
#include "sound.h"
#include "deadlock.h"
#include "hdd.h"
#include "dog.h"
#include "util.h"
#include "hooks.h"
#include "ticket.h"

static int (*real_ioctl)(int fd, int request, int *data) = NULL;
static FILE * (*real_fopen)(const char *, const char *) = NULL;
static int (*real_open)(char *, int) = NULL;
static int* (*real_XCreateWindow)(int* display, int* parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class, int *visual, unsigned long valuemask, int* *attributes) = NULL;
static int (*real_snd_pcm_open)(int **pcm, const char *name, int* stream, int mode) = NULL;
static int (*real_lua_dofile)(int *L, const char *filename) = NULL;
static int (*real_sigaction)(int signum, int* act,int* oldact) = NULL;
static int (*real_XPending)(int *display)=NULL;



// Various ioctl hooks.
int ioctl(int fd, int request, void * data) {
    // HDD information request.
    if (request == ATA_IDENTIFY_DEVICE){
        return fake_ata_data(data);
    }
    // Do a dongle transaction.
    if(request == MICRODOG_XACT){
        return process_microdog_xact(fd,request,data);
    }
    if(fd == FAKE_TICKET_FD){
        parse_ticketcmd(data);
        return 0;
    }
    return real_ioctl(fd, request, data);
}


// Various open hooks
int open(const char * filename, int oflag){
    //printf("open: %s\n",filename);
    // Ticket Endpoint Handling
    if(strstr(filename,OLD_TICKET_ENDPOINT)){
        // Do nothing for now.
        printf("Opening Fake Ticket Device\n");
        return FAKE_TICKET_FD;
    }

    // USB List Handling
    if(strncmp(OLD_PROC_BUS_USB,filename,strlen(OLD_PROC_BUS_USB)) == 0){
        char procbus_path[4096]={0x00};
        fs_resolve_procbus(filename,procbus_path);
        return real_open(procbus_path,oflag);
    }

    // HDD Data Handling
    if(strcmp(filename,OLD_HDD_ENDPOINT) == 0){
        char hdd_path[4096] = {0x00};
        fs_resolve_hdd(filename,hdd_path);
        return real_open(hdd_path,oflag);
    }

    // SETTINGS Directory Handling
    if(strncmp(OLD_SETTINGS_DIR,filename,strlen(OLD_SETTINGS_DIR)) == 0){
        char settings_path[4096];
        fs_resolve_settings(filename,settings_path);
        return real_open(settings_path,oflag);
    }

    return real_open(filename,oflag);
}

// Various fopen hooks
FILE * fopen(const char * filename, const char * mode){
   // printf("fopen: %s\n",filename);
    // /proc/bus/usb/devices Handling
    if(strcmp(OLD_PROC_BUS_USB_DEV,filename) == NULL){
        char devices_path[4096];
        fs_resolve_procbususbdev(filename,devices_path);
        return real_fopen(devices_path,mode);
    }

    // SETTINGS Directory Handling
    if(strncmp(OLD_SETTINGS_DIR,filename,strlen(OLD_SETTINGS_DIR)) == 0){
        char settings_path[4096];
        fs_resolve_settings(filename,settings_path);
        return real_fopen(settings_path,mode);
    }

    // SCRIPT Directory Handling
    if(strncmp(OLD_SCRIPT_DIR,filename,strlen(OLD_SCRIPT_DIR)) == 0){
        char script_path[4096];
        fs_resolve_script(filename,script_path);
        return real_fopen(script_path,mode);
    }

    return real_fopen(filename,mode);
}

// lua_dofile hooks
int lua_dofile (int *L, const char *filename) {

    // SCRIPT Directory Handling
    if(strncmp(OLD_SCRIPT_DIR,filename,strlen(OLD_SCRIPT_DIR)) == 0){
        char script_path[4096];
        fs_resolve_script(filename,script_path);
        return real_lua_dofile(L,script_path);
    }

    return real_lua_dofile(L,filename);
}
// Graphics Patches
int* XCreateWindow(int* display, int* parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class, int *visual, unsigned long valuemask, int* *attributes){

    // Non-NVIDIA Patch
    valuemask = gfx_patch_valuemask(valuemask);

    return real_XCreateWindow(display,parent,x,y,width,height,border_width,depth,class,visual,valuemask,attributes);
}

// Sound Patches
int snd_pcm_open(int **pcm, const char *name, int* stream, int mode){
    char pcm_endpoint[16] = {0x00};
    sound_resolve_pcm_endpoint(pcm_endpoint);
    return real_snd_pcm_open(pcm,pcm_endpoint,stream,mode);
}



// Deadlock Patches - Sigaction Swap
int sigaction(int signum, int *act,int *oldact){

    // Because the game had to be driven by fucking SIGALRM
    if(signum == SIGALRM){
        deadlock_swap_timer_handler(act);
    }

    return real_sigaction(signum,act,oldact);
}

// Deadlock Patches - Calling timer driver from Main thread
int XPending(int *display){
    drive_timer();
    return real_XPending(display);
}


// Exit if a hook failed.
void hook_validate(void* func_addr,char* func_name){
    if(!func_addr){
        printf("error: %s not found\n");
        exit(-1);
    }

}

// Resolve real functions.
void resolve_hooks(){
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    hook_validate(real_ioctl,"ioctl");


    real_lua_dofile = dlsym(RTLD_NEXT,"lua_dofile");
    hook_validate(real_lua_dofile,"lua_dofile");


    real_fopen = dlsym(RTLD_NEXT, "fopen");
    hook_validate(real_fopen,"fopen");

    real_open = dlsym(RTLD_NEXT, "open");
    hook_validate(real_open,"open");

    real_XCreateWindow = dlsym(RTLD_NEXT,"XCreateWindow");
    hook_validate(real_XCreateWindow,"XCreateWindow");

    real_snd_pcm_open = dlsym(RTLD_NEXT,"snd_pcm_open");
    hook_validate(real_snd_pcm_open,"snd_pcm_open");

    real_sigaction = dlsym(RTLD_NEXT,"sigaction");
    hook_validate(real_sigaction,"sigaction");

    real_XPending = dlsym(RTLD_NEXT,"XPending");
    hook_validate(real_XPending,"XPending");
}