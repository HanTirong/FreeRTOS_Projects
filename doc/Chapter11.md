### 任务通知
#### [11-1]  任务通知的本质
1. 队列、信号量、事件组 的同步机制中，两个任务是相互是不知道的，即任务A写数据，但是不知道是哪一个任务读数据  
   任务A/ISR发送数据    ->   通信对象    ->   任务B/ISR接受
2. 任务通知，结构体TCB就包含了内部对象，直接收到其他任务发来的通知
   任务A/ISR发送数据    ->  任务B接受
3. 每一个任务的TCB内都有2个成员：
- 一个是uint8_t类型，用来表示通知状态
- 一个是uint32_t类型，用来表示通知值
``` C
typedef struct tskTaskControlBlock
{
    ......
    /* configTASK_NOTIFICATION_ARRAY_ENTRIES = 1 */
    volatile uint32_t ulNotifiedValue[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
    volatile uint8_t ucNotifyState[ configTASK_NOTIFICATION_ARRAY_ENTRIES ];
    ......
} tskTCB;
```
通知状态有3种取值：
- taskNOT_WAITING_NOTIFICATION ：任务没有等待通知，初始状态
- taskWAITING_NOTIFICATION ：任务在等待通知
- taskNOTIFICATION_RECEIVED ：任务接收到了通知，也成为pending（有数据，待处理）
  
通知值有多种类型：
- 计数值
- 位（类似事件组）
- 任意数值

考虑以下场景： 任务A向任务B发送通知
```
场景1：

任务B: (1)运行  -> (2)阻塞（不是因为等待通知）  ->  (4) 运行 -> (5)某一时刻等待通知 -> (6) 下一时刻
任务A:    ...   -> (3)发送通知，但是无法唤醒B ->  ...  
```

任务B的通知状态：  
（1） taskNOT_WAITING_NOTIFICATION  
（2） taskNOT_WAITING_NOTIFICATION  
（3） taskNOTIFICATION_RECEIVED 即使状态改变也不被唤醒  
（4） taskNOTIFICATION_RECEIVED  
（5） taskNOTIFICATION_RECEIVED   
（6） taskNOT_WAITING_NOTIFICATION 恢复初始状态  
 


```
场景2：

任务B: (1)运行 -> (2)阻塞（因为等待通知） -> (4)运行
任务A:   ....   -> (3)发送通知，唤醒B  ->  ...   
```
任务B的通知状态：  
（1） taskNOT_WAITING_NOTIFICATION  
（2） taskWAITING_NOTIFICATION  
（3） taskNOTIFICATION_RECEIVED  
（4） taskNOT_WAITING_NOTIFICATION 又恢复成初始状态  



任务通知有2套函数：简化版和专业版

|  |  简化版| 专业版 | 
| :----:| :----: | :----: |
| 发出通知 | `xTaskNotifyGive`      `vTaskNotifyGiveFromISR` | `xTaskNotify`    `xTaskNotifyFromISR` | 
| 取出通知 | `ulTaskNotifyTake` | `xTaskNotifyWait` | 



对于简化版有：    
假设任务A调用`xTaskNotifyGive()` 发出通知，一定会让任务B的通知值++，但是对B而言，  
如果B因为调用 `ulTaskNotifyTake()` 取出通知，而状态处于 taskWAITING_NOTIFICATION，B被唤醒同时通知值--    
如果B没有调用 `ulTaskNotifyTake()` 取出通知，其状态会变为 taskNOTIFICATION_RECEIVED 







--- 

#### [11-2]  任务通知实验——通知车辆运行


相关代码：[27_tasknotification_car_game](../MDK5/27_tasknotification_car_game/nwatch/game2.c) 

现象：当小车1到达终点后，任务通知小车2和小车3开始移动  