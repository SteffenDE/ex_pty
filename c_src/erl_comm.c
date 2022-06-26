// https://www.erlang.org/doc/tutorial/erl_interface.html
#include <stdio.h>
#include <unistd.h>

typedef unsigned char byte;

int read_exact(int fd, byte *buf, int len) {
  int i, got = 0;

  do {
    if ((i = read(fd, buf + got, len - got)) <= 0) {
      return (i);
    }
    got += i;
  } while (got < len);

  return (len);
}

int write_exact(int fd, byte *buf, int len) {
  int i, wrote = 0;

  do {
    if ((i = write(fd, buf + wrote, len - wrote)) <= 0)
      return (i);
    wrote += i;
  } while (wrote < len);

  return (len);
}

// we use packet 2 encoding

int read_cmd(int fd, byte *buf, int buf_size) {
  int len;

  if (read_exact(fd, buf, 2) != 2)
    return -1;

  len = (buf[0] << 8) | buf[1];
  if (len > buf_size)
    return -1;

  return read_exact(fd, buf, len);
}

int write_cmd(int fd, byte *buf, int len) {
  byte li;

  li = (len >> 8) & 0xff;
  write_exact(fd, &li, 1);

  li = len & 0xff;
  write_exact(fd, &li, 1);

  return write_exact(fd, buf, len);
}
