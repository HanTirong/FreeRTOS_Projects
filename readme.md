## 本仓库用来记录FreeRTOS学习过程并存储相关代码
---
###  [参考视频](https://www.bilibili.com/video/BV1Jw411i7Fz/)
---
### *2023.9.26*
1. 搭建软件环境
2. 学习.gitignore文件编写规则
3. 学习git基本操作命令，并建立仓库
   - `git init`        初始化一个仓库
   - `git add`      添加文件到暂存区
   - `git commit -m "说明" `    提交
   - `git push <远程仓库的分支> <本地当前分支>`
   - `git status`   查看工作目录状态
   - `git log`      查看提交历史
   - `git remote add <远程仓库名称> <远程仓库URL>`
   -  `git branch`  查看或创建一个分支
   -  `git reset`   清除暂存区的所有文件
---
### *2023.9.27*
1. `xTaskCreate` 创建线程
2.  硬件结构与汇编指令
    - ARM寄存器
      - R0~R7： 低寄存器
      - R8~R12：高寄存器
      - R13（SP）：栈指针
      - R14（LR）：保存返回地址
      - R15（PC）：表示当前指令地址，写入新值即可跳转
    - 读内存 Load
      ```
      LDR R0, [R1, #4]  ; 将R1+4的地址中的数写入R0
      LDRB  1 byte
      LDRH  2 bytes
      ```
    - 写内存
      ```
      STR R0, [R1, #4]  ; 将R0的4个字节写入地址为“R1+4”中
      STRB  1 byte
      STRH  2 bytes
      ```
  
    - 加减
      ```
      ADD
      SUB
      ```
    - 跳转
       ```
       B main     ;直接跳转到main
       BL main    ;先保存返回地址到LR寄存器，然后跳转到main
       ```

---

  