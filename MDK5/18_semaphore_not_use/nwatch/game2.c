/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */
#include <stdlib.h>
#include <stdio.h>

#include "cmsis_os.h"
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "task.h"                       // ARM.FreeRTOS::RTOS:Core
#include "event_groups.h"               // ARM.FreeRTOS::RTOS:Event Groups
#include "semphr.h"                     // ARM.FreeRTOS::RTOS:Core

#include "draw.h"
#include "resources.h"

#include "driver_lcd.h"
#include "driver_ir_receiver.h"
#include "driver_rotary_encoder.h"
#include "driver_mpu6050.h"

#define NOINVERT	false
#define INVERT		true
	
#define CAR_COUNT	3
#define CAR_WIDTH	12
#define CAR_LENGTH	15
#define ROAD_SPEED	6


static uint32_t g_xres, g_yres, g_bpp;
static uint8_t *g_framebuffer;

static uint32_t car1_x, car2_x, car3_x;
	
static const byte carImg[]  = {
	0x40,0xF8,0xEC,0x2C,0x2C,0x38,0xF0,0x10,0xD0,0x30,0xE8,0x4C,0x4C,0x9C,0xF0,
	0x02,0x1F,0x37,0x34,0x34,0x1C,0x0F,0x08,0x0B,0x0C,0x17,0x32,0x32,0x39,0x0F,
};	//两行，所以宽度是16 


static const byte roadMarking[]  = {
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
};	//1行，但是只用到一位数，所以宽度是1

static const byte clearImg[30] = {0};

static QueueHandle_t g_xQueueIR;
static QueueHandle_t g_xQueueCar;

void input_car_task(void)
{
	struct ir_data idata;
	struct input_data input;

	while(1){
		
		xQueueReceive(g_xQueueIR, &idata, portMAX_DELAY);	
		
		if(idata.val == IR_KEY_1){
			input.dev = 1;
			input.val = 4;
		}
		else if(idata.val == IR_KEY_2){
			input.dev = 2;
			input.val = 4;
		}
		else if(idata.val == IR_KEY_3){
			input.dev = 3;
			input.val = 4;
		}
		else{
			input.dev = 0;	
		}
		
		/*写入到汽车显示队列*/
		xQueueSendToBack(g_xQueueCar,&input,0);
	}
}

void move_car1(){
	if(car1_x < 128-16){
		/*清除之前的小车图像*/
		draw_bitmap(car1_x, 2, clearImg, 15, 16, NOINVERT, 0);
		draw_flushArea(car1_x, 2, 15, 16);
		
		car1_x += 8;
		
		/*画出新的小车图像*/
		draw_bitmap(car1_x, 2, carImg, 15, 16, NOINVERT, 0);
		draw_flushArea(car1_x, 2, 15, 16);
	}
}

void move_car2(){
	if(car2_x < 128-16){
		/*清除之前的小车图像*/
		draw_bitmap(car2_x, 21, clearImg, 15, 16, NOINVERT, 0);
		draw_flushArea(car2_x, 21, 15, 16);
		
		car2_x += 8;
		
		/*画出新的小车图像*/
		draw_bitmap(car2_x, 21, carImg, 15, 16, NOINVERT, 0);
		draw_flushArea(car2_x, 21, 15, 16);
	}
}

void move_car3(){
	if(car3_x < 128-16){
		/*清除之前的小车图像*/
		draw_bitmap(car3_x, 41, clearImg, 15, 16, NOINVERT, 0);
		draw_flushArea(car3_x, 41, 15, 16);
		
		car3_x += 1;
		
		/*画出新的小车图像*/
		draw_bitmap(car3_x, 41, carImg, 15, 16, NOINVERT, 0);
		draw_flushArea(car3_x, 41, 15, 16);
	}

}



void draw_line(){
	uint8_t road_len ;
	uint8_t cnt = 16;
	uint8_t i;
	
	road_len = 0;
	for(i = 0; i< cnt; i++){
		draw_bitmap(road_len, 19, roadMarking, 8, 1, NOINVERT, 0);
		draw_flushArea(road_len, 19, 8, 8);
		
		draw_bitmap(road_len, 39, roadMarking, 8, 1, NOINVERT, 0);
		draw_flushArea(road_len, 39, 8, 8);
		road_len += 8;
		
	}
}

void car_game(void)
{

	struct input_data input;
	
	g_framebuffer = LCD_GetFrameBuffer(&g_xres, &g_yres, &g_bpp);
	draw_init();
	draw_end();
	g_xQueueIR = GetQueueIR();
	g_xQueueCar = xQueueCreate(10,sizeof(struct input_data));
	
	xTaskCreate(input_car_task, "InputTask", 128, NULL, osPriorityNormal, NULL);
	// 初始小车的位置
	
	car1_x = 0;
	car2_x = 0;
	car3_x = 0;
	
	draw_bitmap(car1_x, 2, carImg, 15, 16, NOINVERT, 0);
	draw_flushArea(car1_x, 2, 15, 16);
		
	draw_bitmap(car2_x, 21, carImg, 15, 16, NOINVERT, 0);
	draw_flushArea(car2_x, 21, 15, 16);
	
	draw_bitmap(car3_x, 41, carImg, 15, 16, NOINVERT, 0);
	draw_flushArea(car3_x, 41, 15, 16);
	
	while(1){
		draw_line();
		
		//xQueueReceive(g_xQueueCar, &input, portMAX_DELAY);
		/*
		if(input.dev == 1){
			move_car1();
		}
		else if(input.dev == 2){
			move_car2();
		}
		else if(input.dev == 3){
			move_car3();
		}
		*/
		move_car1();
		move_car2();
		move_car3();
		
		vTaskDelay(50);
		
		if(car3_x >= 112 || car2_x >= 112 || car1_x >= 112){
			vTaskDelete(NULL);		
		}
		

	}

}
	
	
#if 0
void car_test(void)
{
	g_framebuffer = LCD_GetFrameBuffer(&g_xres, &g_yres, &g_bpp);
    draw_init();
    draw_end();
	
	draw_bitmap(0, 0, carImg, 15, 16, NOINVERT, 0);
    draw_flushArea(0, 0, 15, 16);
	
	draw_bitmap(0, 16, roadMarking, 8, 1, NOINVERT, 0);
    draw_flushArea(0, 16, 8, 8);
	while(1);
}
# endif