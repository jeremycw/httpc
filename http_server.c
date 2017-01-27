#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <ev.h>
#include <unistd.h>
#include "http_parser.h"
#include "recv_all.h"
#include "http_server.h"
#include "http_response.h"

#define BUFFER_SIZE 1024

typedef struct {
  http_parser_t parser;
  char* buffer;
  int offset;
  int buffer_size;
  int state;
  http_request_t req;
} connection_t;

#define DONE 0
#define HEADERS 1

static inline void end_request(http_server_t* server, connection_t* con, int socket) {
  http_response_t* response = malloc(sizeof(http_response_t));
  http_response_init(response, server->loop, socket);
  server->request_cb(&con->req, response);
  con->req.body = NULL;
  con->req.body_length = 0;
  con->req.header_count = 0;
}

/* Read client message */
void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  http_server_t* server = ev_userdata(loop);
  connection_t* con = (connection_t*)watcher->data;

  if (EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }

  printf("try: %d on %d\n", con->buffer_size, watcher->fd);
  int read = recv_all(watcher->fd, &con->buffer, con->offset, &con->buffer_size);
  printf("read: %d\n", read);

  if (read == 0) {
    //printf("closing\n");
    //close(watcher->fd);
    //perror("shutdown");
    ev_io_stop(loop, watcher);
    free(con->buffer);
    free(con);
    free(watcher);
    return;
  }

  http_parser_parse(&con->parser, con->buffer + con->offset, read);
  con->offset += read;

  http_parser_token_t* tokens = http_parser_tokens(&con->parser);

  for (int i = 0; i < con->parser.token_count; i++) {
    http_parser_token_t tok = tokens[i];
    switch (con->state) {
      case DONE:
        if (tok.type == hpt_REQUEST_LINE) {
          con->state = HEADERS;
          con->req.request_line = con->buffer + tok.index;
          con->req.request_line[tok.length] = '\0';
        } else {
          //ERROR
        }
        break;

      case HEADERS:
        switch (tok.type) {
        case hpt_REQUEST_LINE:
          con->req.request_line = con->buffer + tok.index;
          con->req.request_line[tok.length] = '\0';
          break;

        case hpt_BODY:
          con->req.body = con->buffer + tok.index;
          con->req.body_length = tok.length;
          break;

        case hpt_HEADER:
          con->req.headers[con->req.header_count] = con->buffer + tok.index;
          con->req.headers[con->req.header_count][tok.length] = '\0';
          con->req.header_count++;
          break;

        case hpt_END:
          end_request(server, con, watcher->fd);
          con->state = DONE;
          break;

        default:
          //ERROR
          break;
        }
        break;
    }
  }

  if (con->state == DONE) {
    printf("DONE\n");
    free(con->buffer);
    con->buffer = malloc(BUFFER_SIZE);
    con->buffer_size = BUFFER_SIZE;
    con->offset = 0;
    http_parser_init(&con->parser);
  }

}

/* Accept client requests */
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
  if (EV_ERROR & revents) {
    perror("got invalid event");
    return;
  }

  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_sd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);

  if (client_sd < 0) {
    perror("accept error");
    return;
  }

  struct ev_io *w_client = malloc(sizeof(struct ev_io));
  ev_io_init(w_client, read_cb, client_sd, EV_READ);
  ev_io_start(loop, w_client);
  connection_t* con = calloc(1, sizeof(connection_t));
  con->buffer_size = BUFFER_SIZE;
  con->buffer = malloc(BUFFER_SIZE);
  w_client->data = con;
}

int http_server_listen(http_server_t* server, void (*f)(http_request_t*, http_response_t*), int port) {
  server->loop = ev_default_loop(0);
  server->request_cb = f;
  struct sockaddr_in addr;
  struct ev_io w_accept;

  // Create server socket
  if ((server->socket = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("socket error");
    return -1;
  }

  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  // Bind socket to address
  if (bind(server->socket, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
    perror("bind error");
  }

  // Start listing on the socket
  if (listen(server->socket, 2) < 0) {
    perror("listen error");
    return -1;
  }

  // Initialize and start a watcher to accepts client requests
  ev_io_init(&w_accept, accept_cb, server->socket, EV_READ);
  ev_io_start(server->loop, &w_accept);
  ev_set_userdata(server->loop, server);

  // Start infinite loop
  while (1) {
    ev_loop(server->loop, 0);
  }
}

