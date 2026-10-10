#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>

#include "main.h"

#include "lib/pa1.h"
#include "proc_logic.h"

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

  if ((errcode = init(processes)) != 0)
    goto end;

  register_pid(0, getpid());
  for (size_t i = 1; i <= processes; ++i) {
    int p = fork();
    if (p == 0) {
      Message msg;
      char* text = "Hello!";
      init_proc((local_id)i);
      // TODO: TIMESTAMPS!!
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
        receive(NULL, i, &msg);
        size_t sz = msg.s_header.s_payload_len;
        char* message = (char*)malloc(sizeof(char) * sz + 1);
        if (message == NULL) {
          errcode = errno;
          goto end;
        }
        message[sz] = 0;
        memcpy(message, msg.s_payload, sz);
        printf("%s\n", message);
        free(message);
        message = NULL;
      }
      // TODO:
      //       send FINISH
      //       receive FINISH
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

end:
  if (errcode != 0 && get_lid() == 0)
    fprintf(stderr, "%s\n", strerror(errcode));
  cleanup();
  return errcode;
}
