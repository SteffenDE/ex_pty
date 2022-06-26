#ifndef ERL_COMM_INCLUDED
#define ERL_COMM_INCLUDED

typedef unsigned char byte;

int read_cmd(int fd, byte *buf, int buf_size);
int write_cmd(int fd, byte *buf, int len);

#endif
