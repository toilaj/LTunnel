#include "device.h"

static int tun_alloc(int flags) {

    struct ifreq ifr;
    int fd, err;

    if((fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;
    strncpy(ifr.ifr_name, "Ltun", IFNAMSIZ);

    if((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    return fd;
}

int tun_device_init()
{
    int fd;
    if((fd = tun_alloc(IFF_TUN | IFF_NO_PI)) < 0 ) {
        return -1;
    }
    return fd;
}
