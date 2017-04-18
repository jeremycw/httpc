#ifndef ASYNC_H
#define ASYNC_H

#include <unistd.h>
#include <fcntl.h>

typedef struct {
  char busy[32];
  int handles[32][2];
  int worker_count;
} async_ctx_t;

void async_init(async_ctx_t* async_ctx, int workers);
int async_read_file(async_ctx_t* async_ctx, const char* path, char** ptr, int* len);

#endif
