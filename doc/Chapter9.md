### 信号量与互斥量
#### [9-1]  信号量的本质
1. 信号量是一个特殊的队列，只会把计数加加减减，不涉及数据传输
2. 队列与信号量对比
- 队列： 
    ``` C
    struct queue{
        uint8_t len;    //队列长度
        uint8_t r;      // 环形buf的读索引
        uint8_t w;      // 环形buf的写索引
        uint8_t cnt;    // 队列中的个数
        struct List_t *senderList;     // 等待写数据的任务列表
        struct List_t *recvList;       // 等待读数据的任务列表
    }
    ```  
  - 写队列： send   
    1. 拷贝数据  
    2. cnt++
    3. 唤醒等待读数据的任务  
 
  - 读队列： receive  
     1. 拷贝数据  
     2. cnt--  
     3. 唤醒等待写数据的任务  

- 信号量： 
    ``` C
    struct semaphore{
        uint8_t max_cnt;            //计数值的最大值
        uint8_t cnt;     
        struct List_t *recvList;    // 等待信号量的任务列表
    }
    ```
  - 发信号量：give
    1. cnt++
    2. 唤醒等待信号量的任务
  - 收信号量：take
    1. 如果收到，cnt--
    2. 如果没有收到，就阻塞，即将自己放入recvList中
   
---  
#### [9-2-1] 信号量实验——控制车辆运行

相关代码：[18_semaphore_not_use](../MDK5/18_semaphore_not_use/nwatch/game2.c) 

现象：三辆车同时移动到最右侧，然后任务自杀

相关代码：[19_semaphore_count](../MDK5/19_semaphore_count/nwatch/game2.c) 

现象：创建一个信号量，初始值为2，因此只有两辆车会移动到右边；当信号量释放后，剩下一辆车也会移动到最右边


相关代码：[20_semaphore_binary](../MDK5/20_semaphore_binary/nwatch/game2.c) 

- 信号量中的等待信号量链表中存放的任务顺序：
   - 高优先级的任务排在前面；
   - 同等优先级的，谁先take，谁在前面
- 当信号量来了，会从链表中取出第一个任务运行




---  

#### [9-2-2] 信号量实验——优先级反转
相关代码：[21_semaphore_priority_inversion](../MDK5/21_semaphore_priority_inversion/nwatch/game2.c)   

创建三个任务：  

- 低优先级car1Task: (1)take Sem -> (2)运行一会-> (3)blocked -> (8)运行完give sem  

- 中优先级car2Task: (6)一直运行 -> (7)自杀  

- 高优先级car3Task: (4)take sem err -> (5)blocked -> (9)take sem -> (10)运行完give sem  

优先级反转：
1. 最低优先级任务 take sem
2. 中优先级任务一直运行
3. 最高优先级任务take sem，失败，所以等待







---  



#### [9-3] 互斥量——（解决优先级反转）
相关代码：[22_mutex_priority_inversion](../MDK5/22_mutex_priority_inversion/nwatch/game2.c)   

互斥量是信号量的一个变种，具有优先级继承和优先级恢复的功能  

创建三个任务：  

- 低优先级car1Task: (1)take Mutex -> (2)运行一会-> (3)blocked -> (8)由于互斥量的优先级继承导致优先级大于car2Task,因此继续运行->(9)运行结束，give mutex并自杀  

- 中优先级car2Task: (6)优先级比car1Task高，因此开始运行 -> (7)任务调度被blocked->(12)继续运行  

- 高优先级car3Task: (4)take Mutex err -> (5)blocked -> (10)take Mutex -> (11)因为优先级比car2Task高，运行完give Mutex    





