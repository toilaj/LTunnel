#include "client.h"
static void read_tun_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    static int tun2net = 0;
    int read_bytes;
    int write_bytes;
    int tun_fd = watcher->fd;
    int net_fd = *(int*)(watcher->data);
    char buf[PACKET_SIZE] = {0};
    struct packet_hdr *head;
    head = (struct packet_hdr*)buf;
    read_bytes = read_packet(tun_fd, buf + sizeof(struct packet_hdr), BUF_SIZE); 
    if(read_bytes < 0) {
        return;
    }
    tun2net++;
    debug_log("TUN2NET %lu: Read %d bytes\n", tun2net, read_bytes);
    head->cmd = CMD_DATA;
    head->len = read_bytes;

    write_bytes = write_packet(net_fd, head, PACKET_SIZE);
    if(write_bytes < 0) {
        return;
    }  
    debug_log("TUN2NET %lu: Written %d bytes\n", tun2net, write_bytes);
}
static void read_net_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    static int net2tun = 0;
    int read_bytes;
    int write_bytes;
    int net_fd = watcher->fd;
    int tun_fd = *(int*)(watcher->data);
    char buf[PACKET_SIZE] = {0};
    struct packet_hdr *head;
    head = (struct packet_hdr*)buf;
    read_bytes = read_packet(net_fd, &buf, PACKET_SIZE); 
    if(read_bytes <= 0) {
        return;
    }
    net2tun++;
    debug_log("NET2TUN %lu: Read %d bytes\n", net2tun, read_bytes); 
    write_bytes = write_packet(tun_fd, head->buf, head->len);
    if(write_bytes <= 0) {
        return;
    }  
    debug_log("NET2TUN %lu: Written %d bytes to the tap interface\n", net2tun, write_packet);
}

int client_process(char *server_ip, uint16_t port)
{
    struct sockaddr_in local, remote;
    int read_bytes;
    int wrtie_bytes;
    int tun2net;
    int net2tun;
    int flags;
    int tun_fd;
    int net_fd;
    uint16_t pkt_len;

    struct ev_loop *loop;
    struct ev_io ev_read_tun;
    struct ev_io ev_read_net;

    loop = ev_loop_new(EVFLAG_AUTO);

    if(loop == NULL) {
        exit(1);
    }

    if((tun_fd = tun_device_init()) < 0 ) {
        perror("init Ltun failed");
        exit(1);
    }

    if(iface_configure(DEVICE_NAME, "10.0.0.2", "255.255.255.0") < 0) {
        perror("configure Ltun fail");
        exit(1);
    }

    debug_log("Successfully connected to interface %s\n", "Ltun");

    if((net_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(server_ip);
    remote.sin_port = htons(port);

    if(connect(net_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0){
        perror("connect()");
        exit(1);
    }
    debug_log("CLIENT: Connected to server %s, net_fd = %d, tun_fd = %d\n", inet_ntoa(remote.sin_addr), net_fd, tun_fd);
    set_fd_nonblock(net_fd);
    set_fd_nonblock(tun_fd);
    
    ev_read_tun.data = &net_fd;
    ev_read_net.data = &tun_fd;
    ev_io_init(&ev_read_tun, read_tun_cb, tun_fd, EV_READ);
    ev_io_init(&ev_read_net,read_net_cb, net_fd, EV_READ);
    ev_io_start(loop, &ev_read_net);
    ev_io_start(loop, &ev_read_tun);
    ev_run(loop, 0);
}
    