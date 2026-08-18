#include "terminal.h"
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

// This sets the terminal to its original state
void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

// This sets the terminal to raw mode
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ICANON | ECHO);
  // ICANON -> disable canonical mode, ECHO -> disable echo
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  // TCSAFLUSH -> flush the terminal buffer and wait for all
  // output to be written
}
