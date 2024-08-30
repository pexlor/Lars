#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "dns_route.h"
#include "lars_reactor.h"
#include <pthread.h>
//定义订阅列表数据关系类型，key->modid/cmdid， value->fds(订阅的客户端文件描述符)
typedef std::unordered_map<uint64_t,std::unordered_set<int>> subscribe_map;

//定义发布列表的数据关系类型, key->fd(订阅客户端的文件描述符), value->modids
typedef std::unordered_map<int, std::unordered_set<uint64_t>> publish_map;

class SubscribeList{
public:
    static void init(){
        _instance = new SubscribeList();
    }

    static SubscribeList * instance(){
        pthread_once(&_once,init);
        return _instance;
    }

    void subscribe(uint64_t mod,int fd); //订阅消息
    void unsubscribe(uint64_t mod,int fd);//取消订阅

    void publish(std::vector<uint64_t> &change_mods);//发布消息，其中change_mods是需要发布的那些modid/cmdid组合
    void make_publish_map(listen_fd_set &online_fds, publish_map &need_publish);//根据目前的在线的订阅用户，的到需要通信的发布订阅列表

private:
    SubscribeList();
    SubscribeList(const SubscribeList &) = delete;
    const SubscribeList& operator=(const SubscribeList) = delete;

private:
    static SubscribeList * _instance;
    static pthread_once_t _once;

    subscribe_map _book_list; //订阅清单，目前已经全部订阅的信息清单
    pthread_mutex_t _book_list_lock;

    publish_map _push_list; //发布清单，目前dns即将发布的的客户端及订阅清单
    pthread_mutex_t _push_list_lock;
    
};