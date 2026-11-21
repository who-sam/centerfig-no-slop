#define _POSIX_C_SOURCE 200809L
#include <bits/time.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

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

static long long diff_ns(struct timespec start, struct timespec end) {
  return (long long)(end.tv_sec - start.tv_sec) * 1000000000LL +
         (end.tv_nsec - start.tv_nsec);
}

static void draw(int rows, int cols) {
  int c_row = rows / 2 + 3;
  int c_col = (cols - (int)strlen(msg)) / 2 + 1;
  if (c_col < 1)
    c_col = 1;
  printf("\x1b[2J\x1b[H");
  printf("\x1b[%d;%dH%s", c_row, c_col, msg);
  fflush(stdout);
}

static void draw_timer(int rows, int cols, long long total) {
  int c_row = rows / 2;
  int c_col = (cols - 8) / 2 + 1;
  if (c_col < 1)
    c_col = 1;
  long long hours = total / 3600;
  long long mins = (total % 3600) / 60;
  long long secs = total % 60;
  printf("\x1b[%d;%dH%02lld:%02lld:%02lld", c_row, c_col, hours, mins, secs);
  fflush(stdout);
}

//===========================================================

int main(void) {
  int rows, cols;
  char key;
  long long last_shown = -1;

  struct timespec start, now;
  struct timespec delay = {0, 20 * 1000000LL};
  //   struct timespec {
  //     time_t tv_sec;   // whole seconds
  //     long   tv_nsec;  // nanoseconds (0 to 999,999,999)
  //=======================
  // };

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
  copy.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &copy) == -1) {
    perror("tcsetattr");
    return 1;
  }

  printf("\x1b[?25l");
  fflush(stdout);

  if (clock_gettime(CLOCK_MONOTONIC, &start)) {
    perror("clock_gettime");
    return 1;
  }

  //=======================
  while (running) {
    clock_gettime(CLOCK_MONOTONIC, &now);
    long long total = diff_ns(start, now) / 1000000000LL;

    if (resized) {
      if (get_window_size(&rows, &cols) == -1)
        return 1;

      draw(rows, cols);
      draw_timer(rows, cols, total);

      resized = 0;
    }

    if (total != last_shown) {
      draw_timer(rows, cols, total);
      last_shown = total;
    }

    ssize_t n = read(STDIN_FILENO, &key, 1);
    if (n > 0)
      break;
    if (n == -1 && errno != EINTR)
      break; // real error

    nanosleep(&delay, NULL);
  }
  //=======================
  return 0;
}
