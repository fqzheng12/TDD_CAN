#include "CAN_helper.h"

uint8_t RxMessageData[8];
uint8_t TxMessageData[8];

void CAN_transmit(can_message_t* can_message, uint16_t id, uint8_t* message)
{
	if(can_message->data_cnt == 0)
	{
		can_message->data_cnt = 1;
		message[0] = can_message->message_type;
		message[1] = can_message->command;
		message[2] = can_message->command_type;
		message[3] = (uint8_t)((can_message->data_length) & 0x00FF);
		message[4] = (uint8_t)(((can_message->data_length) >>8) & 0x00FF);		
		message[5] = (uint8_t)((id) & 0x00FF);
		message[6] = (uint8_t)(((id) >>8) & 0x00FF);
		message[7] = can_message->data[0];
		// CAN send 1-byte data here
	}
	else
	{
		if(can_message->data_cnt + 8 >= can_message->data_length)
		{
			memcpy(message, can_message->data + can_message->data_cnt, can_message->data_length - can_message->data_cnt);			
			can_message->data_cnt = can_message->data_length;
			// CAN send (can_message->data_length - can_message->data_byte) byte(s) data here
		}
		else
		{
			memcpy(message, can_message->data + can_message->data_cnt, 8);
			can_message->data_cnt += 8;
			// CAN send 8-byte data here
		}
	}
}
