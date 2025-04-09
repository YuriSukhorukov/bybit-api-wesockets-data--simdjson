#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <chrono>
#include <simdjson.h>

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

using tcp = net::ip::tcp;
using namespace simdjson;

enum TradeSide {
  BUY,
  SELL,
};

struct TradeSideImpl {
  static TradeSide from_string(const char* str) {
    if (str == "Buy")   return TradeSide::BUY;
    if (str == "Sell")  return TradeSide::SELL;
  }
};

enum PriceChangeDirection {
  PlusTick,            // price rise
  ZeroPlusTick,        // trade occurs at the same price as the previous trade, which occurred at a price higher than that for the trade preceding it
  MinusTick,           // price drop
  ZeroMinusTick,       // trade occurs at the same price as the previous trade, which occurred at a price lower than that for the trade preceding it
};

struct PriceChangeDirectionImpl {
  static PriceChangeDirection from_string(const char* str) {
    if (str == "PlusTick")      return PriceChangeDirection::PlusTick;
    if (str == "ZeroPlusTick")  return PriceChangeDirection::ZeroPlusTick;
    if (str == "MinusTick")     return PriceChangeDirection::MinusTick;
    if (str == "ZeroMinusTick") return PriceChangeDirection::ZeroMinusTick;
  }
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
void TradeSnapshot::print() {
    std::cout << "Trade Snapshot = {\n";
    std::cout << "  topic: " << topic << std::endl; 
    std::cout << "  type: " << type << std::endl; 
    std::cout << "  time_stamp: " << time_stamp << std::endl;
    std::cout << "    data = {\n";
  for (auto item: data) {
    std::cout << "      item = {\n";
    std::cout << "        symbol:       " << item.symbol << std::endl;
    std::cout << "        side:         " << item.side << std::endl;
    std::cout << "        volume:       " << item.volume << std::endl;
    std::cout << "        price:        " << item.price << std::endl;
    std::cout << "        direction:    " << item.price_change_direction << std::endl;
    std::cout << "        trade_id:     " << item.trade_id << std::endl;
    std::cout << "        timestamp:    " << item.timestamp << std::endl;
    std::cout << "        block_trade:  " << item.block_trade << std::endl;
    std::cout << "      }\n";
  }
    std::cout << "    }\n";
    std::cout << "}\n";
};

void parse_msg(beast::flat_buffer &buffer) {  
  simdjson::padded_string json_view(
    static_cast<const char*>(buffer.data().data()), buffer.size()
  );
  // std::size_t len = buffer.size();
  // std::size_t allocated_size = buffer.capacity();
  // auto data_ptr = static_cast<const char*>(buffer.data().data());


  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;

  auto error = parser.iterate(json_view).get(doc);
  if (error) {
    std::cerr << "❌ Ошибка парсинга JSON: " << simdjson::error_message(error) << "\n";
    return;
  }

  auto topic = doc["topic"].get<std::string_view>();
  auto type = doc["type"].get<std::string_view>();
  auto ts = doc["ts"].get<std::uint64_t>();
  auto data = doc["data"].get_array();

  if (topic.error() ||
      type.error()  ||
      ts.error()) return;

  TradeSnapshot trade_snapshot = TradeSnapshot {
    .topic = topic.value(),
    .type = type.value(),
    .time_stamp = ts.value(),
    .data = {}
  };

  for (auto item: data) {
    auto s = item["s"].get<std::string_view>();           // 's': Symbol name
    auto S = item["S"].get<std::string_view>();           // 'S': Side of taker. Buy,Sell
    auto v = item["v"].get<std::string_view>();           // 'v': Trade size
    auto p = item["p"].get<std::string_view>();           // 'p': Trade price
    auto L = item["L"].get<std::string_view>();           // 'L': Direction of price change. Unique field for Perps & futures
    auto i = item["i"].get<std::string_view>();           // 'i': Trade ID
    auto T = item["T"].get<std::uint64_t>();              // 'T': The timestamp (ms) that the order is filled
    auto BT = item["BT"].get<bool>();                     // 'BT': Whether it is a block trade order or not

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

  trade_snapshot.print();
}
void do_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws);
void on_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws,
             beast::flat_buffer &buffer,
             beast::error_code ec,
             std::size_t bytes_transferred
) {
  if (ec == websocket::error::closed) {
    std::cout << "🔌 Соединение закрыто сервером\n";
    return;
  } else if (ec) {
    std::cerr << "❌ Ошибка чтения: " << ec.message() << std::endl;
    return;
  }
  do_read(ws); // Читаем следующее сообщение
  std::cout << "// ---------------------------------------------\n";
  auto start = std::chrono::high_resolution_clock::now();
  parse_msg(buffer);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "Execution time: " << duration.count() << "µs.\n";

  buffer.consume(bytes_transferred); // Очистка буфера
  // do_read(ws); // Читаем следующее сообщение
}
void do_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws) {
  auto *buffer = new beast::flat_buffer();
  // auto start = std::chrono::high_resolution_clock::now();

  ws.async_read(
      *buffer,
      [&, buffer](beast::error_code ec, std::size_t bytes_transferred) {
        on_read(ws, *buffer, ec, bytes_transferred);
        delete buffer;
      });
}

int main() {
    try {
      // ssl::context ctx(ssl::context::tls_client);
      // ctx.set_verify_mode(ssl::verify_none);

      net::io_context ioc;
      ssl::context ctx(ssl::context::tlsv12_client);
      
      ctx.set_default_verify_paths();
      ctx.set_verify_mode(ssl::verify_peer);

      auto const host = "stream.bybit.com";
      auto const port = "443";
      auto const target = "/v5/public/linear";

      tcp::resolver resolver(ioc);
      websocket::stream<beast::ssl_stream<tcp::socket>> ws(ioc, ctx);

      auto const results = resolver.resolve(host, port);
      net::connect(ws.next_layer().next_layer(), results);

      if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host)) {
        throw boost::system::system_error(
            ::ERR_get_error(), boost::asio::error::get_ssl_category());
      }

      ws.next_layer().handshake(ssl::stream_base::client);

      ws.handshake(host, target);

      std::cout << "✅ Успешное подключение к Bybit WebSocket API" << std::endl;

      std::string msg = R"({"op":"subscribe","args":["publicTrade.BTCUSDT", "publicTrade.ETHUSDT", "publicTrade.SOLUSDT", "publicTrade.SUIUSDT"]})";
      ws.write(net::buffer(msg));

      do_read(ws);
      ioc.run();
    } catch (const std::exception& e) {
      std::cerr << "Ошибка: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
}
