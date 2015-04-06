#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include <zlib.h>

#include "httpclient.h"
#include "httprequest.h"
#include "httpresponse.h"

int
httpnet_connect(HttpContext context, char *host, int port, request_err_t *err)
{
    struct hostent *he = gethostbyname(host);
    if (he == NULL) {
        *err = HOST_LOOKUP_FAILED;
        return -1;
    }

    if (context->debug) printf("\nHost %s resolved to:\n", host);
    struct in_addr **addr_list = (struct in_addr**)he->h_addr_list;
    char *ip = NULL;
    int i;
    for (i = 0; addr_list[i] != NULL; i++) {
        if (context->debug) printf("  %s\n", inet_ntoa(*addr_list[i]));
        if (i == 0) {
            ip = strdup(inet_ntoa(*addr_list[i]));
        }
    }
    if (context->debug) printf("Connecting to %s:%d...\n", ip, port);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        *err = SOCK_CREATE_FAILED;
        return -1;
    }

    struct timeval tv;
    if (context->read_timeout_ms >= 1000) {
        tv.tv_sec = context->read_timeout_ms / 1000;
        tv.tv_usec = (context->read_timeout_ms % 1000) * 1000;
    } else {
        tv.tv_sec = 0;
        tv.tv_usec = context->read_timeout_ms * 1000;
    }
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET; // ipv4
    server.sin_port = htons(port); // host to network byte order

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        *err = SOCK_CONNECT_FAILED;
        return -1;
    }
    if (context->debug) printf("Connected successfully.\n");

    return sock;
}

gboolean
httpnet_send(HttpContext context, int sock, HttpRequest request, request_err_t *err)
{
    GString *req = g_string_new("");
    g_string_append(req, "GET ");
    g_string_append(req, request->url->path);
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

    if (context->debug) printf("\n---REQUEST START---\n%s---REQUEST END---\n", req->str);

    int sent = 0;
    while (sent < req->len) {
        int res = send(sock, req->str+sent, req->len-sent, 0);
        if (res == -1) {
            *err = SOCK_SEND_FAILED;
            g_string_free(req, TRUE);
            return FALSE;
        }
        sent += res;
    }

    g_string_free(req, TRUE);
    return TRUE;
}

gboolean
httpnet_read_headers(HttpContext context, int sock, HttpResponse response, request_err_t *err)
{
    char header_buf[2];
    memset(header_buf, 0, sizeof(header_buf));
    int res;

    gboolean headers_read = FALSE;
    GString *header_stream = g_string_new("");
    while (!headers_read && ((res = recv(sock, header_buf, 1, 0)) > 0)) {
        g_string_append_len(header_stream, header_buf, res);
        if (g_str_has_suffix(header_stream->str, "\r\n\r\n")) headers_read = TRUE;
        memset(header_buf, 0, sizeof(header_buf));
    }

    if (res < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) *err = SOCK_TIMEOUT;
        else *err = SOCK_RECV_FAILED;
        return FALSE;
    }

    int status = 0;
    char *status_msg = NULL;
    GHashTable *headers_ht = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    if (header_stream->len > 0) {
        gchar **lines = g_strsplit(header_stream->str, "\r\n", -1);

        // protocol line
        char *proto_line = lines[0];
        if (context->debug) printf("\nPROTO LINE: %s\n", proto_line);
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
                if (context->debug) printf("HEADER: %s: %s\n", header_key, header_val);
            }
        }
    }
    g_string_free(header_stream, TRUE);

    response->status = status;
    response->status_msg = status_msg;
    response->headers = headers_ht;

    return TRUE;
}

gboolean
httpnet_read_body(HttpContext context, int sock, HttpResponse response, request_err_t *err)
{
    // Content-Encoding gzip and Content-Length provided
    if ((g_strcmp0(g_hash_table_lookup(response->headers, "Content-Encoding"), "gzip") == 0)
        && g_hash_table_lookup(response->headers, "Content-Length")) {

        int content_length = (int) strtol(g_hash_table_lookup(response->headers, "Content-Length"), NULL, 10);
        if (content_length > 0) {
            GByteArray *body_stream = g_byte_array_new();
            int res = 0;
            int bufsize = BUFSIZ;
            unsigned char content_buf[bufsize+1];
            memset(content_buf, 0, sizeof(content_buf));

            int remaining = content_length;
            if (remaining < bufsize) bufsize = remaining;
            while (remaining > 0 && ((res = recv(sock, content_buf, bufsize, 0)) > 0)) {
                g_byte_array_append(body_stream, content_buf, res);
                remaining = content_length - body_stream->len;
                if (bufsize > remaining) bufsize = remaining;
                memset(content_buf, 0, sizeof(content_buf));
            }

            if (res < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) *err = SOCK_TIMEOUT;
                else *err = SOCK_RECV_FAILED;
                return FALSE;
            }

            if (context->debug) printf("\nGZIPPED LEN: %d\n", body_stream->len);

            char inflated[500000];
            memset(inflated, 0, 500000);

            z_stream infstream;
            infstream.zalloc = Z_NULL;
            infstream.zfree = Z_NULL;
            infstream.opaque = Z_NULL;
            infstream.avail_in = (uInt)body_stream->len;
            infstream.next_in = (Bytef *)body_stream->data;
            infstream.avail_out = (uInt)sizeof(inflated);
            infstream.next_out = (Bytef *)inflated;

            inflateInit2(&infstream, 16+MAX_WBITS);
            inflate(&infstream, Z_NO_FLUSH);
            inflateEnd(&infstream);

            GByteArray *body = g_byte_array_new();
            g_byte_array_append(body, (unsigned char*)inflated, strlen(inflated) + 1);
            response->body = body;
            g_byte_array_free(body_stream, TRUE);
        } else {
            response->body = NULL;
        }

    // Content-Length header provided, attempt to read whole body
    } else if (g_hash_table_lookup(response->headers, "Content-Length")) {
        int content_length = (int) strtol(g_hash_table_lookup(response->headers, "Content-Length"), NULL, 10);
        if (content_length > 0) {
            GByteArray *body_stream = g_byte_array_new();
            int res = 0;
            int bufsize = BUFSIZ;
            unsigned char content_buf[bufsize+1];
            memset(content_buf, 0, sizeof(content_buf));

            int remaining = content_length;
            if (remaining < bufsize) bufsize = remaining;
            while (remaining > 0 && ((res = recv(sock, content_buf, bufsize, 0)) > 0)) {
                g_byte_array_append(body_stream, content_buf, res);
                remaining = content_length - body_stream->len;
                if (bufsize > remaining) bufsize = remaining;
                memset(content_buf, 0, sizeof(content_buf));
            }

            if (res < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) *err = SOCK_TIMEOUT;
                else *err = SOCK_RECV_FAILED;
                return FALSE;
            }

            response->body = body_stream;
        } else {
            response->body = NULL;
        }

    // Transfer-Encoding chunked, read chunks
    } else if (g_strcmp0(g_hash_table_lookup(response->headers, "Transfer-Encoding"), "chunked") == 0) {
        int len_res = 0;
        char len_content_buf[2];
        memset(len_content_buf, 0, sizeof(len_content_buf));
        GString *len_stream = g_string_new("");
        GByteArray *body_stream = g_byte_array_new();
        gboolean cont = TRUE;

        // read one byte at a time
        while (cont && ((len_res = recv(sock, len_content_buf, 1, 0)) > 0)) {
            g_string_append_len(len_stream, len_content_buf, len_res);

            // chunk size read
            if (g_str_has_suffix(len_stream->str, "\r\n")) {
                char *chunk_size_str = strndup(len_stream->str, len_stream->len - 2);

                // chunk size 0, finished
                if (g_strcmp0(chunk_size_str, "0") == 0) {
                    cont = FALSE;
                    continue;
                }

                char *end;
                errno = 0;
                int chunk_size = (int) strtol(chunk_size_str, &end, 16);
                if ((!(errno == 0 && chunk_size_str && !*end)) || (chunk_size < 1)) {
                    *err = RESP_ERROR_PARSING_CHUNK;
                    free(chunk_size_str);
                    return FALSE;
                }
                free(chunk_size_str);

                // handle chunk
                int ch_res = 0;
                int ch_total = 0;
                int ch_bufsize = chunk_size;
                unsigned char ch_content_buf[ch_bufsize+1];
                memset(ch_content_buf, 0, sizeof(ch_content_buf));
                int ch_remaining = chunk_size;

                // read chunk
                while (ch_remaining > 0 && ((ch_res = recv(sock, ch_content_buf, ch_bufsize, 0)) > 0)) {
                    ch_total += ch_res;
                    g_byte_array_append(body_stream, ch_content_buf, ch_res);
                    ch_remaining = chunk_size - ch_total;
                    if (ch_bufsize > ch_remaining) ch_bufsize = ch_remaining;
                    memset(ch_content_buf, 0, sizeof(ch_content_buf));
                }

                if (ch_res < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) *err = SOCK_TIMEOUT;
                    else *err = SOCK_RECV_FAILED;
                    return FALSE;
                }

                // skip terminating \r\n after chunk data
                int skip = 0;
                while (skip < 2 && ((ch_res = recv(sock, ch_content_buf, 1, 0)) > 0)) {
                    skip++;
                    memset(ch_content_buf, 0, sizeof(ch_content_buf));
                }

                if (ch_res < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) *err = SOCK_TIMEOUT;
                    else *err = SOCK_RECV_FAILED;
                    return FALSE;
                }

                // reset length stream
                g_string_free(len_stream, TRUE);
                len_stream = g_string_new("");
            }

            memset(len_content_buf, 0, sizeof(len_content_buf));
        }

        response->body = body_stream;

    } else {
        response->body = NULL;
    }

    return TRUE;
}
