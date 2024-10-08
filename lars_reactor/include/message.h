#pragma once

#include <map>
#include "net_connection.h"
//解决tcp粘包问题的消息头
struct msg_head
{
    int msgid;
    int msglen;
};

//消息头的二进制长度，固定数
#define MESSAGE_HEAD_LEN 8

//消息头+消息体的最大长度限制
#define MESSAGE_LENGTH_LIMIT (65535 - MESSAGE_HEAD_LEN)

//msg 业务回调函数原型

typedef void msg_callback(const char *data, uint32_t len, int msgid, net_connection *net_conn, void *user_data);

class msg_router
{
public:
    msg_router(){
        
    }

    ~msg_router(){

    }

    bool register_msg_router(int msgid,msg_callback *msg_cb,void* user_data)
    {
        if(_router.find(msgid) != _router.end()) {
            //该msgID的回调业务已经存在
            return -1;
        }

        _router[msgid] = msg_cb;
        _args[msgid] = user_data;

        return 0;
    }

    void call(int msgid,uint32_t msglen,const char * data,net_connection *net_conn)
    {
        //判断msgid对应的回调是否存在
        if (_router.find(msgid) == _router.end()) {
            fprintf(stderr, "msgid %d is not register!\n", msgid);
            return;
        }

        //直接取出回调函数，执行
        msg_callback *callback = _router[msgid];
        void *user_data = _args[msgid];
        callback(data, msglen, msgid, net_conn, user_data);
    }

private:
    std::map<int, msg_callback*> _router;
    std::map<int, void*> _args;

};

