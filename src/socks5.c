
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h> 
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <stddef.h>
#include <ev.h>
#include "log.h"
#include "acl.h"
 
extern acl_tree_t *acl_db;

enum SOCKS5_STATE {
    SOCKS5_CONNECT,
    SOCKS5_METHOD_SELECTION,
    SOCKS5_REQUEST1,
    SOCKS5_REQUEST2,
    SOCKS5_ESTABLISHED
};


#define SOCKS5_NOAUTH    0x00
#define SOCKS5_GSSAPI    0x01
#define SOCKS5_USERPW    0x03
#define SOCKS5_NOMETHOD  0xFF  

#define SOCKS5_IPV4      0x01
#define SOCKS5_DOMAIN    0x03
#define SOCKS5_IPV6      0x04

enum SOCKS5_REPLY {
	OK
};

#define SOCKS5_BUFLEN  2048


struct socks5_conn;

struct socks5_peer {
	int fd;
    ev_io ev_write;
	ev_io ev_read;
	ev_io ev_connect;

	char ibuf[SOCKS5_BUFLEN];
    int  ibuf_len;

	char obuf[SOCKS5_BUFLEN];
    int  obuf_len;

	struct socks5_conn *conn;
};

struct socks5_conn {

	struct socks5_peer local;
	struct socks5_peer remote;
	
	int state;
};

struct socks5_server {
	char *host;
	int   port;

	int listen_fd;

	ev_io ev_accept;

	struct ev_loop *loop;
};
 
int setnonblock(int fd)
{
    int flags;
 
    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
            return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) 
            return -1;
 
    return 0;
}

int socks5_new_socket(int (*f)(int, const struct sockaddr *, socklen_t),
              char *address, short port)
{
        struct linger linger;
        struct addrinfo ai, *aitop;
        int fd, on = 1;
        char strport[NI_MAXSERV];

        memset(&ai, 0, sizeof (ai));
        ai.ai_family = AF_INET;
        ai.ai_socktype = SOCK_STREAM;
        ai.ai_flags = f != connect ? AI_PASSIVE : 0;
        snprintf(strport, sizeof (strport), "%d", port);
        if (getaddrinfo(address, strport, &ai, &aitop) != 0) {
                warn("getaddrinfo");
                return (-1);
        }

        /* Create listen socket */
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1) {
                warn("socket");
                return (-1);
        }

        if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
                warn("fcntl(O_NONBLOCK)");
                goto out;
        }

        if (fcntl(fd, F_SETFD, 1) == -1) {
                warn("fcntl(F_SETFD)");
                goto out;
        }

        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on));
        linger.l_onoff = 1;
        linger.l_linger = 5;
        setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));

        if ((f)(fd, aitop->ai_addr, aitop->ai_addrlen) == -1) {

printf("=================\n");
                if (errno != EINPROGRESS) {
                        warn("bind");
                        goto out;
                }
        }

        return (fd);

 out:
        close(fd);
        return (-1);
}

static void read_cb(struct ev_loop *loop, struct ev_io *w, int revents);
static void write_cb(struct ev_loop *loop, struct ev_io *w, int revents);
static void connect_cb(struct ev_loop *loop, struct ev_io *w, int revents);

static int socks5_process_conn(struct ev_loop *loop, struct socks5_peer *local)
{
	struct socks5_conn *conn = local->conn;
	char *buf = local->ibuf;
	int   len = local->ibuf_len;

        /* parse connection message :

                   +----+----------+----------+
                   |VER | NMETHODS | METHODS  |
                   +----+----------+----------+
                   | 1  |    1     | 1 to 255 |
                   +----+----------+----------+

	 */

    local->obuf[0] = 0x05;
    local->obuf[1] = 0x00;
    local->obuf_len = 2;

    conn->state = SOCKS5_REQUEST1;

    ev_io_start(loop, &local->ev_write);

	return 0;
}

static int socks5_process_req(struct ev_loop *loop, struct socks5_peer *local)
{
	struct socks5_conn *conn = local->conn;
	struct socks5_peer *remote = &(conn->remote);
	char *buf = local->ibuf;
	int   len = local->ibuf_len;
	char host[64];
	int hlen;
	uint16_t port;
	uint8_t  atyp = buf[3];

       /*
        +----+-----+-------+------+----------+----------+
        |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
        +----+-----+-------+------+----------+----------+
        | 1  |  1  | X'00' |  1   | Variable |    2     |
        +----+-----+-------+------+----------+----------+
        */



	if (atyp == SOCKS5_IPV4) {
		if (len != 10) {

		}

		if (!inet_ntop(AF_INET, &buf[4], host, sizeof(host))) {
		}

		port = ntohs(*((uint16_t *)&buf[8]));

	} else if (atyp == SOCKS5_IPV6) {
		if (len != 22) {
		}

		if (!inet_ntop(AF_INET6, &buf[4], host, sizeof(host))) {
		}

		port = ntohs(*((uint16_t *)&buf[20]));

	} else if (atyp == SOCKS5_DOMAIN) {
		if (len < 8) {

		}

		hlen = buf[4];
		if (len != (7 + hlen)) {

		}

		memcpy(host, &buf[5], hlen);
		host[hlen] = '\0';
					
		port = ntohs(*((uint16_t *)&buf[5 + hlen]));
	}

printf("connecting : %s %d\n", host, port);
	remote->fd = socks5_new_socket(connect, host, port);	


    ev_io_init(&remote->ev_connect, connect_cb, remote->fd, EV_WRITE);
    ev_io_start(loop, &remote->ev_connect);
printf("--- remote fd = %d\n", remote->fd);

    conn->state = SOCKS5_REQUEST2;


	struct sockaddr_storage saddr;
	struct sockaddr_in *addr = (struct sockaddr_in*)&saddr;
	socklen_t slen = sizeof(saddr);

    if (getsockname(remote->fd, (struct sockaddr*)&saddr, &slen) < 0) {
        /* Notify client of failure and close. */
        return -1;
    }

	/*
        +----+-----+-------+------+----------+----------+
        |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
        +----+-----+-------+------+----------+----------+
        | 1  |  1  | X'00' |  1   | Variable |    2     |
        +----+-----+-------+------+----------+----------+
	*/

	local->obuf[0] = 0x05;
	local->obuf[1] = 0x00;
	local->obuf[2] = 0x00;

	if (saddr.ss_family == AF_INET) {
printf(":::::: 0.1\n");
	    local->obuf[3] = SOCKS5_IPV4;
	    memcpy(local->obuf + 4, &addr->sin_addr, 4);
	    memcpy(local->obuf + 8, &addr->sin_port, 2);
        local->obuf_len = 10;

	} else if (saddr.ss_family == AF_INET6){ 
		local->obuf[3] = SOCKS5_IPV6;
	    memcpy(local->obuf + 4, &addr->sin_addr, 4);
	    memcpy(local->obuf + 8, &addr->sin_port, 2);
        local->obuf_len = 10;
	}

    conn->state = SOCKS5_ESTABLISHED;

    ev_io_start(loop, &local->ev_write);
}

static int socks5_process_req2(struct ev_loop *loop, struct socks5_peer *remote)
{
	struct socks5_conn *conn = remote->conn;
	struct socks5_peer *local = &(conn->local);
	struct sockaddr_storage saddr;
	struct sockaddr_in *addr = (struct sockaddr_in*)&saddr;
	socklen_t slen = sizeof(saddr);

    if (getsockname(remote->fd, (struct sockaddr*)&saddr, &slen) < 0) {
        /* Notify client of failure and close. */
        return -1;
    }

	/*
        +----+-----+-------+------+----------+----------+
        |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
        +----+-----+-------+------+----------+----------+
        | 1  |  1  | X'00' |  1   | Variable |    2     |
        +----+-----+-------+------+----------+----------+
	*/

	local->obuf[0] = 0x05;
	local->obuf[1] = 0x00;
	local->obuf[2] = 0x00;

	if (saddr.ss_family == AF_INET) {
printf(":::::: 0.1\n");
	    local->obuf[3] = SOCKS5_IPV4;
	    memcpy(local->obuf + 4, &addr->sin_addr, 4);
	    memcpy(local->obuf + 8, &addr->sin_port, 2);
        local->obuf_len = 10;

	} else if (saddr.ss_family == AF_INET6){ 
		local->obuf[3] = SOCKS5_IPV6;
	    memcpy(local->obuf + 4, &addr->sin_addr, 4);
	    memcpy(local->obuf + 8, &addr->sin_port, 2);
        local->obuf_len = 10;
	}

    conn->state = SOCKS5_ESTABLISHED;

    ev_io_start(loop, &local->ev_write);
}

int socks5_handshake(struct ev_loop *loop, struct socks5_peer *peer)
{
	struct socks5_conn *conn = peer->conn;

	switch (conn->state) {
	
	case SOCKS5_CONNECT:
		socks5_process_conn(loop, peer);
		break;
	case SOCKS5_REQUEST1:
		socks5_process_req(loop, peer);
        break;
	case SOCKS5_REQUEST2:
		socks5_process_req(loop, peer);
        break;
	}

	return 0;
}
 
int socks5_forward(struct ev_loop *loop, struct socks5_peer *peer)
{
	struct socks5_conn *conn = peer->conn;
	struct socks5_peer *local = &conn->local, *remote = &conn->remote;

printf("======= forward :\n");

	/*
 	 * TODO : use socket splicing to avoid memory copy
 	 */
	if (peer == local) {
		memcpy(remote->obuf, local->ibuf, local->ibuf_len);
		remote->obuf_len = local->ibuf_len;
    	ev_io_start(loop, &remote->ev_write);
	} else {
		memcpy(local->obuf, remote->ibuf, remote->ibuf_len);
		local->obuf_len = remote->ibuf_len;
    	ev_io_start(loop, &local->ev_write);
	}

	return 0;
}

int socks5_close_conn(struct ev_loop *loop, struct socks5_conn *conn)
{
	struct socks5_peer *local = &(conn->local), *remote = &(conn->remote);

    ev_io_stop(loop, &local->ev_read);
    ev_io_stop(loop, &remote->ev_read);
    close(local->fd);
    close(remote->fd);

    free(conn);

    return;
}

static void read_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{ 
	struct socks5_peer *peer = ((struct socks5_peer *)(((char*)w) - offsetof(struct socks5_peer, ev_read)));
	struct socks5_conn *conn = peer->conn;
	struct socks5_peer *local = &conn->local, *remote = &conn->remote;
	int len = 0;

	if (!(revents & EV_READ)){
		return;
	}

	len = read(peer->fd, peer->ibuf, SOCKS5_BUFLEN);
	if (len <= 0) {
		socks5_close_conn(loop, conn);
		return;
	}

	peer->ibuf_len = len;

	if (conn->state == SOCKS5_ESTABLISHED) {
		/* forward traffic from local to remote and vice versa */
	    socks5_forward(loop, peer);
        return;
    } 

	if (peer == remote) {
		return;
	}
			
	if (!socks5_handshake(loop, local)) {

	}

	if (conn->state == SOCKS5_ESTABLISHED) {
		/* start processing rempote socks */
		ev_io_init(&remote->ev_read, read_cb, remote->fd, EV_READ);
		ev_io_init(&remote->ev_write, write_cb, remote->fd, EV_WRITE);
		ev_io_start(loop, &remote->ev_read);
	}
}
 
static void write_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{ 
	struct socks5_peer *peer = ((struct socks5_peer *) (((char*)w) - offsetof(struct socks5_peer, ev_write)));

 	if (!(revents & EV_WRITE) || peer->obuf_len <= 0){
		return;
	}

printf("write = %d\n", peer->obuf_len);

	write(peer->fd, peer->obuf, peer->obuf_len);
	
	peer->obuf_len = 0;

	ev_io_stop(EV_A_ w);
}

static void connect_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{ 
	struct socks5_peer *remote = ((struct socks5_peer *) (((char*)w) - offsetof(struct socks5_peer, ev_connect)));
    int err;
    socklen_t len = sizeof(err);

	printf("::::: CONNECTED!\n");

	ev_io_stop(EV_A_ w);

    getsockopt(remote->fd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err) {
        socks5_close_conn(loop, remote->conn);
		return;
    } 

	socks5_handshake(loop, remote);
}

static void accept_cb (struct ev_loop *loop, struct ev_io *w, int revents)
{
	int client_fd;
	struct socks5_conn *conn = NULL;
	struct socks5_peer *local = NULL, *remote = NULL;
	struct sockaddr_in client_addr;
	uint32_t ip;
	char ipstr[INET_ADDRSTRLEN];

	socklen_t client_len = sizeof(client_addr);
	client_fd = accept(w->fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        return;
    }

printf("::::::: ---- d1\n");

	inet_ntop(AF_INET, &(client_addr.sin_addr), ipstr, INET_ADDRSTRLEN);
printf("::::::: ---- d2 = %s\n", ipstr);

	ip = ntohl(client_addr.sin_addr.s_addr);
printf("::::::: ---- d3 = %p\n", acl_db);
	if (!acl_lookup(acl_db, ip)) {
		INFO("IP %s is blocked by ACL", ipstr);
		close(client_fd);
		return;
	}

	INFO("IP %s is connected", ipstr);
printf("client fd = %d\n", client_fd);

	conn = (struct socks5_conn *)malloc(sizeof(struct socks5_conn));
	if (!conn) {
		return;
	}
	memset(conn, 0, sizeof(struct socks5_conn));
printf("----- 0.1\n");

	local  = &(conn->local);
	remote = &(conn->remote);
printf("----- 0.2\n");
	local->fd = client_fd;
 
printf("----- 0.3\n");
	if (setnonblock(local->fd) < 0)
          err(1, "failed to set client socket to non-blocking");

	local->conn = conn;
	remote->conn = conn;

printf("----- 0.4\n");
	conn->state = SOCKS5_CONNECT;

	ev_io_init(&(local->ev_read), read_cb, local->fd, EV_READ);
    ev_io_init(&(local->ev_write), write_cb, local->fd, EV_WRITE);
	ev_io_start(loop, &(local->ev_read));
}

struct socks5_server* socks5_server_new(char *host, int port)
{
    struct ev_loop *loop = ev_default_loop (0);
	struct socks5_server *server = NULL;
    int listen_fd;
    struct sockaddr_in listen_addr; 
    int reuseaddr_on = 1;
 
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 
    if (listen_fd < 0) {
        ERROR("listen failed");
		return NULL;
	}
 
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
				&reuseaddr_on, sizeof(reuseaddr_on)) == -1) {
        ERROR("setsockopt failed");
		return NULL;
	}
 
    memset(&listen_addr, 0, sizeof(listen_addr));
 
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(port);
 
    if (bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
            err(1, "bind failed");
 
    if (listen(listen_fd, 5) < 0)
            err(1, "listen failed");
 
    if (setnonblock(listen_fd) < 0)
            err(1, "failed to set server socket to non-blocking");

	server = (struct socks5_server *)malloc(sizeof(struct socks5_server));
	if (!server) {
		return NULL;
	}

	server->loop = loop;
	server->host = strdup(host);
	server->port = port;
	server->listen_fd = listen_fd;

	return server;
}

int socks5_server_run(struct socks5_server *server)
{
    ev_io_init(&server->ev_accept, accept_cb, server->listen_fd, EV_READ);
    ev_io_start(server->loop, &server->ev_accept);
    ev_loop(server->loop, 0);
}

int socks5_server_exit(struct socks5_server *server)
{
	close(server->listen_fd);
	free(server->host);
	free(server);

	return 0;
}
 
