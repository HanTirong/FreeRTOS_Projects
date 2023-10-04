/* USER CODE BEGIN Header */
#include "driver_led.h"
#include "driver_lcd.h"
#include "driver_mpu6050.h"
#include "driver_timer.h"
#include "driver_ds18b20.h"
#include "driver_dht11.h"
#include "driver_active_buzzer.h"
#include "driver_passive_buzzer.h"
#include "driver_color_led.h"
#include "driver_ir_receiver.h"
#include "driver_ir_sender.h"
#include "driver_light_sensor.h"
#include "driver_ir_obstacle.h"
#include "driver_ultrasonic_sr04.h"
#include "driver_spiflash_w25q64.h"
#include "driver_rotary_encoder.h"
#include "driver_motor.h"
#include "driver_key.h"
#include "driver_uart.h"

/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static StackType_t g_pucStackOfLightTask[128];	/*Pointer to Unsigned Char*/
static StaticTask_t g_TCBofLightTask;
static TaskHandle_t xLightTaskHandle;

static StackType_t g_pucStackOfColorLEDTask[128];	/*Pointer to Unsigned Char*/
static StaticTask_t g_TCBofColorTask;
static TaskHandle_t xColorTaskHandle;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
struct TaskPrintInfo{
	uint8_t x;
	uint8_t y;
	char name[16];
	
};
static int g_LCDCanUse = 1;		/*保护：防止任务在I2C传输过程中被打断*/
struct TaskPrintInfo Task1Info = {0,0,"Task1"};
struct TaskPrintInfo Task2Info = {0,3,"task2"};
struct TaskPrintInfo Task3Info = {0,6,"TASK3"};
static volatile int g_calc_end = 0;	/*		标志位, 如果不加volatile，则该变量会被优化，
							存入寄存器，每次都从寄存器中去取值，即使该值被更新了，
								仍然从寄存器中读取到原来旧的值		*/
static uint64_t g_time = 0;
static 	uint32_t  g_sum = 0;

void CalculateTask(void *parms){
	
	uint32_t  i = 0;

	g_time = system_get_ns();
	for(;i < 1000000; i++){
		
		g_sum += i;	
	}
	g_calc_end = 1;
	g_time = system_get_ns() - g_time;
	vTaskDelete(NULL);
}


void LCDShowTask(void){
	uint32_t len = 0;
	while(1){
		LCD_PrintString(0, 0, "Waiting");
		vTaskDelay(3000);
		while(g_calc_end == 0);	 /*占用CPU资源*/
		/*打印信息*/
		if(g_LCDCanUse){
			g_LCDCanUse = 0;
			
			LCD_ClearLine(0, 0);
			len = LCD_PrintString(0, 0, "sum:");
			len += LCD_PrintHex(len,0,g_sum,1);
		
			LCD_ClearLine(0, 2);
			len = LCD_PrintString(0, 2, "time(ms):");
			len += LCD_PrintSignedVal(0,5,g_time/100000);
		}
		vTaskDelete(NULL);
	}
	
}
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	TaskHandle_t xSoundTaskHandle;
	BaseType_t ret;

	LCD_Init();
	LCD_Clear();
	/* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
//  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /*创建任务：声*/
  extern void PlayMusic(void *parm);
 // ret = xTaskCreate(PlayMusic, "MusicTask", 128, NULL, osPriorityNormal,&xSoundTaskHandle);
  
  
  /*创建任务：光*/
  //xLightTaskHandle = xTaskCreateStatic(Led_Test, "LightTask", 128, NULL, osPriorityNormal,g_pucStackOfLightTask,&g_TCBofLightTask);
 
  
  /*创建任务：色*/
  //xColorTaskHandle = xTaskCreateStatic(ColorLED_Test, "ColorTask", 128, NULL, osPriorityNormal,g_pucStackOfColorLEDTask,&g_TCBofColorTask);
  
  /*使用同一个函数，创建不同的任务*/
  /*任务1：计算一个数值*/
  ret = xTaskCreate(CalculateTask, "Task1", 128, NULL, osPriorityNormal,NULL);
  /*任务2：将计算结果显示出来*/
  ret = xTaskCreate(LCDShowTask, "Task2", 128, &Task2Info, osPriorityNormal,NULL);
  //ret = xTaskCreate(LcdPrintTask, "Task3", 128, &Task3Info, osPriorityNormal,NULL);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  LCD_Init();
  LCD_Clear();
  
  for(;;)
  {
    //Led_Test();
    //LCD_Test();
	//MPU6050_Test(); 
	//DS18B20_Test();
	//DHT11_Test();
	//ActiveBuzzer_Test();
	//PassiveBuzzer_Test();
	//ColorLED_Test();
	//IRReceiver_Test();	/*影*/
	//IRSender_Test();
	//LightSensor_Test();
	//IRObstacle_Test();
	//SR04_Test();
	//W25Q64_Test();
	//RotaryEncoder_Test();
	//Motor_Test();
	//Key_Test();
	//UART_Test();
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

