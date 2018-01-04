#ifndef _LIBBEX_PRIVATE_H
#define _LIBBEX_PRIVATE_H

#include "c.h"
#include "list.h"
#include "debug.h"

#include "libbex.h"

/*
 * Debug
 */
#define BEX_DEBUG_HELP		(1 << 0)
#define BEX_DEBUG_INIT		(1 << 1)
#define BEX_DEBUG_WSS		(1 << 1)

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


#endif /* _LIBBEX_PRIVATE_H */

