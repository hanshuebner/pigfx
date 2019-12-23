#include "postman.h"

#include "timer.h"
#include "hwutils.h"

#define MAPPED_REGISTERS_BASE 0x20000000

static volatile unsigned int* MAILBOX0READ =
  (unsigned int*)mem_p2v(MAPPED_REGISTERS_BASE + 0xB880);
static volatile unsigned int* MAILBOX0STATUS =
  (unsigned int*)mem_p2v(MAPPED_REGISTERS_BASE + 0xB898);
static volatile unsigned int* MAILBOX0WRITE =
  (unsigned int*)mem_p2v(MAPPED_REGISTERS_BASE + 0xB8A0);

POSTMAN_RETURN_TYPE
postman_recv(unsigned int channel, unsigned int* out_data)
{
  if (channel > 0xF) {
    return POSTMAN_BAD_DATA;
  }

  unsigned int n_skipped = 0;
  unsigned int start_time = time_microsec();

  while (n_skipped < MAILBOX_MAX_MSG_TO_SKIP) {
    // waits for mailbox being ready
    flushcache();
    while (*MAILBOX0STATUS & 0x40000000) // 30th bit is zero when ready
    {
      if (time_microsec() - start_time > MAILBOX_WAIT_TIMEOUT) {
        return POSTMAN_RECV_TIMEOUT;
      }
      flushcache();
    }

    // read the message
    dmb();
    unsigned int msg = *MAILBOX0READ;
    dmb();

    // check mailbox id
    if ((msg & 0xF) == (channel & 0xF)) {
      // mailbox channel ok, return the data
      *out_data = msg >> 4;
      return POSTMAN_SUCCESS;
    }

    if (time_microsec() - start_time > MAILBOX_WAIT_TIMEOUT) {
      return POSTMAN_RECV_TIMEOUT;
    }

    ++n_skipped;
  }

  return POSTMAN_TOO_MANY_MSG;
}

POSTMAN_RETURN_TYPE
postman_send(unsigned int channel, unsigned int data)
{
  if (data & 0xF) {
    // lowest 4-bits of data should be zero, aborting
    return POSTMAN_BAD_DATA;
  }

  // waits for mailbox being ready
  unsigned int start_time = time_microsec();
  while (*MAILBOX0STATUS & 0x80000000) // top bit is zero when ready
  {
    if (time_microsec() - start_time > MAILBOX_WAIT_TIMEOUT) {
      return POSTMAN_SEND_TIMEOUT;
    }
  }

  dmb();
  *MAILBOX0WRITE =
    data | channel; // lowest 4 bits for the mailbox, top 28 bits for the data

  return POSTMAN_SUCCESS;
}
