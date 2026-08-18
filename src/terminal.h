#ifndef TERMINAL_H
#define TERMINAL_H
#include <termios.h>

extern struct termios orig_termios;
void disableRawMode();
void enableRawMode();

#endif