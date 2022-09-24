/*
 * Deadlock Patch for Exceed->NX (Linux Kernel > 2.4.27)
 * Bunch of stuff, SIGALRM might get double-called where they
 * have it, moved it to XPending in main() to fix it.
 * Tried giving it its own thread, game runs too fast.
 */

#include "deadlock.h"
#include <string.h>

// Destination address of the timer thread.
static void* deadlock_timer_drive_addr = NULL;

// Replacement signal handler to keep the sigcall happy.
void empty_sig_handler(int sig){}

// Swap timer function with function that does nothing.
void deadlock_swap_timer_handler(unsigned char *act){
    unsigned int actptr;
    memcpy(&actptr,act,4);
    typedef int func(int);
    deadlock_timer_drive_addr = (func*)actptr;
    *(unsigned int*)act = empty_sig_handler;
}

// Replacement signal driver.
void drive_timer(){
    if(deadlock_timer_drive_addr!=NULL){
        typedef int func(int);
        func* tdr_func = (func*)deadlock_timer_drive_addr;
        tdr_func(SIGALRM);
    }
}

