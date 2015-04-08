#ifndef _HTTPRESPONSE_H
#define _HTTPRESPONSE_H

struct httpresponse_t {
    HttpRequest request;
    int status;
    char *status_msg;
    GHashTable *headers;
    GByteArray *body;
    gboolean body_read;
};

HttpResponse httpresponse_create(HttpRequest request, HttpClientError **err);

#endif