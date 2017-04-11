#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "http_server.h"

#define RES "<h1>Testing</h1>"

void http_request_cb(http_request_t* req, http_response_t* res) {
  (void)req;
  char* buf = malloc(256 * req->header_count);
  int index = 0;
  for (int i = 0; i < req->header_count; i++) {
    int len = strlen(req->headers[i]);
    memcpy(buf+index, req->headers[i], len);
    index += len+1;
    buf[index-1] = '\n';
  }
  http_response_set_content(res, buf, index);
  http_response_send(res);
  free(buf);
}

int main() {
  http_server_t server;
  http_server_listen(&server, http_request_cb, 3033);
}

