#include "common.h" 
#include "client.h"
#include "server.h"
int g_debug;
char *g_process_name;

void debug_log(char *msg, ...){
    va_list argp;
    if(g_debug){
        va_start(argp, msg);
	      vfprintf(stderr, msg, argp);
	      va_end(argp);
    }
}

void err_log(char *msg, ...) {
    va_list argp;
    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);
}

void usage(void)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s -h\n", g_process_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "-s|-c <serverIP>: run in server mode (-s), or run in client mode (-c <serverIP>)\n");
    fprintf(stderr, "-p <port>:listen on the port (run in server mode) or connect to the port (run in client mode), default 10443\n");
    fprintf(stderr, "-v: outputs debug log while running\n");
    fprintf(stderr, "-d: daemon mode\n");
    fprintf(stderr, "-h: prints this help text\n");
    exit(1);
}

static void daemonize(void)
{
    pid_t pid;
    if((pid = fork()) < 0) {
        exit(1);  
    } else if(pid != 0) {
        exit(0);
    }

    setsid();

    if((pid = fork()) < 0) {
        exit(1);
    } else if(pid != 0) {
        exit(0);
    }

    dup(0);
}

int main(int argc, char *argv[]) {
    int option;
    char remote_ip[16] = "";
    unsigned short int port = PORT;
    int cliserv = -1; 
    int daemon = 0;
    g_process_name = argv[0];
    /* Check command line options */
    while((option = getopt(argc, argv, "sc:p:dhv")) > 0){
        switch(option) {
          case 'v':
            g_debug = 1;
            break;
          case 'h':
            usage();
            break;
          case 's':
            cliserv = SERVER;
            break;
          case 'c':
            cliserv = CLIENT;
            strncpy(remote_ip,optarg, 15);
            break;
          case 'p':
            port = atoi(optarg);
            break;
          case 'd':
            daemon = 1;
            break;
          default:
            err_log("Unknown option %c\n", option);
            usage();
        }
    }

    argv += optind;
    argc -= optind;

    if(argc > 0){
        err_log("Too many options!\n");
        usage();
    }
    
    if(daemon) {
        daemonize();
    }

    if(cliserv < 0) {
        err_log("Must specify client or server mode!\n");
        usage();
    } else if((cliserv == CLIENT) && (*remote_ip == '\0')) {
        err_log("Must specify server address!\n");
        usage();
    }

    if(cliserv==CLIENT){
        client_process(remote_ip, port);
    } else {
        server_process(port);
    }
  
    return(0);
}
