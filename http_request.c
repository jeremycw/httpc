#include <stdlib.h>
#include <string.h>
#include "http_request.h"

void http_request_free(http_request_t* request) {
  free(request->request_line);
  if (request->body) {
    free(request->body);
  }
  for (int i = 0; i < request->header_count; i++) {
    free(request->headers[i]);
  }
  memset(request, 0, sizeof(http_request_t));
}

//time_t http_request_if_modified_since(http_request_t* request) {
//}

