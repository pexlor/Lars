#include "dns_route.h"

Route * Route::_instance = nullptr;
static  Route::pthread_once_t _once = PTHREAD_ONCE_INIT;