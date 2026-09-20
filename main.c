#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char* argv[]) {
  int processes;
  if (argc != 3 || strcmp("-p", argv[1]) || sscanf(argv[2], "%d", &processes) != 1) {
    fprintf(stderr, "Wrong format, expected: %s -p <process count>\n", argv[0]);
    return EINVAL;
  }
  if (processes <= 0) {
    fprintf(stderr, "Number of processes should be positive, got: %d\n", processes);
    return EINVAL;
  }
  // TODO: 
  return 0;
}
