#pragma once
class io_buf
{
private:
    /* data */
public:
    //构造函数
    io_buf(int size);

    //清空数据
    void clean();

    //清除已经处理过的数据
    void adjust();
    
    //拷贝函数
    void copy(const io_buf * other_buf);

    void pop(int len);

public:
    //可能需要链表管理
    io_buf * next;
    //容量
    int capacity;
    //有效数据长度
    int length;
    //数据头部
    int head;
    //缓存地址
    char *data;
    
    ~io_buf();
};

