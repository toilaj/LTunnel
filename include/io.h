#ifndef _IO_H_
#define _IO_H_
#include "common.h"

#define FLAG_READ_FIX_LEN 1
#define FLAG_READ_POSSIBLE 0

int read_packet(int fd, void *buffer, int length, char flag);
int write_packet(int fd, void *buffer, int length);
#endif