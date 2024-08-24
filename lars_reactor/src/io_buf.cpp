#include "io_buf.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
/*
class io_buf
{
private:
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

    int getLength();
public:
    //可能需要链表管理
    io_buf * next;

private:
    //容量
    int capoacity;
    //有效数据长度
    int length;
    //数据头部
    int head;
    //缓存地址
    char *data;
private:
    ~io_buf();
};
*/
io_buf::io_buf(int size) 
            :capacity(size)
            ,length(0)
            ,head(0)
            ,next(nullptr)
{
    data = new char[capacity];
    assert(data);
}

void io_buf::clean()
{
    length  = 0;
    head    = 0;
}

void io_buf::adjust()
{
    if(head != 0 && length !=0){
        memmove(data,data+head,length);
        head = 0;
    }
}

void io_buf::copy(const io_buf * other_buf)
{
    memcpy(data,other_buf->data + other_buf->head,other_buf->length);
    head = 0;
    length = other_buf->length;
}

void io_buf::pop(int len)
{
    length -= len;
    head += len;
}

io_buf::~io_buf()
{
    if(data){
        delete data;
    }
}

