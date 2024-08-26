#include <string>
#include <string.h>
#include "tcp_server.h"


//回显业务的回调函数
void callback_busi(const char *data, uint32_t len, int msgid, net_connection *conn, void *user_data)
{
    //序列化
    std::string responseString(data,len);
    conn->send_message(responseString.c_str(), responseString.size(), msgid);
}


int main() 
{

    tcp_server server("127.0.0.1", 5000,4);

    //注册消息业务路由
    server.add_msg_router(1, callback_busi);
    server.start();
    

    return 0;
}
