/*
 * Copyright (C) 2009-2018 Karel Zak <kzak@redhat.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 */

/**
 * SECTION: iter
 * @title: Iterator
 * @short_description: unified iterator
 *
 * The iterator keeps the direction and the last position
 * for access to the internal library tables/lists.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bexP.h"

/**
 * bex_new_iter:
 * @direction: BEX_INTER_{FOR,BACK}WARD direction
 *
 * Returns: newly allocated generic libmount iterator.
 */
struct libbex_iter *bex_new_iter(int direction)
{
	struct libbex_iter *itr = calloc(1, sizeof(*itr));
	if (!itr)
		return NULL;
	itr->direction = direction;
	itr->idx = (size_t) -1;
	return itr;
}

/**
 * bex_free_iter:
 * @itr: iterator pointer
 *
 * Deallocates the iterator.
 */
void bex_free_iter(struct libbex_iter *itr)
{
	free(itr);
}

/**
 * bex_reset_iter:
 * @itr: iterator pointer
 * @direction: BEX_INTER_{FOR,BACK}WARD or -1 to keep the direction unchanged
 *
 * Resets the iterator.
 */
void bex_reset_iter(struct libbex_iter *itr, int direction)
{
	if (direction == -1)
		direction = itr->direction;

	memset(itr, 0, sizeof(*itr));
	itr->direction = direction;
	itr->idx = (size_t) -1;
}

/**
 * bex_iter_get_direction:
 * @itr: iterator pointer
 *
 * Returns: BEX_INTER_{FOR,BACK}WARD
 */
int bex_iter_get_direction(struct libbex_iter *itr)
{
	return itr->direction;
}
