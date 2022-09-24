/*
 * Various Housekeeping Utility Operations
 */

#include "util.h"
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <libconfig.h>

// Helpers
#define UNPROTECT(addr,len) (mprotect((void*)(addr-(addr%len)),len,PROT_READ|PROT_WRITE|PROT_EXEC))
void detour_function(void * new_adr, int addr){
    int call = (int)(new_adr - addr - 5);
    UNPROTECT(addr, 4096);
    *((unsigned char*)(addr)) = 0xE8;
    *((int*)(addr + 1)) = call;
}

// Configuration Parsing Logic
char dog_filename[64];
char sound_device[16];
char hdd_filename[4096];
char emulate_mk6io;
char use_keyboard;
static config_t cfg;
void init_config(){
    // Parse Configuration File
    const char* str;
    config_init(&cfg);
    if(! config_read_file(&cfg, "./config.cfg")) {
        printf("Cant Open ./config.cfg!\n");
        exit(-1);
    }
    config_lookup_string(&cfg, "sound_device", &str);
    strcpy(sound_device,str);
    config_lookup_string(&cfg, "dongle_name", &str);
    strcpy(dog_filename,str);
    config_lookup_string(&cfg, "hdd_name", &str);
    strcpy(hdd_filename,str);
    config_lookup_string(&cfg,"emulate_mk6io",&str);
    if(strcmp(str,"true")){
        emulate_mk6io = 1;
    }else{
        emulate_mk6io = 0;
    }
    config_lookup_string(&cfg,"use_keyboard",&str);
    if(strcmp(str,"true")){
        use_keyboard = 1;
    }else{
        use_keyboard = 0;
    }
    config_destroy(&cfg);
}
// Game version number - used to denote which SCRIPT directory to read from.
int game_version;

// Root path of the ELF - useful for relative path resolution.
char root_path[4096];
void get_root_path(){
    memset(root_path,0x00,4096);
    strcpy(root_path,get_current_dir_name());
}


// Detects the game version based on location of the version string.
// Bad practice - but only 3 versions exist so whatever.
void detect_game_version(){
    // Version 1.01 Test
    if(strncmp((const char*)0x080FE469,"(BUILD",6) == NULL){
        game_version = 101;
    }
    // Version 1.02 Test
    else if(strncmp((const char*)0x080FE5C9,"(BUILD",6) == NULL){
        game_version = 102;
    }
    // Version 1.03 Test
    else if(strncmp((const char*)0x080FF777, "(BUILD", 6) == NULL){
        game_version = 103;
    }else{
        // Version didn't match anything - maybe modified.
        game_version = 0;
    }
}

