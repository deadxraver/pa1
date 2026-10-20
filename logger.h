#ifndef LOGGER_H

#define LOGGER_H

#include <fcntl.h>
#include <stdio.h>

#include "lib/common.h"
#include "lib/pa1.h"

#ifdef DBG
#define LOG_DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG_DBG(...) do {} while(0)
#endif // DBG

#define CL_EVENT() \
  do { \
    FILE* fd = fopen(events_log, "w+"); \
    fclose(fd); \
  } while (0)

#define CL_PIPE() \
  do { \
    FILE* fd = fopen(pipes_log, "w+"); \
    fclose(fd); \
  } while (0)

#define LOG_EVENT(...) \
  do { \
    FILE* fd = fopen(events_log, "a+"); \
    printf(__VA_ARGS__); \
    fprintf(fd, __VA_ARGS__); \
    fclose(fd); \
  } while (0)

#define LOG_PIPE(...) \
  do { \
    FILE* fd = fopen(pipes_log, "a+"); \
    printf(__VA_ARGS__); \
    fprintf(fd, __VA_ARGS__); \
    fclose(fd); \
  } while (0)

#endif // !LOGGER_H
