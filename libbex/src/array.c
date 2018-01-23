

#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>

#include "bexP.h"
#include "strutils.h"

static void free_array(struct libbex_array *ar)
{
	size_t i;

	if (!ar)
		return;

	DBG(ARY, bex_debugobj(ar, "free [items=%zu]", ar->nitems));
	for (i = 0; i < ar->nitems; i++)
		bex_unref_value(ar->items[i]);

	free(ar->items);
	free(ar);
}

/**
 * bex_new_array:
 * @sz: initial size
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the array
 *
 * Returns: newly allocated struct libbex_array
 */
struct libbex_array *bex_new_array(size_t sz)
{
	struct libbex_array *ar = calloc(1, sizeof(*ar));
	if (!ar)
		goto err;

	DBG(ARY, bex_debugobj(ar, "alloc"));
	ar->refcount = 1;
	ar->items = calloc(sz, sizeof(struct libbex_value *));
	if (!ar->items)
		goto err;
	ar->nalloc = sz;
	return ar;
err:
	free_array(ar);
	return NULL;
}

/**
 * bex_ref_array:
 * @ar: array pointer
 *
 * Increments reference counter.
 */
void bex_ref_array(struct libbex_array *ar)
{
	if (ar)
		ar->refcount++;
}

/**
 * bex_unref_array:
 * @ar: array pointer
 *
 * De-increments reference counter, on zero the @ar is automatically
 * deallocated.
 */
void bex_unref_array(struct libbex_array *ar)
{
	if (ar) {
		ar->refcount--;
		if (ar->refcount <= 0)
			free_array(ar);
	}
}

/**
 * bex_array_add:
 * @ar: array
 * @va: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_array_add(struct libbex_array *ar, struct libbex_value *va)
{
	if (!ar || !va)
		return -EINVAL;

	if (ar->nitems == ar->nalloc) {
		void *tmp;
		size_t newsz = ar->nalloc + 10;

		DBG(ARY, bex_debugobj(ar, " resize %zu -> %zu", ar->nalloc, newsz));
		tmp = realloc(ar->items, newsz * sizeof(struct libbex_value *));
		if (!tmp)
			return -ENOMEM;
		ar->items = tmp;
		ar->nalloc = newsz;
	}

	bex_ref_value(va);
	ar->items[ar->nitems] = va;
	ar->nitems++;

	DBG(ARY, bex_debugobj(ar, " add '%s' [%p]", va->name, va));
	return 0;
}

/**
 * bex_array_remove:
 * @ar: array
 * @va: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_array_remove(struct libbex_array *ar, struct libbex_value *va)
{
	size_t i;

	if (!ar || !va)
		return -EINVAL;

	for (i = 0; i < ar->nitems; i++) {
		if (ar->items[i] == va)
			break;
	}

	if (i == ar->nitems)
		return -EINVAL;

	/* move */
	for (; i < ar->nitems - 1; i++)
		ar->items[i] = ar->items[i + 1];

	ar->items[ar->nitems - 1] = NULL;	/* last */
	ar->nitems--;

	DBG(ARY, bex_debugobj(ar, "  remove '%s' [%p]", va->name, va));
	bex_unref_value(va);
	return 0;
}

/**
 * bex_reset_array:
 * @ar: array
 *
 * Removes generated values, zeroize/free the rest.
 */
void bex_reset_array(struct libbex_array *ar)
{
	size_t i = 0;

	if (!ar)
		return;

	DBG(ARY, bex_debugobj(ar, " reseting [nitems=%zu]", ar->nitems));

	do {
		struct libbex_value *va = ar->items[i];

		if (va->generated)
			bex_array_remove(ar, va);
		else {
			bex_reset_value(va);
			i++;
		}
	} while (i < ar->nitems);

	DBG(ARY, bex_debugobj(ar, " reset done [nitems=%zu]", ar->nitems));
}

/**
 * bex_array_is_empty:
 * @ar: array
 *
 * Returns: 0 or 1
 */
int bex_array_is_empty(struct libbex_array *ar)
{
	return !ar || ar->nitems == 0;
}

/**
 * bex_array_get:
 * @ar: array
 * @name: value name
 *
 * Returns: value or NULL
 */
struct libbex_value *bex_array_get(struct libbex_array *ar, const char *name)
{
	size_t i;

	if (!ar || !name)
		return NULL;

	for (i = 0; i < ar->nitems; i++) {
		if (strcmp(ar->items[i]->name, name) == 0) {
			return ar->items[i];
		}
	}

	return NULL;
}

/**
 * bex_array_nget:
 * @ar: array
 * @name: value name
 *
 * Like bex_array_get(), but name do not have to be zero terminated.
 *
 * Returns: value or NULL
 */
struct libbex_value *bex_array_nget(struct libbex_array *ar, const char *name, size_t n)
{
	size_t i;

	if (!ar || !name)
		return NULL;

	for (i = 0; i < ar->nitems; i++) {
		if (strncmp(ar->items[i]->name, name, n) == 0) {
			return ar->items[i];
		}
	}

	return NULL;
}

int bex_array_to_stream(struct libbex_array *ar, FILE *stream)
{
	size_t i;

	if (!ar || !stream)
		return -EINVAL;

	DBG(ARY, bex_debugobj(ar, "converting to string"));

	for (i = 0; i < ar->nitems; i++) {
		struct libbex_value *va = ar->items[i];

		if (i > 0)
			fputs(",", stream);

		switch (va->type) {
		case BEX_TYPE_STR:
			fprintf(stream, "\"%s\": \"%s\"", va->name, va->data.str);
			break;
		case BEX_TYPE_U64:
			fprintf(stream, "\"%s\": %ju", va->name, va->data.u64);
			break;
		case BEX_TYPE_S64:
			fprintf(stream, "\"%s\": %jd", va->name, va->data.s64);
			break;
		case BEX_TYPE_FLOAT:
			fprintf(stream, "\"%s\": %Lg", va->name, va->data.fl);
			break;
		default:
			break;
		}
	}

	return 0;
}

/*
 * returns next NAME and VALUE pair from { name=value, ... } string
 */
static int parse_next(char **optstr, char **name, size_t *namesz,
				char **value, size_t *valsz)
{
	int open_quote = 0;
	char *start = NULL, *stop = NULL, *p, *sep = NULL;
	char *optstr0;

	optstr0 = *optstr;

	if (name)
		*name = NULL;
	if (namesz)
		*namesz = 0;
	if (value)
		*value = NULL;
	if (valsz)
		*valsz = 0;

	while (optstr0 && *optstr0 == ',')
		optstr0++;

	for (p = optstr0; p && *p; p++) {
		if (!start && !stop && (isspace(*p) || *p == '{' || *p == '}'))
			continue;
		if (!start)
			start = p;		/* beginning of the option item */
		if (*p == '"')
			open_quote ^= 1;	/* reverse the status */
		if (open_quote)
			continue;		/* still in quoted block */
		if (!sep && p > start && *p == ':')
			sep = p;		/* name and value separator */
		if (*p == ',')
			stop = p;		/* terminate the option item */
		else if (*(p + 1) == '\0' || *(p + 1) == '}')
			stop = p;		/* end of optstr */
		if (!start || !stop)
			continue;
		if (stop <= start)
			goto error;

		*optstr = *stop ? stop + 1 : stop;

		if (*start == '"')
			start++;
		if (*stop == '"')
			stop--;

		if (name)
			*name = start;
		if (namesz) {
			char *end = sep - 1;
			while (end > start && (*end == '"' || isspace(*end)))
				--end;
			*namesz = (end - start) + 1;
		}

		sep++;
		while (sep < stop && (*sep == '"' || isspace(*sep)))
			sep++;

		while (stop > sep && (*stop == '"' || isspace(*stop) || *stop == ','))
			stop--;
		if (value)
			*value = sep;
		if (valsz)
			*valsz = (stop - sep) + 1;

		return 0;
	}

	return 1;				/* end of optstr */
error:
	DBG(ARY, bex_debug("failed to parse >>>%s<<<", *optstr));
	return -1;
}

/*
 * Fill array from { name: data, ... } string
 */
int bex_array_fill_from_string(struct libbex_array *ar, const char *str)
{
	char *name, *value, *p = (char *) str;
	size_t namesz, valsz;
	struct libbex_value *va = NULL;
	int rc;

	DBG(ARY, bex_debugobj(ar, "filling from string"));

	while (parse_next(&p, &name, &namesz, &value, &valsz) == 0) {
		va = bex_array_nget(ar, name, namesz);
		if (!va) {
			/* add value on the fly */
			char *vname = strndup(name, namesz);

			if (!vname)
				goto err_gen;

			va = __bex_new_value(vname);
			if (!va)
				goto err_gen;

			va->type = BEX_TYPE_STR;
			if (bex_array_add(ar, va))
				goto err_gen;
			bex_value_set_generated(va, 1);
			bex_unref_value(va);
		}

		rc = bex_value_set_from_string(va, value, valsz);
		if (rc)
			break;
	}

	return rc;
err_gen:
	bex_unref_value(va);
	return -ENOMEM;
}

/*
 * returns next VALUE  from [ value, ... ] string
 */
static int parse_next_unnamed(char **optstr, char **value, size_t *valsz)
{
	int open_quote = 0;
	char *start = NULL, *stop = NULL, *p, *sep = NULL;
	char *optstr0;

	optstr0 = *optstr;

	if (value)
		*value = NULL;
	if (valsz)
		*valsz = 0;

	while (optstr0 && *optstr0 == ',')
		optstr0++;

	for (p = optstr0; p && *p; p++) {
		if (!start && !stop && (isspace(*p) || *p == '[' || *p == ']'))
			continue;
		if (!start)
			start = p;		/* beginning of the option item */
		if (*p == '"')
			open_quote ^= 1;	/* reverse the status */
		if (open_quote)
			continue;		/* still in quoted block */
		if (!stop && p > start && *p == ',')
			stop = p;		/* terminate the option item */
		else if (*(p + 1) == '\0' || *(p + 1) == ']')
			stop = p;		/* end of optstr */
		if (!start || !stop)
			continue;
		if (stop <= start)
			goto error;

		*optstr = *stop ? stop + 1 : stop;

		if (*start == '"')
			start++;
		if (*stop == '"')
			stop--;

		if (value)
			*value = start;
		if (valsz)
			*valsz = (stop - sep) + 1;
		return 0;
	}

	return 1;				/* end of optstr */
error:
	DBG(ARY, bex_debug("failed to parse >>>%s<<<", *optstr));
	return -1;
}


int bex_array_fill_unnamed_from_string(struct libbex_array *ar, const char *str, char **next)
{
	char *p = (char *) str;
	int rc = 0;
	size_t i;

	if (bex_array_is_empty(ar))
		return -EINVAL;

	DBG(ARY, bex_debugobj(ar, "filling from unnamed string"));

	for (i = 0; i < ar->nitems; i++) {
		struct libbex_value *va = ar->items[i];
		char *value;
		size_t valsz;

		if (parse_next_unnamed(&p, &value, &valsz) != 0)
			break;

		DBG(ARY, bex_debugobj(ar, "  %s=%s", va->name, value));
		rc = bex_value_set_from_string(va, value, valsz);
		if (rc)
			break;
	}

	if (*p == ']' && next )
		*next = p + 1;

	DBG(ARY, bex_debugobj(ar, "done [rc=%d]", rc));
	return rc;
}

