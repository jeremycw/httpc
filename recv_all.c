#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

int recv_all(int sock, char** buffer, int offset, int* buffer_size) {
  //copy bytes from socket
  char* buf = *buffer + offset;
  int bytes_total = 0;
  int bytes;
  int size_remaining = *buffer_size - offset;
  do {
    if (bytes_total == size_remaining) {
      size_remaining = *buffer_size;
      *buffer_size *= 2;
      *buffer = realloc(*buffer, *buffer_size);
      buf = *buffer + offset;
    }
    bytes = recv(sock, buf + bytes_total, size_remaining, MSG_DONTWAIT);
    if (bytes > 0) bytes_total += bytes;
  } while (bytes > 0);
  return bytes_total;
}

int read_all(int fd, char** buffer, int offset, int* buffer_size) {
  //copy bytes from file
  char* buf = *buffer + offset;
  int bytes_total = 0;
  int bytes;
  int size_remaining = *buffer_size - offset;
  do {
    if (bytes_total == size_remaining) {
      size_remaining = *buffer_size;
      *buffer_size *= 2;
      *buffer = realloc(*buffer, *buffer_size);
      buf = *buffer + offset;
    }
    bytes = read(fd, buf + bytes_total, size_remaining);
    if (bytes > 0) bytes_total += bytes;
  } while (bytes > 0);
  return bytes_total;
}

