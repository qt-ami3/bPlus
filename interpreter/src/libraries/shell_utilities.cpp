using namespace std;
#include <cstdlib>
#include <iostream>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include "../../include/libraries/shell_utilities.h"

void set_buffered_input(bool enable) { //sets terminal input to repeat and detect without `enter`.
  static bool enabled = true;
  static struct termios old;
  struct termios newt;

  if (enable && !enabled) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    enabled = true;
  }
  else if (!enable && enabled) {
    tcgetattr(STDIN_FILENO, &old);
    newt = old;
    newt.c_lflag &= ~(ICANON | ECHO);

    //  VMIN and VTIME only take effect once ICANON is off, and whatever the
    //  terminal happened to carry decides whether a read blocks. Without
    //  setting them a read can return nothing at all, which looks exactly
    //  like a keypress being ignored. One byte, no timeout, is a blocking
    //  single-key read.
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    enabled = false;
  }
}

void clear_screen() {
  #ifdef _WIN32
    system("cls");
  #else
    cout<<"\033[2J\033[H";
  #endif
}

//  True when a byte is already waiting. Used to tell a bare ESC from the start
//  of an arrow key, which would otherwise block waiting for a second byte.
static bool byte_waiting(int milliseconds) {
  struct pollfd watch = {STDIN_FILENO, POLLIN, 0};
  return poll(&watch, 1, milliseconds) > 0;
}

//  read() rather than cin, so nothing sits in an iostream buffer where poll
//  cannot see it.
static bool next_byte(char& c, int milliseconds) {
  if (milliseconds >= 0 && !byte_waiting(milliseconds)) return false;
  return read(STDIN_FILENO, &c, 1) == 1;
}

void read_key(string& key) {
  key.clear();

  char c = 0;
  if (!next_byte(c, -1)) return;          //  End of input.

  if (c == 27) {                          //  ESC, possibly an arrow key.
    char bracket = 0;
    if (!next_byte(bracket, 20) || bracket != '[') { key = "escape"; return; }

    char arrow = 0;
    if (!next_byte(arrow, 20)) { key = "escape"; return; }

    if (arrow == 'A') key = "up";
    else if (arrow == 'B') key = "down";
    else if (arrow == 'C') key = "right";
    else if (arrow == 'D') key = "left";
    else key = "escape";
    return;
  }

  if (c == '\n' || c == '\r') { key = "enter"; return; }

  key = string(1, c);
}
