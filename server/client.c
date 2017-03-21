#include <stdio.h>
#include <stdlib.h>
#include "client.h"

client_t *client_new() 
{
	client_t *client;
	if(!(client = malloc(sizeof(*client)))) {
		perror("malloc");
		return NULL;
	}
	return client;
}

int client_del(client_t *client) 
{
	if(!client) 
		return -1;	
	free(client);
	return 0;
}
