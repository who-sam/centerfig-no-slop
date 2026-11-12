#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>

static struct termios orig;
/*
struct termios {
    tcflag_t c_iflag;   // input flags    (how incoming bytes are processed)
    tcflag_t c_oflag;   // output flags   (how outgoing bytes are processed)
    tcflag_t c_cflag;   // control flags  (baud, data bits, parity... mostly serial)
    tcflag_t c_lflag;   // local flags    (echo, canonical mode, signals)
    cc_t     c_cc[NCCS];// special characters + read() timing (VMIN/VTIME)
    // (plus speed fields on some systems; use the cf* functions for those)
};
*/


void cleanup(void){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
  printf("\x1b[?25h\x1b[2J\x1b[H");
  fflush(stdout);
}

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


//===========================================================


int main() {
  int rows, cols;
  int c_row, c_col;
  char key;
  
  atexit(cleanup);

  if (tcgetattr(STDIN_FILENO, &orig) == -1) {
    perror("tcgetattr");
    return 1;
  }

  struct termios copy = orig;
  copy.c_lflag &= ~(ICANON | ECHO);
  copy.c_cc[VMIN] = 0;
  copy.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &copy) == -1) {
    perror("tcsetattr");
    return 1;
  }

  printf("\x1b[?25l");
  fflush(stdout);

  while (read(STDIN_FILENO, &key, 1) < 1) {
    if (get_window_size(&rows, &cols) == -1) return 1;

    c_row = rows / 2;
    c_col = (cols - (int)strlen("Hello")) / 2 + 1;
    if (c_col < 1) c_col = 1;

    printf("\x1b[2J\x1b[H");
    printf("\x1b[%d;%dHHello", c_row, c_col);
    fflush(stdout);

    usleep(100000);
  }
  return 0;
}
