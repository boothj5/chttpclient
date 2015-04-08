#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "httpclient/httpclient.h"

HttpClientError*
httperror_create(httpclient_errcode_t code, char *message)
{
    HttpClientError *result = malloc(sizeof(HttpClientError));
    result->code = code;
    result->message = strdup(message);

    return result;
}

void
httperror_destroy(HttpClientError *error)
{
    if (error) {
        free(error->message);
        free(error);
    }
}

char *
httperror_getmessage(HttpClientError *err, char *prefix)
{
    if (!prefix) {
        return strdup(err->message);
    } else {
        GString *message = g_string_new(prefix);
        g_string_append(message, ": ");
        g_string_append(message, err->message);

        char *result = message->str;

        g_string_free(message, FALSE);

        return result;
    }
}