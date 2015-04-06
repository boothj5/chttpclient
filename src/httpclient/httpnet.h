#ifndef _HTTPNET_H
#define _HTTPNET_H

#include "httpclient.h"
#include "httpcontext.h"

int httpnet_connect(HttpContext context, httpclient_err_t *err);
gboolean httpnet_send(HttpRequest request, int sock, httpclient_err_t *err);

gboolean httpnet_read_headers(HttpResponse response, int sock, httpclient_err_t *err);
gboolean httpnet_read_body(HttpResponse response, int sock, httpclient_err_t *err);

#endif