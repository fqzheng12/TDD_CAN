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
	uint16_t in_message_id;
	uint16_t out_message_id;
} can_state_t;

static can_message_t can_message_receive, can_message_send;
static can_state_t can_state;

void CAN_protocol_init(void)
{
	memset(&can_message_receive, 0, sizeof(can_message_t));
	memset(&can_message_send, 0, sizeof(can_message_t));
	memset(&can_state, 0, sizeof(can_state_t));
}

PROT_STATE CAN_protocol_get_state(void)
{
	return can_state.state;
}

uint16_t CAN_protocol_get_data_receive_length(void)
{
	return can_message_receive.data_length;
}

void CAN_protocol_set_data_send_length(uint16_t length)
{
	can_message_send.data_cnt = 0;
	can_message_send.data_length = length;
}

void CAN_protocol_reset_data_send_count(void)
{
	can_message_send.data_cnt = 0;
}

uint16_t CAN_protocol_get_in_message_id(void)
{
	return can_state.in_message_id;
}

uint16_t CAN_protocol_get_out_message_id(void)
{
	return can_state.out_message_id;
}

void CAN_protocol_set_out_message_id(uint16_t id)
{
	can_state.out_message_id = id;
}

void CAN_protocol_get_message_receive_info(message_info_t* message_info)
{
	message_info->message_type = can_message_receive.message_type;
	message_info->command      = can_message_receive.command;
	message_info->command_type = can_message_receive.command_type;	
}

void CAN_protocol_set_message_send_info(message_info_t* message_info)
{
	can_message_send.message_type = message_info->message_type;
	can_message_send.command      = message_info->command;
	can_message_send.command_type = message_info->command_type;
}

void CAN_protocol_set_message_send_id(uint16_t id)
{
	can_state.out_message_id = id;
}

int8_t CAN_protocol_in_receiving_data(uint8_t* message, uint8_t size)
{
	if(can_state.state == PROT_IN_RECEIVING)
	{
		if(can_message_receive.data_length > 0)
		{
			if(can_message_receive.data_cnt == 0)
			{
				can_message_receive.data[0] = message[7];
				can_message_receive.data_cnt = 1;
				if(can_message_receive.data_length == 1)
				{
					can_state.state = PROT_IN_SENDING;
					can_message_receive.data_cnt = 0;
					return 1;
				}
				else
				{
					return 0;
				}
			}
			else
			{
				if(can_message_receive.data_cnt + size > can_message_receive.data_length)
				{
					memcpy((can_message_receive.data + can_message_receive.data_cnt), message, (can_message_receive.data_length - can_message_receive.data_cnt));
					can_message_receive.data_cnt = can_message_receive.data_length;
				}
				else
				{
					memcpy((can_message_receive.data + can_message_receive.data_cnt), message, size);
					can_message_receive.data_cnt += size;
				}
				if(can_message_receive.data_cnt == can_message_receive.data_length)
				{
					can_state.state = PROT_IN_SENDING;
					can_message_receive.data_cnt = 0;
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
			can_state.state = PROT_IN_SENDING;
			return 1;
		}
	}
	return -1;
}

int8_t CAN_protocol_out_receiving_data(uint8_t* message, uint8_t size)
{
	if(can_state.state == PROT_OUT_RECEIVING)
	{
		if(can_message_receive.data_length > 0)
		{
			if(can_message_receive.data_cnt == 0)
			{
				can_message_receive.data[0] = message[7];
				can_message_receive.data_cnt = 1;
				if(can_message_receive.data_length == 1)
				{
					can_state.state = PROT_IDLE;
					can_message_receive.data_cnt = 0;
					can_state.out_message_id++;
					return 1;
				}
				else
				{
					return 0;
				}
			}
			else
			{
				if(can_message_receive.data_cnt + size > can_message_receive.data_length)
				{
					memcpy((can_message_receive.data + can_message_receive.data_cnt), message, (can_message_receive.data_length - can_message_receive.data_cnt));
					can_message_receive.data_cnt = can_message_receive.data_length;
				}
				else
				{
					memcpy((can_message_receive.data + can_message_receive.data_cnt), message, size);
					can_message_receive.data_cnt += size;
				}
				if(can_message_receive.data_cnt == can_message_receive.data_length)
				{
					can_state.state = PROT_IDLE;
					can_message_receive.data_cnt = 0;
					can_state.out_message_id++;
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
			can_state.out_message_id++;		
			can_state.state = PROT_IDLE;
			return 1;
		}
	}
	return -1;
}

int8_t CAN_protocol_in_sending_data(uint8_t* message)
{
	if(can_state.state == PROT_IN_SENDING)
	{
		can_message_send.command_type = 0x03; // command type must be respond
		CAN_transmit(&can_message_send, can_state.in_message_id, message);
		if(can_message_send.data_cnt == can_message_send.data_length)
		{
			can_state.state = PROT_IDLE;
			can_state.in_message_id++;
			return 1;
		}
		else
		{
			return 0;
		}
	}
	return -1;
}

int8_t CAN_protocol_out_sending_data(uint8_t* message)
{
	if(can_state.state == PROT_IDLE)
	{
		can_state.state = PROT_OUT_SENDING;
	}
	else if(can_state.state == PROT_OUT_RECEIVING)	// no respond. retry
	{
		can_message_send.data_cnt = 0;
		can_state.state = PROT_OUT_SENDING;
	}
	if(can_state.state == PROT_OUT_SENDING)
	{
		CAN_transmit(&can_message_send, can_state.out_message_id, message);
		if(can_message_send.data_cnt == can_message_send.data_length)
		{
			can_state.state = PROT_OUT_RECEIVING;
			return 1;
		}
		else
		{
			return 0;
		}
	}
	return -1;
}

void CAN_protocol_get_data_receive(uint8_t* data, uint8_t start, uint8_t size)
{
	memcpy(data, can_message_receive.data + start, size);
}

void CAN_protocol_set_data_send(uint8_t* data, uint8_t start, uint8_t size)
{
	memcpy(can_message_send.data + start, data, size);
}

uint8_t CAN_protocol_check_in_receive_message(uint8_t* message)
{
	if(can_state.state == PROT_IDLE)
	{
		uint16_t id;
	
		id = *(uint16_t*)(message + 5);

		if(id == can_state.in_message_id)
		{
			uint16_t length;		
			
			can_state.state = PROT_IN_RECEIVING;
			length = *(uint16_t*)(message + 3);
			can_message_receive.data_length = length;
			can_message_receive.message_type = message[0];
			can_message_receive.command      = message[1];
			can_message_receive.command_type = message[2];
			
			return 1;
		}
		else if(id == can_state.in_message_id - 1)	// a resend?
		{
			uint16_t length;
			uint8_t message_type, command, command_type;
			
			length = *(uint16_t*)(message + 3);
			message_type = message[0];
			command = message[1];
			command_type = message[2];
			if(can_message_receive.data_length == length && can_message_receive.message_type == message_type
			&& can_message_receive.command == command    && can_message_receive.command_type == command_type)
			{
				can_state.state = PROT_IN_RECEIVING;
				can_state.in_message_id = id;
				return 1;
			}
		}
	}
	return 0;
}

uint8_t CAN_protocol_check_out_receive_message(uint8_t* message)
{
	if(can_state.state == PROT_OUT_RECEIVING)
	{
		uint16_t id;
		uint8_t command_type;
	
		id = *(uint16_t*)(message + 5);
		command_type = message[2];

		if(id == can_state.out_message_id && command_type == 0x03) // command type must be respond
		{
			uint16_t length;		
			
			length = *(uint16_t*)(message + 3);
			can_message_receive.data_length = length;
			can_message_receive.message_type = message[0];
			can_message_receive.command      = message[1];
			can_message_receive.command_type = 0x03;
			
			return 1;
		}
	}
	return 0;
}



