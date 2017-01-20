#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include "http_server.h"

#define RES "<h1>Testing</h1>"

void http_request_cb(http_request_t* req, http_response_t* res) {
  (void)req;
  //http_response_set_content(res, RES, sizeof(RES)-1);
  //http_response_send(res);
  http_response_send_file(res, "./http_server.c");
}

int main() {
  http_server_t server;
  http_server_listen(&server, http_request_cb, 3033);
}

