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

message_info_t message_info;

TEST_GROUP(CANProtocolDriver)
{
    void setup()
    {
		memset(RxMessageData, 0, 8);
		memset(TxMessageData, 0, 8);
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

TEST(CANProtocolDriver, testInit)
{
	PROT_STATE state;
	uint16_t in_id;

	state = CAN_protocol_get_state();
	in_id = CAN_protocol_get_in_message_id();
	BYTES_EQUAL(PROT_IDLE, state);
	LONGS_EQUAL(0, in_id);
}

TEST(CANProtocolDriver, rcvMessageChkID)
{
	uint8_t check;
	uint16_t in_id, length;

	RxMessageData[3] = 0x0A;
	RxMessageData[4] = 0x00;
	RxMessageData[5] = 0x00;
	RxMessageData[6] = 0x00;
	
	check = CAN_protocol_check_in_receive_message(RxMessageData);
	in_id = CAN_protocol_get_in_message_id();
	length = CAN_protocol_get_data_receive_length();
	BYTES_EQUAL(1, check);
	LONGS_EQUAL(0, in_id);	
	LONGS_EQUAL(10, length);
}

TEST(CANProtocolDriver, getInMessageData)
{
	uint8_t data;
	int8_t check;
	uint16_t state;

	check = CAN_protocol_in_receiving_data(RxMessageData, 8);
	BYTES_EQUAL(-1, check);

	data = 0;
	RxMessageData[3] = 0x0A;
	RxMessageData[4] = 0x00;
	RxMessageData[5] = 0x00;
	RxMessageData[6] = 0x00;
	RxMessageData[7] = 0x45;
	
	check = CAN_protocol_check_in_receive_message(RxMessageData);
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);
	BYTES_EQUAL(0, check);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IN_RECEIVING, state);	

	RxMessageData[0] = 0x01;
	RxMessageData[1] = 0x02;
	RxMessageData[2] = 0x03;	
	RxMessageData[3] = 0x04;
	RxMessageData[4] = 0x05;
	RxMessageData[5] = 0x06;
	RxMessageData[6] = 0x07;
	RxMessageData[7] = 0x08;
	
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);
	BYTES_EQUAL(0, check);	

	RxMessageData[0] = 0x09;
	RxMessageData[1] = 0x0A;
	RxMessageData[2] = 0x0B;	
	RxMessageData[3] = 0x0C;
	RxMessageData[4] = 0x0D;
	RxMessageData[5] = 0x0E;
	RxMessageData[6] = 0x0F;
	RxMessageData[7] = 0x10;
	
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);	
	BYTES_EQUAL(1, check);
	CAN_protocol_get_data_receive(&data, 0, 1);
	BYTES_EQUAL(0x45, data);
	CAN_protocol_get_data_receive(&data, 9, 1);
	BYTES_EQUAL(9, data);
	CAN_protocol_get_data_receive(&data, 10, 1);
	BYTES_EQUAL(0x0, data);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IN_SENDING, state);
}

TEST(CANProtocolDriver, replyInSendingData)
{
	uint8_t data[10];
	int8_t check;
	uint16_t state;

//	data = 0;
	RxMessageData[0] = 0x03;
	RxMessageData[1] = 0x01;
	RxMessageData[2] = 0x00;
	RxMessageData[3] = 0x0A;
	RxMessageData[4] = 0x00;
	RxMessageData[5] = 0x00;
	RxMessageData[6] = 0x00;
	RxMessageData[7] = 0x45;
	
	check = CAN_protocol_check_in_receive_message(RxMessageData);
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);

	RxMessageData[0] = 0x01;
	RxMessageData[1] = 0x02;
	RxMessageData[2] = 0x03;	
	RxMessageData[3] = 0x04;
	RxMessageData[4] = 0x05;
	RxMessageData[5] = 0x06;
	RxMessageData[6] = 0x07;
	RxMessageData[7] = 0x08;
	
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);

	RxMessageData[0] = 0x09;
	RxMessageData[1] = 0x0A;
	RxMessageData[2] = 0x0B;	
	RxMessageData[3] = 0x0C;
	RxMessageData[4] = 0x0D;
	RxMessageData[5] = 0x0E;
	RxMessageData[6] = 0x0F;
	RxMessageData[7] = 0x10;
	
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);	
	BYTES_EQUAL(1, check);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IN_SENDING, state);
	CAN_protocol_get_message_receive_info(&message_info);
	BYTES_EQUAL(0x03, message_info.message_type);
	BYTES_EQUAL(0x01, message_info.command);
	
	data[0] = 0x45;
	data[1] = 0x08;
	data[2] = 0x07;
	data[3] = 0x06;
	data[4] = 0x05;
	data[5] = 0x04;
	data[6] = 0x03;
	data[7] = 0x02;
	data[8] = 0x01;	
	data[9] = 0x0A;	
	
	CAN_protocol_set_message_send_info(&message_info);
	CAN_protocol_set_data_send_length(480);
	CAN_protocol_set_data_send(data, 0, 10);
	check = CAN_protocol_in_sending_data(TxMessageData);
	BYTES_EQUAL(0x03, TxMessageData[2]);
	BYTES_EQUAL(0xE0, TxMessageData[3]);
	BYTES_EQUAL(0x01, TxMessageData[4]);
	BYTES_EQUAL(0x45, TxMessageData[7]);
	BYTES_EQUAL(0, check);
	check = CAN_protocol_in_sending_data(TxMessageData);
	BYTES_EQUAL(0x08, TxMessageData[0]);
	BYTES_EQUAL(0, check);	
	check = CAN_protocol_in_sending_data(TxMessageData);
	BYTES_EQUAL(0x0A, TxMessageData[0]);	
	BYTES_EQUAL(0, check);	
	state= CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IN_SENDING, state);
}

TEST(CANProtocolDriver, sendOutReceivingData)
{
	uint8_t data[10];
	int8_t check;
	PROT_STATE state;

	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IDLE, state);
	check = CAN_protocol_get_out_message_id();
	BYTES_EQUAL(0, check);

	message_info.message_type = 0x03;
	message_info.command = 0x01;
	message_info.command_type = 0x00;
	
	data[0] = 0x45;
	data[1] = 0x08;
	data[2] = 0x07;
	data[3] = 0x06;
	data[4] = 0x05;
	data[5] = 0x04;
	data[6] = 0x03;
	data[7] = 0x02;
	data[8] = 0x01;	
	data[9] = 0x0A;	
	
	CAN_protocol_set_message_send_info(&message_info);
	CAN_protocol_set_data_send_length(10);
	CAN_protocol_set_data_send(data, 0, 10);
	check = CAN_protocol_out_sending_data(TxMessageData);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_OUT_SENDING, state);
	BYTES_EQUAL(0x03, TxMessageData[0]);
	BYTES_EQUAL(0x01, TxMessageData[1]);
	BYTES_EQUAL(0x00, TxMessageData[2]);
	BYTES_EQUAL(0x0A, TxMessageData[3]);
	BYTES_EQUAL(0x00, TxMessageData[4]);
	BYTES_EQUAL(0x00, TxMessageData[5]);
	BYTES_EQUAL(0x00, TxMessageData[6]);	
	BYTES_EQUAL(0x45, TxMessageData[7]);
	BYTES_EQUAL(0, check);
	
	check = CAN_protocol_out_sending_data(TxMessageData);
	BYTES_EQUAL(0x08, TxMessageData[0]);
	BYTES_EQUAL(0x07, TxMessageData[1]);
	BYTES_EQUAL(0x06, TxMessageData[2]);
	BYTES_EQUAL(0x05, TxMessageData[3]);
	BYTES_EQUAL(0x04, TxMessageData[4]);
	BYTES_EQUAL(0x03, TxMessageData[5]);
	BYTES_EQUAL(0x02, TxMessageData[6]);	
	BYTES_EQUAL(0x01, TxMessageData[7]);
	BYTES_EQUAL(0, check);
	
	check = CAN_protocol_out_sending_data(TxMessageData);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_OUT_RECEIVING, state);
	BYTES_EQUAL(0x0A, TxMessageData[0]);
	BYTES_EQUAL(0x07, TxMessageData[1]);
	BYTES_EQUAL(0x06, TxMessageData[2]);
	BYTES_EQUAL(0x05, TxMessageData[3]);
	BYTES_EQUAL(0x04, TxMessageData[4]);
	BYTES_EQUAL(0x03, TxMessageData[5]);
	BYTES_EQUAL(0x02, TxMessageData[6]);	
	BYTES_EQUAL(0x01, TxMessageData[7]);
	BYTES_EQUAL(1, check);

	RxMessageData[0] = 0x03;
	RxMessageData[1] = 0x01;
	RxMessageData[2] = 0x03;
	RxMessageData[3] = 0x01;
	RxMessageData[4] = 0x00;
	RxMessageData[5] = 0x00;
	RxMessageData[6] = 0x00;
	RxMessageData[7] = 0x45;

	check = CAN_protocol_check_out_receive_message(RxMessageData);
	BYTES_EQUAL(1, check);	
	check = CAN_protocol_out_receiving_data(RxMessageData, 8);
	BYTES_EQUAL(1, check);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IDLE, state);	
	check = CAN_protocol_get_out_message_id();
	BYTES_EQUAL(1, check);
	CAN_protocol_get_data_receive(data, 0, 1);
	BYTES_EQUAL(0x45, data[0]);	
}

TEST(CANProtocolDriver, InRetry)
{
	uint8_t data[10];
	int8_t check;
	uint16_t state;

//	data = 0;
	RxMessageData[0] = 0x03;
	RxMessageData[1] = 0x01;
	RxMessageData[2] = 0x00;
	RxMessageData[3] = 0x0A;
	RxMessageData[4] = 0x00;
	RxMessageData[5] = 0x00;
	RxMessageData[6] = 0x00;
	RxMessageData[7] = 0x45;
	
	check = CAN_protocol_check_in_receive_message(RxMessageData);
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);

	RxMessageData[0] = 0x01;
	RxMessageData[1] = 0x02;
	RxMessageData[2] = 0x03;	
	RxMessageData[3] = 0x04;
	RxMessageData[4] = 0x05;
	RxMessageData[5] = 0x06;
	RxMessageData[6] = 0x07;
	RxMessageData[7] = 0x08;
	
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);

	RxMessageData[0] = 0x09;
	RxMessageData[1] = 0x0A;
	RxMessageData[2] = 0x0B;	
	RxMessageData[3] = 0x0C;
	RxMessageData[4] = 0x0D;
	RxMessageData[5] = 0x0E;
	RxMessageData[6] = 0x0F;
	RxMessageData[7] = 0x10;
	
	check = CAN_protocol_in_receiving_data(RxMessageData, 8);	
	BYTES_EQUAL(1, check);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IN_SENDING, state);
	CAN_protocol_get_message_receive_info(&message_info);
	BYTES_EQUAL(0x03, message_info.message_type);
	BYTES_EQUAL(0x01, message_info.command);
	
	data[0] = 0x45;
	data[1] = 0x08;
	data[2] = 0x07;
	data[3] = 0x06;
	data[4] = 0x05;
	data[5] = 0x04;
	data[6] = 0x03;
	data[7] = 0x02;
	data[8] = 0x01;	
	data[9] = 0x0A;	
	
	CAN_protocol_set_message_send_info(&message_info);
	CAN_protocol_set_data_send_length(10);
	CAN_protocol_set_data_send(data, 0, 10);
	check = CAN_protocol_in_sending_data(TxMessageData);
	BYTES_EQUAL(0x03, TxMessageData[2]);
	BYTES_EQUAL(0x0A, TxMessageData[3]);
	BYTES_EQUAL(0x00, TxMessageData[4]);
	BYTES_EQUAL(0x45, TxMessageData[7]);
	BYTES_EQUAL(0, check);
	check = CAN_protocol_in_sending_data(TxMessageData);
	BYTES_EQUAL(0x08, TxMessageData[0]);
	BYTES_EQUAL(0, check);	
	check = CAN_protocol_in_sending_data(TxMessageData);
	BYTES_EQUAL(0x0A, TxMessageData[0]);	
	BYTES_EQUAL(1, check);	
	state= CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IDLE, state);

	
	RxMessageData[0] = 0x03;
	RxMessageData[1] = 0x01;
	RxMessageData[2] = 0x00;
	RxMessageData[3] = 0x0A;
	RxMessageData[4] = 0x00;
	RxMessageData[5] = 0x00;
	RxMessageData[6] = 0x00;
	RxMessageData[7] = 0x45;
	
	check = CAN_protocol_check_in_receive_message(RxMessageData);
	BYTES_EQUAL(1, check);
	state= CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IN_RECEIVING, state);	
}

TEST(CANProtocolDriver, outRetry)
{
	uint8_t data[10];
	int8_t check;
	PROT_STATE state;

	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_IDLE, state);
	check = CAN_protocol_get_out_message_id();
	BYTES_EQUAL(0, check);

	message_info.message_type = 0x03;
	message_info.command = 0x01;
	message_info.command_type = 0x00;
	
	data[0] = 0x45;
	data[1] = 0x08;
	data[2] = 0x07;
	data[3] = 0x06;
	data[4] = 0x05;
	data[5] = 0x04;
	data[6] = 0x03;
	data[7] = 0x02;
	data[8] = 0x01;	
	data[9] = 0x0A;	
	
	CAN_protocol_set_message_send_info(&message_info);
	CAN_protocol_set_data_send_length(10);
	CAN_protocol_set_data_send(data, 0, 10);
	check = CAN_protocol_out_sending_data(TxMessageData);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_OUT_SENDING, state);
	BYTES_EQUAL(0x03, TxMessageData[0]);
	BYTES_EQUAL(0x01, TxMessageData[1]);
	BYTES_EQUAL(0x00, TxMessageData[2]);
	BYTES_EQUAL(0x0A, TxMessageData[3]);
	BYTES_EQUAL(0x00, TxMessageData[4]);
	BYTES_EQUAL(0x00, TxMessageData[5]);
	BYTES_EQUAL(0x00, TxMessageData[6]);	
	BYTES_EQUAL(0x45, TxMessageData[7]);
	BYTES_EQUAL(0, check);
	
	check = CAN_protocol_out_sending_data(TxMessageData);
	BYTES_EQUAL(0x08, TxMessageData[0]);
	BYTES_EQUAL(0x07, TxMessageData[1]);
	BYTES_EQUAL(0x06, TxMessageData[2]);
	BYTES_EQUAL(0x05, TxMessageData[3]);
	BYTES_EQUAL(0x04, TxMessageData[4]);
	BYTES_EQUAL(0x03, TxMessageData[5]);
	BYTES_EQUAL(0x02, TxMessageData[6]);	
	BYTES_EQUAL(0x01, TxMessageData[7]);
	BYTES_EQUAL(0, check);
	
	check = CAN_protocol_out_sending_data(TxMessageData);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_OUT_RECEIVING, state);
	BYTES_EQUAL(0x0A, TxMessageData[0]);
	BYTES_EQUAL(0x07, TxMessageData[1]);
	BYTES_EQUAL(0x06, TxMessageData[2]);
	BYTES_EQUAL(0x05, TxMessageData[3]);
	BYTES_EQUAL(0x04, TxMessageData[4]);
	BYTES_EQUAL(0x03, TxMessageData[5]);
	BYTES_EQUAL(0x02, TxMessageData[6]);	
	BYTES_EQUAL(0x01, TxMessageData[7]);
	BYTES_EQUAL(1, check);

	// a resend
	check = CAN_protocol_out_sending_data(TxMessageData);
	state = CAN_protocol_get_state();
	BYTES_EQUAL(PROT_OUT_SENDING, state);	
	BYTES_EQUAL(0x03, TxMessageData[0]);
	BYTES_EQUAL(0x01, TxMessageData[1]);
	BYTES_EQUAL(0x00, TxMessageData[2]);
	BYTES_EQUAL(0x0A, TxMessageData[3]);
	BYTES_EQUAL(0x00, TxMessageData[4]);
	BYTES_EQUAL(0x00, TxMessageData[5]);
	BYTES_EQUAL(0x00, TxMessageData[6]);	
	BYTES_EQUAL(0x45, TxMessageData[7]);
	BYTES_EQUAL(0, check);	
}
