#ifndef _HTTPCLIENT_H
#define _HTTPCLIENT_H

#include <glib.h>

#include "httpcontext.h"

typedef enum {
    URL_NO_SCHEME,
    URL_INVALID_SCHEME,
    URL_INVALID_PORT,
    REQ_INVALID_METHOD,
    SOCK_CREATE_FAILED,
    SOCK_CONNECT_FAILED,
    SOCK_SEND_FAILED,
    SOCK_TIMEOUT,
    SOCK_RECV_FAILED,
    HOST_LOOKUP_FAILED,
    RESP_UPSUPPORTED_TRANSFER_ENCODING,
    RESP_ERROR_PARSING_CHUNK,
    GZIP_BUFFER_INSUFFICIENT,
    GZIP_INSUFFICIENT_MEMORY,
    GZIP_CORRUPT,
    GZIP_FAILED
} httpclient_err_t;

typedef struct httpurl_t {
    char *scheme;
    char *host;
    int port;
    char *resource;
} HttpUrl;

typedef struct httprequest_t *HttpRequest;
typedef struct httpresponse_t *HttpResponse;
typedef struct httpcontext_t *HttpContext;

void http_error(char *prefix, httpclient_err_t err);

HttpUrl* httputil_url_parse(char *url_s, httpclient_err_t *err);
void httputil_url_destroy(HttpUrl *url);

HttpContext httpcontext_create(char *host, httpclient_err_t *err);
HttpContext httpcontext_ref(HttpContext ctx);
HttpContext httpcontext_unref(HttpContext ctx);
void httpcontext_debug(HttpContext ctx, gboolean debug);
void httpcontext_read_timeout(HttpContext ctx, int read_timeout_ms);

HttpRequest httprequest_create(HttpContext context, char *resource, char *method, httpclient_err_t *err);
HttpRequest httprequest_ref(HttpRequest request);
HttpRequest httprequest_unref(HttpRequest request);
void httprequest_addheader(HttpRequest request, const char * const key, const char *const val);
HttpResponse httprequest_perform(HttpRequest request, httpclient_err_t *err);

int httpresponse_status(HttpResponse response);
char* httpresponse_status_message(HttpResponse response);
char* httpresponse_body_to_file(HttpResponse response);
char* httpresponse_body_as_string(HttpResponse response);
GHashTable* httpresponse_headers(HttpResponse response);
void httpresponse_destroy(HttpResponse response);

#endif