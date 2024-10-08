#pragma once
#include <unordered_map>
#include "io_buf.h"

/**
 * @param 内存池类
*/

enum MEM_CAP {
    m4K     = 4096, //1024*4
    m16K    = 16384,
    m64K    = 65536,
    m256K   = 262144,
    m1M     = 1048576,//1024*1024*1
    m4M     = 4194304,
    m8M     = 8388608
};


//总内存池最大限制 单位是Kb 所以目前限制是 5GB
#define EXTRA_MEM_LIMIT (5U *1024 *1024) 

/*
 *  定义buf内存池
 *  设计为单例模式
 * */
class buf_pool 
{
public:
    //初始化单例对象
    static void init() {
        //创建单例
        _instance = new buf_pool();
    }

    //获取单例方法
    static buf_pool *instance() {
        //保证init方法在这个进程执行中 只被执行一次
        pthread_once(&_once, init);
        return _instance;
    }

    //开辟一个io_buf
    io_buf *alloc_buf(int N);
    io_buf *alloc_buf() { return alloc_buf(m4K); }

    //重置一个io_buf
    void revert(io_buf *buffer);

private:
    buf_pool();

    //禁用拷贝构造和赋值操作
    buf_pool(const buf_pool&) = delete;
    const buf_pool& operator=(const buf_pool&) = delete;

    //所有buffer的一个map集合句柄
    std::unordered_map<int, io_buf*>  _pool;

    //总buffer池的内存大小 单位为KB
    uint64_t _total_mem;

    //单例对象
    static buf_pool *_instance;

    //用于保证创建单例的init方法只执行一次的锁
    static pthread_once_t _once;

    //用户保护内存池链表修改的互斥锁
    static pthread_mutex_t _mutex;
};