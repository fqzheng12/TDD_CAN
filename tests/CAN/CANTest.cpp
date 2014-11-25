//- ------------------------------------------------------------------
//-    Copyright (c) James W. Grenning -- All Rights Reserved         
//-    For use by owners of Test-Driven Development for Embedded C,   
//-    and attendees of Renaissance Software Consulting, Co. training 
//-    classes.                                                       
//-                                                                   
//-    Available at http://pragprog.com/titles/jgade/                 
//-        ISBN 1-934356-62-X, ISBN13 978-1-934356-62-3               
//-                                                                   
//-    Authorized users may use this source code in your own          
//-    projects, however the source code may not be used to           
//-    create training material, courses, books, articles, and        
//-    the like. We make no guarantees that this source code is       
//-    fit for any purpose.                                           
//-                                                                   
//-    www.renaissancesoftware.net james@renaissancesoftware.net      
//- ------------------------------------------------------------------

extern "C"
{
#include "CAN_protocol.h"
#include "CAN_helper.h"
}

#include "CppUTest/TestHarness.h"

TEST_GROUP(CANProtocolDriver)
{
    void setup()
    {
		memset(RxMessageData, 0, 8);
		RxMessageData[3] = 0x23;
		RxMessageData[4] = 0x01;
		RxMessageData[5] = 0x00;
		RxMessageData[6] = 0x00;
		RxMessageData[7] = 0x45;

		CAN_protocol_init();
		
    }

    void teardown()
    {
    }
};

IGNORE_TEST(CANProtocolDriver, testMessageID)
{
	uint16_t id;
	
	RxMessageData[5] = 0xAA;
	RxMessageData[6] = 0xBB;

	id = *(uint16_t*)(RxMessageData + 5);
	
	LONGS_EQUAL(0xBBAA, id);
}

IGNORE_TEST(CANProtocolDriver, checkMessageID)
{
	uint8_t check;
	uint16_t id;

	check = CAN_protocol_check_rcv_message(RxMessageData);
	id = CAN_protocol_get_message_id();
	
	BYTES_EQUAL(1, check);
	BYTES_EQUAL(0, id);
}

IGNORE_TEST(CANProtocolDriver, importData)
{
	PROT_STATE state;
	uint8_t check;
	uint16_t length;
	uint8_t data;

	state = CAN_protocol_get_state();
	CAN_protocol_get_data(&data, 0, 1);
	BYTES_EQUAL(0, data);
	BYTES_EQUAL(PROT_IDLE, state);
	
	check = CAN_protocol_check_rcv_message(RxMessageData);
	BYTES_EQUAL(1, check);
	BYTES_EQUAL(PROT_RECEIVING, state);	
	
	length = CAN_protocol_get_data_length();	
	LONGS_EQUAL(0x123, length);

	check = CAN_protocol_import_data(RxMessageData, 8);

	CAN_protocol_get_data(&data, 0, 1);
	BYTES_EQUAL(0x45, data);
}

TEST(CANProtocolDriver, importData)
{
	PROT_STATE state;
	uint8_t check;
	uint16_t length;
	uint8_t data;

	state = CAN_protocol_get_state();
	CAN_protocol_get_data(&data, 0, 1);
	BYTES_EQUAL(0, data);
	BYTES_EQUAL(PROT_IDLE, state);

	RxMessageData[3] = 0x0A;
	RxMessageData[4] = 0x00;
	check = CAN_protocol_check_rcv_message(RxMessageData);
	BYTES_EQUAL(1, check);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_RECEIVING, state);		
	length = CAN_protocol_get_data_length();	
	LONGS_EQUAL(0x000A, length);

	check = CAN_protocol_import_data(RxMessageData, 8);
	BYTES_EQUAL(0, check);
	CAN_protocol_get_data(&data, 0, 1);
	BYTES_EQUAL(0x45, data);
	
	RxMessageData[0] = 0x00;
	RxMessageData[1] = 0x01;
	RxMessageData[2] = 0x02;
	RxMessageData[3] = 0x03;
	RxMessageData[4] = 0x04;
	RxMessageData[5] = 0x05;
	RxMessageData[6] = 0x06;
	RxMessageData[7] = 0x07;	
	check = CAN_protocol_import_data(RxMessageData, 8);
	BYTES_EQUAL(0, check);
	CAN_protocol_get_data(&data, 7, 1);
	BYTES_EQUAL(0x06, data);

	RxMessageData[0] = 0x08;
	RxMessageData[1] = 0x09;	
	check = CAN_protocol_import_data(RxMessageData, 8);	
	BYTES_EQUAL(1, check);
	CAN_protocol_get_data(&data, 1, 1);
	BYTES_EQUAL(0x00, data);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_RESPONSE, state);		
	
}
