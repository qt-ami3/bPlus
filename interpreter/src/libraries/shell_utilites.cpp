using namespace std;
#include <cstdlib>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include "../../include/libraries/shell_utilites.h"

void setBufferedInput(bool enable) { //sets terminal input to repeat and detect without `enter`.
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
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    enabled = false;
  }
}

void clearScreen() {
  #ifdef _WIN32
    system("cls");
  #else
    cout<<"\033[2J\033[H";
  #endif
}
