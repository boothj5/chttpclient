#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H

#include <glib.h>

#include "httpclient/httpclient.h"

struct httprequest_t {
    int refcount;
    HttpContext context;
    char *resource;
    char *method;
    GHashTable *headers;
};

#endif