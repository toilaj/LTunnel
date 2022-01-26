#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>
#include <net/route.h>

#include "ev.h"

#define BUF_SIZE (1500)
#define HEADER_SIZE sizeof(struct packet_hdr)
#define PACKET_SIZE (BUF_SIZE + HEADER_SIZE)
#define CLIENT 0
#define SERVER 1
#define PORT 10443

#define IP_HDR_LEN 20
#define ETH_HDR_LEN 14
#define ARP_PKT_LEN 28

#define CMD_DATA 0x22330001
#define CMD_DATA_LEN 0x22330002


struct packet_hdr {
    unsigned int cmd;
    unsigned int len;
    char buf[0];
};



inline static void set_fd_nonblock(int fd) {
    int flags;
    if((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        perror("set_fd_nonblock() 1");
        exit(1);
    }
    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flags) < 0) {
        perror("set_fd_nonblock() 2");
        exit(1);
    } 
    return;
}

inline static int iface_configure(char* ifname, unsigned char *ip, unsigned char *mask)
{
    struct sockaddr_in *sin;
    struct ifreq ifr;
    int res = 0;
    int fd;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    res |= ioctl(fd, SIOCGIFFLAGS, &ifr);
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    res |= ioctl(fd, SIOCSIFFLAGS, &ifr);

    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    sin->sin_family = AF_INET;
    inet_aton(ip, &(sin->sin_addr));
    res |= ioctl(fd, SIOCSIFADDR, &ifr);
    
    inet_aton(mask, &(sin->sin_addr));
    res |= ioctl(fd, SIOCSIFNETMASK, &ifr);

    close(fd);
    return res;
}

void debug_log(char *msg, ...);
void err_log(char *msg, ...);
#endif