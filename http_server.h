#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "http_response.h"
#include "http_request.h"

typedef struct http_server_s {
  void (*request_cb)(http_request_t*, http_response_t*);
} http_server_t;

int http_server_listen(http_server_t* server, void (*f)(http_request_t*, http_response_t*), int port);

#endif
