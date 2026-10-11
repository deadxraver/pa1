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

#include "logger.h"

static int processes = 0;
static int* pids = NULL;
static struct fd_pair** pipes = NULL;
static local_id lid_ = PARENT_ID;

int init(int number_of_children) {
  int errcode = 0;
  processes = number_of_children + 1;

  errcode = init_logger();
  if (errcode)
    goto end;

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
      if (i == j)
        continue;
      if (pipe2(pipes[i][j].fd, O_NONBLOCK)) {
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

void close_unused_pipes(local_id lid) {
  for (size_t i = 0; i < processes; ++i) {
    if (pipes[i] == NULL)
      continue;

    for (size_t j = 0; j < processes; ++j) {
      if (i == j)
        continue;

      if (lid != (local_id)i && pipes[i][j].fd[1] >= 0) {
        close(pipes[i][j].fd[1]);
        pipes[i][j].fd[1] = -1;
      }
      if (lid != (local_id)j && pipes[i][j].fd[0] >= 0) {
        close(pipes[i][j].fd[0]);
        pipes[i][j].fd[0] = -1;
      }
    }
  }
}

void cleanup(void) {
  if (pids) {
    LOG_DBG("cleaning up pids (0x%lx)\n", (unsigned long)pids);
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
    LOG_DBG("cleaning up pipes (0x%lx)\n", (unsigned long)pipes);
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

  LOG_DBG("All good, destroying logger...\n");
  destroy_logger();
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
  ssize_t read_bytes;

  do {
    read_bytes = read(fd, &msg->s_header, sz);
  } while (read_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));
  if (read_bytes < sz)
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

int receive_any(void * self, Message * msg) {
  (void)self;
  for (size_t i = 0; ; i = (i + 1) % processes) {
    if (i == (size_t)lid_)
      continue;
    int fd = pipes[i][lid_].fd[0];
    if (fd < 0)
      continue;
    size_t sz = sizeof(MessageHeader);
    ssize_t read_bytes = read(fd, &msg->s_header, sz);

    if (read_bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      continue;
    if (read_bytes < 0)
      return errno;

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
  return -1; // unreachable
}
