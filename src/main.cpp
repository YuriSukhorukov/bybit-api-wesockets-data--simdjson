#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <chrono>
#include <simdjson.h>
#include "bybit/ws/data.h"
#include "bybit.h"

namespace net         = boost::asio;
namespace ssl         = boost::asio::ssl;
namespace beast       = boost::beast;
namespace websocket   = beast::websocket;

using tcp = net::ip::tcp;
using namespace simdjson;

void do_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws);
void on_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws,
             beast::flat_buffer &buffer,
             beast::error_code ec,
             std::size_t bytes_transferred);



void do_read(websocket::stream<beast::ssl_stream<tcp::socket>> &ws) {
  auto *buffer = new beast::flat_buffer();
  ws.async_read(
      *buffer,
      [&, buffer](beast::error_code ec, std::size_t bytes_transferred) {
        on_read(ws, *buffer, ec, bytes_transferred);
        delete buffer;
      });
}
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
  std::cout << "// ---------------------------------------------\n";
  auto start = std::chrono::high_resolution_clock::now();
  TradeSnapshotImpl::parse(buffer);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "Execution time: " << duration.count() << "µs.\n";

  buffer.consume(bytes_transferred); // Очистка буфера
  do_read(ws); // Читаем следующее сообщение
}

int main() {
  Bybit bybit_instance;
  bybit_instance.ws_api->hello();
    // try {
    //   net::io_context ioc;
    //   ssl::context ctx(ssl::context::tlsv12_client);
      
    //   ctx.set_default_verify_paths();
    //   ctx.set_verify_mode(ssl::verify_peer);

    //   auto const host = "stream.bybit.com";
    //   auto const port = "443";
    //   auto const target = "/v5/public/linear";

    //   tcp::resolver resolver(ioc);
    //   websocket::stream<beast::ssl_stream<tcp::socket>> ws(ioc, ctx);

    //   auto const results = resolver.resolve(host, port);
    //   net::connect(ws.next_layer().next_layer(), results);

    //   if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host)) {
    //     throw boost::system::system_error(
    //         ::ERR_get_error(), boost::asio::error::get_ssl_category());
    //   }

    //   ws.next_layer().handshake(ssl::stream_base::client);

    //   ws.handshake(host, target);

    //   std::cout << "✅ Успешное подключение к Bybit WebSocket API" << std::endl;

    //   std::string msg = R"({"op":"subscribe","args":["publicTrade.BTCUSDT", "publicTrade.ETHUSDT", "publicTrade.SOLUSDT", "publicTrade.SUIUSDT"]})";
    //   ws.write(net::buffer(msg));

    //   do_read(ws);
    //   ioc.run();
    // } catch (const std::exception& e) {
    //   std::cerr << "Ошибка: " << e.what() << std::endl;
    //   return EXIT_FAILURE;
    // }
}
