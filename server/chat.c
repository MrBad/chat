#include <stdio.h>
#include "server.h"

#define DEFAULT_PORT 1980

int main() 
{
	server_t *srv;
	if(!(srv = server_open(DEFAULT_PORT))) {
		return 1;
	}
	server_loop(srv);
	server_close(srv);
	return 0;
}
