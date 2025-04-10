#pragma once
#include <iostream>
#include <memory>
#include <boost/beast/core.hpp>
namespace beast = boost::beast;


enum DataType {
  Snapshot
};
enum TradeSide {
  BUY,
  SELL,
};
enum PriceChangeDirection {
  PlusTick,            // price rise
  ZeroPlusTick,        // trade occurs at the same price as the previous trade, which occurred at a price higher than that for the trade preceding it
  MinusTick,           // price drop
  ZeroMinusTick,       // trade occurs at the same price as the previous trade, which occurred at a price lower than that for the trade preceding it
};


struct DataTypeImpl {
  static DataType from_string(const char* str);
};
struct TradeSideImpl {
  static TradeSide from_string(const char* str);
};
struct PriceChangeDirectionImpl {
  static PriceChangeDirection from_string(const char* str);
};
struct TradeDataItem {
  std::string_view            symbol;                     // 's': Symbol name
  TradeSide                   side;                       // 'S': Side of taker. Buy,Sell
  std::string_view            volume;                     // 'v': Trade size
  std::string_view            price;                      // 'p': Trade price
  PriceChangeDirection        price_change_direction;     // 'L': Direction of price change. Unique field for Perps & futures
  std::string_view            trade_id;                   // 'i': Trade ID
  std::uint64_t               timestamp;                  // 'T': The timestamp (ms) that the order is filled
  bool                        block_trade;                // 'BT': Whether it is a block trade order or not
};
struct TradeSnapshot {
  std::string_view            topic;      // 'topic'
  std::string_view            type;       // 'type'
  std::uint64_t               time_stamp; // 'ts'
  std::vector<TradeDataItem>  data;       // 'data'
  void print();
};



struct TradeSnapshotImpl {
  static std::optional<TradeSnapshot> parse(beast::flat_buffer &buffer);
};



class WebsocketsApi {
public:
  void hello();
};

class Bybit {
public:
  std::unique_ptr<WebsocketsApi> ws_api;
  Bybit();
};