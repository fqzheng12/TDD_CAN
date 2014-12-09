#ifndef __CAN_H
#define	__CAN_H

#include <string.h>
#include "stm32f10x.h"
#include "stm32f10x_can.h"

void CAN_GPIO_Config(uint32_t);
void CAN_NVIC_Config(void);
void CAN_Mode_Config(void);
void CAN_StdID_Filter_Config(uint8_t, uint8_t, uint8_t, uint16_t, uint16_t, uint8_t);
void CAN_ExtID_Filter_Config(uint8_t, uint8_t, uint8_t, uint32_t, uint16_t, uint8_t);
void CAN_SetTxMsg(uint16_t, uint32_t, uint8_t, uint8_t*);

#endif
