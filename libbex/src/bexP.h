#ifndef _LIBBEX_PRIVATE_H
#define _LIBBEX_PRIVATE_H

#include "c.h"
#include "list.h"
#include "debug.h"

#include <libwebsockets.h>
#include <stdio.h>

#include "libbex.h"

/*
 * Debug
 */
#define BEX_DEBUG_HELP		(1 << 0)
#define BEX_DEBUG_INIT		(1 << 1)
#define BEX_DEBUG_WSS		(1 << 2)
#define BEX_DEBUG_PLAT		(1 << 3)

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
	BEX_TYPE_STR,
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
		double		fl;
	} data;
};

struct libbex_array {
	int     refcount;

	size_t	nitems;		/* number of items */
	size_t	nalloc;		/* number of allocated items */

	struct libbex_value	*items;
};

struct libbex_event {
	int	refcount;
	char	*name;

	int	(*callback)(struct libbex_platform *, struct libbex_event *, void *);
	void	*data;

	struct libbex_array	*vals;
	struct list_head	events;		/* platform events list */
};

struct libbex_platform {
	int	refcount;

	char	*uri_path;
	char	*uri_addr;
	char	*uri_prot;
	int	uri_port;
	int	uri_ssl;

	struct list_head	events;

};


#endif /* _LIBBEX_PRIVATE_H */

