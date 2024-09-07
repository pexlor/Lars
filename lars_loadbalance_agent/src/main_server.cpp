#pragma once
#include "lars_reactor.h"
#include "lars.pb.h"
#include "route_lb.h"
#include "lb_agent_logo.h"
#include <netdb.h>
#include <arpa/inet.h>
#include "main_server.h"

struct load_balance_config
{
    //经过若干次获取请求host节点后，试探选择一次overload过载节点
    int probe_num; 

    //初始化host_info主机信息访问成功的个数，防止刚启动时少量失败就认为过载
    int init_succ_cnt;

    //当idle节点切换至over_load时的初始化失败次数，主要为了累计一定成功次数才能切换会idle
    int init_err_cnt;

    //当idle节点失败率高于此值，节点变overload状态
    float err_rate;

    //当overload节点成功率高于此值，节点变成idle状态
    float succ_rate;

    //当idle节点连续失败次数超过此值，节点变成overload状态
    int contin_err_limit;

    //当overload节点连续成功次数超过此值, 节点变成idle状态
    int contin_succ_limit;

    //当前agent本地ip地址(用于上报 填充caller字段)
    uint32_t local_ip;

// *************************************
    //整个窗口的真实失败率阈值
    float window_err_rate;

    //对于某个modid/cmdid下的某个idle状态的host，需要清理一次负载信息的周期
    int idle_timeout;

    //对于某个modid/cmdid/下的某个overload状态的host，在过载队列等待的最大时间
    int overload_timeout;

    //对于每个NEW状态的modid/cmdid，多久更新一下本地路由,秒
    long update_timeout;
};
// *************************************

//--------- 全局资源 ----------
struct load_balance_config lb_config;

//与report_client通信的thread_queue消息队列
thread_queue<lars::ReportStatusRequest>* report_queue = NULL;
//与dns_client通信的thread_queue消息队列
thread_queue<lars::GetRouteRequest>* dns_queue = NULL;

//每个Agent UDP server的 负载均衡器路由 route_lb
route_lb * r_lb[3];



static void create_route_lb()
{
    for (int i = 0; i < 3; i++) {
        int id = i + 1; //route_lb的id从1 开始计数
        r_lb[i]  = new route_lb(id);
        if (r_lb[i] == NULL) {
            fprintf(stderr, "no more space to new route_lb\n");
            exit(1);
        }
    }
}

static void init_lb_agent()
{
    //1. 加载配置文件
    config_file::setPath("./conf/lars_lb_agent.conf");
    lb_config.probe_num = config_file::instance()->GetNumber("loadbalance", "probe_num", 10);
    lb_config.init_succ_cnt = config_file::instance()->GetNumber("loadbalance", "init_succ_cnt", 180);
    lb_config.init_err_cnt = config_file::instance()->GetNumber("loadbalance", "init_err_cnt", 5);
    lb_config.err_rate = config_file::instance()->GetFloat("loadbalance", "err_rate", 0.1);
    lb_config.succ_rate = config_file::instance()->GetFloat("loadbalance", "succ_rate", 0.92);
    lb_config.contin_succ_limit = config_file::instance()->GetNumber("loadbalance", "contin_succ_limit", 10);
    lb_config.contin_err_limit = config_file::instance()->GetNumber("loadbalance", "contin_err_limit", 10);
  //*******************************************
    lb_config.window_err_rate = config_file::instance()->GetFloat("loadbalance", "window_err_rate", 0.7);
    lb_config.idle_timeout = config_file::instance()->GetNumber("loadbalance", "idle_timeout", 15);
    lb_config.overload_timeout = config_file::instance()->GetNumber("loadbalance", "overload_timeout", 15);
  //*******************************************
    //2. 初始化3个route_lb模块
    create_route_lb();

    //3. 加载本地ip
    char my_host_name[1024];
    if (gethostname(my_host_name, 1024) == 0) {
        struct hostent *hd = gethostbyname(my_host_name);

        if (hd)
        {
            struct sockaddr_in myaddr;
            myaddr.sin_addr = *(struct in_addr*)hd->h_addr;
            lb_config.local_ip = ntohl(myaddr.sin_addr.s_addr);
        }
    }

    if (!lb_config.local_ip)  {
        struct in_addr inaddr;
        inet_aton("127.0.0.1", &inaddr);
        lb_config.local_ip = ntohl(inaddr.s_addr);
    }
}

int main(int argc, char **argv)
{
    lars_lbagent_logo(); //打印log

    //1 初始化环境
    init_lb_agent();

    //1.5 初始化LoadBalance的负载均衡器
    

    //2 启动udp server服务,用来接收业务层(调用者/使用者)的消息
    start_UDP_servers();
    
    //3 启动lars_reporter client 线程
    report_queue = new thread_queue<lars::ReportStatusRequest>();
    if (report_queue == NULL) {
        fprintf(stderr, "create report queue error!\n");
        exit(1);
    }
    start_report_client();
    
    //4 启动lars_dns client 线程
    dns_queue = new thread_queue<lars::GetRouteRequest>();
    if (dns_queue == NULL) {
        fprintf(stderr, "create dns queue error!\n");
        exit(1);
    }
    start_dns_client();
    
    while (1) {
        sleep(10);
    }

    return 0;
}
