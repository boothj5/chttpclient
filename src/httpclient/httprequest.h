#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H

#include <glib.h>

typedef struct httpurl_t {
    char *scheme;
    char *host;
    int port;
    char *path;
} HttpUrl;

struct httprequest_t {
    HttpUrl *url;
    char *method;
    GHashTable *headers;
};

#endif