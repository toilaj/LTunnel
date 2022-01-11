#include "client.h"

int client_process(char *server_ip, uint16_t port)
{
    int tun_fd;
    int sock_fd;
    int net_fd;
    int max_fd;
    struct sockaddr_in local, remote;
    char buffer[BUFSIZE];
    int read_bytes;
    int wrtie_bytes;
    int tun2net;
    int net2tun;
    uint16_t pkt_len;

    if(tun_fd = tun_device_init() < 0 ) {
        exit(1);
    }

    debug_log("Successfully connected to interface %s\n", "tun0");

    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(1);
    }

    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(server_ip);
    remote.sin_port = htons(port);

    if(connect(sock_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0){
        perror("connect()");
        exit(1);
    }
    net_fd = sock_fd;
    debug_log("CLIENT: Connected to server %s\n", inet_ntoa(remote.sin_addr));
    max_fd = (tun_fd > net_fd) ? tun_fd : net_fd;

    while(1) {
        int ret;
        fd_set rd_set;

        FD_ZERO(&rd_set);
        FD_SET(tun_fd, &rd_set); 
        FD_SET(net_fd, &rd_set);

        ret = select(max_fd + 1, &rd_set, NULL, NULL, NULL);

        if (ret < 0 && errno == EINTR){
            continue;
        }

        if (ret < 0) {
            perror("select()");
            exit(1);
        }

        if(FD_ISSET(tun_fd, &rd_set)){
            read_bytes = read_packet(tun_fd, buffer, BUFSIZE);
            tun2net++;
            debug_log("TUN2NET %lu: Read %d bytes\n", tun2net, read_bytes);

            pkt_len = htons(read_bytes);
            wrtie_bytes = write_packet(net_fd, &pkt_len, sizeof(pkt_len));
            wrtie_bytes = write_packet(net_fd, buffer, pkt_len);
            debug_log("TUN2NET %lu: Written %d bytes\n", tun2net, write_packet);
        }

        if(FD_ISSET(net_fd, &rd_set)){
            read_bytes = read_packet(net_fd, &pkt_len, sizeof(pkt_len));
            net2tun++;
            pkt_len = ntohs(pkt_len);
            read_bytes = read_packet(net_fd, buffer, pkt_len);
            debug_log("NET2TUN %lu: Read %d bytes\n", net2tun, read_bytes);
            wrtie_bytes = write_packet(tun_fd, buffer, read_bytes);
            debug_log("NET2TUN %lu: Written %d bytes to the tap interface\n", net2tun, write_packet);
        }
    }
}
    