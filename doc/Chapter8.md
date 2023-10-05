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
  
  





#### [8-2-1] 队列实验-多设备玩游戏（思路）






#### [8-2-2] 队列实验-多设备玩游戏（红外）






#### [8-2-3] 队列实验--多设备玩游戏（旋转编码器）





#### [8-3-1] 队列集实验-改进程序框架（思路）





#### [8-3-2] 队列集实验-改进程序框架（编程）





#### [8-3-3] 队列集实验-增加姿态控制






#### [8-4] 队列实验-分发数据给多个任务（赛车游戏）





