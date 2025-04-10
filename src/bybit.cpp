#include "bybit.h"
#include <iostream>
#include <simdjson.h>

DataType DataTypeImpl::from_string(const char* str) {
  if (str == "snapshot") return DataType::Snapshot;
}
TradeSide TradeSideImpl::from_string(const char* str) {
  if (str == "Buy")   return TradeSide::BUY;
  if (str == "Sell")  return TradeSide::SELL;
}
PriceChangeDirection PriceChangeDirectionImpl::from_string(const char* str) {
  if (str == "PlusTick")      return PriceChangeDirection::PlusTick;
  if (str == "ZeroPlusTick")  return PriceChangeDirection::ZeroPlusTick;
  if (str == "MinusTick")     return PriceChangeDirection::MinusTick;
  if (str == "ZeroMinusTick") return PriceChangeDirection::ZeroMinusTick;
}
void TradeSnapshot::print() {
  std::cout << "Trade Snapshot = {\n";
  std::cout << "  topic: " << topic << std::endl; 
  std::cout << "  type: " << type << std::endl; 
  std::cout << "  time_stamp: " << time_stamp << std::endl;
  std::cout << "  data = {\n";
for (auto item: data) {
  std::cout << "    item = {\n";
  std::cout << "      symbol:       " << item.symbol << std::endl;
  std::cout << "      side:         " << item.side << std::endl;
  std::cout << "      volume:       " << item.volume << std::endl;
  std::cout << "      price:        " << item.price << std::endl;
  std::cout << "      direction:    " << item.price_change_direction << std::endl;
  std::cout << "      trade_id:     " << item.trade_id << std::endl;
  std::cout << "      timestamp:    " << item.timestamp << std::endl;
  std::cout << "      block_trade:  " << item.block_trade << std::endl;
  std::cout << "    }\n";
}
  std::cout << "  }\n";
  std::cout << "}\n";
}
std::optional<TradeSnapshot> TradeSnapshotImpl::parse(simdjson::ondemand::document &doc) {
  auto topic = doc["topic"].get<std::string_view>();
  auto type = doc["type"].get<std::string_view>();
  auto ts = doc["ts"].get<std::uint64_t>();
  auto data = doc["data"].get_array();

  if (topic.error() ||
      type.error()  ||
      ts.error()    ||
      data.error()) return std::nullopt;

  TradeSnapshot trade_snapshot = TradeSnapshot {
    .topic = topic.value(),
    .type = type.value(),
    .time_stamp = ts.value(),
    .data = {}
  };

  for (auto item: data) {
    auto s = item["s"].get<std::string_view>();
    auto S = item["S"].get<std::string_view>();
    auto v = item["v"].get<std::string_view>();
    auto p = item["p"].get<std::string_view>();
    auto L = item["L"].get<std::string_view>();
    auto i = item["i"].get<std::string_view>();
    auto T = item["T"].get<std::uint64_t>();   
    auto BT = item["BT"].get<bool>();          

    if (s.error() ||
        S.error() ||
        v.error() ||
        p.error() ||
        L.error() ||
        i.error() ||
        T.error() ||
        BT.error()) continue;

    TradeDataItem trade_data_item = TradeDataItem {
      .symbol                   = s.value(),
      .side                     = TradeSideImpl::from_string(S.value().data()),
      .volume                   = v.value(),
      .price                    = p.value(),
      .price_change_direction   = PriceChangeDirectionImpl::from_string(L.value().data()),
      .trade_id                 = i.value(),
      .block_trade              = BT.value()
    };

    trade_snapshot.data.push_back(trade_data_item);
  }

  return trade_snapshot;
}

void WebsocketsApi::hello() {
  std::cout << "hello bybit websockets api\n";
}

std::optional<TradeSnapshot> WebsocketsApi::get_trade_snapshot(simdjson::ondemand::document &doc) {
  return TradeSnapshotImpl::parse(doc);
}

Bybit::Bybit(): ws_api(std::make_unique<WebsocketsApi>()) {};