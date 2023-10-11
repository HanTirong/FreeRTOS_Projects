### *2023.10.5*  
#### [8-1-1] 数据传输的方法—环形buffer  
1. 任务之间传输数据的方法比较  

|  | 数据个数 | 互斥措施 | 阻塞-唤醒 | 
| :----:| :----: | :----: |:----:|
| 全局变量 | 1 | 无 | 无|
| 环形buffer | 多个 | 无 | 无|
| 队列 | 多个 | 有 |有|  

2. 环形缓冲区  
   
``` CC 
#define RING_BUF_LEN 8

uint8_t ringBuf[RING_BUF_LEN] = {0};
uint8_t r = 0;
uint8_t w = 0;

uint8_t isEmpty(){
    if( w == r)
        return 1;    /*  空     */
    else
        return 0;   /*   非空  */
}

uint8_t isFull(){
    uint8_t next_w = w+1;
    if(next_w == RING_BUF_LEN)
        next_w = 0;

    if(next_w == r)
        return 1;   /*  满了  */
    else 
        return 0;   /*  未满  */
}

void Write(uint8_t val){
    if(isFull() == 0){
        ringBuf[w] = val;
        w++;
        if(w == RING_BUF_LEN){
            w = 0;
        }
    }
}

void Read(uint8_t *val){
    if(isEmpty() == 0){
        *val = ringBuf[r];
        r++;
        if(r == RING_BUF_LEN){
            r = 0;
        }
    }
}
```
通过上述代码，可以总结出：环形buffer中，读和写的过程中都是单独去操作w或r，没有出现某个变量被两个任务去访问和修改，因此比较安全。



#### [8-1-2] 数据传输的方法—队列的本质
- 队列中，数据的读写本质就是环形缓冲区，在此基础上增加了互斥措施、阻塞-唤醒机制。  
- 如果这个队列不传输数据，只调整“数据个数”，他就是信号量semaphore
- 如果信号量中，限定“数据个数”，它就是互斥量mutex
  
- 阻塞-唤醒机制
  - 队列里面肯定有环形buf以及两个链表： *sendList* 、 *recvList* 
  - 假设有一个任务B，它会读队列 `xQueueReceive()` ，然后将读取到的数据进行处理
  1. 任务B刚被创建后，进入就绪态， readyList 指向该任务
  2. 任务B开始执行，执行到`xQueueReceive()`，由于刚开始队列中没有数据，因此被阻塞，该任务被从 *readyList* 移除，放入到该队列的 *recvList* (该链表用来存放想读数据但是没有读取到的任务)以及 *delayedList* 
     1. （情况1）任务A开始运行，写队列，将数据存入环形缓冲区，然后去该队列的 *recvList* 取出第一个任务，唤醒他(即将任务B从 *recvList* 和 *delayedList* 移除，放入 *readyList* )
     2. （情况2）任务B一直在阻塞，队列中没有数据写入，Tick中断会判断 *delayedList* 中的任务是否超时，如果超时，该任务会被唤醒（即将任务B从 *recvList* 和 *delayedList* 移除，放入 *readyList* ）
  
  



### *2023.10.6*  

#### [8-2-1] 队列实验-多设备玩游戏（思路）
- 现有程序里一共两个任务：控制小球移动和控制挡球板移动
- 现有程序控制挡球板任务是 红外遥控器的读取函数`IRReceiver_Read()` 读取环形buffer，每读取到一次数据挡球板会移动一次，如果想要移动的快就需要读取到多次数据；而环形buf中的数据是在红外遥控器的中断回调函数中写入的 `IRReceiver_IRQ_Callback()`
- 现有程序要改为用队列去代替环形buf
  - 在 `IRReceiver_IRQ_Callback()` 中去写队列A
  - 在 `IRReceiver_Read()` 中去读队列A
  - 引入另一种控制球挡板的方法，即旋转编码器，因此在 `RotaryEncoder_IRQ_Callback()` 中写队列B
  - 新建一个 `RotaryEncoderTask()`，该任务中去读队列B，然后将读取的数据进行处理，处理完毕后再写入队列A
    - 处理1：分辨旋转速度
    - 处理2：根据旋转速度决定往队列A中写多少个数据；速度快就多写一点数据，速度慢就少写一点数据
  





#### [8-2-2] 队列实验-多设备玩游戏（红外）

- `xQueueCreate()`
  - 动态分配，除了构建环形buf，还会构建一个结构体，用来管理环形buf；最终结构体作为handle被返回
- `xQueueCreateStatic()`

相关代码：[13_queue_game](../MDK5/13_queue_game/nwatch/game1.c) 

``` txt
代码逻辑：
------------------  
game1.c
|--- xTaskCreate(playMusic_task)
|--- xTaskCreate(game1_task)
                    |--- xQueueCreate(g_xQueuePlatform)    -----------------------------|
                    |--- xQueueCreateStatic(g_xQueueRotary)                             |
                    |--- xTaskCreate(platform_task)                                     |
                                        |--- xQueueReceive(g_xQueuePlatform)  ---read---|
------------------                                                                      |
driver_ir_receiver.c                                                                    |
|--- IRReceiver_IRQ_callback()                                                          |
        |--- xQueueSendToBackFromISR(g_xQueuePlatform)  -------------- write -----------|  // 如果是重复码
        |--- IRReceiver_IRQTimes_Parse()                                                |
                |--- xQueueSendToBackFromISR(g_xQueuePlatform)  ------ write -----------|   

```


#### [8-2-3] 队列实验--多设备玩游戏（旋转编码器）

相关代码：[14_queue_game_multi_input](../MDK5/14_queue_game_multi_input/nwatch/game1.c) 

``` txt
代码逻辑：
---------------------------------    
game1.c
|--- xTaskCreate(playMusic_task)
|--- xTaskCreate(game1_task)
                    |--- xQueueCreate(g_xQueuePlatform) -------------------------------------|
                    |--- xQueueCreateStatic(g_xQueueRotary)  --------------------------|     |
                    |--- xTaskCreate(platform_task)                                    |     |
                                        |--- xQueueReceive(g_xQueuePlatform)  --- read-|---<-|
                    |--- xTaskCreate(RotaryEncoder_task)                               |     |
                                        |--- xQueueReceive(g_xQueueRotary) --- read -<-|     |  // 从队列B取数，
                                        |---   /*Process data*/                        |     |  // 然后处理后转为应用层相关的数据，
                                        |--- xQueueSend(g_xQueuePlatform) --- write ---|--->-|  // 存入队列A
                                                                                       |     | 
                                                                                       |     |
driver_ir_receiver.c                                                                   |     |
|--- IRReceiver_IRQ_callback()                                                         |     |
        |--- xQueueSendToBackFromISR(g_xQueuePlatform)  -------------- write ----------|--->-|  // 如果是重复码
        |--- IRReceiver_IRQTimes_Parse()                                               |     |
                |--- xQueueSendToBackFromISR(g_xQueuePlatform)  ------ write ----------|--->-|  // 把红外遥控器的键值转为游戏控制的按键然后写入队列A
                                                                                       | 
                                                                                       |    
                                                                                       |
driver_rotary_encoder.c                                                                | 
|--- RotaryEncoder_IRQ_Callback()                                                      |
        |---xQueueSendFromISR(g_xQueueRotary)   -------------- write -->-->-->-->-->-->|        // 旋转编码器将硬件相关的数据写入队列B

```


#### [8-3-1] 队列集实验-改进程序框架（思路）
-  参考代码14的逻辑图，可以得出：旋转编码器的架构更为合理，将硬件层的数据与应用层的数据分开进行处理。但是这种做法也是有缺陷的，今后每添加一个硬件，就得创建一个任务函数去转换硬件层的数据到应用层的数据，这样会造成资源浪费。
-  改进：假设每个硬件都是在其中断函数中将硬件数据去写入到各自的队列B；构造一个任务InputTask对所有硬件的队列B进行读取，然后转化为应用层的数据并写入队列A。
   -  问题：InputTask如何及时地读到各个硬件对应的队列B中的数据呢？
      -  轮询
      -  队列集：也是一个队列，里面存放的元素是队列的句柄；举例说明，内部机制如下：
           1. 创建 队列A、队列B
           2. 创建队列集S
           3. 队列A和B 与 队列集S建立联系
           4. 往队列A或B写入数据时，顺便会把队列A或B的句柄写入到队列集S中
           5. 因此在 InputTask 中会读取S得到队列的句柄
           6. 然后通过读取队列句柄得到数据
- 因此，改进后的框架为： 
  1. 假设每个硬件在其中断函数中将硬件数据写入到队列B中；
  2.  每个硬件存放硬件数据的队列B都被加入到队列集S中；
  3.  在InputTask中，读取队列集S得到队列的句柄，然后读取队列得到硬件数据，最后将硬件数据转换成应用层数据写入队列A




#### [8-3-2] 队列集实验-改进程序框架（编程）
相关代码：[15_queueset_game](../MDK5/15_queueset_game/nwatch/game1.c) 
``` txt
代码逻辑：
---------------------------------    
freertos.c
|--- MX_FREERTOS_Init()
        |---IRReceiver_Init()
                |--- xQueueCreate(g_xQueueIR) ----------/*1. crerate queue*/--------------------|   
        |---RotaryEncoder_Init()                                                                |
                |--- xQueueCreate(g_xQueueRotary) -------------------------------------|        |
        |--- xTaskCreate(game1_task)                                                    |        |
game1.c                                                                                |        | 
|--- xTaskCreate(game1_task)                                                           |        | 
                    |--- xQueueCreate(g_xQueuePlatform)                                |        | 
                    |--- xQueueCreateSet(g_xQueueSetInput)                             |        | 
                    |--- xTaskCreate(Input_task)                                       |        |         
                            |--- xQueueSelectFromSet(g_xQueueSetInput, portMAX_DELAY); |        |     // 从队列集中读取有数据的队列句柄       
                            |--- ProcessIRData();                                      |        |     //如果是红外的队列，
                                    |--- xQueueReceive(g_xQueueIR) ---- 5. read--------|<--<--<-| 
                                    |--- /* process data*/                             |        |     //  将硬件数据转换成应用层数据
                                    |--- xQueueSend(g_xQueuePlatform)                  |        |     //  写入应用层队列中
                            |--- ProcessRotaryData();                                  |        |     //  如果是旋转编码器的队列，
                                    |--- xQueueReceive(g_xQueueRotary) --read-<--<--<--|        |
                                    |--- /* process data*/                             |        | 
                                    |--- xQueueSend(g_xQueuePlatform)                  |        |          
                    |--- g_xQueueRotary = GetQueueRotary()   --------------------------|        |                             
                    |--- g_xQueueIR = GetQueueIR() --------/*2. get queue*/------------|        | 
                    |--- xQueueAddToSet(g_xQueueIR, g_xQueueSetInput); --- 3.----------|        | 
	                |--- xQueueAddToSet(g_xQueueRotary, g_xQueueSetInput); ------------|        |         
                    |--- xTaskCreate(platform_task)                                    |        | 
                            |--- xQueueReceive(g_xQueuePlatform)                       |        |     // 读取应用层队列中的数据
                                                                                       |        |                                                                                     
                                                                                       |        | 
driver_ir_receiver.c                                                                   |        |              
|--- IRReceiver_IRQ_callback()                                                         |        |     
        |--- xQueueSendToBackFromISR(g_xQueueIR)  ----------- 4. write ----------------|->-->-->|  // 如果是重复码
        |--- IRReceiver_IRQTimes_Parse()                                               |        |     
                |--- xQueueSendToBackFromISR(g_xQueueIR)  ---- 4. write ---------------|->-->-->|// 把红外遥控器的键值转为游戏控制的按键然后写入队列A
                                                                                       |         
                                                                                       |    
                                                                                       |
driver_rotary_encoder.c                                                                | 
|--- RotaryEncoder_IRQ_Callback()                                                      |
        |---xQueueSendFromISR(g_xQueueRotary)  --------------- write -->-->-->-->-->-->|        // 旋转编码器将硬件相关的数据写入队列B

```



#### [8-3-3] 队列集实验-增加姿态控制
相关代码：[16_queueset_game_mpu6050](../MDK5/16_queueset_game_mpu6050/nwatch/game1.c) 
1. MPU6050模块的驱动程序是通过I2C去读取数据的，而不是中断，因为I2C时间太长，不可能在中断里面去完成。如果用到了中断，则中断服务函数中是去唤醒某个任务，该任务去读I2C，并写硬件数据队列中。
2. 因此需要创建一个任务MPU6050Task，该任务里需要读I2C，然后写硬件队列中；又因为I2C不能一直读，所以还需要vTaskDelay()去阻塞，保证其他任务可以运行。
3. MPU6050的硬件数据队列需要被加入到队列集中
4. InputTask从队列集中读取到队列句柄，然后根据队列句柄得到数据，需要添加对MPU6050硬件数据处理代码，然后写入到挡球板队列中    
***[注]***   如果 MPU6050Task 和 game1Task 并列创建，若MPU6050Task先运行，导致 MPU6050Queue 被写满；当 game1Task 开始运行时，MPU6050Queue不会被加入到队列集中。   
  **因为当一个队列被写满时，它不会被加入到队列集中**




#### [8-4] 队列实验-分发数据给多个任务（赛车游戏）
相关代码：[17_queueset_car_dispatch_htr](../MDK5/17_queueset_car_dispatch_htr/nwatch/game2.c )

```
代码逻辑：
----------------------------------------------
freertos.c
|--- MX_FREERTOS_Init()
        |--- IRReceiver_Init();
                |--- xQueueCreate(g_xQueueIR)    -------------------------------------------------------------------|
        |--- xTaskCreate(car_game)                                                                                  |
                            |--- xQueueCreate(g_xQueueCar)  ----------------------------------------|---------------|
                            |--- xTaskCreate(input_car_task)                                        |               |
                                                |--- xQueueReceive(g_xQueueIR)      ----------------|------read-<-<-|
                                                |--- /*process hardware data to app data*/          |               |
                                                |--- xQueueSendToBack(g_xQueueCar)  ---- write- ->->|               |
                                                                                                    |               |
                                                                                                    |               |
                            |--- /*init car and road*/                                              |               |
                            |--- while(1)                                                           |               |
                                    |--- /*draw road*/                                              |               |
                                    |--- xQueueReceive(g_xQueueCar) ------------------ read ----<-<-|               |
                                    |--- /*move car*/                                                               |
                                                                                                                    |
driver_ir_receiver.c                                                                                                |
|--- IRReceiver_IRQTimes_Parse()                                                                                    |
        |---xQueueSendToBackFromISR(g_xQueueIR)     ------------------------------------------------------ write->->|


```