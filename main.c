#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  int processes;
  int errcode = 0;
  if (argc != 3 || strcmp("-p", argv[1]) || sscanf(argv[2], "%d", &processes) != 1) {
    fprintf(stderr, "Wrong format, expected: %s -p <process count>\n", argv[0]);
    errcode = EINVAL;
    goto end;
  }
  if (processes <= 0) {
    fprintf(stderr, "Number of processes should be positive, got: %d\n", processes);
    errcode = EINVAL;
    goto end;
  }
  int* pids = (int*)malloc(sizeof(int) * processes);
  if (NULL == pids) {
    fprintf(stderr, "Could not alloc mem for pids array\n");
    errcode = ENOMEM;
    goto end;
  }
  memset(pids, -1, sizeof(*pids) * processes);
  for (size_t i = 0; i < processes; ++i) {
    int p = fork();
    if (p == 0) {
      // TODO: idk some child proc stuff =)
    }
    if (p < 0) {
      fprintf(stderr, "Could not create process, %s\n", strerror(p));
      errcode = p;
      goto end;
    }
    if (p > 0) {
      pids[i] = p;
    }
  }
end:
  return errcode;
}
