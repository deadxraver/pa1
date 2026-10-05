#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>

#include "main.h"

static int* pids = NULL;

int main(int argc, char* argv[]) {
  int processes;
  int errcode = 0;
  int id = 0;
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
  pids = (int*)malloc(sizeof(int) * processes);
  if (NULL == pids) {
    errcode = errno;
    fprintf(stderr, "Could not alloc mem for pids array: %s\n", strerror(errcode));
    goto end;
  }
  memset(pids, -1, sizeof(*pids) * (processes + 1));
  pids[0] = getpid();
  for (size_t i = 1; i <= processes; ++i) {
    int p = fork();
    if (p == 0) {
      id = (int)i;
      LOG_DBG("%d: starting...\n", id);
      // TODO: 
      break;
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
  if (pids) {
    if (id == 0) {
      for (size_t i = 1; i <= processes; ++i) {
        if (pids[i] >= 0) {
          LOG_DBG("%d: waiting for pid %d (%d)\n", id, pids[i], (int)i);
          waitpid(pids[i], NULL, 0);
          pids[i] = -1;
        }
      }
    }
    free(pids);
    pids = NULL;
  }
  LOG_DBG("%d exiting...\n", id);
  return errcode;
}
