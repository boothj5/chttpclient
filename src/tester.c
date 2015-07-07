#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include "httpclient/httpclient.h"

int
do_request(HttpRequest request, HttpResponse *response)
{
    HttpClientError *err = NULL;

    *response = httprequest_perform(request, &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    int status = httpresponse_status(*response);
    char *message = httpresponse_status_message(*response);

    printf("Result: %d - %s\n", status, message);

    GHashTable *headers = httpresponse_headers(*response);
    GList *keys = g_hash_table_get_keys(headers);
    GList *curr = keys;
    if (curr) {
        printf("Headers:\n");
        while (curr) {
            printf("  %s: %s\n", (char *)curr->data, (char *)g_hash_table_lookup(headers, curr->data));
            curr = g_list_next(curr);
        }
    }

    char *body = httpresponse_body_as_string(*response, &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    if (body) {
        printf("Body:\n");
        printf("%s\n", body);
    } else {
        printf("Body:\n");
        printf("(empty)\n");
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    HttpClientError *err = NULL;

    HttpContext ctx = httpcontext_create("http://127.0.0.1:7379", &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    httpcontext_debug(ctx, TRUE);
    httpcontext_read_timeout(ctx, 60000 * 10);

    HttpRequest request = httprequest_create(ctx, "/GET/hello", HTTPMETHOD_GET, &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

//    httprequest_addheader(request, "Connection", "close");

    HttpResponse response1;
    int res = do_request(request, &response1);
    if (res != 0) {
        printf("ERROR\n");
    }

    printf("Press enter for next request");
    getchar();

    HttpResponse response2;
    res = do_request(request, &response2);
    if (res != 0) {
        printf("ERROR\n");
    }

    httpresponse_destroy(response1);
    httpresponse_destroy(response2);
    httprequest_unref(request);
    httpcontext_unref(ctx);

    return 0;
}
