#ifndef _HTTPERR_H
#define _HTTPERR_H

HttpClientError* httperror_create(httpclient_err_t code, char *message);

#endif