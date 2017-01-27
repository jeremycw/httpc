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

  send(response->socket, buf, len, 0);
  free(buf);
  free(response);
}

void file_read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  (void)loop;
  http_response_t* response = (http_response_t*)watcher->data;
  if (EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }
  response->send_length = read(watcher->fd, response->send_buffer, 1024);
  response->state = SENDING_BODY;
  if (response->send_length < 0) {
    perror("file read error");
    printf("read buffer: %p\n", response->send_buffer);
  }
  printf("%d) read from file: %d\n", watcher->fd, response->send_length);

  if (response->send_length == 0) {
    ev_io_stop(loop, watcher);
    close(watcher->fd);
    free(watcher);
    return;
  }

}

void socket_write_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  (void)loop;
  http_response_t* response = (http_response_t*)watcher->data;
  if (EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }

  time_t rawtime;
  struct tm * timeinfo;
  printf("%d) WRITE READY: %d\n", watcher->fd, response->state);
next:
  switch (response->state) {
    case SENDING_HEADERS:
      time(&rawtime);
      timeinfo = localtime(&rawtime);
      char* buf;
      int len = asprintf(
          &buf,
          HEADER_TEMPLATE,
          response->status_code,
          status_text[response->status_code],
          asctime(timeinfo),
          response->content_length);
      printf("%s\n", buf);
      errno = 0;
      send(response->socket, buf, len, 0);
      free(buf);
      perror("send header error");
      response->send_buffer = malloc(1024);
      printf("allocated: %p\n", response->send_buffer);
      response->state = SENDING_BODY;
      break;

    case SENDING_BODY:
      errno = 0;
      send(response->socket, response->send_buffer, response->send_length, 0);
      response->sent_bytes += response->send_length;
      if (response->sent_bytes == response->content_length) {
        response->state = SENDING_CLEANUP;
        goto next;
      } else {
        printf("pausing after: %d/%d\n", response->sent_bytes, response->content_length);
        response->state = SENDING_PAUSED;
      }
      perror("send error");
      break;

    case SENDING_CLEANUP:
      printf("%d) releasing write watcher\n", watcher->fd);
      printf("freeing: %p\n", response->send_buffer);
      free(response->send_buffer);
      ev_io_stop(loop, watcher);
      close(watcher->fd);
      free(watcher);
      free(response);
      break;

    case SENDING_PAUSED:
      break;
  }
}

void http_response_send_file(http_response_t* response, const char* path) {
  int fd = open(path, O_RDONLY);
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  struct stat stat;
  fstat(fd, &stat);
  response->content_length = stat.st_size;

  struct ev_io *file_watcher = malloc(sizeof(struct ev_io));
  file_watcher->data = response;
  struct ev_io *sock_watcher = malloc(sizeof(struct ev_io));
  sock_watcher->data = response;
  ev_io_init(file_watcher, file_read_cb, fd, EV_READ);
  ev_io_start(response->loop, file_watcher);
  printf("registering watcher for %d\n", response->socket);
  ev_io_init(sock_watcher, socket_write_cb, response->socket, EV_WRITE);
  ev_io_start(response->loop, sock_watcher);
  response->state = SENDING_HEADERS;
}

void http_response_init(http_response_t* response, struct ev_loop* loop,  int socket) {
  memset(response, 0, sizeof(http_response_t));
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
