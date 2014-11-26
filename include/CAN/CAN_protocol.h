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

#ifndef D_CANProtocol_H
#define D_CANProtocol_H

#define	_CPPUTEST_

#include <stdlib.h>
#include <stdint.h>
#ifdef _CPPUTEST_
#include <memory.h>
#else
#include <string.h>
#endif

typedef enum{
	PROT_IDLE = 0,
	PROT_IN_RECEIVING,
	PROT_IN_SENDING,
	PROT_OUT_SENDING,
	PROT_OUT_RECEIVING,
} PROT_STATE;

typedef struct{
	uint8_t  message_type;
	uint8_t  command;
	uint8_t  command_type;
	uint16_t data_length;
	uint16_t data_cnt;
	uint8_t  data[512];
} can_message_t;

typedef struct{
	uint8_t  message_type;
	uint8_t  command;
	uint8_t  command_type;
} message_info_t;

#ifdef _CPPUTEST_
#include "CAN_helper.h"
#endif

void CAN_protocol_init(void);
PROT_STATE CAN_protocol_get_state(void);

uint16_t CAN_protocol_get_data_receive_length(void);
uint16_t CAN_protocol_get_in_message_id(void);
void CAN_protocol_get_data_receive(uint8_t*, uint8_t, uint8_t);
void CAN_protocol_get_message_receive_info(message_info_t*);

uint8_t CAN_protocol_check_in_receive_message(uint8_t*);
int8_t CAN_protocol_in_receiving_data(uint8_t*, uint8_t);

uint16_t CAN_protocol_get_out_message_id(void);
void CAN_protocol_set_out_message_id(uint16_t);
void CAN_protocol_set_message_send_info(message_info_t*);
void CAN_protocol_set_data_send_length(uint16_t);
void CAN_protocol_reset_data_send_count(void);
void CAN_protocol_set_data_send(uint8_t*, uint8_t, uint8_t);
int8_t CAN_protocol_in_sending_data(uint8_t*);
int8_t CAN_protocol_out_sending_data(uint8_t*);

uint8_t CAN_protocol_check_out_receive_message(uint8_t*);
int8_t CAN_protocol_out_receiving_data(uint8_t*, uint8_t);
#endif  /* D_CANProtocol_H */
