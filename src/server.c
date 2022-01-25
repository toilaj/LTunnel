#include "server.h"
static void read_tun_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    static int tun2net = 0;
    int read_bytes;
    int write_bytes;
    int tun_fd = watcher->fd;
    int net_fd;
    char buf[PACKET_SIZE] = {0};
    struct packet_hdr *head;
    struct connection *con;
    con = (struct connection*)watcher->data;
    net_fd = con->net_fd;
    head = (struct packet_hdr*)buf;
    read_bytes = read_packet(tun_fd, head->buf, BUF_SIZE, FLAG_READ_POSSIBLE); 
    if(read_bytes <= 0) {
        debug_log("TUN2NET cannot read packet: errno = %d\n", errno);
        return;
    }
    tun2net++;
    debug_log("TUN2NET %lu: Read %d bytes\n", tun2net, read_bytes);
    head->cmd = CMD_DATA;
    head->len = read_bytes;

    write_bytes = write_packet(net_fd, head, HEADER_SIZE + head->len);
    if(write_bytes <= 0) {
        debug_log("TUN2NET cannot write packet: errno = %d\n", errno);
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
    int tun_fd;
    char buf[PACKET_SIZE] = {0};
    struct packet_hdr *head;
    struct connection *con;
    int bytes_left;
    char *p = buf;
    con = (struct connection*)watcher->data;
    tun_fd = con->tun_fd;
    head = (struct packet_hdr*)buf;
    read_bytes = read_packet(net_fd, buf, HEADER_SIZE, FLAG_READ_FIX_LEN); 
    if(read_bytes < HEADER_SIZE) {
        debug_log("NET2TUN cannot read packet: errno = %d\n", errno);
        return;
    }
    net2tun++;
    debug_log("NET2TUN %lu: Read head %d bytes\n", net2tun, read_bytes);
    if(head->cmd == CMD_DATA) {
        debug_log("NET2TUN %lu: cmd = 0x%x len = %d\n", net2tun, head->cmd, head->len);
        bytes_left = head->len;
        if(head->len > BUF_SIZE) {
            debug_log("fatal len, error packet!\n");
            return;
        }
        p += HEADER_SIZE;
        while(bytes_left > 0) {
            read_bytes = read_packet(net_fd, p, head->len, FLAG_READ_POSSIBLE); 
            if(read_bytes <= 0) {
                debug_log("NET2TUN cannot read data packet: errno = %d\n", errno);
                read_bytes = 0;
            } 
            p += read_bytes;
            bytes_left -= read_bytes;
        }
    } else {
        debug_log("not have header, error packet!\n");
        return;
    }
    debug_log("NET2TUN %lu: Read data %d bytes\n", net2tun, head->len);
    write_bytes = write_packet(tun_fd, head->buf, head->len);
    if(write_bytes <= 0) {
        debug_log("NET2TUN cannot write packet: errno = %d\n", errno);
        return;
    }  
    debug_log("NET2TUN %lu: Written %d bytes to the tun interface\n", net2tun, write_bytes);
}

static void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    int net_fd;
    int tun_fd;
    struct connection *con;
    net_fd = accept(watcher->fd, NULL, NULL);
    if(net_fd < 0) {
        return;
    }
    con = watcher->data;
    if(con->net_fd > 0 && ev_is_active(con->ev_read_net)) {
        ev_io_stop(con->ev_loop, con->ev_read_net);
        close(con->net_fd);
        ev_io_set(con->ev_read_net, net_fd, EV_READ);
        con->net_fd = net_fd;
        set_fd_nonblock(net_fd); 
        ev_io_start(con->ev_loop, con->ev_read_net); 
        debug_log("SERVER: Client reconnected, net_fd = %d, tun_fd = %d\n", net_fd, con->tun_fd);
        return;
    }
    con->net_fd = net_fd;
    tun_fd = con->tun_fd;
    set_fd_nonblock(net_fd);
    set_fd_nonblock(tun_fd);

    con->ev_read_net->data = con;
    con->ev_read_tun->data = con;

    ev_io_init(con->ev_read_tun, read_tun_cb, tun_fd, EV_READ);
    ev_io_init(con->ev_read_net, read_net_cb, net_fd, EV_READ);

    ev_io_start(con->ev_loop, con->ev_read_net);
    ev_io_start(con->ev_loop, con->ev_read_tun);

    debug_log("SERVER: Client connected, net_fd = %d, tun_fd = %d\n", net_fd, tun_fd);
}

int server_process(uint16_t port)
{
    int tun_fd;
    int sock_fd;
    int net_fd;
    int max_fd;
    struct sockaddr_in local, remote;
    int read_bytes;
    int wrtie_bytes;
    int tun2net;
    int net2tun;
    uint16_t pkt_len;
    int optval = 1;
    socklen_t remotelen;

    struct ev_loop *loop;
    struct ev_io ev_read_tun;
    struct ev_io ev_read_net;
    struct ev_io ev_accept;

    struct connection con;

    memset(&con, 0, sizeof(con));

    con.ev_read_net = &ev_read_net;
    con.ev_read_tun = &ev_read_tun;
    
    loop = ev_loop_new(EVFLAG_AUTO);

    con.ev_loop = loop;
    if((tun_fd = tun_device_init()) < 0 ) {
        exit(1);
    }
    if(iface_configure(DEVICE_NAME, "10.0.0.1", "255.255.255.0") < 0) {
        perror("configure Ltun fail");
        exit(1);
    }
    debug_log("Successfully connected to interface %s\n", "Ltun");

    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0){
        perror("setsockopt()");
        exit(1);
    }
    
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);

    if(bind(sock_fd, (struct sockaddr*) &local, sizeof(local)) < 0){
        perror("bind()");
        exit(1);
    }
    
    if(listen(sock_fd, 5) < 0){
        perror("listen()");
        exit(1);
    }
    set_fd_nonblock(sock_fd);
    /* wait for connection request */
    remotelen = sizeof(remote);
    memset(&remote, 0, remotelen);
    con.tun_fd = tun_fd;
    ev_accept.data = &con;
    ev_io_init(&ev_accept, accept_cb, sock_fd, EV_READ);
    ev_io_start(loop,&ev_accept);
    ev_run(loop, 0);


}