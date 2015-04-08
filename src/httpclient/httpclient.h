#ifndef _HTTPCLIENT_H
#define _HTTPCLIENT_H

#include <glib.h>

#include "httpclient/httpcontext.h"

#define HTTPHKEY_HOST "Host"
#define HTTPHKEY_USER_AGENT "User-Agent"
#define HTTPHKEY_CONTENT_TYPE "Content-Type"
#define HTTPHKEY_CONTENT_LENGTH "Content-Length"
#define HTTPHKEY_CONTENT_ENCODING "Content-Encoding"
#define HTTPHKEY_TRANSFER_ENCODING "Transfer-Encoding"
#define HTTPHKEY_ACCEPT_ENCODING "Accept-Encoding"

#define HTTPHVAL_GZIP "gzip"
#define HTTPHVAL_CHUNKED "chunked"

#define HTTPMETHOD_GET "GET"

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
} httpclient_errcode_t;

typedef struct httpclient_error_t {
    httpclient_errcode_t code;
    char *message;
} HttpClientError;

typedef struct httpurl_t {
    char *scheme;
    char *host;
    int port;
    char *resource;
} HttpUrl;

typedef struct httprequest_t *HttpRequest;
typedef struct httpresponse_t *HttpResponse;
typedef struct httpcontext_t *HttpContext;

char* httperror_getmessage(HttpClientError *err, char *prefix);
void httperror_destroy(HttpClientError *error);

HttpUrl* httputil_url_parse(char *url_s, HttpClientError **err);
void httputil_url_destroy(HttpUrl *url);

HttpContext httpcontext_create(char *host, HttpClientError **err);
HttpContext httpcontext_ref(HttpContext ctx);
HttpContext httpcontext_unref(HttpContext ctx);
void httpcontext_debug(HttpContext ctx, gboolean debug);
void httpcontext_read_timeout(HttpContext ctx, int read_timeout_ms);

HttpRequest httprequest_create(HttpContext context, char *resource, char *method, HttpClientError **err);
HttpRequest httprequest_ref(HttpRequest request);
HttpRequest httprequest_unref(HttpRequest request);
void httprequest_addheader(HttpRequest request, const char * const key, const char *const val);
HttpResponse httprequest_perform(HttpRequest request, HttpClientError **err);

int httpresponse_status(HttpResponse response);
char* httpresponse_status_message(HttpResponse response);
char* httpresponse_body_to_file(HttpResponse response, HttpClientError **err);
char* httpresponse_body_as_string(HttpResponse response, HttpClientError **err);
GHashTable* httpresponse_headers(HttpResponse response);
gboolean httpresponse_header_equals(HttpResponse response, char *key, char *val);
gboolean httpresponse_header_exists(HttpResponse response, char *key);

void httpresponse_destroy(HttpResponse response);

#endif