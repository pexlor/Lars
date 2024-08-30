#pragma once
#include <pthread.h>
#include <unordered_map>
#include <mysql/mysql.h>
#include <unordered_set>
#include "lars_reactor.h"
#include "dns_route.h"
#include "lars.pb.h"
#include "subscribe.h"
/**
 * @param 路由服务类
 * @details 该对象会从数据库中读取对应的服务器信息，每个服务可能对应多个服务器，读取后使用route_map存储
 */
typedef std::unordered_map<uint64_t,std::unordered_set<uint64_t>> route_map;

class Route{
public:
    static void init(){
        _instance = new Route();
    }

    static Route * instance(){
        pthread_once(&_once,init);
        return _instance;
    }

    std::unordered_set<uint64_t> get_hosts(uint64_t modid, uint64_t cmdid){
        return (*_data_pointer)[modid<<32+cmdid];
    }

    int load_version();
    int load_route_data();
    void swap();
    void load_changes(std::vector<uint64_t> &change_list);
    void remove_changes(bool remove_all = true);

private:
    Route();    
    Route(const Route&) = delete;
    Route& operator=(const Route&) = delete;
    void connect_db();
    void build_maps();

private:
    static Route * _instance;
    static pthread_once_t _once;
    long _version;
    MYSQL _db_conn;
    char _sql[1000];
    route_map *_data_pointer; //指向RouterDataMap_A 当前的关系map
    route_map *_temp_pointer; //指向RouterDataMap_B 临时的关系map
    pthread_rwlock_t _map_lock;
    pthread_t _backendThread_tid;
};

