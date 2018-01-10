
#include "bexP.h"

static void free_array(struct libbex_array *ar)
{
	size_t i;

	if (!ar)
		return;

	DBG(ARY, bex_debugobj(ar, "free"));
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

	if (ar->nitems + 1 >= ar->nalloc) {
		void *tmp = realloc(ar->items, ar->nalloc + 1);
		if (!tmp)
			return -ENOMEM;
		ar->items = tmp;
		ar->nalloc++;
	}

	bex_ref_value(va);
	ar->items[ar->nitems] = va;
	ar->nitems++;

	DBG(ARY, bex_debugobj(ar, "add value: %s [%p]", va->name, va));
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
		if (ar->items[i] == va) {
			if (i < ar->nitems - 1)
				memmove(ar->items[i], ar->items[i+1], ar->nitems - i - 1);
			ar->nitems--;
			DBG(ARY, bex_debugobj(ar, "remove value: %s [%p]", va->name, va));
			bex_unref_value(va);
			return 0;
		}
	}

	return -EINVAL;
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
			fputs(", ", stream);

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
		default:
			break;
		}
	}

	return 0;
}

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
	DBG(ARY, bex_debugobj(ar, "failed to parse >>>%s<<<", optstr));
	return -1;
}

int bex_array_fill_from_string(struct libbex_array *ar, const char *str)
{
	char *name, *value, *p = (char *) str;
	size_t namesz, valsz;


	if (bex_array_is_empty(ar))
		return 0;

	while (parse_next(&p, &name, &namesz, &value, &valsz) == 0) {
		struct libbex_value *va = bex_array_nget(ar, name, namesz);
		char *end = NULL;

		if (!va)
			continue;

		errno = 0;

		switch (va->type) {
		case BEX_TYPE_STR:
			free(va->data.str);
			va->data.str = strndup(value, valsz);
			break;
		case BEX_TYPE_U64:
			va->data.u64 = strtoumax(value, &end, 10);
			if (errno || value == end)
				DBG(ARY, bex_debugobj(ar, "strtoumax() failed"));
			break;
		case BEX_TYPE_S64:
			va->data.s64 = strtosmax(value, &end, 10);
			if (errno || value == end)
				DBG(ARY, bex_debugobj(ar, "strtosmax() failed"));
			break;
		default:
			break;
		}
	}

	return 0;
}

