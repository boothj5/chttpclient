#include <stdio.h>
#include <stdlib.h>

#include "httpclient/httpclient.h"

int main(void)
{
    HttpClientError *err = NULL;

    HttpContext ctx = httpcontext_create("http://www.profanity.im", &err);
    if (err) {
        printf("%s\n", err->message);
        httperror_destroy(err);
        return 1;
    }

    httpcontext_debug(ctx, TRUE);
    httpcontext_read_timeout(ctx, 3000);

    HttpRequest request = httprequest_create(ctx, "/reference.html", HTTPMETHOD_GET, &err);
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

    int status = httpresponse_status(response);
    char *message = httpresponse_status_message(response);

    printf("Result: %d - %s\n", status, message);

    httpresponse_destroy(response);
    httprequest_unref(request);
    httpcontext_unref(ctx);

    return 0;
}