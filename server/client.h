#ifndef _CLIENT_H
#define _CLIENT_H

typedef int (*read_t)  (void *ctx, void *buf, int count);
typedef int (*write_t) (void *ctx, void *buf, int count);

typedef struct {
	int fd;
	char nick[32];
	read_t read;
	write_t write;
} client_t;
#endif


client_t *client_new();
int client_del(client_t *client);


