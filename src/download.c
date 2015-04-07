#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include "config.h"
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
    if (url->port > 0) {
        g_string_append_printf(host, ":%d", url->port);
    }

    HttpContext ctx = httpcontext_create(host->str, &r_err);
    if (!ctx) {
        http_error("Error creating context", r_err);
        return 1;
    }

    httpcontext_debug(ctx, TRUE);
    httpcontext_read_timeout(ctx, 3000);

    HttpRequest request = httprequest_create(ctx, url->resource, HTTPMETHOD_GET, &r_err);
    if (!request) {
        http_error("Error creating request", r_err);
        return 1;
    }

    httprequest_addheader(request, HTTPHKEY_USER_AGENT, "HTTPCLIENT/1.0");
    httprequest_addheader(request, HTTPHKEY_ACCEPT_ENCODING, HTTPHVAL_GZIP);

    HttpResponse response = httprequest_perform(request, &r_err);
    if (!response) {
        http_error("Error performing request", r_err);
        return 1;
    }

    printf("\n");

    int status = httpresponse_status(response);
    if (status == 200) {
        char *filename = httpresponse_body_to_file(response);
        printf("Saved to file: %s\n", filename);

        GHashTable *headers = httpresponse_headers(response);
        char *content_type = g_hash_table_lookup(headers, HTTPHKEY_CONTENT_TYPE);
        if (content_type && g_str_has_prefix(content_type, "image/"))
        {
#ifdef PLATFORM_OSX
            GString *command = g_string_new("open ");
#else
            GString *command = g_string_new("eog ");
#endif
            g_string_append(command, filename);
            g_string_append(command, " ");
            g_string_append(command, "> /dev/null 2>&1");

            int res = system(command->str);
            if (res == -1) printf("Failed to open image.\n");

            g_string_free(command, TRUE);
        }
    } else {
        printf("Failed to download.\n");
    }

    httputil_url_destroy(url);

    return 0;
}
