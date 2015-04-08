#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "httpclient/httpclient.h"
#include "httpclient/httpcontext.h"

HttpContext
httpcontext_create(char *host, HttpClientError **err)
{
    HttpUrl *url = httputil_url_parse(host, err);
    if (*err) {
        return NULL;
    }

    HttpContext context = malloc(sizeof(struct httpcontext_t));
    context->scheme = strdup(url->scheme);
    context->host = strdup(url->host);
    context->port = url->port;
    context->socket = -1;
    context->debug = FALSE;
    context->read_timeout_ms = 0;

    context->refcount = 1;

    httputil_url_destroy(url);

    return context;
}

void
httpcontext_debug(HttpContext ctx, gboolean debug)
{
    ctx->debug = debug;
}

void
httpcontext_read_timeout(HttpContext ctx, int read_timeout_ms)
{
    ctx->read_timeout_ms = read_timeout_ms;
}

HttpContext
httpcontext_ref(HttpContext ctx)
{
    ctx->refcount++;
    return ctx;
}

HttpContext
httpcontext_unref(HttpContext ctx)
{
    if (ctx) {
        ctx->refcount--;
        if (ctx->refcount == 0) {
            free(ctx->host);
            free(ctx->scheme);
            free(ctx);
            return NULL;
        } else {
            return ctx;
        }
    } else {
        return NULL;
    }
}

