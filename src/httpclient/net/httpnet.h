#ifndef _HTTPNET_H
#define _HTTPNET_H

#include "httpclient/httpclient.h"
#include "httpclient/httpcontext.h"

int httpnet_connect(HttpContext context, httpclient_err_t *err);
gboolean httpnet_send(HttpRequest request, httpclient_err_t *err);

gboolean httpnet_read_headers(HttpResponse response, httpclient_err_t *err);
gboolean httpnet_read_body(HttpResponse response, httpclient_err_t *err);

void httpnet_close(HttpContext context);

#endif