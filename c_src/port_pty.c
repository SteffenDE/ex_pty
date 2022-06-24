// code adapted from http://www.rkoucha.fr/tech_corner/pty_pdip.html
#include "ei.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

// we use nouse_stdio to allow debugging on stderr
int ERL_READ = 3;
int ERL_WRITE = 4;

static void fail(int place) {
  fprintf(stderr, "Something went wrong %d\n", place);
  exit(1);
}

// see https://www.erlang.org/doc/tutorial/erl_interface.html
int read_exact(char *buf, int len) {
  int i, got = 0;

  do {
    if ((i = read(ERL_READ, buf + got, len - got)) <= 0) {
      return (i);
    }
    got += i;
  } while (got < len);

  return (len);
}

int write_exact(char *buf, int len) {
  int i, wrote = 0;

  do {
    if ((i = write(ERL_WRITE, buf + wrote, len - wrote)) <= 0)
      return (i);
    wrote += i;
  } while (wrote < len);

  return (len);
}

// we use packet 2 encoding

int read_cmd(char *buf) {
  int len;
  if (read_exact(buf, 2) != 2)
    return (-1);
  len = (buf[0] << 8) | buf[1];
  return read_exact(buf, len);
}

int write_cmd(char *buf, int len) {
  char li;

  li = (len >> 8) & 0xff;
  write_exact(&li, 1);

  li = len & 0xff;
  write_exact(&li, 1);

  return write_exact(buf, len);
}

// -----------------------------------------------------

int main(int argc, char *argv[]) {
  // file descriptor master and slave
  int fdm, fds;
  // variable for return code
  int rc;
  char input[150];

  // Check arguments
  if (argc <= 1) {
    fprintf(stderr, "Usage: %s program_name [parameters]\r\n", argv[0]);
    exit(1);
  }

  // see man pty, https://man7.org/linux/man-pages/man7/pty.7.html
  // open master side of pty
  fdm = posix_openpt(O_RDWR);
  if (fdm < 0) {
    fprintf(stderr, "Error %d on posix_openpt()\r\n", errno);
    return 1;
  }
  rc = grantpt(fdm);
  if (rc != 0) {
    fprintf(stderr, "Error %d on grantpt()\r\n", errno);
    return 1;
  }
  rc = unlockpt(fdm);
  if (rc != 0) {
    fprintf(stderr, "Error %d on unlockpt()\r\n", errno);
    return 1;
  }

  int child;
  // Create the child process
  if ((child = fork())) {
    fd_set fd_in;

    // FATHER

    // Close the slave side of the PTY
    close(fds);

    // for ei command parsing
    char buf[100];
    int index = 0;
    int version = 0;
    int arity = 0;
    int type, size;
    char atom[128];
    char *data = NULL;

    // int res = 0;
    // ei_x_buff res_buf;
    ei_init();

    while (1) {
      // Wait for data from standard input and master side of PTY
      // using select
      FD_ZERO(&fd_in);
      FD_SET(ERL_READ, &fd_in);
      FD_SET(fdm, &fd_in);

      rc = select(fdm + 1, &fd_in, NULL, NULL, NULL);
      if (rc == -1) {
        fprintf(stderr, "Error %d on select()\r\n", errno);
        exit(1);
      }

      // data on erlang input
      if (FD_ISSET(ERL_READ, &fd_in)) {
        if (read_cmd(buf) > 0) {
          if (ei_decode_version(buf, &index, &version) != 0)
            fail(1);
          if (ei_decode_tuple_header(buf, &index, &arity) != 0)
            fail(2);
          if (arity != 2 && arity != 3) {
            fail(3);
          }
          if (ei_decode_atom(buf, &index, atom) != 0)
            fail(4);

          if (strncmp(atom, "data", 4) == 0) {
            if (ei_get_type(buf, &index, &type, &size) != 0)
              fail(5);

            data = (char *)malloc(size);
            if (data == NULL) {
              fail(666);
            }

            if (ei_decode_binary(buf, &index, data, (long *)&size) < 0)
              fail(11);

            // write to pty master
            write(fdm, data, size);
          } else if (strncmp(atom, "winsz", 5) == 0) {
            // erlang requests window size change {winsz, rows, cols}
            long rows, cols;
            if (ei_decode_long(buf, &index, &rows) != 0)
              fail(23);
            if (ei_decode_long(buf, &index, &cols) != 0)
              fail(24);
            struct winsize ws;
            ws.ws_row = (int)rows;
            ws.ws_col = (int)cols;
            int r = ioctl(fdm, TIOCSWINSZ, &ws);
            fprintf(stderr, "TIOCSWINSZ rows=%ld cols=%ld ret=%d\r\n", rows,
                    cols, r);
          }

          // if (ei_x_new_with_version(&res_buf) != 0)
          //     fail(6);
          // if (ei_x_encode_long(&res_buf, res) != 0)
          //     fail(7);
          // write_cmd(res_buf.buff, res_buf.index);

          // if (ei_x_free(&res_buf) != 0)
          //     fail(8);
          index = 0;
        } else {
          fprintf(stderr, "Error %d on read standard input\r\n", errno);
          exit(1);
        }
      }

      // data from child on master side of PTY
      if (FD_ISSET(fdm, &fd_in)) {
        rc = read(fdm, input, sizeof(input));
        if (rc > 0) {
          // send data to erlang
          write_cmd(input, rc);
        } else {
          if (rc <= 0) {
            fprintf(stderr, "Error %d on read master PTY\r\n", errno);
            exit(1);
          }
        }
      }
    }
  } else {
    // CHILD
    // open the slave side of the PTY; important that this happens in the
    // child otherwise we get ownership errors
    fds = open(ptsname(fdm), O_RDWR);

    // close the master side of the PTY
    close(fdm);

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

    // As the child is a session leader, set the controlling terminal to be
    // the slave side of the PTY (Mandatory for programs like the shell to
    // make them manage correctly their outputs)
    ioctl(0, TIOCSCTTY, 1);

    // try to set process group id
    setpgid(0, 0);

    // execution of the program
    {
      char **child_av;
      int i;

      // build the command line
      child_av = (char **)malloc(argc * sizeof(char *));
      for (i = 1; i < argc; i++) {
        child_av[i - 1] = strdup(argv[i]);
      }
      child_av[i - 1] = NULL;
      rc = execvp(child_av[0], child_av);
    }

    // program exited
    return rc;
  }
  return 0;
}
