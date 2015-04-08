#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include "httpclient/httpclient.h"
#include "httpclient/httprequest.h"
#include "httpclient/httpresponse.h"
#include "httpclient/net/httpnet.h"

HttpResponse
httpresponse_create(HttpRequest request, HttpClientError **err)
{
    HttpResponse response = malloc(sizeof(struct httpresponse_t));
    response->request = httprequest_ref(request);
    response->status_msg = NULL;
    response->headers = NULL;
    response->body = NULL;
    response->body_read = FALSE;

    // read headers
    httpnet_read_headers(response, err);
    if (*err) {
        httpresponse_destroy(response);
        return NULL;
    }

    return response;
}

void
httpresponse_destroy(HttpResponse response)
{
    if (response) {
        // close socket
        httpnet_close(response->request->context);

        free(response->status_msg);
        if (response->headers) g_hash_table_destroy(response->headers);
        if (response->body) g_byte_array_free(response->body, TRUE);
        httprequest_unref(response->request);
        free(response);
    }
}

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
httpresponse_body_to_file(HttpResponse response, HttpClientError **err)
{
    if (!response->body_read) {
        httpnet_read_body(response, err);
        if (*err) {
            return NULL;
        }
    }

    char *last_slash = g_strrstr(response->request->resource, "/");

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
httpresponse_body_as_string(HttpResponse response, HttpClientError **err)
{
    if (!response->body_read) {
        httpnet_read_body(response, err);
        if (*err) {
            return NULL;
        }
    }

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

gboolean
httpresponse_header_equals(HttpResponse response, char *key, char *val)
{
    if (!key) {
        return FALSE;
    }

    if (!val) {
        return FALSE;
    }

    char *header_val = g_hash_table_lookup(response->headers, key);
    return (g_strcmp0(header_val, val) == 0);
}

gboolean
httpresponse_header_exists(HttpResponse response, char *key)
{
    if (!key) {
        return FALSE;
    }

    char *header_val = g_hash_table_lookup(response->headers, key);
    return (header_val != NULL);
}