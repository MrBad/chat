#ifndef _MSG_H
#define _MSG_H

enum {
	NOT_USED,
	SRV_MSG,
	CHANGE_NICK,
	NICK_CHANGED,
	NICK_INUSE,
	BCAST_MSG,
	PRIV_MSG,
} msg_type_t;

#endif
