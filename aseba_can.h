#ifndef ASEBA_CAN_H
#define ASEBA_CAN_H

#include <stdint.h>

// msg is the message, len is the number of 16bit words
// returns 0 on success
int aseba_can_send(const uint16_t *msg, int len, int id);

// the message is written to msg, returns the length in 16bit words
int aseba_can_receive(uint16_t *msg);

void aseba_can_init(void);

#endif /* ASEBA_CAN_H */


