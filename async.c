#include <libdill.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "async.h"

void handle_error(int rc, const char* msg) {
  if (rc < 0) {
    perror(msg);
    abort();
  }
}

coroutine void read_file(int s) {
  int bytes;
  handle_error(brecv(s, &bytes, sizeof(bytes), -1), "receiving path len");
  char* path = malloc(bytes);
  handle_error(brecv(s, path, bytes, -1), "receiving path");
  int fd = open(path, O_RDONLY);
  free(path);
  int len = lseek(fd, 0, SEEK_END);
  void *data = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
  handle_error(bsend(s, &len, sizeof(len), -1), "sending file len");
  handle_error(bsend(s, data, len, -1), "sending file");
  munmap(data, len);
  close(fd);
}

void async_init(async_ctx_t* ctx, int workers) {
  memset(ctx->busy, 0, sizeof(ctx->busy));
  //fork workers
  for (int i = 0; i < workers; i++) {
    handle_error(ipc_pair(ctx->handles[i]), "generating ipc pair");
    int pid = fork();
    if (pid == 0) {
      while (1) {
        read_file(ctx->handles[i][0]);
      }
    }
  }
}


int async_read_file(async_ctx_t* ctx, const char* path, char** ptr, int* len) {
  int i;
  while (1) {
    i = 0;
    while (ctx->busy[i]) i++;
    if (i == ctx->worker_count) {
      yield();
    } else {
      break;
    }
  }
  printf("acquired free worker\n");
  ctx->busy[i] = 1;
  int bytes = strlen(path)+1;
  int s = ctx->handles[i][1];
  handle_error(bsend(s, &bytes, sizeof(bytes), -1), "sending path len");
  handle_error(bsend(s, path, bytes, -1), "sending path");
  handle_error(brecv(s, &bytes, sizeof(bytes), -1), "receiving file len");
  *ptr = malloc(bytes);
  *len = bytes;
  handle_error(brecv(s, *ptr, bytes, -1), "receiving file");
  ctx->busy[i] = 0;
  printf("releasing worker\n");
  return 0;
}
