#ifndef _HTTPREQUEST_H
#define _HTTPREQUEST_H

#include <glib.h>

#include "httpclient.h"

struct httprequest_t {
    HttpContext context;
    char *resource;
    char *method;
    GHashTable *headers;
};

#endif