#ifndef _LIBBEX_PRIVATE_H
#define _LIBBEX_PRIVATE_H

#include "c.h"
#include "list.h"
#include "debug.h"

#include <stdio.h>
#include <sys/time.h>

#include "libbex.h"

/*
 * Debug
 */
#define BEX_DEBUG_HELP		(1 << 0)
#define BEX_DEBUG_INIT		(1 << 1)
#define BEX_DEBUG_WSS		(1 << 2)
#define BEX_DEBUG_PLAT		(1 << 3)
#define BEX_DEBUG_ARY		(1 << 4)
#define BEX_DEBUG_VAL		(1 << 5)
#define BEX_DEBUG_EVENT		(1 << 6)
#define BEX_DEBUG_CHAN		(1 << 7)

#define BEX_DEBUG_ALL		0xFFFF

BEX_DEBUG_DECLARE_MASK(libbex);
#define DBG(m, x)	__BEX_DBG(libbex, BEX_DEBUG_, m, x)
#define ON_DBG(m, x)	__BEX_DBG_CALL(libbex, BEX_DEBUG_, m, x)
#define DBG_FLUSH	__BEX_DBG_FLUSH(libbex, BEX_DEBUG_)

/*
 * Generic iterator
 */
struct libbex_iter {
        struct list_head        *p;		/* current position */
        struct list_head        *head;		/* start position */
	int			direction;	/* BEX_ITER_{FOR,BACK}WARD */
	size_t			idx;		/* used for arrays */
};

#define IS_ITER_FORWARD(_i)	((_i)->direction == BEX_ITER_FORWARD)
#define IS_ITER_BACKWARD(_i)	((_i)->direction == BEX_ITER_BACKWARD)

#define BEX_ITER_INIT(itr, list) \
	do { \
		(itr)->p = IS_ITER_FORWARD(itr) ? \
				(list)->next : (list)->prev; \
		(itr)->head = (list); \
	} while(0)

#define BEX_ITER_ITERATE(itr, res, restype, member) \
	do { \
		res = list_entry((itr)->p, restype, member); \
		(itr)->p = IS_ITER_FORWARD(itr) ? \
				(itr)->p->next : (itr)->p->prev; \
	} while(0)

enum {
	BEX_TYPE_STR = 1,
	BEX_TYPE_U64,
	BEX_TYPE_S64,
	BEX_TYPE_FLOAT
};

struct libbex_value {
	int	refcount;
	char	*name;
	int	type;

	union {
		char		*str;
		uint64_t	u64;
		int64_t		s64;
		long double	fl;
	} data;

	unsigned int	generated : 1;
};

struct libbex_array {
	int     refcount;

	size_t	nitems;		/* number of items */
	size_t	nalloc;		/* number of allocated items */

	struct libbex_value	**items;
};

struct libbex_channel {
	int	refcount;
	char	*name;
	char	*symbol;
	uint64_t id;

	struct timeval	last_update;

	int	(*callback)(struct libbex_platform *, struct libbex_channel *);
	int     (*verify)(struct libbex_channel *, struct libbex_event *);
	void	*data;

	struct libbex_event	*subscribe;
	struct libbex_array	*reply;

	char	*inbuff;

	struct list_head	channels;		/* platform events list */

	unsigned int	subscribed : 1;
};

struct libbex_event {
	int	refcount;
	char	*name;

	int	(*callback)(struct libbex_platform *, struct libbex_event *);
	void	*data;

	struct libbex_array	*vals;
	struct libbex_array	*reply;

	struct list_head	events;		/* platform events list */
};

struct libbex_platform {
	int	refcount;

	char	*uri_path;
	char	*uri_addr;
	char	*uri_prot;
	int	uri_port;
	int	uri_ssl;

	void	*wss;			/* connection */
	unsigned int	connection_attempts;
	unsigned int	reconnect_timeout;	/* ms */
	unsigned int	service_timeout;

	struct list_head	events;
	struct list_head	channels;

};

/* value.c */
extern struct libbex_value *__bex_new_value(char *name);

/* wss.c */
extern int wss_is_connected(struct libbex_platform *pl);
extern int wss_connect(struct libbex_platform *pl);
extern int wss_disconnect(struct libbex_platform *pl);
extern int wss_service(struct libbex_platform *pl);
extern int wss_send(struct libbex_platform *pl, unsigned char *str, size_t sz);

/* event.c */
extern int bex_is_event_string(const char *str, char **name);

#endif /* _LIBBEX_PRIVATE_H */

