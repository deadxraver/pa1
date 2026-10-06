#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>

struct fd_pair {
  int fd[2];
};

#ifdef DBG
#define LOG_DBG(...) \
  fprintf(stderr, __VA_ARGS__);
#else
#define LOG_DBG(...)
#endif

#endif // !__MAIN_H
