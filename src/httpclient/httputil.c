#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <zlib.h>

#include "httpclient/httpclient.h"
#include "httpclient/httperr.h"

HttpUrl*
httputil_url_parse(char *url_s, HttpClientError **err)
{
    char *scheme = g_uri_parse_scheme(url_s);
    if (!scheme) {
        *err = httperror_create(URL_NO_SCHEME, "No scheme.");
        return NULL;
    }
    if ((strcmp(scheme, "http") != 0) && (strcmp(scheme, "https") != 0)) {
        *err = httperror_create(URL_INVALID_SCHEME, "Invalid scheme.");
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
            *err = httperror_create(URL_INVALID_PORT, "Invalid port.");
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

GByteArray *
httputil_gzip_inflate(GByteArray *deflated)
{
    char inflated[500000];
    memset(inflated, 0, 500000);

    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = (uInt)deflated->len;
    infstream.next_in = (Bytef *)deflated->data;
    infstream.avail_out = (uInt)sizeof(inflated);
    infstream.next_out = (Bytef *)inflated;

    inflateInit2(&infstream, 16+MAX_WBITS);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);

    GByteArray *body_inflate = g_byte_array_new();
    g_byte_array_append(body_inflate, (unsigned char*)inflated, strlen(inflated));

    return body_inflate;
}
