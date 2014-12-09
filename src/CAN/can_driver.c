#include "can_driver.h" 

CanTxMsg TxMessage;


void CAN_GPIO_Config(uint32_t remap)
{
 	GPIO_InitTypeDef GPIO_InitStructure;
   	
  	/*External Clock Setting*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

  	/*IO Setting*/
	if(remap == GPIO_Remap1_CAN1)
	{
		GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);
		/* Configure CAN pin: RX */
    	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	             // ��������
    	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
    	/* Configure CAN pin: TX */
    	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		         // �����������
    	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}
	else if(remap == GPIO_Remap2_CAN1)
	{
		GPIO_PinRemapConfig(GPIO_Remap2_CAN1, ENABLE);
		/* Configure CAN pin: RX */
    	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOD, &GPIO_InitStructure);
    	/* Configure CAN pin: TX */
    	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    
		GPIO_Init(GPIOD, &GPIO_InitStructure);
	}
	else
	{
		/* Configure CAN pin: RX */
    	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
    	/* Configure CAN pin: TX */
    	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
}

void CAN_NVIC_Config(void)
{
   	NVIC_InitTypeDef NVIC_InitStructure;
		/* Configure one bit for preemption priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	 	/*Interrupt Setting*/
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;	   //CAN1 RX0�ж�
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;		   //��ռ���ȼ�0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			   //�����ȼ�Ϊ0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void CAN_Mode_Config(void)
{
   	CAN_InitTypeDef        CAN_InitStructure;
	 	/************************CANͨ�Ų�������**********************************/
	/*CAN�Ĵ�����ʼ��*/
	CAN_DeInit(CAN1);
	CAN_StructInit(&CAN_InitStructure);
	/*CAN��Ԫ��ʼ��*/
	CAN_InitStructure.CAN_TTCM=DISABLE;			   //MCR-TTCM  �ر�ʱ�䴥��ͨ��ģʽʹ��
    CAN_InitStructure.CAN_ABOM=ENABLE;			   //MCR-ABOM  �Զ����߹��� 
    CAN_InitStructure.CAN_AWUM=ENABLE;			   //MCR-AWUM  ʹ���Զ�����ģʽ
    CAN_InitStructure.CAN_NART=DISABLE;			   //MCR-NART  ��ֹ�����Զ��ش�	  DISABLE-�Զ��ش�
    CAN_InitStructure.CAN_RFLM=DISABLE;			   //MCR-RFLM  ����FIFO ����ģʽ  DISABLE-���ʱ�±��ĻḲ��ԭ�б���  
    CAN_InitStructure.CAN_TXFP=DISABLE;			   //MCR-TXFP  ����FIFO���ȼ� DISABLE-���ȼ�ȡ���ڱ��ı�ʾ�� 
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;  //��������ģʽ
    CAN_InitStructure.CAN_SJW=CAN_SJW_1tq;		   //BTR-SJW ����ͬ����Ծ��� 1��ʱ�䵥Ԫ
    CAN_InitStructure.CAN_BS1=CAN_BS1_12tq;		   //BTR-TS1 ʱ���1 ռ����12��ʱ�䵥Ԫ
    CAN_InitStructure.CAN_BS2=CAN_BS2_3tq;		   //BTR-TS1 ʱ���2 ռ����3��ʱ�䵥Ԫ
    CAN_InitStructure.CAN_Prescaler =9;		   ////BTR-BRP �����ʷ�Ƶ��  ������ʱ�䵥Ԫ��ʱ�䳤�� 36/(1+12+3)/9=0.25Mbps
	CAN_Init(CAN1, &CAN_InitStructure);
}


void CAN_StdID_Filter_Config(uint8_t FilterNumber, uint8_t FilterMode, uint8_t FilterScale, uint16_t StdId, uint16_t MaskId, uint8_t FIFO)
{
   CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	/*CAN��������ʼ��*/
	CAN_FilterInitStructure.CAN_FilterNumber=FilterNumber;						//��������
    CAN_FilterInitStructure.CAN_FilterMode=FilterMode;	//�����ڱ�ʶ������λģʽ

	CAN_FilterInitStructure.CAN_FilterScale=FilterScale;	//������λ��
	/* ʹ�ܱ��ı�ʾ�����������ձ�ʾ�������ݽ��бȶԹ��ˣ���չID�������µľ����������ǵĻ��������FIFO0�� */

    CAN_FilterInitStructure.CAN_FilterIdHigh= ((u16)StdId<<5);				//Ҫ���˵�ID��λ 
    CAN_FilterInitStructure.CAN_FilterIdLow= (((u32)0x0000<<3)|(CAN_ID_STD)|(CAN_RTR_DATA)); //Ҫ���˵�ID��λ 
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh= MaskId;			//��������16λÿλ����ƥ��
    CAN_FilterInitStructure.CAN_FilterMaskIdLow= 0x0006;			//��������16λÿλ����ƥ��

	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=FIFO;				//��������������FIFO0
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;			//ʹ�ܹ�����
	CAN_FilterInit(&CAN_FilterInitStructure);
	/*CANͨ���ж�ʹ��*/
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

void CAN_ExtID_Filter_Config(uint8_t FilterNumber, uint8_t FilterMode, uint8_t FilterScale, uint32_t ExtId, uint16_t MaskId, uint8_t FIFO)
{
   CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	/*CAN��������ʼ��*/
	CAN_FilterInitStructure.CAN_FilterNumber=FilterNumber;						//��������
    CAN_FilterInitStructure.CAN_FilterMode=FilterMode;	//�����ڱ�ʶ������λģʽ

	CAN_FilterInitStructure.CAN_FilterScale=FilterScale;	//������λ��
	/* ʹ�ܱ��ı�ʾ�����������ձ�ʾ�������ݽ��бȶԹ��ˣ���չID�������µľ����������ǵĻ��������FIFO0�� */

    CAN_FilterInitStructure.CAN_FilterIdHigh= (((u32)ExtId>>13)&0x0000001F);				//Ҫ���˵�ID��λ 
    CAN_FilterInitStructure.CAN_FilterIdLow= ((((u32)0x0000<<3)&0x0000FFF8)|(CAN_ID_EXT)|(CAN_RTR_DATA)); //Ҫ���˵�ID��λ 
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh= 0x001F;			//��������16λÿλ����ƥ��
    CAN_FilterInitStructure.CAN_FilterMaskIdLow= MaskId;			//��������16λÿλ����ƥ��

	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=FIFO;				//��������������FIFO0
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;			//ʹ�ܹ�����
	CAN_FilterInit(&CAN_FilterInitStructure);
	/*CANͨ���ж�ʹ��*/
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

void CAN_SetTxMsg(uint16_t id, uint32_t id_type, uint8_t dlc, uint8_t* data)
{
	if(IS_CAN_IDTYPE(id_type))
	{
		TxMessage.RTR=CAN_RTR_DATA;				 //���͵�������

		TxMessage.IDE=id_type;
		if(id_type == CAN_ID_EXT)
		{
			TxMessage.ExtId=id;
		}
		else
		{
			TxMessage.StdId=id;
		}

		TxMessage.DLC=dlc;							 //���ݳ���
		memcpy(TxMessage.Data, data, dlc);

  		CAN_Transmit(CAN1, &TxMessage);	
  	}
}
/**************************END OF FILE************************************/

