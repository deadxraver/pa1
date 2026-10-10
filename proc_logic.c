#define _GNU_SOURCE

#include "proc_logic.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>

#include "lib/common.h"
#include "lib/pa1.h"

static int processes = 0;
static int* pids = NULL;
static struct fd_pair** pipes = NULL;
static local_id lid_ = 0;
static int elogfd = -1, plogfd = -1;

#ifdef DBG
#define LOG_DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG_DBG(...) do {} while(0)
#endif

#define LOG_EVENT(...) \
  do { \
    printf(__VA_ARGS__); \
    dprintf(elogfd, __VA_ARGS__); \
  } while (0)

#define LOG_PIPE(...) \
  do { \
    printf(__VA_ARGS__); \
    dprintf(plogfd, __VA_ARGS__); \
  } while (0)

int init(int number_of_children) {
  int errcode = 0;
  processes = number_of_children + 1;
  elogfd = open(events_log, O_RDWR | O_CREAT);
  plogfd = open(pipes_log, O_RDWR | O_CREAT);

  if (elogfd < 0 || plogfd < 0) {
    errcode = elogfd < 0 ? -elogfd : -plogfd;
    goto end;
  }

  pids = (int*)malloc(sizeof(*pids) * processes);
  if (NULL == pids) {
    errcode = errno;
    goto end;
  }
  memset(pids, -1, sizeof(*pids) * processes);

  pipes = (struct fd_pair**)malloc(sizeof(*pipes) * processes);
  if (NULL == pipes) {
    errcode = errno;
    goto end;
  }
  memset(pipes, 0, sizeof(*pipes) * processes);

  for (size_t i = 0; i < processes; ++i) {
    pipes[i] = (struct fd_pair*)malloc(sizeof(*pipes[i]) * processes);
    if (pipes[i] == NULL) {
      errcode = errno;
      goto end;
    }
    memset(pipes[i], -1, sizeof(*pipes[i]) * processes);
    for (size_t j = 0; j < processes; ++j) {
      if (pipe(pipes[i][j].fd)) {
        errcode = errno;
        goto end;
      }
    }
  }

end:
  if (errcode != 0)
    cleanup();
  return errcode;
}

void cleanup(void) {
  if (elogfd >= 0) {
    close(elogfd);
    elogfd = -1;
  }
  if (plogfd >= 0) {
    close(plogfd);
    plogfd = -1;
  }

  if (pids) {
    if (lid_ == 0) {
      for (size_t i = 1; i < processes; ++i) {
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
    for (size_t i = 0; i < processes; ++i) {
      if (pipes[i] == NULL)
        continue;

      for (size_t j = 0; j < processes; ++j) {
        if (pipes[i][j].fd[0] >= 0) {
          close(pipes[i][j].fd[0]);
          pipes[i][j].fd[0] = -1;
        }
        if (pipes[i][j].fd[1] >= 0) {
          close(pipes[i][j].fd[1]);
          pipes[i][j].fd[1] = -1;
        }
      }
      free(pipes[i]);
      pipes[i] = NULL;
    }
    free(pipes);
    pipes = NULL;
  }
}

int register_pid(local_id lid, int pid) {
  pids[lid] = pid;
  return 0;
}

void init_proc(local_id lid) {
  lid_ = lid;
  LOG_EVENT(log_started_fmt, lid_, getpid(), getppid());
}

local_id get_lid(void) {
  return lid_;
}

// ipc.h

int send(void* self, local_id dst, const Message* msg) {
  (void)self;
  int fd = pipes[lid_][dst].fd[1];
  size_t sz =
    sizeof(MessageHeader) + msg->s_header.s_payload_len;

  if (msg->s_header.s_magic != MESSAGE_MAGIC)
    return EINVAL;
  if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN)
    return E2BIG;
  if (msg->s_header.s_type > CS_RELEASE || msg->s_header.s_type < STARTED)
    return EINVAL;
  if (write(fd, msg, sz) < sz)
    return EIO;

  return 0;
}

int send_multicast(void* self, const Message* msg) {
  for (local_id i = 0; i < processes; ++i) {
    int ret;

    if (i == lid_)
      continue;

    ret = send(self, i, msg);
    if (ret)
      return ret;
  }

  return 0;
}

int receive(void* self, local_id from, Message* msg) {
  (void)self;
  int fd = pipes[from][lid_].fd[0];
  size_t sz = sizeof(MessageHeader);

  if (read(fd, &msg->s_header, sz) < sz)
    return EIO;
  if (msg->s_header.s_magic != MESSAGE_MAGIC)
    return EINVAL;
  if (msg->s_header.s_payload_len > MAX_PAYLOAD_LEN)
    return E2BIG;
  if (msg->s_header.s_type > CS_RELEASE || msg->s_header.s_type < STARTED)
    return EINVAL;

  sz = msg->s_header.s_payload_len;
  if (read(fd, msg->s_payload, sz) < sz)
    return EIO;

  return 0;
}
