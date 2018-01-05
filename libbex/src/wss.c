
#include "bexP.h"
#include <libwebsockets.h>

#define  SESSION_DATASZ	150


struct wss_data {

	struct lws_context	*context;
	struct lws		*wsi;
	struct libbex_platform  *pl;
};

static struct wss_data *__wss;	/* TODO! */

static int wss_callback(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct wss_data *wss = __wss;
	struct libbex_platform  *pl;

	pl = (struct libbex_platform *) wss->pl;
	if (!pl) {
		DBG(WSS, bex_debug("CALLBACK: no platform pointer"));
		return 0;
	}

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		DBG(WSS, bex_debugobj(wss, "CALLBACK: client extablished"));
		lws_callback_on_writable(wss->wsi);
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		DBG(WSS, bex_debugobj(wss, "CALLBACK: client incomming data"));
		printf("data: >>%s<<\n", (char *)in);
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
	{
		DBG(WSS, bex_debugobj(wss, "CALLBACK: client writeable"));

		unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 100 + LWS_SEND_BUFFER_POST_PADDING];
		unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

		size_t n = sprintf((char *)p, "{\"event\":\"ping\", \"cid\": 1234 }");
		lws_write(wss->wsi, p, n, LWS_WRITE_TEXT );
		break;
	}

	case LWS_CALLBACK_CLOSED:
		DBG(WSS, bex_debugobj(wss, "CALLBACK: close"));
		wss->wsi = NULL;
		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		DBG(WSS, bex_debugobj(wss, "CALLBACK: connection error %s", in ? (char *) in : ""));
		wss->wsi = NULL;
		break;

	default:
		break;
	}

	return 0;
}

int wss_is_connected(struct libbex_platform *pl)
{
	return pl && pl->wss && ((struct wss_data *) pl->wss)->wsi;
}

static const struct lws_protocols __wss_protocols[] =
{
	{
		.name = "",
		.callback = wss_callback,
		.per_session_data_size = 0,
		.rx_buffer_size = 4024
	},
	{ NULL, NULL, 0, 0 } /* end */
};

int wss_connect(struct libbex_platform *pl)
{
	struct wss_data *wss;
        struct lws_client_connect_info cinfo;
	unsigned int try;

	if (!pl)
		return -EINVAL;

	wss = (struct wss_data *) pl->wss;
	if (!wss) {
	        struct lws_context_creation_info info;

		__wss = wss = calloc(1, sizeof(struct wss_data));
		if (!wss)
			return -ENOMEM;
		DBG(WSS, bex_debugobj(wss, "alloc"));

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
		wss->pl = pl;
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

	DBG(WSS, bex_debugobj(wss, "... done [%s]", wss->wsi ?  "CONNECTED" : "FAILED"));
	return wss->wsi ? 0 : -1;
}

int wss_disconnect(struct libbex_platform *pl)
{
	struct wss_data *wss;

	if (!pl || !pl->wss)
		return -EINVAL;

	wss = (struct wss_data *) pl->wss;

	if (wss->wsi) {
		DBG(WSS, bex_debugobj(wss, "close connection"));
		lws_close_reason(wss->wsi, LWS_CLOSE_STATUS_NORMAL, NULL, 0);
	}

	DBG(WSS, bex_debugobj(wss, "destroy context"));
	lws_context_destroy(wss->context);

	DBG(WSS, bex_debugobj(wss, "free"));
	free(wss);
	pl->wss = NULL;
	__wss = NULL;
	return 0;
}

int wss_service(struct libbex_platform *pl)
{
	struct wss_data *wss;

	if (!pl || !pl->wss)
		return -EINVAL;

	wss = (struct wss_data *) pl->wss;

	DBG(WSS, bex_debugobj(wss, "service"));
	lws_service(wss->context, pl->service_timeout);

	return 0;
}

