#ifndef __UTIL
#define __UTIL

extern int game_version;
extern char root_path[4096];
extern char dog_filename[64];
extern char sound_device[16];
extern char hdd_filename[4096];
extern char emulate_mk6io;
extern char use_keyboard;


void detour_function(void * new_adr, int addr);
// Functions
void detect_game_version();
void get_root_path();
void init_config();



#endif
