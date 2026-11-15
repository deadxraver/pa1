#ifndef __PROC_LOGIC_H
#define __PROC_LOGIC_H

#include <stdbool.h>

#include "lib/ipc.h"
#include "lib/banking.h"

struct fd_pair {
  int fd[2];
};

int init(int number_of_processes);
void cleanup(void);

int register_pid(local_id lid, int pid);

void init_proc(local_id lid, int account);

local_id get_lid(void);

int get_money(void);

bool add_money(int amount);

void send_history(void);

void close_unused_pipes(local_id lid);

#endif // !__PROC_LOGIC_H
