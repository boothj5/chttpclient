#ifndef _HTTPNET_H
#define _HTTPNET_H

#include "httpclient.h"
#include "httpcontext.h"

int httpnet_connect(HttpContext context, char *host, int port, request_err_t *err);
gboolean httpnet_send(HttpContext context, int sock, HttpRequest request, request_err_t *err);

gboolean httpnet_read_headers(HttpContext context, int sock, HttpResponse response, request_err_t *err);
gboolean httpnet_read_body(HttpContext context, int sock, HttpResponse response, request_err_t *err);

#endif