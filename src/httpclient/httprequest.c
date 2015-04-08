#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <glib.h>

#include "httpclient/httprequest.h"
#include "httpclient/net/httpnet.h"
#include "httpclient/httpclient.h"
#include "httpclient/httpresponse.h"
#include "httpclient/httperr.h"

HttpRequest
httprequest_create(HttpContext context, char *resource, char *method, HttpClientError **err)
{
    if (g_strcmp0(method, HTTPMETHOD_GET) != 0) {
        *err = httperror_create(REQ_INVALID_METHOD, "Unsupported method.");
        return NULL;
    }

    HttpRequest request = malloc(sizeof(struct httprequest_t));
    request->context = httpcontext_ref(context);
    request->resource = strdup(resource);
    request->method = strdup(method);
    request->headers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    request->refcount = 1;

    GString *host_val = g_string_new("");
    g_string_append(host_val, request->context->host);

    if (request->context->port != 80) g_string_append_printf(host_val, ":%d", request->context->port);

    httprequest_addheader(request, HTTPHKEY_HOST, host_val->str);
    g_string_free(host_val, FALSE);

    return request;
}

void
httprequest_addheader(HttpRequest request, const char * const key, const char *const val)
{
    g_hash_table_replace(request->headers, strdup(key), strdup(val));
}

HttpResponse
httprequest_perform(HttpRequest request, HttpClientError **err)
{
    // connect socket
    httpnet_connect(request->context, err);
    if (*err) {
        return NULL;
    }

    // send request
    httpnet_send(request, err);
    if (*err) {
        return NULL;
    }

    // create response
    HttpResponse response = httpresponse_create(request, err);
    if (*err) {
        return NULL;
    }

    return response;
}

HttpRequest
httprequest_ref(HttpRequest request)
{
    request->refcount++;
    return request;
}

HttpRequest
httprequest_unref(HttpRequest request)
{
    if (request) {
        request->refcount--;
        if (request->refcount == 0) {
            free(request->resource);
            free(request->method);
            httpcontext_unref(request->context);
            g_hash_table_destroy(request->headers);
            free(request);
            return NULL;
        } else {
            return request;
        }
    } else {
        return NULL;
    }
}
