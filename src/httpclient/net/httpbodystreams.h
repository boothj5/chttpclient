#ifndef _HTTPBODYSTREAMS_H
#define _HTTPBODYSTREAMS_H

GByteArray* httpbodystream_len(int socket, int len, httpclient_err_t *err);
GByteArray* httpbodystream_chunked(int socket, httpclient_err_t *err);

#endif