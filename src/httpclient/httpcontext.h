#ifndef _HTTPCONTEXT_H
#define _HTTPCONTEXT_H

#include <glib.h>

struct httpcontext_t {
    gboolean debug;
    int read_timeout_ms;
};

#endif