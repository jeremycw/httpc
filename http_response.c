#include <time.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ev.h>
#include <unistd.h>
#include <errno.h>
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

#define HEADER_TEMPLATE "HTTP/1.1 %d %s\r\nDate: %.24s\r\nContent-Length: %d\r\n\r\n"
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

  send(response->socket, buf, len, 0);
  free(buf);
  free(response);
}

void file_read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  http_response_t* response = (http_response_t*)watcher->data;
  if (EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }
  char buf[1024];
  int bytes = read(watcher->fd, buf, 1024);

  if (bytes == 0) {
    ev_io_stop(loop, watcher);
    free(watcher);
    free(response);
    return;
  }

  errno = 0;
  send(response->socket, buf, bytes, 0);
}

void http_response_send_file(http_response_t* response, const char* path) {
  int fd = open(path, O_RDONLY);
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  struct stat stat;
  fstat(fd, &stat);
  int bytes = stat.st_size;

  time_t rawtime;
  struct tm * timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  char* buf;
  int len = asprintf(
      &buf,
      HEADER_TEMPLATE,
      response->status_code,
      status_text[response->status_code],
      asctime(timeinfo),
      bytes);
  struct ev_io *w_client = malloc(sizeof(struct ev_io));
  w_client->data = response;
  ev_io_init(w_client, file_read_cb, fd, EV_READ);
  ev_io_start(response->loop, w_client);
  send(response->socket, buf, len, 0);
}

void http_response_init(http_response_t* response, struct ev_loop* loop,  int socket) {
  memset(response, sizeof(http_response_t), 0);
  response->socket = socket;
  response->status_code = 200;
  response->loop = loop;
}

void http_response_set_status(http_response_t* response, int status_code) {
  response->status_code = status_code;
}

void http_response_set_content(http_response_t* response, const char* content, int content_length) {
  response->content = content;
  response->content_length = content_length;
}
