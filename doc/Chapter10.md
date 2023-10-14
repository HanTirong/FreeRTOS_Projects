### 事件组
#### [10-1]  事件组的本质
1. 事件组每bit表示一个事件，高8位不用，用来表示事件的关系是or还是and  
2. 有一个链表，存放等待事件发生的任务  
3. 每个任务等待什么事件，就让对应bit位置1  


---  



#### [10-2]  事件组实验————车辆协同
相关代码：[23_eventgroup_broadcast](../MDK5/23_eventgroup_broadcast/nwatch/game2.c)  
现象：car1到站后，广播通知car2、car3启动

相关代码：[24_eventgroup_and](../MDK5/24_eventgroup_and/nwatch/game2.c)  
现象：car1或car2到站后，car3启动

相关代码：[25_eventgroup_or](../MDK5/25_eventgroup_or/nwatch/game2.c)  
现象：car1和car2到站后，car3启动  

---  

#### [10-3]  事件组实验————改进姿态控制


相关代码：[26_eventgroup_mpu6050](../MDK5/26_eventgroup_mpu6050/Drivers/DshanMCU-F103/driver_mpu6050.c)

在 [16_queueset_game_mpu6050](../MDK5/16_queueset_game_mpu6050/nwatch/game1.c) 中，由于I2C读写时序长，因此在任务中去进行读写，该任务中读写后会进行 vTaskDelay(50),即该任务会一直读写I2C，浪费资源

现在将上述改为：中断通过事件组去唤醒该任务，即I2C读写任务不会一直执行，提高效率

- 如何通过cubeMX配置GPIO中断？
  - 搜索引脚PB5，配置为GPIO_EXIT5
  - 在左侧System Core选择GPIO，然后点击GPIO5，配置中断触发模式是上升沿还是其他
  - 在NVIC去enable该中断
  - 最后，点击生成代码，如果配置无误，则会生成该中断的中断函数，后续只需要在中断函数内添加内容即可
  - **注：此处生成的中断函数是WEAK函数**


