#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <chrono>
#include <simdjson.h>
#include "bybit.h"

namespace net         = boost::asio;
namespace ssl         = boost::asio::ssl;
namespace beast       = boost::beast;
namespace websocket   = beast::websocket;

using tcp = net::ip::tcp;
using namespace simdjson;

void do_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws, 
             Bybit &bybit_instance);
void on_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws,
             beast::flat_buffer &buffer,
             beast::error_code ec,
             std::size_t bytes_transferred,
             Bybit &bybit_instance);



void do_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws, 
             Bybit &bybit_instance
) {
  auto *buffer = new beast::flat_buffer();
  ws.async_read(
      *buffer,
      [&, buffer](beast::error_code ec, std::size_t bytes_transferred) {
        on_read(ws, *buffer, ec, bytes_transferred, bybit_instance);
        delete buffer;
      });
}
void on_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws,
             beast::flat_buffer &buffer,
             beast::error_code ec,
             std::size_t bytes_transferred,
             Bybit &bybit_instance
) {
  if (ec == websocket::error::closed) {
    std::cout << "🔌 Соединение закрыто сервером\n";
    return;
  } else if (ec) {
    std::cerr << "❌ Ошибка чтения: " << ec.message() << std::endl;
    return;
  }
  std::cout << "// ---------------------------------------------\n";
  auto start = std::chrono::high_resolution_clock::now();
  
  
  simdjson::padded_string json_view(static_cast<const char*>(buffer.data().data()), buffer.size());
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  auto error = parser.iterate(json_view).get(doc);
  if (error) {
    std::cerr << "❌ Ошибка парсинга JSON: " << simdjson::error_message(error) << "\n";
    buffer.consume(bytes_transferred);
    do_read(ws, bybit_instance);
    return;
  } 

  auto type = doc["type"].get<std::string_view>();
  if (type.error()) {
    buffer.consume(bytes_transferred);
    do_read(ws, bybit_instance);
    return;
  }
  
  DataType data_type = DataTypeImpl::from_string(type.value().data());
  if (data_type == DataType::Snapshot) {
    auto trade_snapshot = bybit_instance.ws_api->get_trade_snapshot(doc);
    if (trade_snapshot) {
      trade_snapshot->print();
    }
  }  


  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "Execution time: " << duration.count() << "µs.\n";

  buffer.consume(bytes_transferred);
  do_read(ws, bybit_instance);
}

int main() {
    try {
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

      Bybit bybit_instance;

      do_read(ws, bybit_instance);
      ioc.run();
    } catch (const std::exception& e) {
      std::cerr << "Ошибка: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
}
