#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "http_request.h"

void http_request_free(http_request_t* request) {
  free(request->request_line);
  if (request->body) {
    free(request->body);
  }
  for (int i = 0; i < request->header_count; i++) {
    free(request->headers[i]->key);
    free(request->headers[i]->value);
    free(request->headers[i]);
  }
  memset(request, 0, sizeof(http_request_t));
}

http_header_t* http_request_alloc_header(http_request_t* request) {
  http_header_t* h = malloc(sizeof(http_header_t));
  h->key = malloc(INITIAL_HEADER_KEY_BUF_SIZE);
  h->value = malloc(INITIAL_HEADER_VALUE_BUF_SIZE);
  h->ksize = INITIAL_HEADER_KEY_BUF_SIZE;
  h->vsize = INITIAL_HEADER_VALUE_BUF_SIZE;
  request->headers[request->header_count++] = h;
  return h;
}

time_t http_request_if_modified_since(http_request_t* request) {
  for (int i = 0; i < request->header_count; i++) {
    if (strcmp(request->headers[i]->key, "If-Modified-Since") == 0) {
      struct tm tm;
      strptime(request->headers[i]->value, "%a, %d %b %Y %T GMT", &tm);
      return mktime(&tm);
    }
  }
  return 0;
}

