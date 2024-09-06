#include "route_lb.h"
#include "lars.pb.h"


//构造初始化
route_lb::route_lb(int id):_id(id)
{
    pthread_mutex_init(&_mutex, NULL); //初始化锁
}

//agent 上报某主机的获取结果
void route_lb::report_host(lars::ReportRequest req)
{
    int modid = req.modid();
    int cmdid = req.cmdid();
    int retcode = req.retcode();
    int ip = req.host().ip();
    int port = req.host().port();
    
    uint64_t key = ((uint64_t)modid << 32) + cmdid;

    pthread_mutex_lock(&_mutex);
    if (_route_lb_map.find(key) != _route_lb_map.end()) {
        load_balance *lb = _route_lb_map[key];

        lb->report(ip, port, retcode);

        //上报信息给远程reporter服务器
        lb->commit();
    }

    pthread_mutex_unlock(&_mutex);
}

/*
get_host在获取host的时候是一个动态的获取模式，如果根据当前的modid/cmdid请求的load_balance模块来获取，
如果load_balance存在，则直接调用load_balance的choice_one_host()方法获取。 
如果load_balance不存在，需要新建load_balance，并且当前的load_balance所携带的host信息，
需要从远程dns service下载拉取下来，调用pull()方法来实现。 */

//agent获取一个host主机，将返回的主机结果存放在rsp中
int route_lb::get_host(int modid, int cmdid, lars::GetHostResponse &rsp)
{
    int ret = lars::RET_SUCC;

    //1. 得到key
    uint64_t key = ((uint64_t)modid << 32) + cmdid;

    pthread_mutex_lock(&_mutex);
    //2. 当前key已经存在_route_lb_map中
    if (_route_lb_map.find(key) != _route_lb_map.end()) {
        //2.1 取出对应的load_balance
        load_balance *lb = _route_lb_map[key];

        if (lb->empty() == true) {
            //存在lb 里面的host为空，说明正在pull()中，还没有从dns_service返回来,所以直接回复不存在
            assert(lb->status == load_balance::PULLING);
            rsp.set_retcode(lars::RET_NOEXIST);
        }
        else {
            ret = lb->choice_one_host(rsp);
            rsp.set_retcode(ret);
            //TODO 超时重拉路由
        }
    }
    //3. 当前key不存在_route_lb_map中
    else {
        //3.1 新建一个load_balance
        load_balance *lb = new load_balance(modid, cmdid);
        if (lb == NULL) {
            fprintf(stderr, "no more space to create loadbalance\n");
            exit(1);
        }

        //3.2 新建的load_balance加入到map中
        _route_lb_map[key] = lb;

        //3.3 从dns service服务拉取具体的host信息
        lb->pull();

        //3.4 设置rsp的回执retcode
        rsp.set_retcode(lars::RET_NOEXIST);

        ret = lars::RET_NOEXIST;
    }

    pthread_mutex_unlock(&_mutex);

    return ret;
}

//根据Dns Service返回的结果更新自己的route_lb_map
int route_lb::update_host(int modid, int cmdid, lars::GetRouteResponse &rsp)
{
    //1. 得到key
    uint64_t key = ((uint64_t)modid << 32) + cmdid;

    pthread_mutex_lock(&_mutex);

    //2. 在_route_map中找到对应的key 
    if (_route_lb_map.find(key) != _route_lb_map.end()) {
        
        load_balance *lb = _route_lb_map[key];

        if (rsp.host_size() == 0) {
            //2.1 如果返回的结果 lb下已经没有任何host信息，则删除该key
            delete lb;
            _route_lb_map.erase(key);
        }
        else {
            //2.2 更新新host信息
            lb->update(rsp);
        }
    }
    
    pthread_mutex_unlock(&_mutex);

    return 0;
}