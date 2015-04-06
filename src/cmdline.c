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

    httpclient_err_t r_err;
    HttpUrl *url = httputil_url_parse(arg_url, &r_err);
    if (!url) {
        http_error("Error parsing URL", r_err);
        return 1;
    }

    GString *host = g_string_new("");
    g_string_append_printf(host, "%s://%s", url->scheme, url->host);
    if (url->port != 80) {
        g_string_append_printf(host, ":%d", url->port);
    }

    HttpContext ctx = httpcontext_create(host->str, &r_err);
    if (!ctx) {
        http_error("Error creating context", r_err);
        return 1;
    }

    httpcontext_debug(ctx, FALSE);
    httpcontext_read_timeout(ctx, 1000);

    g_string_free(host, TRUE);

    HttpRequest request = httprequest_create(ctx, url->resource, "GET", &r_err);
    if (!request) {
        http_error("Error creating request", r_err);
        return 1;
    }

    httprequest_addheader(request, "User-Agent", "HTTPCLIENT/1.0");
    httprequest_addheader(request, "Accept-Encoding", "gzip");

    HttpResponse response = httprequest_perform(request, &r_err);
    if (!response) {
        http_error("Error performing request", r_err);
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

    char *body = httpresponse_body_as_string(response);
    if (body) {
        printf("Body:\n%s\n", body);
        free(body);
    }

    httputil_url_destroy(url);

    return 0;
}
