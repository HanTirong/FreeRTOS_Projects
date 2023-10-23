### 中断管理
#### [13-1]  任务和中断的两套API函数

1. 为什么要有两套API
- 以 `xQueueSend()`  和 `xQueueSendToBackFromISR()` 举例
  
``` C
BaseType_t xQueueSend(
            QueueHandle_t    xQueue,            // 要写的队列
            const void       *pvItemToQueue,    // 要写入的元素
            TickType_t       xTicksToWait       // 超时时间
                     );


BaseType_t xQueueSendToBackFromISR(
            QueueHandle_t xQueue,               // 要写的队列
            const void *pvItemToQueue,          //要写入的元素
            BaseType_t *pxHigherPriorityTaskWoken   // 是否有更高优先级的任务需要被唤醒
                                   );
```
- 在任务中可以被阻塞，再中断中不能阻塞
- 如果任务A调用xQueueSend写队列的过程中，唤醒了任务B，且B的优先级高于A则超时时间会超过xTicksToWait
- 在中断中，写队列，只需要判断是否要唤醒任务B，不需要马上切换，直到中断退出前才切换






#### [13-2]  FromISR示例-改进实时性
示例如下：  
``` C

void XXX_ISR()
{
    int i;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    for (i = 0; i < N; i++)
    {
        xQueueSendToBackFromISR(..., &xHigherPriorityTaskWoken); /* 被多次调用 */
    }
	
    /* 最后再决定是否进行任务切换 
     * xHigherPriorityTaskWoken为pdTRUE时才切换
     */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

```



