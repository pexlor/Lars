#include "lars_reactor.h"
#include "dns_route.h"
#include "lars.pb.h"

void get_route(const char *data, uint32_t len, int msgid, net_connection *net_conn, void *user_data)
{
    //1. 解析proto文件
    lars::GetRouteRequest req;

    req.ParseFromArray(data, len);

     
    //2. 得到modid 和 cmdid
    int modid, cmdid;

    modid = req.modid();
    cmdid = req.cmdid();
    
    //3. 根据modid/cmdid 获取 host信息
    auto hosts = Route::instance()->get_hosts(modid, cmdid);

    //4. 将数据打包成protobuf
    lars::GetRouteResponse rsp;

    rsp.set_modid(modid);
    rsp.set_cmdid(cmdid);
    
    for (auto it = hosts.begin(); it != hosts.end(); it ++) {
        uint64_t ip_port = *it;
        lars::HostInfo host;
        host.set_ip((uint32_t)(ip_port >> 32));
        host.set_port((int)(ip_port));
        rsp.add_host()->CopyFrom(host);
    }
    
    //5. 发送给客户端
    std::string responseString;
    rsp.SerializeToString(&responseString);
    net_conn->send_message(responseString.c_str(), responseString.size(), lars::ID_GetRouteResponse);
}

int main(int argc, char **argv)
{
    std::string ip =  "0.0.0.0";
    short port =  5000;
    //创建tcp服务器
    tcp_server *server = new tcp_server(ip.c_str(), port);
    //注册路由业务
    server->add_msg_router(lars::ID_GetRouteRequest, get_route);
    //开始事件监听    
    printf("lars dns service ....\n");
    server->start();

    return 0;
}