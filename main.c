#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>

#include "lib/pa2345.h"
#include "lib/banking.h"
#include "proc_logic.h"
#include "logger.h"

static void do_child_biz(void) {
  while (1) {
    Message msg;
    receive_any(NULL, &msg);
    if (STOP == msg.s_header.s_type)
      return;
    if (TRANSFER != msg.s_header.s_type)
      continue;
    TransferOrder to = *(TransferOrder*)msg.s_payload;
    if (get_lid() == to.s_src) {
      add_money(-to.s_amount);
      LOG_EVENT(log_transfer_out_fmt,
                msg.s_header.s_local_time,
                to.s_src, to.s_amount, to.s_dst);
      send(NULL, to.s_dst, &msg);
    }
    else if (get_lid() == to.s_dst) {
      add_money(to.s_amount);
      msg.s_header.s_payload_len = 0;
      msg.s_header.s_type = ACK;
      LOG_EVENT(log_transfer_in_fmt,
                msg.s_header.s_local_time,
                to.s_dst, to.s_amount, to.s_src);
      send(NULL, PARENT_ID, &msg);
    }
    else {
      // mistake
      continue;
    }
  }
}

int main(int argc, char* argv[]) {
  int processes;
  int errcode = 0;
  int* accounts = NULL;

  if (argc < 3 || strcmp("-p", argv[1]) || sscanf(argv[2], "%d", &processes) != 1) {
    fprintf(stderr, "Wrong format, expected: %s -p <process count> { X... }\n", argv[0]);
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
  if (argc - 3 != processes) {
    fprintf(stderr, "Expected %d accounts in args\n", processes);
    errcode = EINVAL;
    goto end;
  }

  accounts = (int*)malloc(processes * sizeof(int));
  if (NULL == accounts) {
    fprintf(stderr, "Failed to alloc mem for accounts array\n");
    errcode = ENOMEM;
    goto end;
  }

  for (size_t i = 0; i < processes; ++i) {
    if (sscanf(argv[i + 3], "%d", accounts + i) != 1 || accounts[i] <= 0) {
      fprintf(stderr, "Incorrect number format: %s\n", argv[i + 3]);
      errcode = EINVAL;
      goto end;
    }
  }

  if ((errcode = init(processes)) != 0)
    goto end;

  CL_PIPE();
  CL_EVENT();

  register_pid(0, getpid());
  for (size_t i = 1; i <= processes; ++i) {
    int p = fork();
    if (p == 0) {
      char text[128];
      Message msg;
      init_proc((local_id)i, accounts[i - 1]);
      close_unused_pipes(get_lid());
      msg.s_header.s_magic = MESSAGE_MAGIC;
      msg.s_header.s_type = STARTED;
      msg.s_header.s_local_time = get_physical_time();
      snprintf(text, sizeof(text), log_started_fmt,
               msg.s_header.s_local_time, get_lid(), getpid(), getppid(), accounts[i - 1]);
      msg.s_header.s_payload_len = strlen(text);
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

      LOG_EVENT(log_received_all_started_fmt, get_physical_time(), get_lid());

      do_child_biz();

      msg.s_header.s_magic = MESSAGE_MAGIC;
      msg.s_header.s_type = DONE;
      msg.s_header.s_local_time = get_physical_time();
      snprintf(text, sizeof(text), log_done_fmt, msg.s_header.s_local_time, get_lid(), get_money());
      msg.s_header.s_payload_len = strlen(text);
      memcpy(msg.s_payload, text, msg.s_header.s_payload_len);
      LOG_EVENT(log_done_fmt, msg.s_header.s_local_time, get_lid(), get_money());
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

      LOG_EVENT(log_received_all_done_fmt, get_physical_time(), get_lid());
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
  for (size_t i = 1; i <= processes; ++i) {
    Message msg;

    errcode = receive(NULL, (local_id)i, &msg);
    if (errcode)
      goto end;

    assert(STARTED == msg.s_header.s_type);

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

  bank_robbery(NULL, processes);
  Message msg;
  msg.s_header.s_magic = MESSAGE_MAGIC;
  msg.s_header.s_type = STOP;
  msg.s_header.s_local_time = get_physical_time();
  msg.s_header.s_payload_len = 0;
  send_multicast(NULL, &msg);

  if (get_lid() == PARENT_ID) {
    Message msg;
    for (size_t i = 1; i <= processes; ++i) {
      errcode = receive(NULL, (local_id)i, &msg);
      if (errcode)
        goto end;

      assert(DONE == msg.s_header.s_type);

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

      wait(&errcode);
      if (errcode)
        goto end;
    }
  }

end:
  if (errcode != 0 && get_lid() == PARENT_ID)
    fprintf(stderr, "%s\n", strerror(errcode));
  cleanup();

  if (accounts != NULL)
    free(accounts);
  accounts = NULL;

  return errcode;
}
