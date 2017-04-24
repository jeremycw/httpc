#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libdill.h>
#include <assert.h>
#include "http_server.h"
#include "http_response.h"

#define BUFFER_SIZE 256
#define CONTENT_LENGTH "Content-Length"

typedef struct {
  char c;
  int peeked;
  int s;
} parser_t;

char parser_next_char(parser_t* parser) {
  if (parser->peeked) {
    parser->peeked = 0;
    return parser->c;
  } else {
    char c;
    brecv(parser->s, &c, 1, now() + 60000);
    return c;
  }
}

char parser_peek(parser_t* parser) {
  brecv(parser->s, &parser->c, 1, -1);
  parser->peeked = 1;
  return parser->c;
}

void read_to_crlf(parser_t* parser, char* buf, int size) {
  char c;
  int i;
  errno = 0;
  for (i = 0; (c = parser_next_char(parser)) != '\r' && c != '\n'; i++) {
    if (errno != 0) return;
    if (i < size-1) buf[i] = c;
  }
  buf[i] = '\0';
  if (c == '\r' && parser_peek(parser) == '\n') {
    parser_next_char(parser);
  }
}

void parse_request_line(parser_t* parser, http_request_t* request) {
  request->request_line = malloc(BUFFER_SIZE);
  read_to_crlf(parser, request->request_line, BUFFER_SIZE);
  if (errno != 0) {
    free(request->request_line);
  }
}

#define eat_white_space() \
    do { \
      while ((c = parser_next_char(parser)) == ' ' || c == '\t'); \
    } while(0)

#define store_char(buf) \
  assert(i < BUFFER_SIZE); \
  buf[i++] = c

#define eat_separator() \
  do { \
    if (c == ':') { \
      eat_white_space(); \
    } else if (c == ' ' || c == '\t') { \
      eat_white_space(); \
      if (c == ':') { \
        eat_white_space(); \
      } \
    } \
  } while (0)

void parse_header(parser_t* parser, http_request_t* request) {
  int i;
  char c;
  http_header_t* h = malloc(sizeof(http_header_t));
  request->headers[request->header_count++] = h;
  for (i = 0; (c = parser_next_char(parser)) == CONTENT_LENGTH[i]; ) {
    store_char(h->key);
  }
  //If header is Content-Length
  if (i == (int)sizeof(CONTENT_LENGTH)-1) {
    h->key[i] = '\0';
    eat_separator();
    i = 0;
    //Get body length from Content-Length header
    do {
      store_char(h->value);
      request->body_length *= 10;
      request->body_length += c - '0';
    } while ((c = parser_next_char(parser)) >= '0' && c <= '9');
    store_char(h->value);
    h->value[i-1] = '\0';
    if (c != '\r' && c != '\n') {
      eat_white_space();
    }
    if (c == '\r' && parser_peek(parser) == '\n') {
      parser_next_char(parser);
    }
  //If not Content-Length just read in line
  } else {
    do {
      store_char(h->key);
    } while ((c = parser_next_char(parser)) != ' ' && c != '\t' && c != ':');
    h->key[i] = '\0';
    eat_separator();
    i = 0;
    store_char(h->value);
    read_to_crlf(parser, h->value+1, BUFFER_SIZE-i);
  }
}

coroutine void http_session(http_server_t* server, int s) {
  char c;
  http_request_t request;
  parser_t parser;
  parser.s = s;
  while (1) {
    parse_request_line(&parser, &request);
    if (errno != 0) return; //Usually because the other end closed the socket
    while ((c = parser_peek(&parser)) != '\r' && c != '\n') {
      parse_header(&parser, &request);
    }
    if (c == '\r' && parser_peek(&parser) == '\n') {
      parser_next_char(&parser);
    }
    //Read in the body of the request
    if (request.body_length > 0) {
      request.body = malloc(request.body_length + 1);
      brecv(s, request.body, request.body_length, -1);
      request.body[request.body_length] = '\0';
    }
    http_response_t response;
    http_response_init(&response, s);
    //Call the application callback
    server->request_cb(&request, &response);
    http_request_free(&request);
  }
}

int http_server_listen(http_server_t* server, void (*f)(http_request_t*, http_response_t*), int port) {
  server->request_cb = f;
  struct ipaddr addr;
  int rc = ipaddr_local(&addr, NULL, port, 0);
  assert(rc == 0);
  int ls = tcp_listen(&addr, 10);
  if (ls < 0) {
    perror("Can't open listening socket");
    return 1;
  }

  while (1) {
    int s = tcp_accept(ls, NULL, -1);
    if (s <= 0) {
      continue;
    }
    //Spin off coroutine for new http session
    go(http_session(server, s));
  }
}

