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
#include "beep.h"
#include "driver_dht11.h"

#define NOINVERT	false
#define INVERT		true

#define sprintf_P  sprintf
#define PSTR(a)  a

#define PLATFORM_WIDTH	12
#define PLATFORM_HEIGHT	4

#define BLOCK_COLS		32
#define BLOCK_ROWS		5
#define BLOCK_COUNT		(BLOCK_COLS * BLOCK_ROWS)

typedef struct{
	float x;
	float y;
	float velX;
	float velY;
}s_ball;

static const byte block[] ={
	0x07,0x07,0x07,
};

static const byte platform[] ={
	0x60,0x70,0x50,0x10,0x30,0xF0,0xF0,0x30,0x10,0x50,0x70,0x60,
};

static const byte ballImg[] ={
	0x03,0x03,
};

static const byte clearImg[] ={
	0,0,0,0,0,0,0,0,0,0,0,0,
};

static bool btnExit(void);
static bool btnRight(void);
static bool btnLeft(void);
void game1_draw(void);

static byte uptMove;
static s_ball ball;
static bool* blocks;
static byte lives, lives_origin;
static uint score;
static byte platformX;

static uint32_t g_xres, g_yres, g_bpp;
static uint8_t *g_framebuffer;



static QueueSetHandle_t g_xQueueSetInput; /*输入设备的队列集*/ 
QueueHandle_t g_xQueuePlatform; /*队列A： 挡球板的队列*/

static QueueSetHandle_t g_xQueueIR;
static QueueSetHandle_t g_xQueueRotary;
static QueueHandle_t g_xQueueMPU6050; /* MPU6050队列 */

static TimerHandle_t g_TimerDHT11;

/* 挡球板任务 */
static void platform_task(void *params)
{
    byte platformXtmp = platformX;    
    uint8_t dev, data, last_data;
	struct input_data idata;
    // Draw platform
    draw_bitmap(platformXtmp, g_yres - 8, platform, 12, 8, NOINVERT, 0);
    draw_flushArea(platformXtmp, g_yres - 8, 12, 8);
    
    while (1)
    {
        /* 读取应用层的队列的数据 */
		//if (0 == IRReceiver_Read(&dev, &data))
		xQueueReceive(g_xQueuePlatform, &idata, portMAX_DELAY);	
		uptMove = idata.val;
		{      	
			// Hide platform
            draw_bitmap(platformXtmp, g_yres - 8, clearImg, 12, 8, NOINVERT, 0);
            draw_flushArea(platformXtmp, g_yres - 8, 12, 8);
            
            // Move platform
            if(uptMove == UPT_MOVE_RIGHT)
                platformXtmp += 3;
            else if(uptMove == UPT_MOVE_LEFT)
                platformXtmp -= 3;
            uptMove = UPT_MOVE_NONE;
            
            // Make sure platform stays on screen
            if(platformXtmp > 250)
                platformXtmp = 0;
            else if(platformXtmp > g_xres - PLATFORM_WIDTH)
                platformXtmp = g_xres - PLATFORM_WIDTH;
            
            // Draw platform
            draw_bitmap(platformXtmp, g_yres - 8, platform, 12, 8, NOINVERT, 0);
            draw_flushArea(platformXtmp, g_yres - 8, 12, 8);
            
            platformX = platformXtmp;
            
		}
    }
}

static void ProcessIRData(void){
	struct ir_data idata;
	static struct input_data input;
	
	xQueueReceive(g_xQueueIR, &idata, 0);
	
	if(idata.val == IR_KEY_LEFT){
		input.dev = idata.dev;
		input.val = UPT_MOVE_LEFT;	
	}
	else if(idata.val == IR_KEY_RIGHT){
		input.dev = idata.dev;
		input.val = UPT_MOVE_RIGHT;	
	}
	else if(idata.val == IR_KEY_REPEAT){
		/*重复码，保持不变*/
	}
	else{
		input.dev = idata.dev;
		input.val = UPT_MOVE_NONE;
	}
	/*写挡球板队列*/ 
	xQueueSend(g_xQueuePlatform, &input, 0);
}
static void ProcessRotaryData(void){
	
	struct Rotary_data rdata;
	struct ir_data idata;
	int left, cnt, i;
	
	
	/*读旋转编码器的硬件数据队列*/
	xQueueReceive(g_xQueueRotary, &rdata, portMAX_DELAY);	
	
	/*处理数据*/
	// 判断速度： 负数表示向左转动，正数表示向右转动
	if(rdata.speed < 0){
		left = 1;
		rdata.speed = 0 - rdata.speed;
		
	}else{
		left = 0;
	}
	
	//cnt = rdata.speed / 10;
	//if(!cnt)
	//	cnt = 1;

	if(rdata.speed > 100){
		cnt = 4;
	}else if(rdata.speed > 50){
		cnt = 2;
	}else{
		cnt = 1;
		
	}
		

	/*写队列A*/
	idata.dev = 1;
	idata.val = left? UPT_MOVE_LEFT: UPT_MOVE_RIGHT;
	for(i =0; i<cnt;i++){
		xQueueSend(g_xQueuePlatform, &idata, 0);
	}	

}

static void ProcessMPU6050Data(void){
	struct mpu6050_data mdata;
	static struct input_data input;
	
	uint8_t left;
	
	xQueueReceive(g_xQueueMPU6050, &mdata, 0);
	
	/*判断角度，大于90度表示往左移动挡球板，小于90度表示往右移动挡球板*/
	if(mdata.angle_x > 90)
	{
		input.val = UPT_MOVE_LEFT;
	}else if(mdata.angle_x < 90)
	{
		input.val =  UPT_MOVE_RIGHT;
	}else
	{
		input.val = UPT_MOVE_NONE;
	}
	/*写挡球板队列*/
	input.dev = 1;
	xQueueSend(g_xQueuePlatform, &input, 0);
}

static void Input_task(void){

	QueueSetMemberHandle_t xQueueHandle;
	while(1)
	{	
		/*读队列集，得到有数据的队列句柄*/
		xQueueHandle = xQueueSelectFromSet(g_xQueueSetInput, portMAX_DELAY);
		if(xQueueHandle)
		{
			/*读队列句柄，得到数据并处理数据,最后写队列*/
			if(xQueueHandle == g_xQueueIR)
			{
				ProcessIRData();
			}
			else if(xQueueHandle == g_xQueueRotary)
			{
				ProcessRotaryData();
			}else if(xQueueHandle == g_xQueueMPU6050)
			{
				ProcessMPU6050Data();
			}
						
		}	
	}

}

void DHT11Timer_Func( TimerHandle_t xTimer )
{
	int hum;
	int temp;
	int err;
	char buff[16];
	/* 读取DHT11的温湿度 */
	/*暂停调度器*/
	vTaskSuspendAll();
	err = DHT11_Read(&hum, &temp);
	/*恢复调度器*/
	xTaskResumeAll();
	if(err == 0)
	{
		/*	在OLED显示出来*/
		sprintf_P(buff, "%dC  %d%%", temp,hum);
		draw_string(buff, false, 40, 0);
	}
	else
	{
		/*	在OLED显示出来*/
		draw_string("err     ", false, 40, 0);
	}
	
}

void game1_task(void *params)
{		
    uint8_t dev, data, last_data;
    
    g_framebuffer = LCD_GetFrameBuffer(&g_xres, &g_yres, &g_bpp);
    draw_init();
    draw_end();
	
	buzzer_init(); /*蜂鸣器初始化*/
	DHT11_Init();
	
	/*创建一个定时器*/
	g_TimerDHT11 = xTimerCreate("DHT11_Timer", 
								2000,
								pdTRUE,
								NULL,
								DHT11Timer_Func);
	/*	启动定时器	*/
	xTimerStart(g_TimerDHT11, portMAX_DELAY);
	/*创建队列、创建队列集、创建输入任务Input_task*/
    g_xQueuePlatform = xQueueCreate(10,sizeof(struct input_data));
	g_xQueueSetInput = xQueueCreateSet(ROTARY_QUEUE_LEN + IR_QUEUE_LEN + MPU6050_QUEUE_LEN);
	xTaskCreate(Input_task, "Input_task", 128, NULL, osPriorityNormal, NULL);

	
	g_xQueueRotary = GetQueueRotary();
	g_xQueueIR = GetQueueIR();
	g_xQueueMPU6050 = GetQueueMPU6050();
		
	xQueueAddToSet(g_xQueueIR, g_xQueueSetInput);
	xQueueAddToSet(g_xQueueRotary, g_xQueueSetInput);
	xQueueAddToSet(g_xQueueMPU6050, g_xQueueSetInput);
	/*当一个队列被写满时，它不会被加入到队列集中*/
	/*g_xQueueMPU6050被放入队列集以后再去创建任务*/
	extern void MPU6050_Task(void);
	xTaskCreate(MPU6050_Task, "MPU6050Task", 128, NULL, osPriorityNormal, NULL);
	
	uptMove = UPT_MOVE_NONE;

	ball.x = g_xres / 2;
	ball.y = g_yres - 10; 
        
	ball.velX = -0.5;
	ball.velY = -0.6;
//	ball.velX = -1;
//	ball.velY = -1.1;

	blocks = pvPortMalloc(BLOCK_COUNT);
    memset(blocks, 0, BLOCK_COUNT);
	
	lives = lives_origin = 3;
	score = 0;
	platformX = (g_xres / 2) - (PLATFORM_WIDTH / 2);
	
	/*任务1：左右移动控制挡球板*/
    xTaskCreate(platform_task, "platform_task", 128, NULL, osPriorityNormal, NULL);

    while (1)
    {
        game1_draw();/*任务2：控制球*/
        //draw_end();
        vTaskDelay(50);
    }
}

static bool btnExit()
{
	
	vPortFree(blocks);
	if(lives == 255)
	{
		//game1_start();
	}
	else
	{
		//pwrmgr_setState(PWR_ACTIVE_DISPLAY, PWR_STATE_NONE);	
		//animation_start(display_load, ANIM_MOVE_OFF);
		vTaskDelete(NULL);
	}
	return true;
}

static bool btnRight()
{
	uptMove = UPT_MOVE_RIGHT;
	return false;
}

static bool btnLeft()
{
	uptMove = UPT_MOVE_LEFT;
	return false;
}

void game1_draw()
{
	bool gameEnded = ((score >= BLOCK_COUNT) || (lives == 255));

	byte platformXtmp = platformX;

    static bool first = 1;

	// Move ball
	// hide ball
	draw_bitmap(ball.x, ball.y, clearImg, 2, 2, NOINVERT, 0);
    draw_flushArea(ball.x, ball.y, 2, 8);

    // Draw platform
    //draw_bitmap(platformX, g_yres - 8, platform, 12, 8, NOINVERT, 0);
    //draw_flushArea(platformX, g_yres - 8, 12, 8);
	
	if(!gameEnded)
	{
		ball.x += ball.velX;
		ball.y += ball.velY;
	}

	bool blockCollide = false;
	const float ballX = ball.x;
	const byte ballY = ball.y;

	// Block collision
	byte idx = 0;
	LOOP(BLOCK_COLS, x)
	{
		LOOP(BLOCK_ROWS, y)
		{
			if(!blocks[idx] && ballX >= x * 4 && ballX < (x * 4) + 4 && ballY >= (y * 4) + 8 && ballY < (y * 4) + 8 + 4)
			{
//				buzzer_buzz(100, TONE_2KHZ, VOL_UI, PRIO_UI, NULL);
				buzzer_buzz(2000, 100);
				// led_flash(LED_GREEN, 50, 255); // 100ask todo
				blocks[idx] = true;

                // hide block
                draw_bitmap(x * 4, (y * 4) + 8, clearImg, 3, 8, NOINVERT, 0);                
                draw_flushArea(x * 4, (y * 4) + 8, 3, 8);                
				blockCollide = true;
				score++;
			}
			idx++;
		}
	}


	// Side wall collision
	if(ballX > g_xres - 2)
	{
		if(ballX > 240)
			ball.x = 0;		
		else
			ball.x = g_xres - 2;
		ball.velX = -ball.velX;		
	}
	if(ballX < 0)
  {
		ball.x = 0;		
		ball.velX = -ball.velX;	
  }

	// Platform collision
	bool platformCollision = false;
	if(!gameEnded && ballY >= g_yres - PLATFORM_HEIGHT - 2 && ballY < 240 && ballX >= platformX && ballX <= platformX + PLATFORM_WIDTH)
	{
		platformCollision = true;
		// buzzer_buzz(200, TONE_5KHZ, VOL_UI, PRIO_UI, NULL); // 100ask todo
		buzzer_buzz(5000, 200);
		ball.y = g_yres - PLATFORM_HEIGHT - 2;
		if(ball.velY > 0)
			ball.velY = -ball.velY;
		ball.velX = ((float)rand() / (RAND_MAX / 2)) - 1; // -1.0 to 1.0
	}

	// Top/bottom wall collision
	if(!gameEnded && !platformCollision && (ballY > g_yres - 2 || blockCollide))
	{
		if(ballY > 240)
		{
			// buzzer_buzz(200, TONE_2_5KHZ, VOL_UI, PRIO_UI, NULL); // 100ask todo
			buzzer_buzz(2000, 200);
			ball.y = 0;
		}
		else if(!blockCollide)
		{
			// buzzer_buzz(200, TONE_2KHZ, VOL_UI, PRIO_UI, NULL); // 100ask todo
			buzzer_buzz(2000, 200);
			ball.y = g_yres - 1;
			lives--;
		}
		ball.velY *= -1;
	}

	// Draw ball
	draw_bitmap(ball.x, ball.y, ballImg, 2, 2, NOINVERT, 0);
    draw_flushArea(ball.x, ball.y, 2, 8);

    // Draw platform
    //draw_bitmap(platformX, g_yres - 8, platform, 12, 8, NOINVERT, 0);
    //draw_flushArea(platformX, g_yres - 8, 12, 8);

    if (first)
    {
        first = 0;
        
    	// Draw blocks
    	idx = 0;
    	LOOP(BLOCK_COLS, x)
    	{
    		LOOP(BLOCK_ROWS, y)
    		{
    			if(!blocks[idx])
    			{
    				draw_bitmap(x * 4, (y * 4) + 8, block, 3, 8, NOINVERT, 0);
                    draw_flushArea(x * 4, (y * 4) + 8, 3, 8);                
    			}
    			idx++;
    		}
    	}
        
    }

	// Draw score
	char buff[6];
	sprintf_P(buff, PSTR("%u"), score);
	draw_string(buff, false, 0, 0);

    // Draw lives
    if(lives != 255)
    {
        LOOP(lives_origin, i)
        {
            if (i < lives)
                draw_bitmap((g_xres - (3*8)) + (8*i), 1, livesImg, 7, 8, NOINVERT, 0);
            else
                draw_bitmap((g_xres - (3*8)) + (8*i), 1, clearImg, 7, 8, NOINVERT, 0);
            draw_flushArea((g_xres - (3*8)) + (8*i), 1, 7, 8);    
        }
    }   

	// Got all blocks
	if(score >= BLOCK_COUNT)
		draw_string_P(PSTR(STR_WIN), false, 50, 32);

	// No lives left (255 because overflow)
	if(lives == 255)
		draw_string_P(PSTR(STR_GAMEOVER), false, 34, 32);

}

