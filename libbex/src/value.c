
#include "bexP.h"

static void value_reset(struct libbex_value *va)
{
	if (va->type == BEX_TYPE_STR)
		free(va->data.str);
	va->type = 0;
}

static void free_value(struct libbex_value *va)
{
	if (!va)
		return;

	free(va->name);
	value_reset(va);
	free(va);
}

/**
 * bex_new_value:
 * @name: value name
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the filesystem.
 *
 * Returns: newly allocated struct libbex_value.
 */
struct libbex_value *bex_new_value(const char *name)
{
	struct libbex_value *va = calloc(1, sizeof(*va));
	if (!va)
		goto err;

	va->refcount = 1;
	va->name = strdup(name);
	if (!va->name)
		goto err;
	return va;
err:
	free_value(va);
	return NULL;
}


/**
 * bex_ref_value:
 * @va: value pointer
 *
 * Increments reference counter.
 */
void bex_ref_value(struct libbex_value *va)
{
	if (va)
		va->refcount++;
}

/**
 * bex_unref_value:
 * @va: value pointer
 *
 * De-increments reference counter, on zero the @va is automatically
 * deallocated.
 */
void bex_unref_value(struct libbex_value *va)
{
	if (va) {
		va->refcount--;
		if (va->refcount <= 0)
			free_value(va);
	}
}

int bex_value_set_u64_data(struct libbex_value *va, uint64_t num)
{
	value_reset(va);
	va->data.u64 = num;
	va->type = BEX_TYPE_U64;
	return 0;
}

struct libbex_value *bex_new_value_u64(const char *name, uint64_t n)
{
	struct libbex_value *va = bex_new_value(name);
	if (va)
		bex_value_set_u64_data(va, n);
	return va;
}

int bex_value_set_s64_data(struct libbex_value *va, int64_t num)
{
	value_reset(va);
	va->data.s64 = num;
	va->type = BEX_TYPE_S64;
	return 0;
}

struct libbex_value *bex_new_value_s64(const char *name, int64_t n)
{
	struct libbex_value *va = bex_new_value(name);
	if (va)
		bex_value_set_s64_data(va, n);
	return va;
}

int bex_value_set_str_data(struct libbex_value *va, const char *str)
{
	value_reset(va);
	va->data.str = strdup(str);
	va->type = BEX_TYPE_STR;
	return 0;
}
struct libbex_value *bex_new_value_str(const char *name, const char *str)
{
	struct libbex_value *va = bex_new_value(name);
	if (va)
		bex_value_set_str_data(va, str);
	return va;
}

