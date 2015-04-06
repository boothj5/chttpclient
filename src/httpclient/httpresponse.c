#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include "httpclient.h"
#include "httprequest.h"
#include "httpresponse.h"

int
httpresponse_status(HttpResponse response)
{
    return response->status;
}

char*
httpresponse_status_message(HttpResponse response)
{
    return response->status_msg;
}

char*
httpresponse_body_to_file(HttpResponse response)
{
    char *last_slash = g_strrstr(response->url->path, "/");

    char *filename = NULL;
    if (last_slash && g_strcmp0(last_slash + 1, "\0") != 0) {
        filename = strdup(last_slash + 1);
    } else {
        filename = strdup("body.out");
    }

    FILE *f = fopen(filename, "wb");
    int i;
    for (i = 0; i < response->body->len; i++) {
        fwrite(&response->body->data[i], sizeof(unsigned char), 1, f);
    }
    fclose(f);

    return filename;
}

char*
httpresponse_body_as_string(HttpResponse response)
{
    if (response->body) {
        char *body = strndup((char *)response->body->data, response->body->len);
        return body;
    } else {
        return NULL;
    }
}

GHashTable*
httpresponse_headers(HttpResponse response)
{
    return response->headers;
}