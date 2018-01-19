
#include "bexP.h"

void bex_reset_value(struct libbex_value *va)
{
	if (va->type == BEX_TYPE_STR)
		free(va->data.str);

	memset(&va->data, 0, sizeof(va->data));
}

static void free_value(struct libbex_value *va)
{
	if (!va)
		return;

	DBG(VAL, bex_debugobj(va, "free [name=%s]", va->name));
	free(va->name);
	bex_reset_value(va);
	free(va);
}

struct libbex_value *__bex_new_value(char *name)
{
	struct libbex_value *va = calloc(1, sizeof(*va));
	if (!va)
		goto err;

	DBG(VAL, bex_debugobj(va, "alloc [name=%s]", name));
	va->refcount = 1;
	va->name = name;
	return va;
err:
	free_value(va);
	return NULL;
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
	char *p = strdup(name);

	if (!p)
		return NULL;

	return __bex_new_value(p);
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

/**
 * bex_value_set_generated:
 * @va: value pointer
 *
 * Marks value as generated (see bex_reset_event(), etc.)
 *
 * Returns: 0 on success, <0 on error.
 */
int bex_value_set_generated(struct libbex_value *va, int status)
{
	if (!va)
		return -EINVAL;
	va->generated = status ? 1 : 0;
	return 0;
}

int bex_value_set_u64(struct libbex_value *va, uint64_t num)
{
	bex_reset_value(va);
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
	bex_reset_value(va);
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
	bex_reset_value(va);
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

int bex_value_set_float(struct libbex_value *va, long double num)
{
	bex_reset_value(va);
	va->data.fl = num;
	va->type = BEX_TYPE_FLOAT;
	return 0;
}

long double bex_value_get_float(struct libbex_value *va)
{
	return va->data.fl;
}

struct libbex_value *bex_new_value_float(const char *name, long double n)
{
	struct libbex_value *va = bex_new_value(name);
	if (va)
		bex_value_set_float(va, n);
	return va;
}
