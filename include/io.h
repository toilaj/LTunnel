#ifndef _IO_H_
#define _IO_H_
#include "common.h"
int read_packet(int fd, void *buffer, int length);
int write_packet(int fd, void *buffer, int length);
#endif