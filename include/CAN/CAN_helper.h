#ifndef D_CANTest_H
#define D_CANTest_H

#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

#include "CAN_protocol.h"

extern uint8_t RxMessageData[8];
extern uint8_t TxMessageData[8];

void CAN_transmit(can_message_t*, uint16_t, uint8_t*);
#endif  /* D_CANTest_H */