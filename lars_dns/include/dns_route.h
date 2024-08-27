#pragma once
#include <pthread.h>
#include <unorder_map.h>
#include <mysql/mysql.h>
#include <set.h>

typedef unordered_map<uint64_t,unordered_set<uint64_t>> route_map;

class Route{
public:
    static void init(){
        _instance = new Route();
    }

    static Route * instance(){
        pthread_once(&_once,init);
    }

private:
    Route();    
    Route(const Route&) = delete;
    Route& operator=(const Route&) = delete;

private:
    static Route * _instance;
    static pthread_once_t _once;

    MYSQL _db_conn;
    char _sql[1000];
    
    route_map *_data_pointer; //指向RouterDataMap_A 当前的关系map
    route_map *_temp_pointer; //指向RouterDataMap_B 临时的关系map
    pthread_rwlock_t _map_lock
};

