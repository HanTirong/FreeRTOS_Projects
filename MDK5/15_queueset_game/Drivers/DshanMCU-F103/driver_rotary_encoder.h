
#ifndef _DRIVER_ROTARY_ENCODER_H
#define _DRIVER_ROTARY_ENCODER_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#define ROTARY_QUEUE_LEN	10
struct Rotary_data {
	int32_t cnt;
	int32_t speed;
}; 

QueueHandle_t GetQueueRotary(void);

/**********************************************************************
 * 函数名称： RotaryEncoder_Init
 * 功能描述： 旋转编码器的初始化函数
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期：      版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/05	     V1.0	  韦东山	      创建
 ***********************************************************************/
void RotaryEncoder_Init(void);

/**********************************************************************
 * 函数名称： RotaryEncoder_Write
 * 功能描述： 旋转编码器的读取函数
 * 输入参数： 无
 * 输出参数： pCnt   - 用于保存计数值 
 *            pSpeed - 用于保存速度(正数表示顺时针旋转,负数表示逆时针旋转,单位:每秒转动次数)
 *            pKey   - 用于保存按键状态(1-按键被按下, 0-按键被松开)
 * 返 回 值： 无
 * 修改日期：      版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/05	     V1.0	  韦东山	      创建
 ***********************************************************************/
void RotaryEncoder_Read(int32_t *pCnt, int32_t *pSpeed, int32_t *pKey);


/**********************************************************************
 * 函数名称： RotaryEncoder_Test
 * 功能描述： 旋转编码器测试程序
 * 输入参数： 无
 * 输出参数： 无
 *            无
 * 返 回 值： 无
 * 修改日期        版本号     修改人        修改内容
 * -----------------------------------------------
 * 2023/08/05        V1.0     韦东山       创建
 ***********************************************************************/
void RotaryEncoder_Test(void);


#endif /* _DRIVER_ROTARY_ENCODER_H */

