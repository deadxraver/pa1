#include <assert.h>
#include <string.h>

#include "lib/banking.h"
#include "lib/ipc.h"

void transfer(void* parent_data, local_id src, local_id dst,
              balance_t amount)
{
  TransferOrder to;
  Message msg;
  to.s_src = src;
  to.s_dst = dst;
  to.s_amount = amount;
  msg.s_header.s_magic = MESSAGE_MAGIC;
  msg.s_header.s_payload_len = sizeof(to);
  msg.s_header.s_type = TRANSFER;
  msg.s_header.s_local_time = get_physical_time();
  memcpy(msg.s_payload, &to, sizeof(to));
  // TODO: 
  // * get acknoledge
  assert(send(parent_data, src, &msg) == 0);
  assert(receive(parent_data, src, &msg) == 0);
  assert(msg.s_header.s_type == ACK);
}

