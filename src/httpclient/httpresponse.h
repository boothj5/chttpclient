#ifndef _HTTPRESPONSE_H
#define _HTTPRESPONSE_H

struct httpresponse_t {
    HttpRequest request;
    int status;
    char *status_msg;
    GHashTable *headers;
    GByteArray *body;
};

HttpResponse httpresponse_create(HttpRequest request);

#endif