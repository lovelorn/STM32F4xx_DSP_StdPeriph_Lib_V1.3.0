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
//	//判断是否为USART1的TX DMA中断
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
//	if(USART_GetITStatus(USART1,USART_IT_RXNE) == SET)//接收中断
//	{
//		RcvData = (uint8_t)(USART1->DR & (uint8_t)0xFF);
//		USART1_RxITHandle(RcvData);
//	}
//	if(USART_GetITStatus(USART1,USART_IT_TC) == SET)//发送中断
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


/*UartRxBufferInit函数说明   对串口的中断式缓存进行初始化   */
/*输入参数：                                                */
/*         UsartRxBuffer  :接收缓存结构体指针               */
/*返回值：                                                  */
/*         无                                               */
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

/*UartRxBufferInit函数说明   对串口的DMA发送缓存进行初始化  */
/*输入参数：                                                */
/*         UsartTxDMABuffer  :发送缓存结构体指针            */
/*返回值：                                                  */
/*         无                                               */
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


/*Usart1Init函数说明  对串口1进行初始化                     */
/*输入参数：                                                */
/*         BandRate  :波特率                                */
/*         PortAlign :管脚位置                              */
/*                   0:PA9-TX  PA10-RX                      */
/*                   1:PB6-TX  PB7 -RX                      */
/*返回值：                                                  */
/*        0:初始化成功    1：初始化失败                     */
/*                                                          */
uint32_t USART1_Init(uint32_t BandRate,uint32_t PortAlign)
{
	NVIC_InitTypeDef Usart1NVIC;
	//启动USART1的APB时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	//启动GPIOA的APB时钟
	
	if(PortAlign == 0x00)
	{
		//配置PA9和PA10为特殊功能引脚?
		GPIO_InitTypeDef USART1_IO;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

		
		USART1_IO.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; 
		USART1_IO.GPIO_Mode = GPIO_Mode_AF;
		USART1_IO.GPIO_OType = GPIO_OType_PP;
		USART1_IO.GPIO_PuPd = GPIO_PuPd_NOPULL;
		USART1_IO.GPIO_Speed = GPIO_Medium_Speed;
		GPIO_Init(GPIOA,&USART1_IO);
		//将PA9和PA10与USART1关联起来
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);
	}
	else if(PortAlign == 0x01)
	{
		//配置PA9和PA10为特殊功能引脚?
		GPIO_InitTypeDef USART1_IO;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
		
		USART1_IO.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; 
		USART1_IO.GPIO_Mode = GPIO_Mode_AF;
		USART1_IO.GPIO_OType = GPIO_OType_PP;
		USART1_IO.GPIO_PuPd = GPIO_PuPd_NOPULL;
		USART1_IO.GPIO_Speed = GPIO_Medium_Speed;
		GPIO_Init(GPIOB,&USART1_IO);
		//将PA9和PA10与USART1关联起来
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_USART1);
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource7,GPIO_AF_USART1);			
	}
	else //管脚约束错误，关闭时钟
	{
		//启动USART1的APB时钟
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
		//启动GPIOA的APB时钟
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, DISABLE);	
		return 1;
	}
	
	//设置串口参数
	USART_InitTypeDef USART1_Init;
	USART1_Init.USART_BaudRate = BandRate;
	USART1_Init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART1_Init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART1_Init.USART_Parity = USART_Parity_No;
	USART1_Init.USART_StopBits = USART_StopBits_1;
	USART1_Init.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1,&USART1_Init);

	
	//开始串口中断
	Usart1NVIC.NVIC_IRQChannel = USART1_IRQn;
	Usart1NVIC.NVIC_IRQChannelPreemptionPriority = 0;
	Usart1NVIC.NVIC_IRQChannelSubPriority = 0;
	Usart1NVIC.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&Usart1NVIC);
	
	//开启USART1的接收数据中断使能
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE );
	
	//开启DMA2的时钟使能
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);	

	
	//初始化串口接收缓存和发送DMA缓存
	UsartRxBufferInit(&USART1_RX_Buffer);
	UsartTxDMABufferInit(&USART1_TX_DMABuffer);
	
	//开启USART1
	USART_Cmd(USART1,ENABLE);	
	
	memset((void *)TxBuffer,0x32,128);

	return 0;
}


/*UsartRxBufferPush函数说明   往串口接收缓存中压入一个数据  */
/*输入参数：                                                */
/*         UsartRxBuffer  :接收缓存结构体指针               */
/*         data           :串口接收的数据                   */
/*返回值：                                                  */
/*         0: 操作成功  1：操作失败                         */
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

/*UsartRxBufferRcv函数说明   从串口接收缓存中读取数据       */
/*输入参数：                                                */
/*         UsartRxBuffer  :接收缓存结构体指针               */
/*         buffer         :串口接收缓存                     */
/*         len            :计划读取的数据长度               */
/*返回值：                                                  */
/*         0: 操作成功  1：操作失败                         */
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


/*Usart_TX_DMA_CheckBuffer函数说明  通过DMA发送串口数据     */
/*输入参数：                                                */
/*         TxDMABuffer    :发送缓存结构体指针               */
/*返回值：                                                  */
/*         无                                               */
/*                                                          */
void Usart_TX_DMA_CheckBuffer(USART_TXDMABufferType *TxDMABuffer)
{
	DMA_InitTypeDef USART1_DMA_InitStructure;
	NVIC_InitTypeDef DMA2NVIC;
	//uint32_t IsDMATxFinished = (DMA_GetFlagStatus(DMA2_Stream7,DMA_IT_TCIF7) == RESET);
	
	if(TxDMABuffer->DMA_Status == USART_DMA_TRANS_DONE)//发送完成一个数据包，调整指针
	{
		//调整队列的读指针
		TxDMABuffer->Tail++;
		if(TxDMABuffer->Tail == MAX_USART_TX_NUM_OF_BLOCKS)
		{
			TxDMABuffer->Tail = 0;
		}
		//调整队列个数
		TxDMABuffer->Count--;
	}
	
	if(TxDMABuffer->Count > 0)
	{
		if(TxDMABuffer->DMA_Status == USART_DMA_IDLE || TxDMABuffer->DMA_Status == USART_DMA_TRANS_DONE)//第一次发送或者DMA中断完成
		{
			//从缓存中读一个包，并且准备给DMA发送
			uint32_t index = TxDMABuffer->Tail;			
			
			//初始化DMA的内存首地址和数据个数			
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
			USART1_DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full ;//不起作用
			USART1_DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
			USART1_DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
			DMA_Init(DMA2_Stream7,&USART1_DMA_InitStructure);
		
			//开启DMA中断使能
			DMA_ITConfig(DMA2_Stream7,DMA_IT_TC,ENABLE);		
			
			DMA_Cmd(DMA2_Stream7,ENABLE);	
			while (DMA_GetCmdStatus(DMA2_Stream7) != ENABLE)
			{
			}		
			//允许USART1的发送DMA
			USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);		
			//开启DMA发送完成中断
			DMA2NVIC.NVIC_IRQChannel = DMA2_Stream7_IRQn;
			DMA2NVIC.NVIC_IRQChannelCmd = ENABLE;
			DMA2NVIC.NVIC_IRQChannelPreemptionPriority = 0;
			DMA2NVIC.NVIC_IRQChannelSubPriority = 0;
			NVIC_Init(&DMA2NVIC);		

			//修改DMA状态
			TxDMABuffer->DMA_Status = USART_DMA_TRANSMITTING;			
		}
		else if(TxDMABuffer->DMA_Status == USART_DMA_TRANSMITTING)
		{
			//正在发送，暂时不做任何处理
			return;
		}
	}
	else if(TxDMABuffer->Count == 0)
	{
		if(TxDMABuffer->DMA_Status == USART_DMA_IDLE)//第一次发送
		{
			//什么情况会进入这种模式？？？
			return;
		}
		else if(TxDMABuffer->DMA_Status == USART_DMA_TRANS_DONE)//DMA中断处理
		{
			//缓存区的全部数据包都发完了，休息休息
			DMA_Cmd(DMA2_Stream7,DISABLE);	
			TxDMABuffer->DMA_Status = USART_DMA_IDLE;
			return;
		}
		else if(TxDMABuffer->DMA_Status == USART_DMA_TRANSMITTING)
		{
			//不可能进入这种状态
			return;
		}	
	}
	
	
	
	//如果正在进行DMA发送，则返回
	if(TxDMABuffer->DMA_Status == USART_DMA_TRANSMITTING)
	{
		return ;
	}
	//如果队列为空，则返回
	if(TxDMABuffer->Count == 0 && (TxDMABuffer->DMA_Status != USART_DMA_TRANSMITTING))
	{
		DMA_Cmd(DMA2_Stream7,DISABLE);	
		TxDMABuffer->DMA_Status = USART_DMA_IDLE;
		return ;
	}
	//如果DMA发送完成，并且队列不为空，则启动DMA传输
	if((TxDMABuffer->DMA_Status != USART_DMA_TRANSMITTING) && TxDMABuffer->Count > 0)
	{
		uint32_t index = TxDMABuffer->Tail;

		//调整队列的读指针
		TxDMABuffer->Tail++;
		if(TxDMABuffer->Tail == MAX_USART_TX_NUM_OF_BLOCKS)
		{
			TxDMABuffer->Tail = 0;
		}
		//调整队列个数
		TxDMABuffer->Count--;
		
		//初始化DMA的内存首地址和数据个数
		
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
		USART1_DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full ;//不起作用
		USART1_DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
		USART1_DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
		DMA_Init(DMA2_Stream7,&USART1_DMA_InitStructure);
	
		//开启DMA中断使能
		DMA_ITConfig(DMA2_Stream7,DMA_IT_TC,ENABLE);		
		
		DMA_Cmd(DMA2_Stream7,ENABLE);	
		while (DMA_GetCmdStatus(DMA2_Stream7) != ENABLE)
		{
		}		
		//允许USART1的发送DMA
		USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);		
		//开启DMA发送完成中断
		DMA2NVIC.NVIC_IRQChannel = DMA2_Stream7_IRQn;
		DMA2NVIC.NVIC_IRQChannelCmd = ENABLE;
		DMA2NVIC.NVIC_IRQChannelPreemptionPriority = 0;
		DMA2NVIC.NVIC_IRQChannelSubPriority = 0;
		NVIC_Init(&DMA2NVIC);		
	}	
}


/*Usart1_TX_DMA函数说明   从串口1采用DMA发送数据            */
/*输入参数：                                                */
/*         TxDMABuffer  :发送队列指针                      */
/*         Buffer       :发送缓存区指针                     */
/*         size         :数据长度                           */
/*返回值：                                                  */
/*         0: 操作成功  1：操作失败                         */
/*                                                          */
uint8_t Usart_TX_DMA(USART_TXDMABufferType *TxDMABuffer,uint8_t *Buffer, uint32_t size)
{
	uint32_t i = 0;
	uint32_t index = 0;
	if(size == 0)
	{
		return 1;
	}
	//如果队列中没有足够的空间，则操作失败
	if((TxDMABuffer->Count + (size -1)/MAX_USART_TX_BLOCK_SIZE +1) > MAX_USART_TX_NUM_OF_BLOCKS)
	{
		return 1;
	}
	//将数据放入队列中
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
	

	//进行DMA发送
	Usart_TX_DMA_CheckBuffer(TxDMABuffer);
	
	return 0;
}

/*USART1_Rx函数说明   从串口接收缓存中读取数据              */
/*输入参数：                                                */
/*         buffer         :串口接收缓存                     */
/*         len            :计划读取的数据长度               */
/*返回值：                                                  */
/*         0: 操作成功  返回指定长度的数据                  */
/*         1：操作失败  不返回任何数据                      */
/*                                                          */
uint8_t USART1_Rx(uint8_t *buffer,uint32_t len)
{
	return UsartRxBufferRcv(&USART1_RX_Buffer,buffer,len);
}


/*USART1_Tx函数说明   从串口1采用DMA发送数据                */
/*输入参数：                                                */
/*         Buffer       :发送缓存区指针                     */
/*         size         :数据长度                           */
/*返回值：                                                  */
/*         0: 操作成功  发送指定长度的数据                  */
/*         1：操作失败  不发送任何数据                      */
/*                                                          */
uint8_t USART1_Tx(uint8_t *buffer,uint32_t len)
{
	return Usart_TX_DMA(&USART1_TX_DMABuffer,buffer,len);
}


/*USART1_RxITHandle函数说明   串口1接收中断服务程序        */
/*输入参数：                                                */
/*         data           :串口接收的数据                   */
/*返回值：                                                  */
/*         0: 操作成功  1：操作失败                         */
/*                                                          */
uint8_t USART1_RxITHandle(uint8_t data)
{
	return UsartRxBufferPush(&USART1_RX_Buffer,data);
}

/*USART1_TxDMAITHandle函数说明   串口1发送中断服务程序      */
/*输入参数：                                                */
/*         data           :串口接收的数据                   */
/*返回值：                                                  */
/*                                                          */
void USART1_TxDMAITHandle(void )
{
	USART1_TX_DMABuffer.DMA_Status = USART_DMA_TRANS_DONE;
	Usart_TX_DMA_CheckBuffer(&USART1_TX_DMABuffer);
}

/*Usart_TX_DMA_CheckBuffer函数说明  通过DMA发送串口数据     */
/*输入参数：                                                */
/*         TxDMABuffer    :发送缓存结构体指针               */
/*返回值：                                                  */
/*         无                                               */
/*                                                          */

