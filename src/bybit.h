#pragma once
#include <iostream>
#include <memory>

class WebsocketsApi {
public:
  void hello();
};

class Bybit {
public:
  std::unique_ptr<WebsocketsApi> ws_api;
  Bybit();
};