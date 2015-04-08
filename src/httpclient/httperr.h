#ifndef _HTTPERR_H
#define _HTTPERR_H

HttpClientError* httperror_create(httpclient_errcode_t code, char *message);

#endif