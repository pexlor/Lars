#pragma once
#include <functional>
/*
 * 定义一些IO复用机制或者其他异常触发机制的事件封装
 *
 * */
class event_loop;

//IO事件触发的回调函数
typedef void io_callback(event_loop *loop, int fd, void *args);

/*
 * 封装一次IO触发实现 
 * */
struct io_event 
{
    io_event():read_callback(nullptr),write_callback(nullptr),rcb_args(nullptr),wcb_args(nullptr) {}
    int mask; //EPOLLIN EPOLLOUT
    io_callback *read_callback; //EPOLLIN事件 触发的回调
    io_callback *write_callback;//EPOLLOUT事件 触发的回调
    io_callback *close_callback;//关闭事件
    void *rcb_args; //read_callback的回调函数参数
    void *wcb_args; //write_callback的回调函数参数
    void *ccb_args; 
};