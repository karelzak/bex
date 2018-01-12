
#include "bexP.h"
#include <libwebsockets.h>

#define wss_count_bufsiz(x)		(LWS_SEND_BUFFER_PRE_PADDING + x + LWS_SEND_BUFFER_POST_PADDING)
#define BEX_WSS_MINBUFSIZ		wss_count_bufsiz(125)

struct wss_ctl {
	struct lws_context	*context;
	struct lws		*wsi;
	struct libbex_platform  *pl;

	struct list_head	pending_data;
	struct list_head	free_data;
	unsigned char		*buf;
	size_t			bufsz;

	unsigned int		established : 1;
};

struct wss_iovec {
	unsigned char	*buf;
	size_t		sz;

	struct list_head	vects;
};

static int wss_write(struct wss_ctl *wss);

static int wss_callback(struct lws *wsi,
			enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct wss_ctl *wss = (struct wss_ctl *) user;


	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		DBG(WSS, bex_debug("CALLBACK: client extablished"));
		if (wss)
			wss->established = 1;
		lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		DBG(WSS, bex_debug("CALLBACK: client incomming data"));
		if (wss && in)
			bex_platform_receive(wss->pl, in);
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		DBG(WSS, bex_debug("CALLBACK: client writeable"));
		if (wss)
			wss_write(wss);
		break;

	case LWS_CALLBACK_CLOSED:
		DBG(WSS, bex_debug("CALLBACK: close"));
		if (wss)
			wss->established = 0;
		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		DBG(WSS, bex_debug("CALLBACK: connection error %s", in ? (char *) in : ""));
		if (wss)
			wss->established = 0;
		break;

	default:
		break;
	}

	return 0;
}

int wss_is_connected(struct libbex_platform *pl)
{
	return pl && pl->wss && ((struct wss_ctl *) pl->wss)->wsi;
}

static const struct lws_protocols __wss_protocols[] =
{
	{
		.name = "",
		.callback = wss_callback,
		.rx_buffer_size = 4024
	},
	{ NULL, NULL, 0, 0 } /* end */
};

int wss_connect(struct libbex_platform *pl)
{
	struct wss_ctl *wss = NULL;
        struct lws_client_connect_info cinfo;
	unsigned int try;

	if (!pl)
		return -EINVAL;

	DBG(WSS, bex_debug("connect"));

	wss = (struct wss_ctl *) pl->wss;
	if (!wss) {
	        struct lws_context_creation_info info;

		lws_set_log_level(0, NULL);
		ON_DBG(WSS, lws_set_log_level(LLL_ERR|LLL_WARN|LLL_NOTICE|LLL_INFO|LLL_DEBUG|LLL_PARSER|LLL_HEADER|LLL_CLIENT, NULL));

		wss = calloc(1, sizeof(struct wss_ctl));
		if (!wss)
			return -ENOMEM;
		DBG(WSS, bex_debugobj(wss, "alloc"));
		wss->pl = pl;
		INIT_LIST_HEAD(&wss->pending_data);
		INIT_LIST_HEAD(&wss->free_data);

		memset(&info, 0, sizeof info);
		info.port = CONTEXT_PORT_NO_LISTEN;
		info.iface = NULL;
		info.protocols = __wss_protocols;
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
		info.gid = -1;
		info.uid = -1;
		info.options = 0;
#if defined(LWS_OPENSSL_SUPPORT)
		info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#endif
		DBG(WSS, bex_debugobj(wss, "create context"));
		wss->context = lws_create_context(&info);
		if (!wss->context) {
			DBG(WSS, bex_debugobj(wss, "failed to create a context"));
			return -EINVAL;
		}

		pl->wss = (void *) wss;
		DBG(WSS, bex_debugobj(wss, "initialize data done"));
	}

	if (wss->wsi) {
		DBG(WSS, bex_debugobj(wss, "already connected"));
		return 0;
	}

	memset(&cinfo, 0, sizeof cinfo);
	cinfo.context = wss->context;
	cinfo.ssl_connection = pl->uri_ssl;
	cinfo.port = pl->uri_port;
	cinfo.address = pl->uri_addr;
	cinfo.path = pl->uri_path;
	cinfo.host = pl->uri_addr;
	cinfo.origin = pl->uri_addr;
	cinfo.ietf_version_or_minus_one = -1;
	cinfo.protocol = "";
	cinfo.userdata = wss;

	for (try = 0; try < pl->connection_attempts; try++) {
		DBG(WSS, bex_debugobj(wss, "#%u connecting...", try));
		wss->wsi = lws_client_connect_via_info(&cinfo);
		if (wss->wsi)
			break;
		if (pl->reconnect_timeout) {
			DBG(WSS, bex_debugobj(wss, "  timeout [%u]", pl->reconnect_timeout));
			xusleep(pl->reconnect_timeout);
		}
	}

	/* wait to fully initialize connection */
	while (!wss->established)
		lws_service(wss->context, 50);

	DBG(WSS, bex_debugobj(wss, "... done [%s]", wss->wsi ?  "CONNECTED" : "FAILED"));
	return wss->wsi ? 0 : -1;
}

int wss_disconnect(struct libbex_platform *pl)
{
	struct wss_ctl *wss;

	if (!pl || !pl->wss)
		return -EINVAL;

	wss = (struct wss_ctl *) pl->wss;

	if (wss->wsi) {
		DBG(WSS, bex_debugobj(wss, "close connection"));
		lws_close_reason(wss->wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
	}

	DBG(WSS, bex_debugobj(wss, "destroy context"));
	lws_context_destroy(wss->context);

	DBG(WSS, bex_debugobj(wss, "free"));
	free(wss);
	pl->wss = NULL;
	return 0;
}

int wss_service(struct libbex_platform *pl)
{
	struct wss_ctl *wss;

	if (!pl || !pl->wss)
		return -EINVAL;

	wss = (struct wss_ctl *) pl->wss;

	DBG(WSS, bex_debugobj(wss, "service [timeout=%d]", pl->service_timeout));
	if (!wss->established) {
		wss->wsi = NULL;
		wss_connect(pl);
	}
	if (!wss->wsi)
		DBG(WSS, bex_debugobj(wss, "no connection"));
	else
		lws_service(wss->context, pl->service_timeout);

	return 0;
}

int wss_send(struct libbex_platform *pl, unsigned char *str, size_t sz)
{
	struct wss_ctl *wss;
	struct wss_iovec *io = NULL;

	if (!pl || !pl->wss)
		return -EINVAL;

	wss = (struct wss_ctl *) pl->wss;
	DBG(WSS, bex_debugobj(wss, "add new pending data [sz=%zu]", sz));

	if (list_empty(&wss->free_data)) {
		io = calloc(1, sizeof(struct wss_iovec));
		if (!io)
			return -ENOMEM;
		DBG(WSS, bex_debugobj(wss, "alloc iovec [%p]", io));
	} else {
		io = list_first_entry(&wss->free_data, struct wss_iovec, vects);
		list_del(&io->vects);
		memset(io, 0, sizeof(*io));
		DBG(WSS, bex_debugobj(wss, "reuse iovec [%p]", io));
	}

	INIT_LIST_HEAD(&io->vects);
	io->sz = sz;
	io->buf = str;
	list_add_tail(&io->vects, &wss->pending_data);

	/* inform libwebsockets that we want to send data */
	lws_callback_on_writable( wss->wsi );
	return 0;
}

/* write all pending data */
static int wss_write(struct wss_ctl *wss)
{
	struct list_head *pe, *pnext;

	DBG(WSS, bex_debugobj(wss, "writing pending data... "));

	if (list_empty(&wss->pending_data)) {
		DBG(WSS, bex_debugobj(wss, " no data pending"));
		return 0;
	}

	list_for_each_safe(pe, pnext, &wss->pending_data) {
		struct wss_iovec *io = list_entry(pe, struct wss_iovec, vects);
		size_t sz = wss_count_bufsiz(io->sz);
		unsigned char *p;

		/* (re)allocate buffer */
		if (wss->bufsz < sz) {
			size_t newsz = sz < BEX_WSS_MINBUFSIZ ? BEX_WSS_MINBUFSIZ : sz;
			unsigned char *tmp = realloc(wss->buf, newsz);

			DBG(WSS, bex_debugobj(wss, " (re)allocated new write buffer [sz=%zu]", newsz));
			if (!tmp)
				return -ENOMEM;

			wss->buf = tmp;
			wss->bufsz = newsz;
		}

		DBG(WSS, bex_debugobj(wss, " writing iovec [%p, sz=%zu]", io, io->sz));

		/* copy data to buffer */
		p = wss->buf + LWS_SEND_BUFFER_PRE_PADDING;
		memcpy(p, io->buf, io->sz);

		/* send */
		lws_write(wss->wsi, p, io->sz, LWS_WRITE_TEXT);

		/* deallocate and move to unused */
		DBG(WSS, bex_debugobj(wss, " remove from pending iovec [%p]", io));
		free(io->buf);
		list_del(&io->vects);
		list_add_tail(&io->vects, &wss->free_data);
	}

	return 0;
}
