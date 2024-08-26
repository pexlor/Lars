#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string>

#include "tcp_client.h"


struct Qps
{
    Qps() {
        last_time = time(NULL); 
        succ_cnt = 0;
    }

    long last_time;//最后一次发包时间 ms为单位
    int succ_cnt; //成功收到服务器回显的次数
};

//客户端业务
void busi(const char *data, uint32_t len, int msgid, net_connection  *conn, void *user_data)
{
    Qps *qps = (Qps*)user_data; //用户参数

    long current_time = time(NULL);
    if (current_time - qps->last_time >= 1) {
        //如果当前时间比最后记录时间大于1秒，那么我们进行记录
        printf("---> qps = %d <----\n", qps->succ_cnt);
        qps->last_time = current_time;//记录最后时间
        qps->succ_cnt = 0;//清空成功次数
    }
    ++qps->succ_cnt;
    std::string requestString = "1234545ads";

    conn->send_message(requestString.c_str(), requestString.size(), msgid);
}

//创建链接成功之后
void connection_start(net_connection *client, void *args)
{
    std::string requestString = "1234545ads";

    int msgid = 1;//与server端的消息路由一致
    client->send_message(requestString.c_str(), requestString.size(), msgid);
}

void *client_thread_main(void *args)
{
    //给服务端发包
     
    event_loop loop; 

    tcp_client client(&loop, "127.0.0.1", 5000, "qps client");

    Qps qps;

    //设置回调
    client.add_msg_router(1, busi, (void*)&qps);

    //设置链接创建成功之后Hook
    client.set_conn_start(connection_start);

    loop.event_process();

    return NULL;
}


int main(int argc, char **argv) 
{
    if (argc == 1) {
        printf("Usage: ./client [threadNum]\n");
        return 1;
    }

    //创建N个线程
    int thread_num = atoi(argv[1]);
    pthread_t *tids;
    tids = new pthread_t[thread_num];

    for (int i = 0; i < thread_num; i++) {
        pthread_create(&tids[i], NULL, client_thread_main, NULL);
    }

    for (int i = 0; i < thread_num; i++) {
        pthread_join(tids[i], NULL);
    }

    return 0;
}