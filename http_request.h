#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unistd.h>

#define INITIAL_HEADER_KEY_BUF_SIZE 32
#define INITIAL_HEADER_VALUE_BUF_SIZE 128

typedef struct {
  char* key;
  char* value;
  int ksize;
  int vsize;
} http_header_t;

typedef struct {
  char* request_line;
  http_header_t* headers[32];
  char* body;
  int body_length;
  int header_count;
} http_request_t;

time_t http_request_if_modified_since(http_request_t* request);
int http_request_connection_close(http_request_t* request);
void http_request_free(http_request_t* request);
http_header_t* http_request_alloc_header(http_request_t* request);

#endif
