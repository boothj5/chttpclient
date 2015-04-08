#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>

#include <glib.h>

#include "httpclient/httpclient.h"
#include "httpclient/httperr.h"

GByteArray*
httpbodystream_len(int socket, int len, HttpClientError **err)
{
    if (len > 0) {
        GByteArray *body_stream = g_byte_array_new();
        int res = 0;
        int bufsize = BUFSIZ;
        unsigned char content_buf[bufsize+1];
        memset(content_buf, 0, sizeof(content_buf));

        int remaining = len;
        if (remaining < bufsize) bufsize = remaining;
        while (remaining > 0 && ((res = recv(socket, content_buf, bufsize, 0)) > 0)) {
            g_byte_array_append(body_stream, content_buf, res);
            remaining = len - body_stream->len;
            if (bufsize > remaining) bufsize = remaining;
            memset(content_buf, 0, sizeof(content_buf));
        }

        if (res < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) *err = httperror_create(SOCK_TIMEOUT, "Socket read timeout.");
            else *err = httperror_create(SOCK_RECV_FAILED, "Socket read failed.");
            return NULL;
        }

        return body_stream;
    } else {
        return NULL;
    }
}

GByteArray*
httpbodystream_chunked(int socket, HttpClientError **err)
{
    int len_res = 0;
    char len_content_buf[2];
    memset(len_content_buf, 0, sizeof(len_content_buf));
    GString *len_stream = g_string_new("");
    GByteArray *body_stream = g_byte_array_new();
    gboolean cont = TRUE;

    // read one byte at a time
    while (cont && ((len_res = recv(socket, len_content_buf, 1, 0)) > 0)) {
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
                *err = httperror_create(RESP_ERROR_PARSING_CHUNK, "Error parsing chunked content.");
                free(chunk_size_str);
                return NULL;
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
            while (ch_remaining > 0 && ((ch_res = recv(socket, ch_content_buf, ch_bufsize, 0)) > 0)) {
                ch_total += ch_res;
                g_byte_array_append(body_stream, ch_content_buf, ch_res);
                ch_remaining = chunk_size - ch_total;
                if (ch_bufsize > ch_remaining) ch_bufsize = ch_remaining;
                memset(ch_content_buf, 0, sizeof(ch_content_buf));
            }

            if (ch_res < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) *err = httperror_create(SOCK_TIMEOUT, "Socket read timeout.");
                else *err = httperror_create(SOCK_RECV_FAILED, "Socket read failed.");
                return NULL;
            }

            // skip terminating \r\n after chunk data
            int skip = 0;
            while (skip < 2 && ((ch_res = recv(socket, ch_content_buf, 1, 0)) > 0)) {
                skip++;
                memset(ch_content_buf, 0, sizeof(ch_content_buf));
            }

            if (ch_res < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) *err = httperror_create(SOCK_TIMEOUT, "Socket read timeout.");
                else *err = httperror_create(SOCK_RECV_FAILED, "Socket read failed.");
                return NULL;
            }

            // reset length stream
            g_string_free(len_stream, TRUE);
            len_stream = g_string_new("");
        }

        memset(len_content_buf, 0, sizeof(len_content_buf));
    }

    return body_stream;
}