#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>

#ifdef DBG
#define LOG_DBG(fmt, ...) \
  fprintf(stderr, fmt, __VA_ARGS__);
#else
#define LOG_DBG(fmt, ...)
#endif

#endif // !__MAIN_H
