#include "dns_route.h"

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

/**
 * 后台周期性检查route信息的更新变化业务
 */
void *check_route_changes(void *args)
{
    int wait_time = 10;//10s自动修改一次，也可以从配置文件读取
    long last_load_time = time(NULL);
    //清空全部的RouteChange
    Route::instance()->remove_changes(true);
    //1 判断是否有修改
    while (true) {
        sleep(1);
        long current_time = time(NULL);

        //1.1 加载RouteVersion得到当前版本号
        int ret = Route::instance()->load_version();
        if (ret == 1) {
            //version改版 有modid/cmdid修改
            //2 如果有修改

            //2.1 将最新的RouteData加载到_temp_pointer中
            if (Route::instance()->load_route_data() == 0) {
                //2.2 更新_temp_pointer数据到_data_pointer map中
                Route::instance()->swap();
                last_load_time = current_time;//更新最后加载时间
            }

            //2.3 获取被修改的modid/cmdid对应的订阅客户端,进行推送         
            std::vector<uint64_t> changes;
            Route::instance()->load_changes(changes);

            //推送
            SubscribeList::instance()->publish(changes);

            //2.4 删除当前版本之前的修改记录tcp_server *server
            Route::instance()->remove_changes();
        }
        else {
            //3 如果没有修改
            if (current_time - last_load_time >= wait_time) {
                //3.1 超时,加载最新的temp_pointer
                if (Route::instance()->load_route_data() == 0) {
                    //3.2 _temp_pointer数据更新到_data_pointer map中
                    Route::instance()->swap();
                    last_load_time = current_time;
                }
            }
        }
    }
    return NULL;
}

Route::Route()
{
    pthread_rwlock_init(&_map_lock,NULL);
    _data_pointer = new route_map();
    _temp_pointer = new route_map();

    this->connect_db();//连接数据库
    this->build_maps();

    int ret = pthread_create(&_backendThread_tid,NULL,check_route_changes,NULL);
    if(ret == -1){
        perror("create backthread errror");
        exit(-1);
    }
    pthread_detach(_backendThread_tid);//分离线程
}

void Route::connect_db()
{
    // std::string db_host =  "127.0.0.1";
    // short db_port =3306;
    // std::string db_user = "pexlor";
    // std::string db_passwd = "234432rT";
    // std::string db_name = "lars_dns";

    // --- mysql数据库配置---
    std::string db_host = config_file::instance()->GetString("mysql", "db_host", "127.0.0.1");
    short db_port = config_file::instance()->GetNumber("mysql", "db_port", 3306);
    std::string db_user = config_file::instance()->GetString("mysql", "db_user", "root");
    std::string db_passwd = config_file::instance()->GetString("mysql", "db_passwd", "aceld");
    std::string db_name = config_file::instance()->GetString("mysql", "db_name", "lars_dns");

    mysql_init(&_db_conn);

    mysql_options(&_db_conn, MYSQL_OPT_CONNECT_TIMEOUT, "30");
  
    char reconnect = 1; 
    mysql_options(&_db_conn, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(&_db_conn, db_host.c_str(), db_user.c_str(), db_passwd.c_str(), db_name.c_str(), db_port, NULL, 0)) {
        fprintf(stderr, "Failed to connect mysql\n");
        exit(1);
    }
}

/**
 * @param 查询路由表
 */
void Route::build_maps()
{
    int ret = 0;

    snprintf(_sql, 1000, "SELECT * FROM RouteData;");//查询路由信息表的sql语句
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
/**
 * 加载当前版本号
 */
int Route::load_version()
{
    snprintf(_sql,1000,"SELECT version from RouteVersion WHERE id = 1;");
    int ret = mysql_real_query(&_db_conn, _sql, strlen(_sql));
    if (ret)
    {
        fprintf(stderr, "load version error: %s\n", mysql_error(&_db_conn));
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(&_db_conn);
    if (!result)
    {
        fprintf(stderr, "mysql store result: %s\n", mysql_error(&_db_conn));
        return -1;
    }

    long line_num = mysql_num_rows(result);
    if (line_num == 0)
    {
        fprintf(stderr, "No version in table RouteVersion: %s\n", mysql_error(&_db_conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    //得到version

    long new_version = atol(row[0]);

    if (new_version == this->_version)
    {
        //加载成功但是没有修改
        return 0;
    }
    this->_version = new_version;
    printf("now route version is %ld\n", this->_version);

    mysql_free_result(result);

    return 1;
}

/**
 * 加载RouteData信息表，到_temp_pointer中
 */
int Route::load_route_data()
{
    _temp_pointer->clear();

    snprintf(_sql, 100, "SELECT * FROM RouteData;");

    int ret = mysql_real_query(&_db_conn, _sql, strlen(_sql));
    if (ret)
    {
        fprintf(stderr, "load version error: %s\n", mysql_error(&_db_conn));
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(&_db_conn);
    if (!result)
    {
        fprintf(stderr, "mysql store result: %s\n", mysql_error(&_db_conn));
        return -1;
    }

    long line_num = mysql_num_rows(result);
    MYSQL_ROW row;
    for (long i = 0;i < line_num; ++i)
    {
        row = mysql_fetch_row(result);
        int modid = atoi(row[1]);
        int cmdid = atoi(row[2]);
        unsigned ip = atoi(row[3]);
        int port = atoi(row[4]);

        uint64_t key = ((uint64_t)modid << 32) + cmdid;
        uint64_t value = ((uint64_t)ip << 32) + port;

        (*_temp_pointer)[key].insert(value);
    }
    //printf("load data to tmep succ! size is %lu\n", _temp_pointer->size());

    mysql_free_result(result);

    return 0;
}

/**
 * 同步到Data表中，交互指针实现
 */
void Route::swap()
{
    pthread_rwlock_wrlock(&_map_lock);
    route_map *temp = _data_pointer;
    _data_pointer = _temp_pointer;
    _temp_pointer = temp;
    pthread_rwlock_unlock(&_map_lock);
}

/**
 * 加载routechange得到修改的modid/cmdid，将结果放在vector中
 */
void Route::load_changes(std::vector<uint64_t> &change_list)
{
    //读取当前版本之前的全部修改 
    snprintf(_sql, 1000, "SELECT modid,cmdid FROM RouteChange WHERE version <= %ld;", _version);
    int ret = mysql_real_query(&_db_conn, _sql, strlen(_sql));
    if (ret)
    {
        fprintf(stderr, "mysql_real_query: %s\n", mysql_error(&_db_conn));
        return ;
    }

    MYSQL_RES *result = mysql_store_result(&_db_conn);
    if (!result)
    {
        fprintf(stderr, "mysql_store_result %s\n", mysql_error(&_db_conn));
        return ;
    }

    long lineNum = mysql_num_rows(result);
    if (lineNum == 0)
    {
        fprintf(stderr,  "No version in table ChangeLog: %s\n", mysql_error(&_db_conn));
        return ;
    }
    MYSQL_ROW row;
    for (long i = 0;i < lineNum; ++i)
    {
        row = mysql_fetch_row(result);
        int modid = atoi(row[0]);
        int cmdid = atoi(row[1]);
        uint64_t key = (((uint64_t)modid) << 32) + cmdid;
        change_list.push_back(key);
    }
    mysql_free_result(result);
}
/**
 * 清空修改记录
 */
void Route::remove_changes(bool remove_all)
{
    if (remove_all == false)
    {
        snprintf(_sql, 1000, "DELETE FROM RouteChange WHERE version <= %ld;", _version);
    }
    else
    {
        snprintf(_sql, 1000, "DELETE FROM RouteChange;");
    }
    int ret = mysql_real_query(&_db_conn, _sql, strlen(_sql));
    if (ret != 0)
    {
        fprintf(stderr, "delete RouteChange: %s\n", mysql_error(&_db_conn));
        return ;
    } 
    return;
}