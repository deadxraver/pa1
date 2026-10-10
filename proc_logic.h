#ifndef __PROC_LOGIC_H
#define __PROC_LOGIC_H

#include "main.h"
#include "lib/ipc.h"

int init(int number_of_processes);
void cleanup(void);

int register_pid(local_id lid, int pid);

void init_proc(local_id lid);

local_id get_lid(void);

#endif // !__PROC_LOGIC_H
