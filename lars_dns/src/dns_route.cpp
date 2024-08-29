#include "dns_route.h"
#include <string.h>
/*
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
    char _sql[1000];s

    route_map *_data_pointer; //指向RouterDataMap_A 当前的关系map
    route_map *_temp_pointer; //指向RouterDataMap_B 临时的关系map
    pthread_rwlock_t _map_lock
};
*/




Route * Route::_instance = nullptr;
pthread_once_t Route::_once = PTHREAD_ONCE_INIT;

Route::Route()
{
    pthread_rwlock_init(&_map_lock,NULL);
    _data_pointer = new route_map();
    _temp_pointer = new route_map();

    this->connect_db();
    this->build_maps();
}

void Route::connect_db()
{
    std::string db_host =  "127.0.0.1";
    short db_port =3306;
    std::string db_user = "root";
    std::string db_passwd = "aceld";
    std::string db_name = "lars_dns";
    mysql_init(&_db_conn);

    mysql_options(&_db_conn, MYSQL_OPT_CONNECT_TIMEOUT, "30");
  
    char reconnect = 1; 
    mysql_options(&_db_conn, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(&_db_conn, db_host.c_str(), db_user.c_str(), db_passwd.c_str(), db_name.c_str(), db_port, NULL, 0)) {
        fprintf(stderr, "Failed to connect mysql\n");
        exit(1);
    }
}

void Route::build_maps()
{
    int ret = 0;

    snprintf(_sql, 1000, "SELECT * FROM RouteData;");//查询路由信息表
    ret = mysql_real_query(&_db_conn, _sql, strlen(_sql));
    if ( ret != 0) {
        fprintf(stderr, "failed to find any data, error %s\n", mysql_error(&_db_conn));
        exit(1);
    }

    //得到结果集
    MYSQL_RES *result = mysql_store_result(&_db_conn);
    
    //得到行数
    long line_num = mysql_num_rows(result);

    MYSQL_ROW row;
    for (long i = 0; i < line_num; i++) {
        row = mysql_fetch_row(result);
        int modID = atoi(row[1]);
        int cmdID = atoi(row[2]);
        unsigned ip = atoi(row[3]);
        int port = atoi(row[4]);

        //组装map的key，有modID/cmdID组合
        uint64_t key = ((uint64_t)modID << 32) + cmdID;
        uint64_t value = ((uint64_t)ip << 32) + port;

        printf("modID = %d, cmdID = %d, ip = %lu, port = %d\n", modID, cmdID, ip, port);

        //插入到RouterDataMap_A中
        (*_data_pointer)[key].insert(value);
    }

    mysql_free_result(result);
}
