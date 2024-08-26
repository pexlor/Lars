#pragma once
#include <pthread.h>
#include <unorder_map.h>

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
};

