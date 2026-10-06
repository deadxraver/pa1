#include "proc_logic.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>

static int processes = 0;
static int* pids = NULL;
static struct fd_pair* pipes = NULL;
static local_id lid_ = 0;

int init(int number_of_children) {
  int errcode = 0;

  pids = (int*)malloc(sizeof(*pids) * (number_of_children + 1));
  if (NULL == pids) {
    errcode = errno;
    goto end;
  }
  memset(pids, -1, sizeof(*pids) * (number_of_children + 1));

  pipes = (struct fd_pair*)malloc(sizeof(*pipes) * (number_of_children + 1));
  if (NULL == pipes) {
    errcode = errno;
    goto end;
  }

  memset(pipes, -1, sizeof(*pipes) * (number_of_children + 1));
  for (size_t i = 0; i <= number_of_children; ++i) {
    if (pipe(pipes[i].fd)) {
      errcode = errno;
      goto end;
    }
  }

end:
  if (errcode != 0)
    cleanup();
  return errcode;
}

void cleanup(void) {
  if (pids) {
    if (lid_ == 0) {
      for (size_t i = 1; i <= processes; ++i) {
        if (pids[i] >= 0) {
          LOG_DBG("%d: waiting for pid %d (%d)\n", lid_, pids[i], (int)i);
          waitpid(pids[i], NULL, 0);
          pids[i] = -1;
        }
      }
    }
    free(pids);
    pids = NULL;
  }
  if (pipes) {
    for (size_t i = 0; i <= processes; ++i) {
      if (pipes[i].fd[0] >= 0) {
        close(pipes[i].fd[0]);
        pipes[i].fd[0] = -1;
      }
      if (pipes[i].fd[1] >= 0) {
        close(pipes[i].fd[1]);
        pipes[i].fd[1] = -1;
      }
    }
    free(pipes);
    pipes = NULL;
  }
}

int register_pid(local_id lid, int pid) {
  pids[lid] = pid;
  return 0;
}

void set_lid(local_id lid) {
  lid_ = lid;
}

local_id get_lid(void) {
  return lid_;
}
