#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "httpclient.h"

HttpUrl*
httputil_url_parse(char *url_s, httpclient_err_t *err)
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
    int port = 80;
    if (url_s[pos] == ':') {
        pos++;
        start = pos;
        while (url_s[pos] != '/' && pos < (int)strlen(url_s)) pos++;
        port_s = strndup(&url_s[start], pos - start);
    }

    if (port_s && g_strcmp0(port_s, "80") != 0) {
        char *end;
        errno = 0;
        port = (int) strtol(port_s, &end, 10);
        if ((!(errno == 0 && port_s && !*end)) || (port < 1)) {
            *err = URL_INVALID_PORT;
            g_free(scheme);
            free(host);
            free(port_s);
            return NULL;
        }
    }

    char *resource = NULL;
    if (url_s[pos] != '\0') {
        resource = strdup(&url_s[pos]);
    } else {
        resource = strdup("/");
    }

    HttpUrl *url = malloc(sizeof(struct httpurl_t));
    url->scheme = scheme;
    url->host = host;
    url->port = port;
    url->resource = resource;

    return url;
}

void
httputil_url_destroy(HttpUrl *url)
{
    if (url) {
        free(url->scheme);
        free(url->host);
        free(url->resource);
        free(url);
    }
}
