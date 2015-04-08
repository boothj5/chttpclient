#ifndef _HTTPNET_H
#define _HTTPNET_H

#include "httpclient/httpclient.h"
#include "httpclient/httpcontext.h"

int httpnet_connect(HttpContext context, HttpClientError **err);
void httpnet_send(HttpRequest request, HttpClientError **err);

void httpnet_read_headers(HttpResponse response, HttpClientError **err);
void httpnet_read_body(HttpResponse response, HttpClientError **err);

void httpnet_close(HttpContext context);

#endif