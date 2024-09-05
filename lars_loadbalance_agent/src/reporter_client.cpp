#include "lars_reactor.h"
#include "main_server.h"
#include <pthread.h>


//typedef void io_callback(event_loop *loop, int fd, void *args);
//只要thread_queue有数据，loop就会触发此回调函数来处理业务
void new_report_request(event_loop *loop, int fd, void *args)
{
    tcp_client *client = (tcp_client*)args;

    //1. 将请求数据从thread_queue中取出，
    std::queue<lars::ReportStatusRequest>  msgs;

    //2. 将数据放在queue队列中
    report_queue->recv(msgs);
    
    //3. 遍历队列，通过client依次将每个msg发送给reporter service
    while (!msgs.empty()) {
        lars::ReportStatusRequest req = msgs.front();
        msgs.pop();

        std::string requestString;
        req.SerializeToString(&requestString);

        //client 发送数据
        client->send_message(requestString.c_str(), requestString.size(), lars::ID_ReportStatusRequest);
    }
}

/*
report client主要是实现thread_queue的回调业务
udp server会定期的上传上报数据到reporter，那么请求对于report client就是透传给reporter serivce即可
*/
void *report_client_thread(void* args)
{
    printf("report client thread start\n");
    event_loop loop;
    //1 加载配置文件得到repoter ip + port
    std::string ip = "0.0.0.0";
    short port = 5000; //待定

    //2 创建客户端
    tcp_client client(&loop, ip.c_str(), port, "reporter client");

    //3 将 thread_queue消息回调事件，绑定到loop中
    report_queue->set_loop(&loop);
    report_queue->set_callback(new_report_request, &client);

    //4 启动事件监听
    loop.event_process();

    return NULL;
}

void start_report_client()
{
    //开辟一个线程
    pthread_t tid;

    //启动线程业务函数
    int ret = pthread_create(&tid, NULL, report_client_thread, NULL);
    if (ret == -1) {
        perror("pthread_create");
        exit(1);
    }

    //设置分离模式
    pthread_detach(tid);
}