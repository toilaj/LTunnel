#ifndef _SERVER_H_
#define _SERVER_H_
#include "common.h"
#include "device.h"
#include "io.h"

struct connection {
    struct ev_loop *ev_loop;
    struct ev_io *ev_read_tun;
    struct ev_io *ev_read_net;
    int tun_fd;
    int net_fd;
};
int server_process(uint16_t port);
#endif