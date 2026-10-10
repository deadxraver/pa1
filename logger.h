#ifndef LOGGER_H

#define LOGGER_H

#include <stdio.h>
#include "lib/pa1.h"

FILE* event_fd(void);

FILE* pipe_fd(void);

int init_logger(void);

void destroy_logger(void);

#ifdef DBG
#define LOG_DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG_DBG(...) do {} while(0)
#endif // DBG

#define LOG_EVENT(...) \
  do { \
    printf(__VA_ARGS__); \
    fprintf(event_fd(), __VA_ARGS__); \
  } while (0)

#define LOG_PIPE(...) \
  do { \
    printf(__VA_ARGS__); \
    fprintf(pipe_fd(), __VA_ARGS__); \
  } while (0)

#endif // !LOGGER_H
