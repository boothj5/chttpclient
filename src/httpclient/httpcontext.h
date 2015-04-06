#ifndef _HTTPCONTEXT_H
#define _HTTPCONTEXT_H

#include <glib.h>

struct httpcontext_t {
    int refcount;
    char *scheme;
    char *host;
    int port;
    gboolean debug;
    int read_timeout_ms;
};

#endif