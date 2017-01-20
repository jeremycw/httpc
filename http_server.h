#include "http_response.h"

typedef struct {
  char* request_line;
  char* headers[32];
  char* body;
  int body_length;
  int header_count;
} http_request_t;

typedef struct http_server_s {
  void (*request_cb)(http_request_t*, http_response_t*);
  struct ev_loop* loop;
  int socket;
} http_server_t;

int http_server_listen(http_server_t* server, void (*f)(http_request_t*, http_response_t*), int port);
