# 问题记录

1. 大量内存泄漏问题  
（1）connection断开后，没有释放connection(已解决)
    重新设计connection所有权，添加EPOLLRDHUP事件
2. 原本链接分配问题（已解决）
   代码小问题

# Reactor性能
4个io线程   
10个并发连接  
平均qps17w，提升（17-13）/13 = 30%