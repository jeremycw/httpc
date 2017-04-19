#include <unistd.h>
#include <sys/mman.h>
#include <libdill.h>
#include <stdio.h>
#include <fcntl.h>

void handle_error(int rc, const char* msg) {
  if (rc < 0) {
    perror(msg);
    abort();
  }
}

void read_file(int s) {
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

int main() {
  int s = ipc_connect("/tmp/httpc.ipc", -1);
  while (1) {
    read_file(s);
  }
}

