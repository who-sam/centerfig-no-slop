#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t resized = 1;
static const char *msg = "Hello";
static struct termios orig;
// struct termios {
//     tcflag_t c_iflag;   // input flags    (how incoming bytes are processed)
//     tcflag_t c_oflag;   // output flags   (how outgoing bytes are processed)
//     tcflag_t c_cflag;   // control flags  (baud, data bits, parity... mostly
//     serial) tcflag_t c_lflag;   // local flags    (echo, canonical mode,
//     signals) cc_t     c_cc[NCCS];// special characters + read() timing
//     (VMIN/VTIME)
//     // (plus speed fields on some systems; use the cf* functions for those)
// };

static void signal_handler(int signo) {
  switch (signo) {
  case SIGINT:
    running = 0;
    break;
  case SIGTERM:
    running = 0;
    break;
  case SIGWINCH:
    resized = 1;
    break;
  }
}

static void cleanup(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
  printf("\x1b[?25h\x1b[2J\x1b[H");
  fflush(stdout);
}

static int get_window_size(int *rows, int *cols) {
  struct winsize ws;
  // struct winsize
  //   {
  //     unsigned short int ws_row;
  //     unsigned short int ws_col;
  //     unsigned short int ws_xpixel;
  //     unsigned short int ws_ypixel;
  //   };

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
    perror("ioctl(TIOCGWINSZ)");
    return -1;
  }

  *rows = ws.ws_row;
  *cols = ws.ws_col;
  return 0;
}

static void draw(int col, int row) {
  printf("\x1b[2J\x1b[H");
  printf("\x1b[%d;%dH%s", row, col, msg);
  fflush(stdout);
}

//===========================================================

int main(void) {
  int rows, cols;
  int c_row, c_col;
  char key;

  if (tcgetattr(STDIN_FILENO, &orig) == -1) {
    perror("tcgetattr");
    return 1;
  }

  atexit(cleanup);

  struct sigaction sa;
  // struct sigaction {
  //   void     (*sa_handler)(int);                         // simple handler
  //   void     (*sa_sigaction)(int, siginfo_t *, void *);  // advanced handler
  //   (needs SA_SIGINFO) sigset_t   sa_mask; // extra signals to block during
  //   handler int        sa_flags;                                 // option
  //   switches
  // };
  memset(&sa, 0, sizeof sa); // zero everything
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask); // block no extra signals
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
    return 1;
  }
  if (sigaction(SIGTERM, &sa, NULL) == -1) {
    perror("sigaction");
    return 1;
  }
  if (sigaction(SIGWINCH, &sa, NULL) == -1) {
    perror("sigaction");
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

  while (running) {

    if (resized) {
      if (get_window_size(&rows, &cols) == -1)
        return 1;

      c_row = rows / 2;
      c_col = (cols - (int)strlen(msg)) / 2 + 1;
      if (c_col < 1)
        c_col = 1;

      draw(c_col, c_row);

      resized = 0;
    }

    ssize_t n = read(STDIN_FILENO, &key, 1);
    if (n > 0)
      break;
    if (n == -1 && errno != EINTR)
      break; // real error
  }
  return 0;
}
