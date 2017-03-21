#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_IP  "127.0.0.1"
#define DEFAULT_PORT 1980

typedef struct {
	int fd;
	char ip [15];
	int port;
} client_t;

client_t *client_open(char *ip, int port) 
{
	int fd;
	client_t *client;
	struct sockaddr_in caddr;
	
	if(!(client = calloc(1, sizeof(*client)))) {
		perror("calloc");
		return NULL;
	}
	strncpy(client->ip, ip, sizeof(client->ip));
	client->port = port;
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return NULL;
	}
	memset(&caddr, 0, sizeof(caddr));
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(client->port);
	if(inet_aton(client->ip, &caddr.sin_addr) == 0) {
		perror("inet_aton");
		return NULL;
	}
	if(connect(fd, (struct sockaddr*)&caddr, sizeof(caddr)) < 0) {
		perror("connect");
		return NULL;
	}
	client->fd = fd;
	return client;
}

static void client_close(client_t *client) 
{
	if(!client)
		return;
	if(client->fd)
		close(client->fd);
	free(client);
}

void client_loop(client_t *client) 
{
	int nb;
	char buf[1024];
	fd_set rset;
	int sel_ret;
	if(!client)
		return;
	struct timeval tv;
	while(1) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&rset);
		FD_SET(0, &rset);
		FD_SET(client->fd, &rset);
		sel_ret = select(client->fd+1, &rset, NULL, NULL, &tv);
		if(sel_ret < 0) {
			perror("select");
			return;
		}
		if(sel_ret == 0) {
			continue;
		}
		if(FD_ISSET(0, &rset)) {
			nb = read(0, buf, sizeof(buf)-1);
			if(nb < 0) {
				perror("read");
				return;
			}
			if(nb == 0) {
				printf("exiting...\n");
				return;
			}
			buf[nb] = 0;
			if(write(client->fd, buf, strlen(buf)) != nb) {
				perror("write");
				return;
			}
		}
		if(FD_ISSET(client->fd, &rset)) {
			nb = read(client->fd, buf, sizeof(buf)-1);
			if(nb < 0) {
				perror("read"); 
				return;
			}
			if(nb == 0) {
				printf("Server closed connection\n");
				return;
			}
			buf[nb] = 0;
			printf(">%s", buf);
		}
	}
}

int main(int argc, char *argv[])
{
	client_t *client;
	char ip[15]; 
	int port;
	
	if(argc != 3) {
		printf("Usage: %s ip port\n", argv[0]);
		printf("Using default: %s:%d\n", DEFAULT_IP, DEFAULT_PORT);
		strcpy(ip, DEFAULT_IP);
		port = DEFAULT_PORT;
	} else {
		strncpy(ip, argv[1], sizeof(ip));
		port = atoi(argv[2]);
	}
	if(!(client = client_open(ip, port)) < 0) {
		fprintf(stderr, "Cannot connect to %s:%d\n", client->ip, client->port);
		free(client);
		return 1;
	}
	client_loop(client);
	client_close(client);
	return 0;
}
