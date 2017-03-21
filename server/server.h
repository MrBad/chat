#ifndef _SERVER_H
#define _SERVER_H

#include "client.h"

typedef struct {
	int fd;
	int port;
	client_t **clients;
	int clisz;
	int num_clients;

} server_t;


server_t *server_open(int port);
void server_close(server_t *srv);
void server_loop();

#endif
