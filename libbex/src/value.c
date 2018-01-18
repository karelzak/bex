
#include "bexP.h"

static void value_reset(struct libbex_value *va)
{
	if (va->type == BEX_TYPE_STR) {
		free(va->data.str);
		va->data.str = NULL;
	}
}

static void free_value(struct libbex_value *va)
{
	if (!va)
		return;

	DBG(VAL, bex_debugobj(va, "free [name=%s]", va->name));
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

	DBG(VAL, bex_debugobj(va, "alloc [name=%s]", name));
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

int bex_value_set_u64(struct libbex_value *va, uint64_t num)
{
	value_reset(va);
	va->data.u64 = num;
	va->type = BEX_TYPE_U64;
	return 0;
}

uint64_t bex_value_get_u64(struct libbex_value *va)
{
	return va->data.u64;
}

struct libbex_value *bex_new_value_u64(const char *name, uint64_t n)
{
	struct libbex_value *va = bex_new_value(name);
	if (va)
		bex_value_set_u64(va, n);
	return va;
}

int bex_value_set_s64(struct libbex_value *va, int64_t num)
{
	value_reset(va);
	va->data.s64 = num;
	va->type = BEX_TYPE_S64;
	return 0;
}

int64_t bex_value_get_s64(struct libbex_value *va)
{
	return va->data.s64;
}

struct libbex_value *bex_new_value_s64(const char *name, int64_t n)
{
	struct libbex_value *va = bex_new_value(name);
	if (va)
		bex_value_set_s64(va, n);
	return va;
}

int bex_value_set_str(struct libbex_value *va, const char *str)
{
	value_reset(va);
	if (str)
		va->data.str = strdup(str);
	va->type = BEX_TYPE_STR;
	return 0;
}

char *bex_value_get_str(struct libbex_value *va)
{
	return va->data.str;
}

struct libbex_value *bex_new_value_str(const char *name, const char *str)
{
	struct libbex_value *va = bex_new_value(name);
	if (va)
		bex_value_set_str(va, str);
	return va;
}

int bex_value_set_float(struct libbex_value *va, double num)
{
	value_reset(va);
	va->data.fl = num;
	va->type = BEX_TYPE_FLOAT;
	return 0;
}

double bex_value_get_float(struct libbex_value *va)
{
	return va->data.fl;
}

struct libbex_value *bex_new_value_float(const char *name, double n)
{
	struct libbex_value *va = bex_new_value(name);
	if (va)
		bex_value_set_float(va, n);
	return va;
}


