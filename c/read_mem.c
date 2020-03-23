#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  int fd = open("/dev/timestamps", O_RDONLY | O_SYNC);
  unsigned int *pointer = calloc(2300*2, sizeof(unsigned int));
  ssize_t unread = read(fd, pointer, 2300*2*4);
  for (int i = 0; i < 100; ++i)  {
  	printf("%8u\t%8u\n", pointer[i*2], pointer[i*2 + 1]);
  }
  free(pointer);
  close(fd);
}