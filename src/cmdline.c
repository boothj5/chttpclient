#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include "httpclient/httpclient.h"

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("You must specify a URL\n");
        return 1;
    }

    char *arg_url = argv[1];

    HttpClientError *err = NULL;

    HttpUrl *url = httputil_url_parse(arg_url, &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    GString *host = g_string_new("");
    g_string_append_printf(host, "%s://%s", url->scheme, url->host);
    if (url->port != 80) {
        g_string_append_printf(host, ":%d", url->port);
    }

    HttpContext ctx = httpcontext_create(host->str, &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    httpcontext_debug(ctx, TRUE);
    httpcontext_read_timeout(ctx, 3000);

    g_string_free(host, TRUE);

    HttpRequest request = httprequest_create(ctx, url->resource, HTTPMETHOD_GET, &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    httprequest_addheader(request, HTTPHKEY_USER_AGENT, "HTTPCLIENT/1.0");
    httprequest_addheader(request, HTTPHKEY_ACCEPT_ENCODING, HTTPHVAL_GZIP);

    HttpResponse response = httprequest_perform(request, &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    printf("\n");

    int status = httpresponse_status(response);
    printf("Status code    : %d\n", status);

    char *status_message = httpresponse_status_message(response);
    if (status_message) {
        printf("Status message : %s\n", status_message);
    }
    printf("\n");

    GHashTable *headers = httpresponse_headers(response);
    GList *keys = g_hash_table_get_keys(headers);
    GList *curr = keys;
    if (curr) {
        printf("Headers:\n");
        while (curr) {
            printf("  %s: %s\n", (char *)curr->data, (char *)g_hash_table_lookup(headers, curr->data));
            curr = g_list_next(curr);
        }
        printf("\n");
    }

    char *body = httpresponse_body_as_string(response, &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    printf("Body:\n%s\n", body);
    free(body);

    httputil_url_destroy(url);

    httpresponse_destroy(response);
    httprequest_unref(request);
    httpcontext_unref(ctx);

    return 0;
}
