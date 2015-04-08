#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "httpclient/httpclient.h"

HttpClientError*
httperror_create(httpclient_err_t code, char *message)
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

void
http_error(char *prefix, httpclient_err_t err)
{
    GString *full_msg = g_string_new("");
    if (prefix) {
        g_string_append_printf(full_msg, "%s: ", prefix);
    }
    switch (err) {
        case URL_NO_SCHEME:
            g_string_append(full_msg, "no scheme.");
            break;
        case URL_INVALID_SCHEME:
            g_string_append(full_msg, "invalid scheme.");
            break;
        case URL_INVALID_PORT:
            g_string_append(full_msg, "invalid port.");
            break;
        case REQ_INVALID_METHOD:
            g_string_append(full_msg, "unsupported method.");
            break;
        case SOCK_CREATE_FAILED:
            g_string_append(full_msg, "failed to create socket.");
            break;
        case SOCK_CONNECT_FAILED:
            g_string_append(full_msg, "socket connect failed.");
            break;
        case SOCK_SEND_FAILED:
            g_string_append(full_msg, "socket send failed.");
            break;
        case SOCK_TIMEOUT:
            g_string_append(full_msg, "socket read timeout.");
            break;
        case SOCK_RECV_FAILED:
            g_string_append(full_msg, "socket read failed.");
            break;
        case HOST_LOOKUP_FAILED:
            g_string_append(full_msg, "host lookup failed.");
            break;
        case RESP_UPSUPPORTED_TRANSFER_ENCODING:
            g_string_append(full_msg, "Unsupported transfer encoding.");
            break;
        case RESP_ERROR_PARSING_CHUNK:
            g_string_append(full_msg, "Error parsing chunked body.");
            break;
        case GZIP_BUFFER_INSUFFICIENT:
            g_string_append(full_msg, "GZIP buffer insufficient.");
            break;
        case GZIP_INSUFFICIENT_MEMORY:
            g_string_append(full_msg, "GZIP failed due to insufficient memory.");
            break;
        case GZIP_CORRUPT:
            g_string_append(full_msg, "GZIP contents corrupt.");
            break;
        case GZIP_FAILED:
            g_string_append(full_msg, "GZIP inflate failed for unknown reason.");
            break;
        default:
            g_string_append(full_msg, "unknown.");
            break;
    }

    printf("%s\n", full_msg->str);
    g_string_free(full_msg, TRUE);
}