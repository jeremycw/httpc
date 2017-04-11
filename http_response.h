#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

typedef struct {
  int dill_handle;
  int status_code;
  const char* content;
  int content_length;
} http_response_t;

void http_response_send(http_response_t* response);
void http_response_init(http_response_t* response, int socket);
void http_response_set_status(http_response_t* response, int status_code);
void http_response_set_content(http_response_t* response, const char* content, int content_length);
void http_response_send_file(http_response_t* response, const char* path);

#endif
