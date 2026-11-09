#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

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

int main() {
  int rows, cols;
  while (1) {
    get_window_size(&rows, &cols);
    printf("rows = %d\n", rows);
    printf("cols = %d\n", cols);

    sleep(1);
  }
  return 0;
}
