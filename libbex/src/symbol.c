
#include "bexP.h"

static const struct libbex_symbol symbols[] = {
	{
		.name	= "XRPUSD",
		.left	= "XRP",
		.right	= "USD",
		.price	= "%.04Lf",
		.amount	= "%+6.0Lf",
	},{
		.name	= "BTCUSD",
		.left	= "BTC",
		.right	= "USD",
		.price	= "%.01Lf",
		.amount	= "%+7.02Lf",
	},{
		.name	= "ETHUSD",
		.left	= "ETH",
		.right	= "USD",
		.price	= "%.02Lf",
		.amount	= "%+7.02Lf",
	}

};

const struct libbex_symbol *bex_get_symbol(const char *name)
{
	size_t i;

	if (*name == 't' || *name == 'f')
		name++;

	for (i = 0; i < ARRAY_SIZE(symbols); i++) {
		if (strcmp(name, symbols[i].name) == 0)
			return &symbols[i];
	}

	return NULL;
}

const char *bex_symbol_get_name(const struct libbex_symbol *sy)
{
	return sy ? sy->name : NULL;
}

const char *bex_symbol_get_leftname(const struct libbex_symbol *sy)
{
	return sy ? sy->left : NULL;
}

const char *bex_symbol_get_rightname(const struct libbex_symbol *sy)
{
	return sy ? sy->right : NULL;
}

const char *bex_symbol_get_price_format(const struct libbex_symbol *sy)
{
	return sy ? sy->price : NULL;
}

const char *bex_symbol_get_amount_format(const struct libbex_symbol *sy)
{
	return sy ? sy->amount : NULL;
}
