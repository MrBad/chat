#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include "client.h"
#include "server.h"

static int server_listen(server_t *srv) 
{
	struct sockaddr_in saddr;
	socklen_t addr_len = sizeof(saddr);
	int en = 1;
	if((srv->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return -1;
	}
	setsockopt(srv->fd, SOL_SOCKET, SO_REUSEADDR, &en, sizeof(en));

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(srv->port);
	if(bind(srv->fd, (struct sockaddr*) &saddr, addr_len) < 0) {
		perror("bind");
		return -1;
	}
	if(listen(srv->fd, 32) < 0) {
		perror("listen");
		return -1;
	}
	return 0;
}

server_t *server_open(int port) 
{
	printf("Starting server on port: %d\n", port);
	server_t *srv;
	if(!(srv = calloc(1, sizeof(*srv)))) {
		perror("malloc");
		return NULL;
	}
	srv->port = port;
	if(server_listen(srv) < 0) {
		return NULL;
	}
	return srv;	
}

static client_t *
alloc_client(server_t *srv) 
{
	int i;
	if(srv->num_clients == srv->clisz) {
		i = srv->clisz == 0 ? 2 : srv->clisz * 2;
		srv->clients = realloc(srv->clients, i * sizeof(*srv->clients));
		if(!srv->clients) {
			perror("realloc");
			return NULL;
		}
		while(srv->clisz < i)
			srv->clients[srv->clisz++] = NULL;
	}
	// find free slot //
	for(i = 0; i < srv->clisz; i++) {
		if(!srv->clients[i]) {
			srv->clients[i] = client_new();
			return srv->clients[i];	
		}
	}
	return NULL;
}

void server_close(server_t *srv) 
{
	int i;
	printf("Closing server\n");
	close(srv->fd);
	for(i = 0; i < srv->clisz; i++) {
		if(!srv->clients[i]) continue;
		close(srv->clients[i]->fd);
		client_del(srv->clients[i]);
	}
	free(srv);
}

int server_accept(server_t *srv) 
{
	struct sockaddr_in caddr; // client addr who connects to server
	socklen_t addr_len = sizeof(caddr);
	client_t *cli;
	int fd;
	
	if((fd = accept(srv->fd, (struct sockaddr*)&caddr, &addr_len)) < 0) {
		perror("accept");
		return -1;
	}
	if(!(cli = alloc_client(srv))) 
		return -1;
	cli->fd = fd;
	printf("New connection from %s:%d\n", 
			inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

	return 0;
}

int server_read_msg(server_t *srv, int id) 
{
	char buf[1024];
	int n, i;
	if((n = read(srv->clients[id]->fd, buf, sizeof(buf)-1)) < 0) {
		perror("read");
		return -1; // ??? or kill client //		
	} 
	else if(n == 0) {
		struct sockaddr_in caddr;
		socklen_t addr_len = sizeof(caddr);
		if(getpeername(srv->clients[id]->fd,(struct sockaddr*)&caddr, &addr_len) == 0)
			printf("Client %s: %d closed connection\n",
					inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
		close(srv->clients[id]->fd);
		client_del(srv->clients[id]);
		srv->clients[id] = NULL;
		return 0;
	}
	buf[n] = 0;
	for(i = 0; i < srv->clisz; i++)
		if(srv->clients[i])
			write(srv->clients[i]->fd, buf, strlen(buf));
	return 0;
}

void server_loop(server_t *srv) 
{
	int i, maxfd;
	fd_set rset;
	int sel_ret;
	struct timeval tv;
	while(1) {
		FD_ZERO(&rset);
		FD_SET(srv->fd, &rset); // add master socket to set
		maxfd = srv->fd;
		for(i = 0; i < srv->clisz; i++) { 
			if(srv->clients[i]) {
				FD_SET(srv->clients[i]->fd, &rset);
				if(srv->clients[i]->fd > maxfd)
					maxfd = srv->clients[i]->fd; // add client socket
			}
		}
		tv.tv_sec = 2; 
		tv.tv_usec = 0;
		sel_ret = select(maxfd+1, &rset, NULL, NULL, &tv);
		if(sel_ret < 0) {
			if(errno == EINTR)
				continue;
			return;
		} else if(sel_ret == 0) {
			//write(1, ".", 1);
			continue;
		} else {
			if(FD_ISSET(srv->fd, &rset)) {
				server_accept(srv);
			}
			for(i = 0; i < srv->clisz; i++) {
				if(!srv->clients[i]) continue;
				if(FD_ISSET(srv->clients[i]->fd, &rset)) {
					server_read_msg(srv, i);
				}
			}
		}
	}
}
