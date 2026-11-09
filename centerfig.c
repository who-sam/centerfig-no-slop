#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>



int get_window_size(int *rows, int *cols) {
  struct winsize ws;
  /*
  struct winsize
    {
      unsigned short int ws_row;
      unsigned short int ws_col;
      unsigned short int ws_xpixel;
      unsigned short int ws_ypixel;
    };
  */

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
    perror("ioctl(TIOCGWINSZ)");
    return -1;
  }

  *rows = ws.ws_row;
  *cols = ws.ws_col;
  return 0;
}

void show_cursor(void) {
  printf("\x1b[?25h\x1b[2J\x1b[H");
  fflush(stdout);
}

void handle_sigint(int sig) {
  (void)sig;
  show_cursor();
  _exit(0);
}

//===========================================================


int main() {
  int rows, cols;
  int c_row, c_col;

  atexit(show_cursor);
  signal(SIGINT, handle_sigint);

  printf("\x1b[?25l");
  fflush(stdout);

  while (1) {
    if (get_window_size(&rows, &cols) == -1) return 1;

    c_row = rows / 2;
    c_col = cols / 2 - (int)strlen("Hello") / 2;
    if (c_col < 1) c_col = 1;

    printf("\x1b[2J\x1b[H");
    printf("\x1b[%d;%dHHello", c_row, c_col);
    fflush(stdout);

    sleep(1);
  }
  return 0;
}
