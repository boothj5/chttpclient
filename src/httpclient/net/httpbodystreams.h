#ifndef _HTTPBODYSTREAMS_H
#define _HTTPBODYSTREAMS_H

GByteArray* httpbodystream_len(int socket, int len, HttpClientError **err);
GByteArray* httpbodystream_chunked(int socket, HttpClientError **err);

#endif