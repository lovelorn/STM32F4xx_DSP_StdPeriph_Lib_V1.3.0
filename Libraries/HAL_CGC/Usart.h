#ifndef __USART_H__
#define __USART_H__
#include "stm32f4xx.h"

#define MAX_USART_RX_BUFFER_DEPTH     ((uint32_t)256)
#define MAX_USART_TX_NUM_OF_BLOCKS    ((uint32_t)64)
#define MAX_USART_TX_BLOCK_SIZE       ((uint32_t)32)


typedef enum {
	USART_DMA_TRANSMITTING,
	USART_DMA_TRANS_DONE,
	USART_DMA_IDLE
} USART_DMA_STATUS;


typedef struct
{
	uint8_t Buffer[MAX_USART_RX_BUFFER_DEPTH];
	uint32_t Head;
	uint32_t Tail;
	uint32_t Count;
} USART_RxCircleBufferType;

typedef struct
{
	uint8_t Buffer[MAX_USART_TX_NUM_OF_BLOCKS][MAX_USART_TX_BLOCK_SIZE];
	uint8_t Block_Used_Size[MAX_USART_TX_NUM_OF_BLOCKS];//如果小于等于0，则代表对应的缓存块为空，如果大于0，则代表缓存块中保存的数据个数。
	uint32_t Head;
	uint32_t Tail;
	uint32_t Count;
	USART_DMA_STATUS DMA_Status;
} USART_TXDMABufferType;




//extern USART_RxCircleBufferType USART1_RX_Buffer;
//extern USART_TXDMABufferType    USART1_TX_DMABuffer;

extern uint32_t USART1_Init(uint32_t BandRate,uint32_t PortAlign);

//extern void UsartRxBufferInit(USART_RxCircleBufferType *UsartRxBuffer);

//extern void UsartTxDMABufferInit(USART_TXDMABufferType *UsartTxDMABuffer);

//extern uint8_t UsartRxBufferPush(USART_RxCircleBufferType *UsartRxBuffer,uint8_t data);

//extern uint8_t UsartRxBufferRcv(USART_RxCircleBufferType *UsartRxBuffer,uint8_t *buffer, uint32_t len);

//extern uint8_t Usart_TX_DMA(USART_TXDMABufferType *TxDMABuffer,uint8_t *Buffer, uint32_t size);

//extern void Usart_TX_DMA_CheckBuffer(USART_TXDMABufferType *TxDMABuffer);

extern uint8_t USART1_RxITHandle(uint8_t data);

extern void USART1_TxDMAITHandle(void);


extern uint8_t USART1_Rx(uint8_t *buffer,uint32_t len);

extern uint8_t USART1_Tx(uint8_t *buffer,uint32_t len);


#endif
