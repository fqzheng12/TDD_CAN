/***
 * Excerpted from "Test-Driven Development for Embedded C",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/jgade for more book information.
***/
/*- ------------------------------------------------------------------ -*/
/*-    Copyright (c) James W. Grenning -- All Rights Reserved          -*/
/*-    For use by owners of Test-Driven Development for Embedded C,    -*/
/*-    and attendees of Renaissance Software Consulting, Co. training  -*/
/*-    classes.                                                        -*/
/*-                                                                    -*/
/*-    Available at http://pragprog.com/titles/jgade/                  -*/
/*-        ISBN 1-934356-62-X, ISBN13 978-1-934356-62-3                -*/
/*-                                                                    -*/
/*-    Authorized users may use this source code in your own           -*/
/*-    projects, however the source code may not be used to            -*/
/*-    create training material, courses, books, articles, and         -*/
/*-    the like. We make no guarantees that this source code is        -*/
/*-    fit for any purpose.                                            -*/
/*-                                                                    -*/
/*-    www.renaissancesoftware.net james@renaissancesoftware.net       -*/
/*- ------------------------------------------------------------------ -*/

#include "CAN_protocol.h"

typedef struct{
	PROT_STATE state;
	uint8_t  message_type;
	uint8_t  command;
	uint8_t  command_type;
	uint16_t message_id;
	uint16_t data_length;
	uint16_t data_cnt;
	uint8_t  data[256];
} can_message_t;

static can_message_t can_message;

void CAN_protocol_init(void)
{
	memset(&can_message, 0, sizeof(can_message_t));
}

PROT_STATE CAN_protocol_get_state(void)
{
	return can_message.state;
}

uint16_t CAN_protocol_get_data_length(void)
{
	return can_message.data_length;
}

uint16_t CAN_protocol_get_message_id(void)
{
	return can_message.message_id;
}

int8_t CAN_protocol_import_data(uint8_t* message, uint8_t size)
{
	if(can_message.state == PROT_RECEIVING)
	{
		if(can_message.data_length > 0)
		{
			if(can_message.data_cnt == 0)
			{
				can_message.data[0] = message[7];
				can_message.data_cnt = 1;
				return 0;
			}
			else
			{
				if(can_message.data_cnt + size > can_message.data_length)
				{
					memcpy((can_message.data + can_message.data_cnt), message, (can_message.data_length - can_message.data_cnt));
					can_message.data_cnt = can_message.data_length;
				}
				else
				{
					memcpy((can_message.data + can_message.data_cnt), message, size);
					can_message.data_cnt += size;
				}
				if(can_message.data_cnt == can_message.data_length)
				{
					can_message.state = PROT_RESPONSE;
					can_message.data_cnt = 0;
					return 1;
				}
				else
				{
					return 0;
				}
			}
		}
		else
		{
			can_message.state = PROT_RESPONSE;
			return 1;
		}
	}
	return -1;
}

void CAN_protocol_get_data(uint8_t* data, uint8_t start, uint8_t size)
{
	memcpy(data, can_message.data + start, size);
}

uint8_t CAN_protocol_check_rcv_message(uint8_t* message)
{
	if(can_message.state == PROT_IDLE)
	{
		uint16_t id;
	
		id = *(uint16_t*)(message + 5);

		if(id == can_message.message_id)
		{
			uint16_t length;		
			
			can_message.state = PROT_RECEIVING;
			length = *(uint16_t*)(message + 3);
			can_message.data_length = length;
			can_message.message_type = message[0];
			can_message.command      = message[1];
			can_message.command_type = message[2];
			
			return 1;
		}
	}
	return 0;
}

