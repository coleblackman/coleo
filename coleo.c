/*** includes ***/

// ctype.h provides functions for testing characters
#include <ctype.h>

#include <sys/ioctl.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <termios.h>
#include <unistd.h>

/*** defines ***/

// We want this to be the quit function
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig {
  struct termios orig_termios;
};

struct editorConfig E;


/*** terminal ***/

// Error handling, prints error message and exits
void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  // Prints descriptive message
  perror(s);
  // Exit status of non-zero means failure
  exit(1);
}

void disableRawMode() {
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsettattr");
}

void enableRawMode() {
  // We don't want to be in canonical mode - where keyboard input is only sent
  // when the user presses enter. We want raw mode - where we process each keypress as it occurs

  /* Turning off ECHO mode
   *
   * Note that a struct(ure) in C/C++ is a data type that groups items of different types into a single type
   * tcgetattr(), from <termios.h>, reads the current attributes of the terminal into a struct
   * tcsetattr(), from <termios.h>, can set those terminal attributes
   * TCSAFLUSH is one of the possible arguments for tcsetattr() and tells it how to deal with queued I/O
   * Other options: TCSANOW (the change shall occur immediately), TCSADRAIN ( the change shall occur after all output written to fildes is transmitted. This function should be used when changing parameters that affect output) 
   */
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  // Use the globally defined structure containing terminal attributes at start
  struct termios raw = E.orig_termios;

  
  // Set this bit to 0, "&=" means bitwise AND, "~" is bitwise negation. 
  // ICANON is a canonical mode flag, we want to disable it
  // ISIG disables <C-c> and <C-z>, ISIG disables <C-s> and <C-q>, IEXTEN disables <C-v>
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  // VMIN sets minimum number of bytes of input before read() can return
  // VTIME sets maximum amount of time to wait before read() returns (tenths of a second)
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  // STDIN_FILENO is a macro that is always 0, but we use the name for readability. 
  // TCSAFlUSH discards unread input after execution before applying terminal changes
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");

}

// Waits for a keypress and returns it
char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  return c;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/** output **/

void editorDrawRows() {
  int y;
  for(y = 0; y < 24; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void editorRefreshScreen() {
  // Clear screen VT100 escape codes
  write(STDOUT_FILENO, "\x1b[2J", 4);
  // Reposition cursor
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** input ***/
void editorProcessKeypress() {
  char c = editorReadKey();
  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
  }
}

/*** init ***/

int main() {
  enableRawMode();
  while (1) { 
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
