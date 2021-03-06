#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libdill.h>
#include <aio.h>
#include <fcntl.h>
#include "http_response.h"

char* status_text[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  
  //100s
  "Continue", "Switching Protocols", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //200s
  "OK", "Created", "Accepted", "Non-Authoratative Information", "No Content",
  "Reset Content", "Partial Content", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //300s
  "Multiple Choices", "Moved Permanently", "Found", "See Other", "Not Modified",
  "Use Proxy", "", "Temporary Redirect", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //400s
  "Bad Request", "Unauthorized", "Payment Required", "Forbidden", "Not Found",
  "Method Not Allowed", "Not Acceptable", "Proxy Authentication Required",
  "Request Timeout", "Conflict",
  "Gone", "Length Required", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //500s
  "Internal Server Error", "Not Implemented", "Bad Gateway", "Service Unavailable",
  "Gateway Timeout", "", "", "", "", ""
};

#define HEADER_TEMPLATE "HTTP/1.1 %d %s\r\nDate: %.24s\r\nConnection: keep-alive\r\nContent-Length: %d\r\n\r\n"
#define RES_TEMPLATE HEADER_TEMPLATE "%.*s"

void http_response_send(http_response_t* response) {
  time_t rawtime;
  struct tm * timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  char* buf;

  int len = asprintf(
      &buf,
      RES_TEMPLATE,
      response->status_code,
      status_text[response->status_code],
      asctime(timeinfo),
      response->content_length,
      response->content_length,
      response->content);

  bsend(response->dill_handle, buf, len, -1);
  free(buf);
}

void http_response_init(http_response_t* response, int dill_handle) {
  memset(response, 0, sizeof(http_response_t));
  response->dill_handle = dill_handle;
  response->status_code = 200;
}

void http_response_set_status(http_response_t* response, int status_code) {
  response->status_code = status_code;
}

void http_response_set_content(http_response_t* response, const char* content, int content_length) {
  response->content = content;
  response->content_length = content_length;
}

int read_file(const char* path, char** ptr, int* len) {
  int fd = open(path, O_RDONLY);
  int chunk_size = 2048, err, ret;
  *ptr = NULL;
  *len = 0;
  struct aiocb aiocb;
  do {
    memset(&aiocb, 0, sizeof(aiocb));
    *ptr = realloc(*ptr, chunk_size + *len);
    aiocb.aio_fildes = fd;
    aiocb.aio_nbytes = chunk_size;
    aiocb.aio_buf = *ptr + *len;
    aiocb.aio_offset = *len;
    aio_read(&aiocb);
    while ((err = aio_error(&aiocb)) == EINPROGRESS) yield();
    ret = aio_return(&aiocb);
    *len += ret;
  } while (ret == chunk_size);
  close(fd);
  return 0;
}


void http_response_send_file(http_response_t* response, const char* path) {
  char* ptr;
  int len;
  read_file(path, &ptr, &len);
  http_response_set_content(response, ptr, len);
  http_response_send(response);
  free(ptr);
}
