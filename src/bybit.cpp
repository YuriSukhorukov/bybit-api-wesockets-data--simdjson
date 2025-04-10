#include "bybit.h"
#include <iostream>

void WebsocketsApi::hello() {
  std::cout << "hello bybit websockets api\n";
}

Bybit::Bybit(): ws_api(std::make_unique<WebsocketsApi>()) {};