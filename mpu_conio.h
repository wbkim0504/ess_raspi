
#ifndef _MPU_CONIO_H_
#define _MPU_CONIO_H_

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

void set_conio_terminal_mode();
int kbhit();
int getch();

#endif

