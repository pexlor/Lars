# 问题记录

1. 大量内存泄漏问题（1）connection断开后，没有释放connection(已解决)重新设计connection所有权，添加EPOLLRDHUP事件
2. 原本链接分配问题（已解决）
   代码小问题，将任务均匀分配到给线程任务队列

# Reactor框架性能测试

4个io线程
10个并发连接
平均qps17w，提升（17-13）/13 = 30%

## 网络框架层提升性能的措施

1. 使用了内存池，每个连接的缓冲区从内存池里面拿
2. 每个线程都有自己的任务队列，tcp_server分配任务时直接发送到对应线程的任务队列

## 待改进方面

1. 负载均衡的线程池，可加入优先级策略
2. 

# DNS服务流程

![DNS SERVICE](./res/3-Lars-dnsserver.webp "DNS SERVICE")

# report service 流程

# LoadBalance Agent模块

一个服务称为一个模块，一个模块由modid+cmdid来标识
modid+cmdid的组合表示一个远程服务，这个远程服务一般部署在多个节点上

## 节点获取服务

业务方每次要向远程服务发送消息时，先利用modid+cmdid去向LB Agent获取一个可用节点，然后向该节点发送消息，完成一次远程调用；具体获取modid+cmdid下的哪个节点是由LB Agent负责的

## 节点调用上报服务

对LB Agent节点的一次远程调用后，调用结果会汇报给LB Agent，以便LB Agent根据自身的LB算法来感知远程服务节点的状态是空闲还是过载，进而控制节点获取时的节点调度.

LB Agent拥有5个线程，一个LB算法：

UDP Server服务，并运行LB算法，对业务提供节点获取和节点调用结果上报服务；为了增大系统吞吐量，使用3个UDP Server服务互相独立运行LB算法：modid+cmdid % 3 = i的那些模块的服务与调度，由第i+1个UDP Server线程负责

Dns Service Client：是dnsserver的客户端线程，负责根据需要，向dnsserver获取一个模块的节点集合（或称为获取路由）；UDP Server会按需向此线程的MQ写入获取路由请求，DSS Client将MQ到来的请求转发到dnsserver，之后将dnsserver返回的路由信息更新到对应的UDP Server线程维护的路由信息中

Report Service Client：是reporter的客户端线程，负责将每个模块下所有节点在一段时间内的调用结果、过载情况上报到reporter Service端，便于观察情况、做报警；本身消费MQ数据，UDP Server会按需向MQ写入上报状态请求。


### 负载均衡算法
● Load Balance从空闲队列拿出队列头部节点，作为选取的节点返回，同时将此节点重追到队列尾部；
● probe机制 ：如果此模块过载队列非空，则每经过probe_num次节点获取后（默认=10），给过载队列中的节点一个机会，从过载队列拿出队列头部节点，作为选取的节点返回，让API试探性的用一下，同时将此节点重追到队列尾部；
● 如果空闲队列为空，说明整个模块过载了，返回过载错误；且也会经过probe_num次节点获取后（默认=10），给过载队列中的节点一个机会，从过载队列拿出队列头部节点，作为选取的节点返回，让API试探性的用一下，同时将此节点重追到队列尾部；

