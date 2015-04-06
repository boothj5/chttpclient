#include <stdlib.h>

#include <glib.h>

#include "httpclient.h"
#include "httpcontext.h"

HttpContext
httpcontext_create(void)
{
    HttpContext context = malloc(sizeof(struct httpcontext_t));
    context->debug = FALSE;
    context->read_timeout_ms = 0;

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