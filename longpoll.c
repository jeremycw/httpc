#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "http_server.h"

void http_request_cb(http_request_t* req, http_response_t* res) {
  (void)req;
  http_response_send_file(res, "website.html");
}

int main() {
  http_server_t server;
  http_server_listen(&server, http_request_cb, 3033);
}

