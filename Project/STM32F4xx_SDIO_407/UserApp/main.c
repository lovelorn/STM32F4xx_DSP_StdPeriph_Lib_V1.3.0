/**
  ******************************************************************************
  * @file    SDIO/SDIO_uSDCard/main.c 
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    13-November-2013
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "usart.h"
#include "string.h"
#include "ff.h"
#include "diskio.h"
#include "stdio.h"
#include "rtc.h"
#include "ucos_ii.h"
#include "stdlib.h"
#include "lcd.h"
/** @addtogroup STM32F4xx_StdPeriph_Examples
  * @{
  */

/** @addtogroup SDIO_uSDCard
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

/* Private define ------------------------------------------------------------*/
#define SD_BUFFER_SIZE    16384
#define APP_TASK0_STK_SIZE				512
#define APP_TASK0_PRIO					8
#define FATFS_TASK_PROD                 9
#define LCD_TASK_PROD                   10

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static OS_STK		App_Task0Stack[APP_TASK0_STK_SIZE];
static OS_STK		Fatfs_TestTaskStack[APP_TASK0_STK_SIZE];
static OS_STK		LCD_TestTaskStack[APP_TASK0_STK_SIZE];

const uint32_t SD_SPEED_UNIT[8] = {100,1000,10000,100000,0,0,0,0};//单位是Kb/s
const uint8_t SD_SPEED_VALUEX10[16] = {0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};//数值为了保存方便，都乘以10了，在计算时，需要除以10

/* Private function prototypes -----------------------------------------------*/
void RTC_TimeShow(void);

static void App_Task0(void *p_arg) ;
static void  Fatfs_TestTask(void *p_arg) ;
static void LCD_Display(void *p_arg);


/* Private functions ---------------------------------------------------------*/
int8_t msg1[] = "STM32F407 Usart & SDIO/SD Card/Fatfs Base on uCOS-II Test Program\r\n";
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
 	uint8_t os_err;

	__disable_irq(); //禁止所有中断

	OSInit(); //初始化uCOS-II实时内核

	os_err = OSTaskCreateExt((void (*)(void *)) App_Task0, //创建一个初始任务
                             (void          * ) 0,
                             (OS_STK        * )&App_Task0Stack[APP_TASK0_STK_SIZE - 1],
                             (uint8_t         ) APP_TASK0_PRIO,
                             (uint16_t        ) APP_TASK0_PRIO,
                             (OS_STK        * )&App_Task0Stack[0],
                             (INT32U          ) APP_TASK0_STK_SIZE,
                             (void          * )0,
                             (uint16_t        )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
	if(os_err == OS_ERR_NONE)
	{
		OSTaskNameSet(APP_TASK0_PRIO, (uint8_t *)"Start Task", &os_err); //给新任务命名
	}
	
    OSStart(); //启动uCOS-II内核
}


/* 初始任务*/
static void App_Task0(void *p_arg) 
{	
 	uint8_t os_err;
	
	(void) p_arg;
	
	OS_CPU_SysTickInit();
	
	USART1_Init(115200,0);
	
	USART1_Tx((uint8_t *)msg1,strlen((const char *)msg1));
	
	/* Initialize LEDs available on EVAL board */
	STM_EVAL_LEDInit(LED1);
	STM_EVAL_LEDInit(LED2);
	STM_EVAL_LEDInit(LED3);
	//STM_EVAL_LEDInit(LED4);  
	

	if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x32F2)
	{  
		/* RTC configuration  */
		RTC_Config();
		/* Display the RTC Time and Alarm */
		RTC_TimeShow();
	}
	else
	{
		/* Check if the Power On Reset flag is set */
		if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
		{
			/* Power On Reset occurred     */
			STM_EVAL_LEDOn(LED3);
		}
		/* Check if the Pin Reset flag is set */
		else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
		{
			/* External Reset occurred */
			//STM_EVAL_LEDOn(LED4);
		}

		/* Enable the PWR clock */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

		/* Allow access to RTC */
		PWR_BackupAccessCmd(ENABLE);

		/* Wait for RTC APB registers synchronisation */
		RTC_WaitForSynchro();
		
		/* Clear the RTC Alarm Flag */
		RTC_ClearFlag(RTC_FLAG_ALRAF);
		
		/* Display the RTC Time and Alarm */
		RTC_TimeShow();
	}		

	os_err = OSTaskCreateExt((void (*)(void *)) Fatfs_TestTask, //创建一个初始任务
                             (void          * ) 0,
                             (OS_STK        * )&Fatfs_TestTaskStack[APP_TASK0_STK_SIZE - 1],
                             (uint8_t         ) FATFS_TASK_PROD,
                             (uint16_t        ) FATFS_TASK_PROD,
                             (OS_STK        * )&Fatfs_TestTaskStack[0],
                             (INT32U          ) APP_TASK0_STK_SIZE,
                             (void          * )0,
                             (uint16_t        )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
	if(os_err == OS_ERR_NONE)
	{
		OSTaskNameSet(FATFS_TASK_PROD, (uint8_t *)"Fatfs Test Task", &os_err); //给新任务命名
	}
	

	os_err = OSTaskCreateExt((void (*)(void *)) LCD_Display, //创建一个初始任务
                             (void          * ) 0,
                             (OS_STK        * )&LCD_TestTaskStack[APP_TASK0_STK_SIZE - 1],
                             (uint8_t         ) LCD_TASK_PROD,
                             (uint16_t        ) LCD_TASK_PROD,
                             (OS_STK        * )&LCD_TestTaskStack[0],
                             (INT32U          ) APP_TASK0_STK_SIZE,
                             (void          * )0,
                             (uint16_t        )(OS_TASK_OPT_STK_CLR | OS_TASK_OPT_STK_CHK));
	if(os_err == OS_ERR_NONE)
	{
		OSTaskNameSet(LCD_TASK_PROD, (uint8_t *)"LCD Test Task", &os_err); //给新任务命名
	}
	


	while(1)
	{
		OSTimeDly(1000);
		//RTC_TimeShow();
	}
	


}



static void  Fatfs_TestTask(void *p_arg)
{
	uint32_t i;
	FATFS fs;
	FILINFO fno;
	DIR dir;
	FIL File;
	uint32_t byteswritted;
	FRESULT  fresult;
	//DSTATUS diskresult;
	int8_t *stringPoint;
	static int8_t buffer[SD_BUFFER_SIZE];

	uint32_t StartTime,EndTime;	

	SD_CardInfo sdcardinfo;

	uint32_t SDCardCap_MB = 0;
	uint32_t SDCardSpeed = 0;
	
	(void) p_arg;
	

	while(1)
	{
	    if(SD_Detect() != SD_PRESENT)
	    {
			stringPoint = "\r\nSD Card Not Present, Waiting for SD Inserted\r\n";
			USART1_Tx((uint8_t *)stringPoint,strlen((const char *)stringPoint));
			OSTimeDly(2000);
			continue;
	    }

		stringPoint = "\r\nSD Card inserted, press 's' to start the SD Card File write speed test\r\n#:";
		USART1_Tx((uint8_t *)stringPoint,strlen((const char *)stringPoint));

		if(USART1_Rx((uint8_t *)buffer,1) == 1 )//没有收到任何数据，则返回
		{
			OSTimeDly(2000);
			continue;
		}
		if(buffer[0] != 's')//没有接收到's'，则返回
		{
			OSTimeDly(2000);
			continue;
		}

		stringPoint = "\r\nStart to Test SD file Write speed\r\n#:";
		USART1_Tx((uint8_t *)stringPoint,strlen((const char *)stringPoint));
		
		if(f_mount(0, &fs) == FR_OK)
		{
			stringPoint = "Successfully Mount the SD Card\r\n";
			USART1_Tx((uint8_t *)stringPoint,strlen((const char *)stringPoint));
			
		}
		else 
		{
			stringPoint = "Mount the SD Card Failed,please try another card.\r\n";
			USART1_Tx((uint8_t *)stringPoint,strlen((const char *)stringPoint));
			while(SD_Detect() == SD_PRESENT)
			{

			}
		}
		
		fresult = f_open(&File,"lala.txt",FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
		if( fresult== FR_OK)
		{
			SD_GetCardInfo(&sdcardinfo);
			SDCardCap_MB = sdcardinfo.CardCapacity/1024/1024;
			SDCardSpeed = SD_SPEED_UNIT[sdcardinfo.SD_csd.MaxBusClkFrec&0x07];
			SDCardSpeed *= SD_SPEED_VALUEX10[(sdcardinfo.SD_csd.MaxBusClkFrec>>3)&0x0F];
			SDCardSpeed /= 10;
			
			sprintf((char *)buffer,"SD Card Capacity = %d MB.\r\n",SDCardCap_MB);
			USART1_Tx((uint8_t *)buffer,strlen((const char *)buffer));	

			sprintf((char *)buffer,"SD Card Support CLK Freq = %d KHz.\r\n",SDCardSpeed);
			USART1_Tx((uint8_t *)buffer,strlen((const char *)buffer));	
			
		
			stringPoint = "Successfully Open the File 'lala.txt'\r\n";
			USART1_Tx((uint8_t *)stringPoint,strlen((const char *)stringPoint));
		}
		else 
		{
			stringPoint = "unknown error. Failed to open the File 'lala.txt',,please try another card.\r\n";
			USART1_Tx((uint8_t *)stringPoint,strlen((const char *)stringPoint));
			while(SD_Detect() == SD_PRESENT)
			{

			}			
		}
		for(i =0; i < SD_BUFFER_SIZE;i++)
		{
			buffer[i] = '0'+ i % 10;
		}		
		
		StartTime = OSTimeGet();
		for(i = 0; i < 5000;i++)
		{	
			fresult = f_write (&File,(void *)buffer,SD_BUFFER_SIZE,&byteswritted);
			if(fresult != FR_OK)
				break;
		}
		EndTime = OSTimeGet();
		
		if( fresult == FR_OK)
		{
			//sprintf((char *)buffer,"try to write %d bytes of string, actual writed %d bytes\r\n",512,byteswritted);
			//stringPoint = "Successfully Open the File 'lalalalalala.txt'";
			//USART1_Tx((uint8_t *)buffer,strlen((const char *)buffer));
			sprintf((char *)buffer,"buffer size = %d byte,Write %d bytes of data, cost %d ms.\r\nAverage Speed = %dKB/s\r\n",SD_BUFFER_SIZE,i*SD_BUFFER_SIZE,(EndTime - StartTime),((i*SD_BUFFER_SIZE)/(EndTime - StartTime)));
			USART1_Tx((uint8_t *)buffer,strlen((const char *)buffer));			
		}
		else 
		{
			stringPoint = "unknown error. Failed to wirte the File 'lala.txt',,please try another card.\r\n";
			USART1_Tx((uint8_t *)stringPoint,strlen((const char *)stringPoint));
			while(SD_Detect() == SD_PRESENT)
			{

			}
		}
		f_close (&File);
		f_mount(0,NULL);

		sprintf((char *)buffer,"End of Testing FatFS on SDIO mode of DMA!\r\n");
		USART1_Tx((uint8_t *)buffer,strlen((const char *)buffer));
		//// end of fatfs testing

	
		OSTimeDly(2000);
	}


}


static void LCD_Display(void *p_arg)
{
	static int8_t LCD_Buffer[100];
	static RTC_TimeTypeDef  RTC_TimeStructure;										 


	(void) p_arg;

	LCD_Init();
	LCD_Clear(BLUE);
	LCD_String(20,20,"LCD FSMC Display",RED);
	LCD_Line(60,50,200,300,YELLOW);
	LCD_Rect(80,150,200,270,GREEN);
	LCD_Circle(150,100,60,WHITE);
	
	
	while(1)
	{
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		sprintf((char*)LCD_Buffer,"%0.2d:%0.2d:%0.2d",RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
		LCD_String(20,40,(u8 *)LCD_Buffer,RED);		
		OSTimeDly(1000);
	}

}



void RTC_TimeShow(void)
{
	uint8_t buffer[100];
	RTC_TimeTypeDef  RTC_TimeStructure;										 
	
	/* Get the current Time */
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	/* Display time Format : hh:mm:ss */
	sprintf((char*)buffer,"%0.2d:%0.2d:%0.2d\r\n",RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
	USART1_Tx((uint8_t *)buffer,strlen((const char *)buffer));
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
