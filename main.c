#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>

#include "lib/pa1.h"
#include "proc_logic.h"
#include "logger.h"

int main(int argc, char* argv[]) {
  int processes;
  int errcode = 0;
  if (argc != 3 || strcmp("-p", argv[1]) || sscanf(argv[2], "%d", &processes) != 1) {
    fprintf(stderr, "Wrong format, expected: %s -p <process count>\n", argv[0]);
    errcode = EINVAL;
    goto end;
  }
  if (processes <= 0 || processes > MAX_PROCESS_ID) {
    fprintf(stderr, "Number of processes should be positive and not "
            "greater than %d, got: %d\n", MAX_PROCESS_ID, processes);
    processes = 0;
    errcode = EINVAL;
    goto end;
  }

  if ((errcode = init(processes)) != 0)
    goto end;

  register_pid(0, getpid());
  for (size_t i = 1; i <= processes; ++i) {
    int p = fork();
    if (p == 0) {
      char text[128];
      Message msg;
      init_proc((local_id)i);
      close_unused_pipes(get_lid());
      snprintf(text, sizeof(text), log_started_fmt,
               get_lid(), getpid(), getppid());
      msg.s_header.s_magic = MESSAGE_MAGIC;
      msg.s_header.s_payload_len = strlen(text);
      msg.s_header.s_type = STARTED;
      memcpy(msg.s_payload, text, msg.s_header.s_payload_len);
      errcode = send_multicast(NULL, &msg);
      if (errcode)
        goto end;

      for (local_id i = 1; i <= processes; ++i) {
        if (i == get_lid())
          continue;

        errcode = receive(NULL, i, &msg);
        if (errcode)
          goto end;

        size_t sz = msg.s_header.s_payload_len;
        char* message = (char*)malloc(sizeof(char) * sz + 1);
        if (message == NULL) {
          errcode = errno;
          goto end;
        }
        message[sz] = 0;
        memcpy(message, msg.s_payload, sz);
        LOG_PIPE("%s\n", message);
        free(message);
        message = NULL;
      }

      LOG_EVENT(log_received_all_started_fmt, get_lid());

      // 'useful' work

      LOG_EVENT(log_done_fmt, get_lid());

      snprintf(text, sizeof(text), log_done_fmt, get_lid());
      msg.s_header.s_magic = MESSAGE_MAGIC;
      msg.s_header.s_payload_len = strlen(text);
      msg.s_header.s_type = DONE;
      memcpy(msg.s_payload, text, msg.s_header.s_payload_len);
      errcode = send_multicast(NULL, &msg);
      if (errcode)
        goto end;

      for (local_id i = 1; i <= processes; ++i) {
        if (i == get_lid())
          continue;

        errcode = receive(NULL, i, &msg);
        if (errcode)
          goto end;

        size_t sz = msg.s_header.s_payload_len;
        char* message = (char*)malloc(sizeof(char) * sz + 1);
        if (message == NULL) {
          errcode = errno;
          goto end;
        }
        message[sz] = 0;
        memcpy(message, msg.s_payload, sz);
        LOG_PIPE("%s\n", message);
        free(message);
        message = NULL;
      }

      LOG_EVENT(log_received_all_done_fmt, get_lid());
      goto end;
    }
    if (p < 0) {
      errcode = p;
      goto end;
    }
    if (p > 0) {
      register_pid(i, p);
    }
  }

  close_unused_pipes(PARENT_ID);

  if (get_lid() == PARENT_ID) {
    Message msg;
    for (size_t j = 0; j < 2; ++j) {
      for (size_t i = 1; i <= processes; ++i) {
        errcode = receive(NULL, (local_id)i, &msg);
        if (errcode)
          goto end;

        size_t sz = msg.s_header.s_payload_len;
        char* message = (char*)malloc(sizeof(char) * sz + 1);
        if (message == NULL) {
          errcode = errno;
          goto end;
        }
        message[sz] = 0;
        memcpy(message, msg.s_payload, sz);
        LOG_PIPE("%s\n", message);
        free(message);
        message = NULL;
      }
    }
  }

end:
  if (errcode != 0 && get_lid() == PARENT_ID)
    fprintf(stderr, "%s\n", strerror(errcode));
  cleanup();
  return errcode;
}
