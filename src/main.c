#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "acl.h"
#include "conf.h"

#define BANNER  "Tiksock : high performance socks5 proxy server"
#define VERSION "0.9.1"

char *tiksock_conf = "/etc/tiksock.conf";
char *log_file = NULL;
char *log_level = NULL;
int   is_daemon = 1;
char *host;
int   port;
acl_tree_t *acl_db = NULL;

void print_usage()
{
    printf("%s, v%s\n", BANNER, VERSION);
    printf("Usage: tiksock -f config\n");
    printf("  -h               print help.\n");
    printf("  -f tiksock.conf  specify tiksock configuration file.\n");
    printf("\n");
}

int tiksock_getopt(int argc, char **argv)
{
    int c;

    opterr = 0;
    while ((c = getopt (argc, argv, "hf:")) != -1) {
        switch (c) {
        case 'h':
            return 0;
        case 'f':
            tiksock_conf = optarg;
            break;
        case '?':
            if (optopt == 'f') {
               printf("Please specify tiksock.conf.\n");
			}
        default:
	    return 0;
        }
    }

    if (optind < argc) {
        return 0;
    }

    return 1;
}

int init()
{
	acl_db = acl_new();

    if (!load_conf(tiksock_conf)) {
		return 0;
    }

    if (!log_open(log_file, log_level)) {
		return 0;
    }

	return 1;
}

int deinit()
{
	if (acl_db) {
		acl_free(acl_db);
	}

}

int main(int argc, char **argv)
{
    struct socks5_server *server = NULL;

    if (!tiksock_getopt(argc, argv)) {
        print_usage();
		return -1;
    }

	if (!init()) {
		return -1;
	}

    if (is_daemon)
        daemon(0, 0);

    server = socks5_server_new(host, port);
    if (!server) {
		ERROR("Couldn't create tiksock server.");
		return -1;
    }

	INFO("Start tiksock server.");

    socks5_server_run(server);

    socks5_server_exit(server);

	deinit();

    return 0;
}

