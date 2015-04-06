#include <stdio.h>

#include "httpclient/httpclient.h"

int main(void)
{
    httpclient_err_t r_err;

    HttpContext ctx = httpcontext_create("http://www.profanity.im", &r_err);
    if (!ctx) {
        http_error("Error creating context", r_err);
        return 1;
    }

    httpcontext_debug(ctx, TRUE);
    httpcontext_read_timeout(ctx, 3000);

    HttpRequest request = httprequest_create(ctx, "/reference.html", "GET", &r_err);
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

    int status = httpresponse_status(response);
    char *message = httpresponse_status_message(response);

    printf("Result: %d - %s\n", status, message);

    httpresponse_destroy(response);
    httprequest_unref(request);
    httpcontext_unref(ctx);

    return 0;
}