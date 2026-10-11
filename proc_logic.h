#ifndef __PROC_LOGIC_H
#define __PROC_LOGIC_H

#include "lib/ipc.h"

struct fd_pair {
  int fd[2];
};

int init(int number_of_processes);
void cleanup(void);

int register_pid(local_id lid, int pid);

void init_proc(local_id lid);

local_id get_lid(void);

void close_unused_pipes(local_id lid);

#endif // !__PROC_LOGIC_H
