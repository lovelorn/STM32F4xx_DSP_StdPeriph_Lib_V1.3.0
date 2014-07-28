/**
  ******************************************************************************
  * @file    Usart.c
  * @author  Lovelorn
  * @version V1.0.0
  * @date    18-June-2014
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Universal synchronous asynchronous receiver
  *          transmitter (USART1):           
  *           + Initialization and Configuration
  *           + Data transfers
  *           + DMA transfers management
  *           + Interrupts and flags management 
  *           
  @verbatim       
 ===============================================================================
                        ##### How to use this driver #####
 ===============================================================================
    [..]
      (#) Initialization the USART1 using USART1_Init(,) function;
  
      (#) Transmit data using USART1_Tx() function. nerver try to transmit more
          than MAX_USART_TX_NUM_OF_BLOCKS*MAX_USART_TX_BLOCK_SIZE bytes at one time	  
  
      (#) Receive data using USART1_Rx() function.
          
      (#) in the stm32f4xx_it.c add the following codes;*/

/**********************start here **********************************************/
//  /**
//  * @brief  This function handles DMA2_Stream7 Usart1 DMA Tx interrupt request.
//  * @param  None
//  * @retval None
//  */
//void DMA2_Stream7_IRQHandler(void)
//{
//	//�ж��Ƿ�ΪUSART1��TX DMA�ж�
//	if(DMA_GetITStatus(DMA2_Stream7,DMA_IT_TCIF7))
//	{
//		DMA_ClearFlag(DMA2_Stream7,DMA_FLAG_TCIF7);
//	    DMA_ClearITPendingBit(DMA2_Stream7,DMA_IT_TCIF7);
//		USART1_TxDMAITHandle();
//	}
//	
//}

///**
//  * @brief  This function handles Usart1 Rx Handler.
//  * @param  None
//  * @retval None
//  */
//void USART1_IRQHandler(void)
//{
//	uint8_t RcvData;
//	if(USART_GetITStatus(USART1,USART_IT_RXNE) == SET)//�����ж�
//	{
//		RcvData = (uint8_t)(USART1->DR & (uint8_t)0xFF);
//		USART1_RxITHandle(RcvData);
//	}
//	if(USART_GetITStatus(USART1,USART_IT_TC) == SET)//�����ж�
//	{
//		
//	}
//}
  
/********************end here **************************************/
    


#include "Usart.h"
#include "string.h"

uint8_t TxBuffer[128];

USART_RxCircleBufferType USART1_RX_Buffer;
USART_TXDMABufferType    USART1_TX_DMABuffer;


/*UartRxBufferInit����˵��   �Դ��ڵ��ж�ʽ������г�ʼ��   */
/*���������                                                */
/*         UsartRxBuffer  :���ջ���ṹ��ָ��               */
/*����ֵ��                                                  */
/*         ��                                               */
/*                                                          */
void UsartRxBufferInit(USART_RxCircleBufferType *UsartRxBuffer)
{
	uint32_t i;
	UsartRxBuffer->Count = 0;
	UsartRxBuffer->Head = 0;
	UsartRxBuffer->Tail = 0;
	for(i = 0; i < MAX_USART_RX_BUFFER_DEPTH; i++)
	{
		UsartRxBuffer->Buffer[i] = 0;
	}
}

/*UartRxBufferInit����˵��   �Դ��ڵ�DMA���ͻ�����г�ʼ��  */
/*���������                                                */
/*         UsartTxDMABuffer  :���ͻ���ṹ��ָ��            */
/*����ֵ��                                                  */
/*         ��                                               */
/*                                                          */
void UsartTxDMABufferInit(USART_TXDMABufferType *UsartTxDMABuffer)
{
	uint32_t i,j;
	UsartTxDMABuffer->Count = 0;
	UsartTxDMABuffer->Head = 0;
	UsartTxDMABuffer->Tail = 0;
	UsartTxDMABuffer->DMA_Status = USART_DMA_IDLE;
	for(i = 0;i < MAX_USART_TX_NUM_OF_BLOCKS;i++)
	{
		UsartTxDMABuffer->Block_Used_Size[i] = 0;
		for(j = 0; j < MAX_USART_TX_BLOCK_SIZE;j++)
		{
			UsartTxDMABuffer->Buffer[i][j] = 0;
		}
	}
}


/*Usart1Init����˵��  �Դ���1���г�ʼ��                     */
/*���������                                                */
/*         BandRate  :������                                */
/*         PortAlign :�ܽ�λ��                              */
/*                   0:PA9-TX  PA10-RX                      */
/*                   1:PB6-TX  PB7 -RX                      */
/*����ֵ��                                                  */
/*        0:��ʼ���ɹ�    1����ʼ��ʧ��                     */
/*                                                          */
uint32_t USART1_Init(uint32_t BandRate,uint32_t PortAlign)
{
	NVIC_InitTypeDef Usart1NVIC;
	//����USART1��APBʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	//����GPIOA��APBʱ��
	
	if(PortAlign == 0x00)
	{
		//����PA9��PA10Ϊ���⹦������?
		GPIO_InitTypeDef USART1_IO;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

		
		USART1_IO.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; 
		USART1_IO.GPIO_Mode = GPIO_Mode_AF;
		USART1_IO.GPIO_OType = GPIO_OType_PP;
		USART1_IO.GPIO_PuPd = GPIO_PuPd_NOPULL;
		USART1_IO.GPIO_Speed = GPIO_Medium_Speed;
		GPIO_Init(GPIOA,&USART1_IO);
		//��PA9��PA10��USART1��������
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
	}
	else if(PortAlign == 0x01)
	{
		//����PA9��PA10Ϊ���⹦������?
		GPIO_InitTypeDef USART1_IO;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
		
		USART1_IO.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; 
		USART1_IO.GPIO_Mode = GPIO_Mode_AF;
		USART1_IO.GPIO_OType = GPIO_OType_PP;
		USART1_IO.GPIO_PuPd = GPIO_PuPd_NOPULL;
		USART1_IO.GPIO_Speed = GPIO_Medium_Speed;
		GPIO_Init(GPIOB,&USART1_IO);
		//��PA9��PA10��USART1��������
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_USART1);
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource7,GPIO_AF_USART1);			
	}
	else //�ܽ�Լ�����󣬹ر�ʱ��
	{
		//����USART1��APBʱ��
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
		//����GPIOA��APBʱ��
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, DISABLE);	
		return 1;
	}
	
	//���ô��ڲ���
	USART_InitTypeDef USART1_Init;
	USART1_Init.USART_BaudRate = BandRate;
	USART1_Init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART1_Init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART1_Init.USART_Parity = USART_Parity_No;
	USART1_Init.USART_StopBits = USART_StopBits_1;
	USART1_Init.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1,&USART1_Init);

	
	//��ʼ�����ж�
	Usart1NVIC.NVIC_IRQChannel = USART1_IRQn;
	Usart1NVIC.NVIC_IRQChannelPreemptionPriority = 0;
	Usart1NVIC.NVIC_IRQChannelSubPriority = 0;
	Usart1NVIC.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&Usart1NVIC);
	
	//����USART1�Ľ��������ж�ʹ��
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE );
	
	//����DMA2��ʱ��ʹ��
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);	

	
	//��ʼ�����ڽ��ջ���ͷ���DMA����
	UsartRxBufferInit(&USART1_RX_Buffer);
	UsartTxDMABufferInit(&USART1_TX_DMABuffer);
	
	//����USART1
	USART_Cmd(USART1,ENABLE);	
	
	memset((void *)TxBuffer,0x32,128);

	return 0;
}


/*UsartRxBufferPush����˵��   �����ڽ��ջ�����ѹ��һ������  */
/*���������                                                */
/*         UsartRxBuffer  :���ջ���ṹ��ָ��               */
/*         data           :���ڽ��յ�����                   */
/*����ֵ��                                                  */
/*         0: �����ɹ�  1������ʧ��                         */
/*                                                          */
uint8_t UsartRxBufferPush(USART_RxCircleBufferType *UsartRxBuffer,uint8_t data)
{
	if(UsartRxBuffer->Count == MAX_USART_RX_BUFFER_DEPTH)
	{
		return 1;
	}
	UsartRxBuffer->Buffer[UsartRxBuffer->Head] = data;
	UsartRxBuffer->Head++;
	if(UsartRxBuffer->Head == MAX_USART_RX_BUFFER_DEPTH)
	{
		UsartRxBuffer->Head = 0;
	}
	UsartRxBuffer->Count++;
	return 0;
}

/*UsartRxBufferRcv����˵��   �Ӵ��ڽ��ջ����ж�ȡ����       */
/*���������                                                */
/*         UsartRxBuffer  :���ջ���ṹ��ָ��               */
/*         buffer         :���ڽ��ջ���                     */
/*         len            :�ƻ���ȡ�����ݳ���               */
/*����ֵ��                                                  */
/*         0: �����ɹ�  1������ʧ��                         */
/*                                                          */
uint8_t UsartRxBufferRcv(USART_RxCircleBufferType *UsartRxBuffer,uint8_t *buffer, uint32_t len)
{
	uint32_t temp;
	if(UsartRxBuffer->Count < len)
	{
		return 1;
	}
	if((UsartRxBuffer->Tail + len) >= MAX_USART_RX_BUFFER_DEPTH)
	{
		temp = MAX_USART_RX_BUFFER_DEPTH - UsartRxBuffer->Tail;
		memcpy(buffer,&UsartRxBuffer->Buffer[UsartRxBuffer->Tail],temp);
		memcpy(&buffer[temp],UsartRxBuffer->Buffer,len-temp);
		UsartRxBuffer->Tail = len-temp;
	}
	else 
	{
		memcpy(buffer,&UsartRxBuffer->Buffer[UsartRxBuffer->Tail],len);
		UsartRxBuffer->Tail += len;
	}
	UsartRxBuffer->Count -= len;
	return 0;
}


/*Usart_TX_DMA_CheckBuffer����˵��  ͨ��DMA���ʹ�������     */
/*���������                                                */
/*         TxDMABuffer    :���ͻ���ṹ��ָ��               */
/*����ֵ��                                                  */
/*         ��                                               */
/*                                                          */
void Usart_TX_DMA_CheckBuffer(USART_TXDMABufferType *TxDMABuffer)
{
	DMA_InitTypeDef USART1_DMA_InitStructure;
	NVIC_InitTypeDef DMA2NVIC;
	//uint32_t IsDMATxFinished = (DMA_GetFlagStatus(DMA2_Stream7,DMA_IT_TCIF7) == RESET);
	
	if(TxDMABuffer->DMA_Status == USART_DMA_TRANS_DONE)//�������һ�����ݰ�������ָ��
	{
		//�������еĶ�ָ��
		TxDMABuffer->Tail++;
		if(TxDMABuffer->Tail == MAX_USART_TX_NUM_OF_BLOCKS)
		{
			TxDMABuffer->Tail = 0;
		}
		//�������и���
		TxDMABuffer->Count--;
	}
	
	if(TxDMABuffer->Count > 0)
	{
		if(TxDMABuffer->DMA_Status == USART_DMA_IDLE || TxDMABuffer->DMA_Status == USART_DMA_TRANS_DONE)//��һ�η��ͻ���DMA�ж����
		{
			//�ӻ����ж�һ����������׼����DMA����
			uint32_t index = TxDMABuffer->Tail;			
			
			//��ʼ��DMA���ڴ��׵�ַ�����ݸ���			
			DMA_DeInit(DMA2_Stream7);
			while (DMA_GetCmdStatus(DMA2_Stream7) != DISABLE)
			{
			}		
			USART1_DMA_InitStructure.DMA_Channel = DMA_Channel_4;
			USART1_DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)TxDMABuffer->Buffer[index];
			USART1_DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t )&(USART1->DR);
			USART1_DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
			USART1_DMA_InitStructure.DMA_BufferSize = TxDMABuffer->Block_Used_Size[index];
			USART1_DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
			USART1_DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte ;
			USART1_DMA_InitStructure.DMA_Mode = DMA_Mode_Normal ;
			USART1_DMA_InitStructure.DMA_Priority = DMA_Priority_High;
			USART1_DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
			USART1_DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable ;
			USART1_DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
			USART1_DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full ;//��������
			USART1_DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
			USART1_DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
			DMA_Init(DMA2_Stream7,&USART1_DMA_InitStructure);
		
			//����DMA�ж�ʹ��
			DMA_ITConfig(DMA2_Stream7,DMA_IT_TC,ENABLE);		
			
			DMA_Cmd(DMA2_Stream7,ENABLE);	
			while (DMA_GetCmdStatus(DMA2_Stream7) != ENABLE)
			{
			}		
			//����USART1�ķ���DMA
			USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);		
			//����DMA��������ж�
			DMA2NVIC.NVIC_IRQChannel = DMA2_Stream7_IRQn;
			DMA2NVIC.NVIC_IRQChannelCmd = ENABLE;
			DMA2NVIC.NVIC_IRQChannelPreemptionPriority = 0;
			DMA2NVIC.NVIC_IRQChannelSubPriority = 0;
			NVIC_Init(&DMA2NVIC);		

			//�޸�DMA״̬
			TxDMABuffer->DMA_Status = USART_DMA_TRANSMITTING;			
		}
		else if(TxDMABuffer->DMA_Status == USART_DMA_TRANSMITTING)
		{
			//���ڷ��ͣ���ʱ�����κδ���
			return;
		}
	}
	else if(TxDMABuffer->Count == 0)
	{
		if(TxDMABuffer->DMA_Status == USART_DMA_IDLE)//��һ�η���
		{
			//ʲô������������ģʽ������
			return;
		}
		else if(TxDMABuffer->DMA_Status == USART_DMA_TRANS_DONE)//DMA�жϴ���
		{
			//��������ȫ�����ݰ��������ˣ���Ϣ��Ϣ
			DMA_Cmd(DMA2_Stream7,DISABLE);	
			TxDMABuffer->DMA_Status = USART_DMA_IDLE;
			return;
		}
		else if(TxDMABuffer->DMA_Status == USART_DMA_TRANSMITTING)
		{
			//�����ܽ�������״̬
			return;
		}	
	}
	
	
	
	//������ڽ���DMA���ͣ��򷵻�
	if(TxDMABuffer->DMA_Status == USART_DMA_TRANSMITTING)
	{
		return ;
	}
	//�������Ϊ�գ��򷵻�
	if(TxDMABuffer->Count == 0 && (TxDMABuffer->DMA_Status != USART_DMA_TRANSMITTING))
	{
		DMA_Cmd(DMA2_Stream7,DISABLE);	
		TxDMABuffer->DMA_Status = USART_DMA_IDLE;
		return ;
	}
	//���DMA������ɣ����Ҷ��в�Ϊ�գ�������DMA����
	if((TxDMABuffer->DMA_Status != USART_DMA_TRANSMITTING) && TxDMABuffer->Count > 0)
	{
		uint32_t index = TxDMABuffer->Tail;

		//�������еĶ�ָ��
		TxDMABuffer->Tail++;
		if(TxDMABuffer->Tail == MAX_USART_TX_NUM_OF_BLOCKS)
		{
			TxDMABuffer->Tail = 0;
		}
		//�������и���
		TxDMABuffer->Count--;
		
		//��ʼ��DMA���ڴ��׵�ַ�����ݸ���
		
		DMA_DeInit(DMA2_Stream7);
		while (DMA_GetCmdStatus(DMA2_Stream7) != DISABLE)
		{
		}		
		USART1_DMA_InitStructure.DMA_Channel = DMA_Channel_4;
		USART1_DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)TxDMABuffer->Buffer[index];
		USART1_DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t )&(USART1->DR);
		USART1_DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
		USART1_DMA_InitStructure.DMA_BufferSize = TxDMABuffer->Block_Used_Size[index];
		USART1_DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		USART1_DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte ;
		USART1_DMA_InitStructure.DMA_Mode = DMA_Mode_Normal ;
		USART1_DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		USART1_DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		USART1_DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable ;
		USART1_DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
		USART1_DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full ;//��������
		USART1_DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
		USART1_DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
		DMA_Init(DMA2_Stream7,&USART1_DMA_InitStructure);
	
		//����DMA�ж�ʹ��
		DMA_ITConfig(DMA2_Stream7,DMA_IT_TC,ENABLE);		
		
		DMA_Cmd(DMA2_Stream7,ENABLE);	
		while (DMA_GetCmdStatus(DMA2_Stream7) != ENABLE)
		{
		}		
		//����USART1�ķ���DMA
		USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);		
		//����DMA��������ж�
		DMA2NVIC.NVIC_IRQChannel = DMA2_Stream7_IRQn;
		DMA2NVIC.NVIC_IRQChannelCmd = ENABLE;
		DMA2NVIC.NVIC_IRQChannelPreemptionPriority = 0;
		DMA2NVIC.NVIC_IRQChannelSubPriority = 0;
		NVIC_Init(&DMA2NVIC);		
	}	
}


/*Usart1_TX_DMA����˵��   �Ӵ���1����DMA��������            */
/*���������                                                */
/*         TxDMABuffer  :���Ͷ���ָ��                      */
/*         Buffer       :���ͻ�����ָ��                     */
/*         size         :���ݳ���                           */
/*����ֵ��                                                  */
/*         0: �����ɹ�  1������ʧ��                         */
/*                                                          */
uint8_t Usart_TX_DMA(USART_TXDMABufferType *TxDMABuffer,uint8_t *Buffer, uint32_t size)
{
	uint32_t i = 0;
	uint32_t index = 0;
	if(size == 0)
	{
		return 1;
	}
	//���������û���㹻�Ŀռ䣬�����ʧ��
	if((TxDMABuffer->Count + (size -1)/MAX_USART_TX_BLOCK_SIZE +1) > MAX_USART_TX_NUM_OF_BLOCKS)
	{
		return 1;
	}
	//�����ݷ��������
	do
	{
		index = TxDMABuffer->Head;
		if((size - i) >= MAX_USART_TX_BLOCK_SIZE)
		{
			TxDMABuffer->Block_Used_Size[index] = MAX_USART_TX_BLOCK_SIZE;
			memcpy(TxDMABuffer->Buffer[index],Buffer+i,MAX_USART_TX_BLOCK_SIZE);
			TxDMABuffer->Head++;
			if(TxDMABuffer->Head >= MAX_USART_TX_NUM_OF_BLOCKS)
			{
				TxDMABuffer->Head -= MAX_USART_TX_NUM_OF_BLOCKS;
			}		
			TxDMABuffer->Count++;	
		}
		else
		{
			TxDMABuffer->Block_Used_Size[index] = size-i;
			memcpy(TxDMABuffer->Buffer[index],Buffer+i,size-i);
			TxDMABuffer->Head++;
			if(TxDMABuffer->Head >= MAX_USART_TX_NUM_OF_BLOCKS)
			{
				TxDMABuffer->Head -= MAX_USART_TX_NUM_OF_BLOCKS;
			}
			TxDMABuffer->Count++;			
		}
		i+=MAX_USART_TX_BLOCK_SIZE;
	}while(i < size);
	

	//����DMA����
	Usart_TX_DMA_CheckBuffer(TxDMABuffer);
	
	return 0;
}

/*USART1_Rx����˵��   �Ӵ��ڽ��ջ����ж�ȡ����              */
/*���������                                                */
/*         buffer         :���ڽ��ջ���                     */
/*         len            :�ƻ���ȡ�����ݳ���               */
/*����ֵ��                                                  */
/*         0: �����ɹ�  ����ָ�����ȵ�����                  */
/*         1������ʧ��  �������κ�����                      */
/*                                                          */
uint8_t USART1_Rx(uint8_t *buffer,uint32_t len)
{
	return UsartRxBufferRcv(&USART1_RX_Buffer,buffer,len);
}


/*USART1_Tx����˵��   �Ӵ���1����DMA��������                */
/*���������                                                */
/*         Buffer       :���ͻ�����ָ��                     */
/*         size         :���ݳ���                           */
/*����ֵ��                                                  */
/*         0: �����ɹ�  ����ָ�����ȵ�����                  */
/*         1������ʧ��  �������κ�����                      */
/*                                                          */
uint8_t USART1_Tx(uint8_t *buffer,uint32_t len)
{
	return Usart_TX_DMA(&USART1_TX_DMABuffer,buffer,len);
}


/*USART1_RxITHandle����˵��   ����1�����жϷ������        */
/*���������                                                */
/*         data           :���ڽ��յ�����                   */
/*����ֵ��                                                  */
/*         0: �����ɹ�  1������ʧ��                         */
/*                                                          */
uint8_t USART1_RxITHandle(uint8_t data)
{
	return UsartRxBufferPush(&USART1_RX_Buffer,data);
}

/*USART1_TxDMAITHandle����˵��   ����1�����жϷ������      */
/*���������                                                */
/*         data           :���ڽ��յ�����                   */
/*����ֵ��                                                  */
/*                                                          */
void USART1_TxDMAITHandle(void )
{
	USART1_TX_DMABuffer.DMA_Status = USART_DMA_TRANS_DONE;
	Usart_TX_DMA_CheckBuffer(&USART1_TX_DMABuffer);
}

/*Usart_TX_DMA_CheckBuffer����˵��  ͨ��DMA���ʹ�������     */
/*���������                                                */
/*         TxDMABuffer    :���ͻ���ṹ��ָ��               */
/*����ֵ��                                                  */
/*         ��                                               */
/*                                                          */

