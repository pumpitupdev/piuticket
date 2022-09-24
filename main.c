/*
    Main Entry-Point
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#include "hdd.h"

#include "util.h"

#include "hooks.h"
#include "dog.h"

// Entry Point.
void __attribute__((constructor)) initialize(void){
    // Get root path of the ELF to find helper files.
    get_root_path();

    // Detect current game version to help with SCRIPT redirection.
    detect_game_version();
    printf("Game Version: %d Detected!\n",game_version);

     // Resolve API hooks.
    /*
     * It's a good idea to do this first with the way things link, most
     * functions, well, any that use a function we're hooking will crash
     * if they run before this as I don't feel like checking if the
     * function pointer is null and reloading before using it.
     */
    resolve_hooks();

    // Get config.cfg values - WARNING, this has to happen after resolving hooks!
    init_config();

    // Get HDD ATA Data information from HDD file.
    // Note - This HAS to happen after resolving the hooks or shit breaks.
    get_ata_data();

    // Set up emulated dongle.
    // TODO - Make Dongle Path Configurable
    char dongle_path[4096] = {0x00};
    sprintf(dongle_path,"%s/%s",root_path,dog_filename);
    init_microdoge(dongle_path);

}