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
#include <ctype.h>
#include "msg.h"


#define DEFAULT_IP  "127.0.0.1"
#define DEFAULT_PORT 1980


typedef struct {
	int fd;			// socket
	char ip [15];	// server ip
	int port;		// server port
	char nick[32];
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
	char buf[1024], msg[1024];
	fd_set rset;
	int sel_ret;
	struct timeval tv;
	
	if(!client)
		return;

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
			if((nb = read(0, buf, sizeof(buf)-1)) < 0) {
				perror("read");
				return;
			}
			if(nb == 0) {
				printf("exiting...\n");
				return;
			}
			buf[nb] = 0;
			nb = snprintf(msg, sizeof(msg), "%d: %s\n", BCAST_MSG, buf);
			if(write(client->fd, msg, nb) != nb) {
				perror("write");
				return;
			}
		}

		if(FD_ISSET(client->fd, &rset)) {
			if((nb = read(client->fd, buf, sizeof(buf)-1)) < 0) {
				perror("read"); 
				return;
			}
			if(nb == 0) {
				printf("Server closed connection\n");
				return;
			}
			buf[nb] = 0;
			printf("%s", buf);
		}
	}
}

int trim(char *str) 
{
	int len;
	if(!str)
		return -1;
	len = strlen(str);
	while(len > 0 && isspace(str[len-1]))
		str[--len] = 0;
	return 0;
}

static int validNick(char *nick) 
{
	char *p = nick;
	if(!nick) 
		return -1;
	if(!isalpha(*p)) {
		fprintf(stderr, "nick should start with a letter\n");
		return -1;
	}
	while(*p) {
		if(!isalnum(*p) && *p != '-' && *p!='.' && *p!='_') {
			fprintf(stderr, "nick should contain only letters, numbers, -, ., _, [got: %c]\n", *p);
			return -1;
		}
		p++;
	}
	if(!isalnum(*(p-1))) {
		fprintf(stderr, "nick should ends only in a letter or number\n");
		return -1;
	}
	return 0;
}

static int set_nick(client_t *client) 
{	
	char yourNick[] = "Enter your nick: ";
	int nb, type;
	char buf[1024], *msg;
	while(1) {
		write(1, yourNick, strlen(yourNick));
		if((nb = read(0, client->nick, sizeof(client->nick)-1)) <= 0) {
			return -1;
		}

		client->nick[nb] = 0;
		trim(client->nick);
		if(validNick(client->nick) == 0) {
			break;
		}
	}
	snprintf(buf, sizeof(buf), "%d: %s\n", CHANGE_NICK, client->nick);
	write(client->fd, buf, strlen(buf));
	
	if((nb = read(client->fd, buf, sizeof(buf)-1)) <= 0) {
		return -1;
	}
	buf[nb] = 0;
	
	type = atoi(buf);
	if(!(msg = strchr(buf, ':'))) {
		fprintf(stderr, "malformed message: %s\n", msg);
		return -1;
	}
	if(type == NICK_CHANGED) {
		msg += 2;
		trim(msg);
		strncpy(client->nick, msg, sizeof(client->nick));
		fprintf(stdout, "Welcome to chat %s\n", client->nick);
	} else if(type == NICK_INUSE) {
		fprintf(stdout, "Nickname %s in use\n", client->nick);
		return -1;	
	}
	return 0;
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
	if(!(client = client_open(ip, port))) {
		fprintf(stderr, "Cannot connect to %s:%d\n", ip, port);
		free(client);
		return 1;
	}
	printf("Connected to %s:%d\n", client->ip, client->port);
	while(set_nick(client) < 0);
	
	client_loop(client);
	client_close(client);
	return 0;
}
