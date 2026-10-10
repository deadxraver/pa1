#include "logger.h"

#include <fcntl.h>
#include "lib/common.h"
#include <errno.h>

static FILE* elogfd = NULL;
static FILE* plogfd = NULL;

int init_logger(void) {
  elogfd = fopen(events_log, "rw");
  plogfd = fopen(pipes_log, "rw");

  if (NULL == elogfd || NULL == plogfd)
    return errno;

  return 0;
}

void destroy_logger(void) {
  if (NULL == elogfd) {
    fclose(elogfd);
    elogfd = NULL;
  }
  if (NULL == plogfd) {
    fclose(plogfd);
    plogfd = NULL;
  }
}

FILE* event_fd(void) {
  return elogfd;
}

FILE* pipe_fd(void) {
  return plogfd;
}
