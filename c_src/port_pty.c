// code adapted from http://www.rkoucha.fr/tech_corner/pty_pdip.html
#define _XOPEN_SOURCE 600
#include "ei.h"
#include "erl_comm.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

typedef unsigned char byte;
// we use nouse_stdio to allow debugging on stderr
int ERL_READ = 3;
int ERL_WRITE = 4;
extern char **environ;

#define ERL_BUF_SIZE 1024

#define read_cmd_erl(buf) read_cmd(ERL_READ, buf, ERL_BUF_SIZE)
#define write_cmd_erl(buf, len) write_cmd(ERL_WRITE, buf, len)

#define PARENT_READ read_pipe[0]
#define PARENT_WRITE write_pipe[1]
#define CHILD_WRITE read_pipe[1]
#define CHILD_READ write_pipe[0]

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define SRCLOC " [" __FILE__ ":" STR(__LINE__) "]"

#define debug 0
#define DEBUG(Cond, Fmt, ...)                                                  \
  if (Cond)                                                                    \
    fprintf(stderr, Fmt SRCLOC "\r\n", ##__VA_ARGS__);

static void fail(int place) {
  fprintf(stderr, "Something went wrong %d\n", place);
  exit(1);
}

// tty opts magic

void set_tty_opt(struct termios *tio, char atom[], long value) {
#define STRLEN(s) (sizeof(s) / sizeof(s[0]))
#define TTYCHAR(NAME, VALUE)                                                   \
  if (strncmp(atom, VALUE, STRLEN(VALUE)) == 0) {                              \
    DEBUG(debug, "set tty_char %s\r\n", #NAME);                                \
    tio->c_cc[NAME] = value;                                                   \
    return;                                                                    \
  }

#define TTYMODE(NAME, FIELD, VALUE)                                            \
  if (strncmp(atom, VALUE, STRLEN(VALUE)) == 0) {                              \
    if (value) {                                                               \
      DEBUG(debug, "enable %s = %lu\r\n", #NAME, value);                       \
      tio->FIELD |= NAME;                                                      \
    } else {                                                                   \
      DEBUG(debug, "disable %s = %lu\r\n", #NAME, value);                      \
      tio->FIELD &= ~NAME;                                                     \
    }                                                                          \
    return;                                                                    \
  }

#define TTYSPEED(NAME, FIELD, VALUE)                                           \
  if (strncmp(atom, VALUE, STRLEN(VALUE)) == 0) {                              \
    tio->FIELD = value;                                                        \
    return;                                                                    \
  }

#include "ttymodes.h"

#undef TTYCHAR
#undef TTYMODE
#undef STRLEN
}

// -----------------------------------------------------

int main(int argc, char *argv[]) {
  // file descriptor master and slave
  int fdm, fds;
  // variable for return code
  int rc;

  DEBUG(debug, "i am %d\r\n", getpid());

  // see man pty, https://man7.org/linux/man-pages/man7/pty.7.html
  // open master side of pty
  fdm = posix_openpt(O_RDWR);
  if (fdm < 0) {
    DEBUG(debug, "Error %d on posix_openpt()\r\n", errno);
    return 1;
  }
  rc = grantpt(fdm);
  if (rc != 0) {
    DEBUG(debug, "Error %d on grantpt()\r\n", errno);
    return 1;
  }
  rc = unlockpt(fdm);
  if (rc != 0) {
    DEBUG(debug, "Error %d on unlockpt()\r\n", errno);
    return 1;
  }

  int child;
  int read_pipe[2];
  int write_pipe[2];

  if (pipe(read_pipe) == -1)
    fail(__LINE__);
  if (pipe(write_pipe) == -1)
    fail(__LINE__);

  // for ei command parsing
  byte buf[ERL_BUF_SIZE];
  int index = 0;
  int version = 0;
  int arity = 0;
  int type, size;
  char atom[128];
  char *data = NULL;
  int res = 0;
  erlang_ref reply_ref;
  ei_x_buff res_buf;

  // int res = 0;
  // ei_x_buff res_buf;
  if (ei_init() != 0)
    fail(__LINE__);

  // Create the child process
  if ((child = fork())) {
    close(CHILD_READ);
    close(CHILD_WRITE);

    fd_set fd_in;

    // FATHER

    // we start in command mode (0)
    // where messages are exchanged between parent and child
    int mode = 0;

    while (1) {
      // Wait for data from standard input and master side of PTY
      // using select
      FD_ZERO(&fd_in);
      FD_SET(ERL_READ, &fd_in);
      FD_SET(PARENT_READ, &fd_in);
      FD_SET(fdm, &fd_in);

      int max_fd = PARENT_READ;
      if (fdm > PARENT_READ) {
        max_fd = fdm;
      }
      rc = select(max_fd + 1, &fd_in, NULL, NULL, NULL);
      if (rc == -1) {
        DEBUG(debug, "Error %d on select()\r\n", errno);
        exit(1);
      }

      // data on erlang input
      if (FD_ISSET(ERL_READ, &fd_in)) {
        rc = read_cmd_erl(buf);
        if (rc > 0) {
          if (ei_decode_version(buf, &index, &version) != 0)
            fail(__LINE__);
          if (ei_decode_tuple_header(buf, &index, &arity) != 0)
            fail(__LINE__);
          if (arity != 2 && arity != 3 && arity != 4) {
            fail(__LINE__);
          }
          if (ei_decode_atom(buf, &index, atom) != 0)
            fail(__LINE__);

          if (strncmp(atom, "data", 4) == 0) {
            if (ei_get_type(buf, &index, &type, &size) != 0)
              fail(__LINE__);

            data = (char *)malloc(size + 1);
            if (data == NULL) {
              fail(__LINE__);
            }

            if (ei_decode_binary(buf, &index, data, (long *)&size) < 0)
              fail(__LINE__);

            data[size] = '\0';

            // write to pty master
            write(fdm, data, size);
            free(data);
          } else if (strncmp(atom, "winsz", 5) == 0) {
            write_cmd(PARENT_WRITE, buf, rc);
          } else if (strncmp(atom, "pty_opts", 9) == 0) {
            // we must change the pty settings from the slave side, just relay
            // the message to the child
            write_cmd(PARENT_WRITE, buf, rc);
          } else if (strncmp(atom, "exec", 4) == 0) {
            mode = 1;
            write_cmd(PARENT_WRITE, buf, rc);
          } else {
            DEBUG(debug, "other command!\r\n");
            fail(__LINE__);
          }
          // reset index for new message
          index = 0;
        } else {
          DEBUG(debug, "Error %d on read standard input\r\n", errno);
          exit(1);
        }
      }

      // data from child on master side of PTY
      if (FD_ISSET(fdm, &fd_in)) {
        rc = read(fdm, buf, ERL_BUF_SIZE);
        if (rc > 0) {
          // send data to erlang
          if (ei_x_new_with_version(&res_buf) != 0)
            fail(__LINE__);
          if (ei_x_encode_tuple_header(&res_buf, 2) != 0)
            fail(__LINE__);
          if (ei_x_encode_atom(&res_buf, "data") != 0)
            fail(__LINE__);
          if (ei_x_encode_binary(&res_buf, buf, rc) != 0)
            fail(__LINE__);
          write_cmd_erl(res_buf.buff, res_buf.index);
          if (ei_x_free(&res_buf) != 0)
            fail(__LINE__);
        } else {
          if (rc <= 0) {
            DEBUG(debug, "Error %d on read master PTY\r\n", errno);
            exit(1);
          }
        }
      }

      if (FD_ISSET(PARENT_READ, &fd_in)) {
        rc = read_cmd(PARENT_READ, buf, ERL_BUF_SIZE);
        if (rc > 0) {
          write_cmd_erl(buf, rc);
        } else {
          if (rc <= 0) {
            DEBUG(debug, "Error %d on read parent\r\n", errno);
            exit(1);
          }
        }
      }
    }
  } else {
    char **child_av;
    char **env;
    DEBUG(debug, "hello from child %d\r\n", child);
    index = 0;
    // CHILD
    close(PARENT_READ);
    close(PARENT_WRITE);

    // open the slave side of the PTY; important that this happens in the
    // child otherwise we get ownership errors
    fds = open(ptsname(fdm), O_RDWR);

    close(fdm);

    while (1) {
      rc = read_cmd(CHILD_READ, buf, ERL_BUF_SIZE);
      if (rc > 0) {
        if (ei_decode_version(buf, &index, &version) != 0)
          fail(__LINE__);
        if (ei_decode_tuple_header(buf, &index, &arity) != 0)
          fail(__LINE__);
        if (arity != 2 && arity != 3 && arity != 4) {
          fail(__LINE__);
        }
        if (ei_decode_atom(buf, &index, atom) != 0)
          fail(__LINE__);

        if (strncmp(atom, "winsz", 5) == 0) {
          // erlang requests window size change {winsz, rows, cols}
          if (ei_decode_ref(buf, &index, &reply_ref) != 0)
            fail(__LINE__);
          long rows, cols;
          if (ei_decode_long(buf, &index, &rows) != 0)
            fail(__LINE__);
          if (ei_decode_long(buf, &index, &cols) != 0)
            fail(__LINE__);
          struct winsize ws;
          ws.ws_row = (int)rows;
          ws.ws_col = (int)cols;
          int r = ioctl(fds, TIOCSWINSZ, &ws);
          DEBUG(debug, "TIOCSWINSZ rows=%ld cols=%ld ret=%d\r\n", rows, cols,
                r);

          // answer :ok | {:error, r}
          if (ei_x_new_with_version(&res_buf) != 0)
            fail(__LINE__);
          if (ei_x_encode_tuple_header(&res_buf, 3) != 0)
            fail(__LINE__);
          if (ei_x_encode_atom(&res_buf, "response") != 0)
            fail(__LINE__);
          if (ei_x_encode_ref(&res_buf, &reply_ref) != 0)
            fail(__LINE__);
          if (r != 0) {
            if (ei_x_encode_tuple_header(&res_buf, 2) != 0)
              fail(__LINE__);
            if (ei_x_encode_atom(&res_buf, "error") != 0)
              fail(__LINE__);
            if (ei_x_encode_long(&res_buf, r) != 0)
              fail(__LINE__);
          } else {
            if (ei_x_encode_atom(&res_buf, "ok") != 0)
              fail(__LINE__);
          }

          write_cmd(CHILD_WRITE, res_buf.buff, res_buf.index);
          if (ei_x_free(&res_buf) != 0)
            fail(__LINE__);
        } else if (strncmp(atom, "pty_opts", 9) == 0) {
          // erlang wants to set pty opts
          // format is a keyword list, see
          // https://www.erlang.org/doc/man/ssh_connection.html#type-term_mode
          if (ei_decode_ref(buf, &index, &reply_ref) != 0)
            fail(__LINE__);
          if (ei_decode_list_header(buf, &index, &arity) != 0)
            fail(__LINE__);
          int list_length = arity;
          struct termios ios;
          tcgetattr(fds, &ios);
          for (int i = 0; i < list_length; i++) {
            if (ei_decode_tuple_header(buf, &index, &arity) != 0)
              fail(__LINE__);
            if (arity != 2) {
              fail(__LINE__);
            }
            if (ei_decode_atom(buf, &index, atom) != 0)
              fail(__LINE__);

            long value;
            if (ei_decode_long(buf, &index, &value) != 0)
              fail(__LINE__);

            set_tty_opt(&ios, atom, value);
          }
          tcsetattr(fds, TCSANOW, &ios);

          // answer
          if (ei_x_new_with_version(&res_buf) != 0)
            fail(__LINE__);
          if (ei_x_encode_tuple_header(&res_buf, 3) != 0)
            fail(__LINE__);
          if (ei_x_encode_atom(&res_buf, "response") != 0)
            fail(__LINE__);
          if (ei_x_encode_ref(&res_buf, &reply_ref) != 0)
            fail(__LINE__);
          if (ei_x_encode_atom(&res_buf, "ok") != 0)
            fail(__LINE__);

          write_cmd(CHILD_WRITE, res_buf.buff, res_buf.index);
          if (ei_x_free(&res_buf) != 0)
            fail(__LINE__);
        } else if (strncmp(atom, "exec", 4) == 0) {
          if (ei_decode_list_header(buf, &index, &arity) != 0)
            fail(__LINE__);

          int i;
          // build the command line
          child_av = (char **)malloc((arity + 1) * sizeof(char *));
          for (i = 1; i <= arity; i++) {
            if (ei_get_type(buf, &index, &type, &size) != 0)
              fail(__LINE__);

            data = (char *)malloc(size + 1);
            if (data == NULL) {
              fail(__LINE__);
            }

            if (ei_decode_binary(buf, &index, data, (long *)&size) < 0)
              fail(__LINE__);

            data[size] = '\0';

            DEBUG(debug, "command part: %s\r\n", data);
            child_av[i - 1] = strdup(data);
            free(data);
          }
          child_av[i - 1] = NULL;
          // decode tail of list
          if (ei_decode_list_header(buf, &index, &arity) != 0)
            fail(__LINE__);

          // decode env
          if (ei_decode_list_header(buf, &index, &arity) != 0)
            fail(__LINE__);
          DEBUG(debug, "env arity %d\r\n", arity);
          env = (char **)malloc((arity + 1) * sizeof(char *));
          for (i = 1; i <= arity; i++) {
            if (ei_get_type(buf, &index, &type, &size) != 0)
              fail(__LINE__);

            data = (char *)malloc(size + 1);
            if (data == NULL) {
              fail(__LINE__);
            }

            if (ei_decode_binary(buf, &index, data, (long *)&size) < 0)
              fail(__LINE__);

            data[size] = '\0';

            DEBUG(debug, "env part %d: %s\r\n", size, data);
            env[i - 1] = strdup(data);
            free(data);
          }
          env[i - 1] = NULL;

          if (fork()) {
            DEBUG(debug, "forked!\r\n");
          } else {
            // child breaks
            break;
          }
        } else {
          DEBUG(debug, "other command!\r\n");
          fail(__LINE__);
        }

        index = 0;
      } else {
        if (rc <= 0) {
          DEBUG(debug, "Error %d on read slave PTY stdin\r\n", errno);
          exit(1);
        }
      }
    }

    DEBUG(debug, "hello from exec child %d\r\n", getpid());
    // slave side of the PTY becomes the standard input and outputs of
    // the child process
    close(0); // close standard input (current terminal)
    close(1); // close standard output (current terminal)
    close(2); // close standard error (current terminal)

    dup(fds); // PTY becomes standard input (0)
    dup(fds); // PTY becomes standard output (1)
    dup(fds); // PTY becomes standard error (2)

    // now the original file descriptor is useless
    close(fds);

    // make the current process a new session leader
    setsid();

    // As the child is a session leader, set the controlling terminal to
    // be the slave side of the PTY (Mandatory for programs like the
    // shell to make them manage correctly their outputs)
    ioctl(0, TIOCSCTTY, 1);

    // try to set process group id
    setpgid(0, 0);

    environ = env;
    rc = execvp(child_av[0], child_av);
    // send exit code to erlang
    if (ei_x_new_with_version(&res_buf) != 0)
      fail(__LINE__);
    if (ei_x_encode_tuple_header(&res_buf, 2) != 0)
      fail(__LINE__);
    if (ei_x_encode_atom(&res_buf, "exit") != 0)
      fail(__LINE__);
    if (ei_x_encode_long(&res_buf, errno) != 0)
      fail(__LINE__);
    write_cmd_erl(res_buf.buff, res_buf.index);
    if (ei_x_free(&res_buf) != 0)
      fail(__LINE__);

    free(child_av);
    free(env);
  }
  return 0;
}
