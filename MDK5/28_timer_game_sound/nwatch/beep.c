#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmsis_os.h"
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "task.h"                       // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h"               // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"                     // ARM.FreeRTOS::RTOS:Core

#include "driver_passive_buzzer.h"


static TimerHandle_t g_TimerSound;

void GameSoundTimer_Func( TimerHandle_t xTimer )
{
	
	PassiveBuzzer_Control(0);
	
}
int buzzer_init(void)
{
	/*初始化蜂鸣器*/
	PassiveBuzzer_Init();
	
	/*创建一个定时器*/
	g_TimerSound = xTimerCreate("GameSound", 
								200,
								pdFALSE,
								NULL,
								GameSoundTimer_Func);
	
}



void buzzer_buzz(int freq, int time_ms)
{
	PassiveBuzzer_Set_Freq_Duty(freq, 50);	
	/*启动定时器*/
	xTimerChangePeriod(g_TimerSound, time_ms, 0);

}


