#include <libdill.h>
#include <stdio.h>
#include <string.h>
#include "async.h"

void handle_error(int rc, const char* msg) {
  if (rc < 0) {
    perror(msg);
    abort();
  }
}

void async_init(async_ctx_t* ctx, int workers) {
  remove("/tmp/httpc.ipc");
  int s = ipc_listen("/tmp/httpc.ipc", 10);
  memset(ctx->busy, 0, sizeof(ctx->busy));
  ctx->worker_count = workers;
  //fork workers
  for (int i = 0; i < workers; i++) {
    int pid = fork();
    if (pid == 0) {
      execl("./httpc-worker", "httpc-worker", NULL);
    } else {
      ctx->handles[i] = ipc_accept(s, -1);
    }
  }
}

int available_worker(async_ctx_t* ctx) {
  for (int i = 0; i < ctx->worker_count; i++) {
    if (!ctx->busy[i]) {
      return i;
    }
  }
  return -1;
}

int async_read_file(async_ctx_t* ctx, const char* path, char** ptr, int* len) {
  int index;
  while ((index = available_worker(ctx)) == -1) yield();
  ctx->busy[index] = 1;
  int bytes = strlen(path)+1;
  int s = ctx->handles[index];
  handle_error(bsend(s, &bytes, sizeof(bytes), -1), "sending path len");
  handle_error(bsend(s, path, bytes, -1), "sending path");
  handle_error(brecv(s, &bytes, sizeof(bytes), -1), "receiving file len");
  *ptr = malloc(bytes);
  *len = bytes;
  handle_error(brecv(s, *ptr, bytes, -1), "receiving file");
  ctx->busy[index] = 0;
  return 0;
}
