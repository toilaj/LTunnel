#include "io.h"
int read_packet(int fd, void *buffer, int length, char flag)
{
    int bytes_left = length;
    int bytes_read = 0;
    char *ptr = buffer;
    if(ptr == NULL) {
        return 0;
    }
    while(bytes_left > 0) {
        bytes_read = read(fd, ptr, bytes_left);
        if(bytes_read < 0) {
            if(errno == EINTR) {
                continue;
            } else if(errno == EAGAIN) {
                if(flag == FLAG_READ_FIX_LEN) {
                    continue;
                } else {
                    return length - bytes_left;
                }
            } else {
                return -1;
            }
        } else if(bytes_read == 0) {
            break;
        }
        bytes_left -= bytes_read;
        ptr += bytes_read;
    }
    return length - bytes_left;
}

int write_packet(int fd, void *buffer, int length)
{
    int bytes_left = length;
    int written_bytes;
    char *ptr = buffer;
    if(ptr == NULL) {
        return 0;
    }
    while(bytes_left > 0) {
        written_bytes = write(fd, ptr, bytes_left);
        if(written_bytes <= 0) {
            if(errno == EINTR) {
                written_bytes = 0;
            } else {
                return -1;
            }
        }
        bytes_left -= written_bytes;
        ptr += written_bytes;
    }
    return length - bytes_left;
}