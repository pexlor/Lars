#include "subscribe.h"
#include <unordered_set>
/*
class SubscribeList{
public:
    static void init(){
        _instance = new SubscribeList();
    }

    static SubscribeList * instance(){
        pthread_once(&_once,init);
        return _instance;
    }

    void subscribe(uint64_t mod,int fd);
    void unsubscribe(uint64_t mod,int fd);

    void publish();
    void make_publish_map();

private:
    SubscribeList();
    SubscribeList(const SubscribeList &) = delete;
    const SubscribeList& operator=(const SubscribeList) = delete;

private:
    static SubscribeList * _instance;
    static pthread_once_t _once;
    
    subscribe_map _book_list; //订阅清单
    pthread_mutex_t _book_list_lock;

    publish_map _push_list; //发布清单
    pthread_mutex_t _push_list_lock;

}; */
extern tcp_server *server;//server 外链接声明，server定义在上层服务中


//静态变量初始化
SubscribeList * SubscribeList::_instance = nullptr;
pthread_once_t SubscribeList::_once = PTHREAD_ONCE_INIT;

SubscribeList::SubscribeList()
{
    
}

/**
 *  订阅服务
 * @param mod  modid<<32 + comid
 * @param fd 订阅客户端的套接字fd
*/
void SubscribeList::subscribe(uint64_t mod, int fd)
{
    //将mod->fd的关系加入到_book_list中
    pthread_mutex_lock(&_book_list_lock);
    _book_list[mod].insert(fd);
    pthread_mutex_unlock(&_book_list_lock);
}

//取消订阅
void SubscribeList::unsubscribe(uint64_t mod, int fd)
{
    //将mod->fd关系从_book_list中删除
    pthread_mutex_lock(&_book_list_lock);
    if (_book_list.find(mod) != _book_list.end()) {
        _book_list[mod].erase(fd);
        if (_book_list[mod].empty() == true) {
            _book_list.erase(mod);
        }
    }
    pthread_mutex_unlock(&_book_list_lock);

}

void push_change_task(event_loop *loop, void *args)
{
    SubscribeList *subscribe = (SubscribeList*)args;

    //1 获取全部的在线客户端fd
    std::unordered_set<int> online_fds;
    loop->get_listen_fds(online_fds);
    
    //2 从subscribe的_push_list中 找到与online_fds集合匹配，放在一个新的publish_map里
    publish_map need_publish;
    subscribe->make_publish_map(online_fds, need_publish);

    //3 依次从need_publish取出数据 发送给对应客户端链接
 
    for (auto it = need_publish.begin(); it != need_publish.end(); it++) {
        int fd = it->first; //fd

        //遍历 fd对应的 modid/cmdid集合
        for (auto st = it->second.begin(); st != it->second.end(); st++) {
            //一个modid/cmdid
            int modid = int((*st) >> 32);
            int cmdid = int(*st);

            //组装pb消息，发送给客户
            lars::GetRouteResponse rsp; 
            rsp.set_modid(modid);
            rsp.set_cmdid(cmdid);

            //通过route查询对应的host ip/port信息 进行组装
            std::unordered_set<uint64_t> hosts = Route::instance()->get_hosts(modid, cmdid) ;
            for (auto hit = hosts.begin(); hit != hosts.end(); hit++) {
                uint64_t ip_port_pair = *hit;
                lars::HostInfo host_info;
                host_info.set_ip((uint32_t)(ip_port_pair >> 32));
                host_info.set_port((int)ip_port_pair);
                //添加到rsp中
                rsp.add_host()->CopyFrom(host_info);
            }

            //给当前fd 发送一个更新消息
            std::string responseString;
            rsp.SerializeToString(&responseString);

            //通过fd取出链接信息
            net_connection *conn = tcp_server::conns[fd];
            if (conn != NULL) {
                conn->send_message(responseString.c_str(), responseString.size(), lars::ID_GetRouteResponse);
            }
        }
    }
}

//根据在线用户fd得到需要发布的列表
void SubscribeList::make_publish_map(
            listen_fd_set &online_fds, 
            publish_map &need_publish)
{
    publish_map::iterator it;

    pthread_mutex_lock(&_push_list_lock);
    //遍历_push_list 找到 online_fds匹配的数据，放到need_publish中
    for (it = _push_list.begin(); it != _push_list.end(); it++)  {
        //it->first 是 fd
        //it->second 是 modid/cmdid
        if (online_fds.find(it->first) != online_fds.end()) {
            //匹配到
            //当前的键值对移动到need_publish中
            need_publish[it->first] = _push_list[it->first];
            //当该组数据从_push_list中删除掉
            _push_list.erase(it);
        }
    }

    pthread_mutex_unlock(&_push_list_lock);
}

/**
 * 发布消息
 * @param change_mods 需要发布的那些modid/cmdid组合
 */
void SubscribeList::publish(std::vector<uint64_t> &change_mods)
{
    //1 将change_mods已经修改的mod->fd 
    //  放到 发布清单_push_list中 
    pthread_mutex_lock(&_book_list_lock);
    pthread_mutex_lock(&_push_list_lock);

    //在_book_list找到这些mods并加入_push_list中
    for (auto it = change_mods.begin(); it != change_mods.end(); it++) {
        uint64_t mod = *it;
        if (_book_list.find(mod) != _book_list.end()) {
            //将mod下面的fd set集合拷迁移到 _push_list中
            for (auto fds_it = _book_list[mod].begin(); fds_it != _book_list[mod].end(); fds_it++) {
                int fd = *fds_it;
                _push_list[fd].insert(mod);
            }
        }
    }

    pthread_mutex_unlock(&_push_list_lock);
    pthread_mutex_unlock(&_book_list_lock);
    //2 通知各个线程去执行推送任务
    //这里解释一下为什么要给全部的线程推送发布服务，是因为不知道要发布的fd在那个线程里面，只好丢给线程自己去判断
    server->thread_poll()->send_task(push_change_task, this);
}