## 先在这里写一下具体设计框架

## 1. 整个网络的类
* 所有的EV nodes构成的矩阵结构
* base station
* 每一个节点对应一个进程，初始化时启动所有进程

## 2. base station进程/类
* 接收信息，处理，发送信息

## 3. EV node进程/类
* 打印充电点可用信息
* 与其他EV node通信
* 与base station通信
* 收到termination时进程结束

## 4. 测试
* 数据集主要就是每个EV node随时间变化的可用节点情况，EV node进程隔一段时间读一条，作为自己的log mesg