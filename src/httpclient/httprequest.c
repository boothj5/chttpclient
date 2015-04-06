#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <glib.h>

#include "httprequest.h"
#include "httpnet.h"
#include "httpclient.h"
#include "httpresponse.h"

HttpUrl*
_url_parse(char *url_s, request_err_t *err)
{
    char *scheme = g_uri_parse_scheme(url_s);
    if (!scheme) {
        *err = URL_NO_SCHEME;
        return NULL;
    }
    if ((strcmp(scheme, "http") != 0) && (strcmp(scheme, "https") != 0)) {
        *err = URL_INVALID_SCHEME;
        g_free(scheme);
        return NULL;
    }

    int pos = 0;
    if (strcmp(scheme, "http") == 0) {
        pos = strlen("http://");
    } else {
        pos = strlen("https://");
    }
    int start = pos;
    while (url_s[pos] != '/' && url_s[pos] != ':' && pos < (int)strlen(url_s)) pos++;
    char *host = strndup(&url_s[start], pos - start);

    char *port_s = NULL;
    if (url_s[pos] == ':') {
        pos++;
        start = pos;
        while (url_s[pos] != '/' && pos < (int)strlen(url_s)) pos++;
        port_s = strndup(&url_s[start], pos - start);
    } else {
        port_s = strdup("80");
    }

    char *end;
    errno = 0;
    int port = (int) strtol(port_s, &end, 10);
    if ((!(errno == 0 && port_s && !*end)) || (port < 1)) {
        *err = URL_INVALID_PORT;
        g_free(scheme);
        free(host);
        free(port_s);
        return NULL;
    }

    char *path = NULL;
    if (url_s[pos] != '\0') {
        path = strdup(&url_s[pos]);
    } else {
        path = strdup("/");
    }

    HttpUrl *url = malloc(sizeof(struct httpurl_t));
    url->scheme = scheme;
    url->host = host;
    url->port = port;
    url->path = path;

    return url;
}

HttpUrl*
_url_copy(HttpUrl *url)
{
    if (url) {
        HttpUrl *result = malloc(sizeof(struct httpurl_t));
        result->scheme = strdup(url->scheme);
        result->host = strdup(url->host);
        result->port = url->port;
        result->path = strdup(url->path);
        return result;
    } else {
        return NULL;
    }
}

void
httprequest_addheader(HttpRequest request, const char * const key, const char *const val)
{
    g_hash_table_replace(request->headers, strdup(key), strdup(val));
}

HttpRequest
httprequest_create(char *url_s, char *method, request_err_t *err)
{
    if (g_strcmp0(method, "GET") != 0) {
        *err = REQ_INVALID_METHOD;
        return NULL;
    }

    HttpUrl *url = _url_parse(url_s, err);
    if (!url) {
        return NULL;
    }

    HttpRequest request = malloc(sizeof(struct httprequest_t));
    request->url = url;
    request->method = strdup(method);
    request->headers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    GString *host_val = g_string_new("");
    g_string_append(host_val, request->url->host);
    if (request->url->port != 80) g_string_append_printf(host_val, ":%d", request->url->port);
    httprequest_addheader(request, "Host", host_val->str);
    g_string_free(host_val, FALSE);

    return request;
}

HttpResponse
httprequest_perform(HttpContext context, HttpRequest request, request_err_t *err)
{
    // connect socket
    int sock = httpnet_connect(context, request->url->host, request->url->port, err);
    if (sock == -1) return NULL;

    // send request
    gboolean sent = httpnet_send(context, sock, request, err);
    if (!sent) return NULL;

    // read response
    HttpResponse response = malloc(sizeof(struct httpresponse_t));
    response->url = _url_copy(request->url);

    // read headers
    gboolean headers_read = httpnet_read_headers(context, sock, response, err);
    if (!headers_read) return NULL;

    // read body
    gboolean body_read = httpnet_read_body(context, sock, response, err);
    if (!body_read) return NULL;

    close(sock);

    return response;
}