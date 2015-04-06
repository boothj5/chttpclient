#ifndef _HTTPRESPONSE_H
#define _HTTPRESPONSE_H

struct httpresponse_t {
    HttpRequest request;
    char *proto;
    int status;
    char *status_msg;
    GHashTable *headers;
    GByteArray *body;
};

#endif