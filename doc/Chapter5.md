### 任务管理 
#### [5-1-1]  创建任务  
相关代码 ： [05_createTask](../MDK5/05_createTask/Core/Src/freertos.c)
1. 任务三要素： 做什么（函数）、栈和TCB、优先级，由于栈和TCB可以动态分配得到，也可以静态分配得到，因此有：
- `xTaskCreate` 动态分配
- `xTaskCreateStatic` 静态分配

--- 

#### [5-1-2]  估算栈大小  
1. 栈里面存了：返回地址LR以及其他寄存器、局部变量、保存现场，因此选择最复杂的调用关系去估算栈大小。
- 返回地址LR以及其他寄存器：与函数调用深度有关
- 局部变量：与使用到的局部变量的个数、大小有关
- 保存现场：16个寄存器，64bytes
2. 计算栈大小最精确的办法是看反汇编

--- 
#### [5-2] 使用任务参数 
相关代码 ： [06_createTaskUseParms](../MDK5/06_createTaskUseParms/Core/Src/freertos.c) 
``` C
/*创建任务需要用到的函数的输入参数，设置为全局变量*/
struct TaskPrintInfo Task1Info = {0,0,"Task1"};
struct TaskPrintInfo Task2Info = {0,3,"task2"};
struct TaskPrintInfo Task3Info = {0,6,"TASK3"};

void LcdPrintTask(void *parms){
	struct TaskPrintInfo *pInfo = (struct TaskPrintInfo *)parms;
	uint32_t  cnt = 0;
	int len; 
	while(1){
		if(g_LCDCanUse){
			/*打印信息*/
			g_LCDCanUse = 0;
			len = LCD_PrintString(pInfo->x,pInfo->y,pInfo->name);
			len += LCD_PrintString(len,pInfo->y,":"); /*因为传入的name可能长度不同*/
			LCD_PrintSignedVal(len,pInfo->y,cnt++);
			g_LCDCanUse = 1;
		}
		mdelay(500);    
        /*
            延时是为了保障大概率在此时进行任务切换，
        使得下一个任务进来时，g_LCDCanUse为1，可以在OLED上打印出信息
        */
	}
}

void MX_FREERTOS_Init(void) {
    BaseType_t ret;
    /*使用同一个函数，创建不同的任务*/
    ret = xTaskCreate(LcdPrintTask, "Task1", 128, &Task1Info, osPriorityNormal,NULL);
    ret = xTaskCreate(LcdPrintTask, "Task2", 128, &Task2Info, osPriorityNormal,NULL);
    ret = xTaskCreate(LcdPrintTask, "Task3", 128, &Task3Info, osPriorityNormal,NULL);
}
```
有两个问题还没解决：
- ***如何互斥地访问LCD?***  &nbsp;&nbsp;&nbsp;&nbsp;  (此处使用全局变量来保证大概率，但不是万无一失)
- ***为何创建后的任务3先开始运行？***

--- 

#### [5-3] 删除任务  
相关代码 ： [07_delateTask](../MDK5/07_delateTask/Core/Src/freertos.c)   
`vTaskDelete`
频繁的创建和删除任务不好，因为频繁的动态分配与释放内存会造成碎片，并且任务删除时直接终止任务，并没有进行清零的工作。


--- 

#### [5-4] 优先级与阻塞 
相关代码 ：   [08_task_priority](../MDK5/08_task_priority/Core/Src/freertos.c)    
提高播放音乐任务的优先级，使用`vTaskDelay`进行延时，该函数会在运行过程中主动放弃CPU资源，进入阻塞状态


--- 
#### [5-5-1] 任务状态  
相关代码 ：   [09_task_suspend](../MDK5/09_task_suspend/Core/Src/freertos.c)  
1. 任务一创建好就是就绪状态，马上就可以运行，转换为运行状态
2. 运行态中，通过调用一些阻塞API，状态切换为阻塞态Blocked
3. 阻塞状态时，调用`vTaskSuspend`转换为挂起状态
4. 挂起状态时，调用`vTaskResume`转换为就绪状态，马上就可以运行
5. 就绪状态是，也可以通过调用`vTaskSuspend`转换为挂起状态


--- 
#### [5-5-2]  任务管理与调度  
1. 调度
- 相同优先级的任务轮流运行
- 最高优先级的任务先运行
  - 只要有高优先级的任务没有执行完，低优先级任务无法运行
  - 一旦高优先级任务就绪，它会马上运行
  - 如果最高优先级的任务有多个，它们轮流运行
1. 程序用**链表**来管理任务  
- 变量 *pxReadyTasksLists[configMAX_PRIORITIES]* 中第N个成员存储了优先级为N的处于Ready/Running状态的任务
- `vTaskStartScheduler()`函数中会创建一个空闲任务 *prvIdleTask* ，该任务优先级为0
- 有一个全局变量 *pxCurrentTCB* ，每当创建好一个任务，会指向此时优先级最高的任务；若创建了多个优先级相同的任务，则指向最后一个创建的任务
- 定义 *TICK_RATE_HZ* 为Tick中断频率，在Tick中断中会进行计数累加，以及调度（遍历ReadList，从高优先级到低优先级遍历，找到第一个非空的链表，把 *pxCurrentTCB* 指向下一个任务，来启动它）
- 当调用`vTaskDelay()`，任务进入阻塞状态，此时该任务会从 *ReadyList* 删除，被存入某个 *DelayedTaskList* ，然后触发调度
- 假设`vTaskDelay(2)`，即需要等待2个Tick，当时间到了以后，在tick中断中除了计数之外，还会判断 *DelayedTaskList* 中的任务是否可以被恢复，如果可以则放回到Readlist中，然后进行调度
- 当调用`vTaskSuspend()`，任务会被移动到 *SuspendList*
- 当调用`vTaskResume()`，任务会被移动到 *Readylist*


--- 
#### [5-5-3] 空闲任务  
1. 如果一个任务函数不经过其他处理，执行完了直接返回的话，会进入到`prvTaskExitError()`，该函数内部会关闭所有的中断，导致所有的任务不再可以运行
2. 任务如何退出？
   - 自杀：`vTaskDelete(NULL)`，自杀完，由空闲任务去释放资源，回收栈和TCB
   - 他杀： `vTaskDelete(&TaskHandle)`，谁杀的，就由谁去释放资源，回收栈和TCB
   - 如果有很多个任务自杀，空闲任务根本无法去回收栈和TCB，有可能导致内存不足
3. 因此，要有良好的编程习惯
   - 事件驱动，例如按下某个按键，再执行某个函数
   - 休眠时，延时函数不要使用，例如不要用`mdelay()`，改为`vTaskDelay()`


--- 
#### [5-6] 两个delay函数  
相关代码 ：   [11_task_delay](../MDK5/11_task_delay/Core/Src/freertos.c)   
 1. `vTaskDelay()` 至少等待指定个数的tick后才变为就绪状态
 2. `vTaskDelayUntil(&preTime, tickCounts)` 等到指定的绝对时刻，才能变为就绪态
    - 在该函数中，计时到了以后，除了会更新被唤醒的时间preTime + tickCounts，还会让该任务进入就绪态

---