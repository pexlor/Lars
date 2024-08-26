#include "lars_reactor.h"
#include <mysql/mysql.h>

int main(int argc, char **argv)
{
    event_loop loop;

    //加载配置文件
    //config_file::setPath("conf/lars_dns.conf");
    std::string ip = "127.0.0.1";//config_file::instance()->GetString("reactor", "ip", "0.0.0.0");
    short port = 5000; //config_file::instance()->GetNumber("reactor", "port", 7778);


    //创建tcp服务器
    tcp_server *server = new tcp_server(ip.c_str(), port);

    //注册路由业务
    

    //测试mysql接口
    MYSQL dbconn;
    mysql_init(&dbconn);

    //开始事件监听    
    printf("lars dns service ....\n");

    server->start();
    return 0;
}