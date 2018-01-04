
#include "bexP.h"

static void free_array(struct libbex_array *ar)
{
	size_t i;

	if (!ar)
		return;

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
		tmp = realloc(ar->items, ar->nalloc + 1);
		if (!tmp)
			return -ENOMEM;
		ar->items = tmp;
		ar->nalloc++;
	}

	bex_ref_value(va);
	ar->items[ar->nitems] = va;
	ar->nitems++;

	DBG(ARY, bex_debugobj(ar, "add value: %s", va->name));
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
				memmove(ar->items[i], ar->items[i+1], ar->nitems - i - 1));
			ar->nitems--;
			bex_unref_value(va);
			return 0;
		}
	}

	return -EINVAL;
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
	struct libbex_value *va;

	if (!ar || !name)
		return NULL;

	for (i = 0; i < ar->nitems; i++) {
		if (strcmp(ar->items[i].name, name) == 0) {
			return ar->items[i];
		}
	}

	return NULL;
}

char *bex_array_to_string(struct libbex_array *ar, const char *first, const char *firstval)
{
	size_t i, sz = 0;
	struct libbex_value *va;
	FILE *stream;
	char **res = NULL;

	if (!ar)
		return NULL;

	stream = open_memstream(data, &sz);
	if (!stream)
		return NULL;

	fputs("{ ", stream);

	if (first)
		fprintf(stream, "\"%s\": \"%s\"", first, firstval);

	for (i = 0; i < ar->nitems; i++) {
		struct libbex_value *va = ar->items[i];

		if (i > 0 || first)
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

	fputs("}", stream);
	fclose(stream);
	return res;
}

