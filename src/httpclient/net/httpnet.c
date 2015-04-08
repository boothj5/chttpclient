#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include "httpclient/httpclient.h"
#include "httpclient/httprequest.h"
#include "httpclient/httpresponse.h"
#include "httpclient/httputil.h"
#include "httpclient/net/httpbodystreams.h"
#include "httpclient/httperr.h"

void
httpnet_connect(HttpContext context, HttpClientError **err)
{
    struct hostent *he = gethostbyname(context->host);
    if (he == NULL) {
        *err = httperror_create(HOST_LOOKUP_FAILED, "Host lookup failed.");
        return;
    }

    if (context->debug) printf("\nHost %s resolved to:\n", context->host);
    struct in_addr **addr_list = (struct in_addr**)he->h_addr_list;
    char *ip = NULL;
    int i;
    for (i = 0; addr_list[i] != NULL; i++) {
        if (context->debug) printf("  %s\n", inet_ntoa(*addr_list[i]));
        if (i == 0) {
            ip = strdup(inet_ntoa(*addr_list[i]));
        }
    }
    if (context->debug) printf("Connecting to %s:%d...\n", ip, context->port);

    context->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (context->socket == -1) {
        *err = httperror_create(SOCK_CREATE_FAILED, "Failed to create socket.");
        return;
    }

    struct timeval tv;
    if (context->read_timeout_ms >= 1000) {
        tv.tv_sec = context->read_timeout_ms / 1000;
        tv.tv_usec = (context->read_timeout_ms % 1000) * 1000;
    } else {
        tv.tv_sec = 0;
        tv.tv_usec = context->read_timeout_ms * 1000;
    }
    setsockopt(context->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET; // ipv4
    server.sin_port = htons(context->port); // host to network byte order

    if (connect(context->socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        *err = httperror_create(SOCK_CONNECT_FAILED, "Socket connect failed");
        return;
    }
    if (context->debug) printf("Connected successfully.\n");
}

void
httpnet_send(HttpRequest request, HttpClientError **err)
{
    GString *req = g_string_new("");
    g_string_append(req, "GET ");
    g_string_append(req, request->resource);
    g_string_append(req, " HTTP/1.1");
    g_string_append(req, "\r\n");

    GList *header_keys = g_hash_table_get_keys(request->headers);
    GList *header = header_keys;
    while (header) {
        char *key = header->data;
        char *val = g_hash_table_lookup(request->headers, key);
        g_string_append_printf(req, "%s: %s", key, val);
        g_string_append(req, "\r\n");
        header = g_list_next(header);
    }
    g_list_free(header_keys);

    g_string_append(req, "\r\n");

    if (request->context->debug) printf("\n---REQUEST START---\n%s---REQUEST END---\n", req->str);

    int sent = 0;
    while (sent < req->len) {
        int res = send(request->context->socket, req->str+sent, req->len-sent, 0);
        if (res == -1) {
            *err = httperror_create(SOCK_SEND_FAILED, "Socket send failed.");
            g_string_free(req, TRUE);
            return;
        }
        sent += res;
    }

    g_string_free(req, TRUE);
}

void
httpnet_read_headers(HttpResponse response, HttpClientError **err)
{
    char header_buf[2];
    memset(header_buf, 0, sizeof(header_buf));
    int res;

    gboolean headers_read = FALSE;
    GString *header_stream = g_string_new("");
    while (!headers_read && ((res = recv(response->request->context->socket, header_buf, 1, 0)) > 0)) {
        g_string_append_len(header_stream, header_buf, res);
        if (g_str_has_suffix(header_stream->str, "\r\n\r\n")) headers_read = TRUE;
        memset(header_buf, 0, sizeof(header_buf));
    }

    if (res < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) *err = httperror_create(SOCK_TIMEOUT, "Socket read timeout.");
        else *err = httperror_create(SOCK_RECV_FAILED, "Socket read failed.");
        return;
    }

    int status = 0;
    char *status_msg = NULL;
    GHashTable *headers_ht = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    if (header_stream->len > 0) {
        gchar **lines = g_strsplit(header_stream->str, "\r\n", -1);

        // protocol line
        char *proto_line = lines[0];
        if (response->request->context->debug) printf("\nPROTO LINE: %s\n\n", proto_line);
        gchar **proto_chunks = g_strsplit(proto_line, " ", 3);

        status = (int) strtol(proto_chunks[1], NULL, 10);
        if (proto_chunks[2]) {
            status_msg = strdup(proto_chunks[2]);
        }

        g_strfreev(proto_chunks);

        // headers
        int count = 1;
        while (count < g_strv_length(lines)) {
            gchar **header_chunks = g_strsplit(lines[count++], ":", 2);
            if (header_chunks[0]) {
                char *header_key = strdup(header_chunks[0]);
                char *header_val = strdup(header_chunks[1]);
                g_strstrip(header_key);
                g_strstrip(header_val);
                g_hash_table_replace(headers_ht, header_key, header_val);
                if (response->request->context->debug) printf("HEADER: %s: %s\n", header_key, header_val);
            }
        }
    }
    g_string_free(header_stream, TRUE);

    response->status = status;
    response->status_msg = status_msg;
    response->headers = headers_ht;
}

void
httpnet_read_body(HttpResponse response, HttpClientError **err)
{
    GByteArray *body_contents = NULL;
    int socket = response->request->context->socket;
    response->body = NULL;

    // read full
    if (httpresponse_header_exists(response, HTTPHKEY_CONTENT_LENGTH)) {
        char *content_length_str = g_hash_table_lookup(response->headers, HTTPHKEY_CONTENT_LENGTH);
        int content_length = (int) strtol(content_length_str, NULL, 10);
        body_contents = httpbodystream_len(socket, content_length, err);
        if (*err) {
            return;
        }

    // read chunked
    } else if (httpresponse_header_equals(response, HTTPHKEY_TRANSFER_ENCODING, HTTPHVAL_CHUNKED)) {
        body_contents = httpbodystream_chunked(socket, err);
        if (*err) {
            return;
        }
    }

    if (body_contents) {
        // gzipped
        if (httpresponse_header_equals(response, HTTPHKEY_CONTENT_ENCODING, HTTPHVAL_GZIP)) {
            response->body = httputil_gzip_inflate(body_contents);
            g_byte_array_free(body_contents, TRUE);

        // normal
        } else {
            response->body = body_contents;
        }
    }

    response->body_read = TRUE;
}

void
httpnet_close(HttpContext context)
{
    if (context->socket != -1) {
        close(context->socket);
        context->socket = -1;
    }
}