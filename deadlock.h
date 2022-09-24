#ifndef __DEADLOCK_H
#define __DEADLOCK_H

#define SIGALRM 0x0E

void deadlock_swap_timer_handler(unsigned char* act);
void drive_timer();
#endif
