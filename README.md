# 问题记录

1. 大量内存泄漏问题   
（1）connection断开后，没有释放connection(已解决)  
    重新设计connection所有权，添加EPOLLRDHUP事件
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

