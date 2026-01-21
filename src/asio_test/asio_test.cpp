#include "asio_test.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace asio = boost::asio;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;

int asio_test() {
	try {
		std::string host = "jsonplaceholder.typicode.com";
		std::string path = "/todos/1";

		boost::asio::io_context io_context;

		tcp::resolver resolver(io_context);
		tcp::socket socket(io_context);

		auto endpoints = resolver.resolve(host, "http");
		boost::asio::connect(socket, endpoints);

		std::string request = fmt::format(
				"GET {} HTTP/1.1\r\n"
				"Host: {}\r\n"
				"Accept: application/json\r\n"
				"Connection: close\r\n\r\n",
				path, host);
		boost::asio::write(socket, boost::asio::buffer(request));

		boost::asio::streambuf response;
		boost::system::error_code ec;

		boost::asio::read(socket, response, ec);

		if (ec && ec != boost::asio::error::eof) {
			throw boost::system::system_error(ec);
		}
		std::string response_str((std::istreambuf_iterator<char>(&response)),
														 std::istreambuf_iterator<char>());
		fmt::print(fg(fmt::color::red), "--- DEBUG START ---\n");
		fmt::print(
				"Full Response From Server ({} "
				"bytes):\n<<<\n{}\n>>>\n",
				response_str.length(), response_str);

		size_t separator_pos = response_str.find("\r\n\r\n");
		if (separator_pos != std::string::npos) {
			std::string json_body = response_str.substr(separator_pos + 4);

			fmt::print("Extracted JSON Body ({} bytes):\n<<<\n{}\n>>>\n", json_body.length(), json_body);
			fmt::print(fg(fmt::color::red), "--- DEBUG END ---\n\n");

			json data = json::parse(json_body);

			fmt::print(fg(fmt::color::cyan), "Todo Item #{}\n", data["id"].get<int>());
			fmt::print("User ID:   {}\n", data["userId"].get<int>());
			fmt::print("Title:     {}\n", data["title"].get<std::string>());
			fmt::print("Completed: {}\n", data["completed"].get<bool>() ? "Yes" : "No");

		} else {
			fmt::print("Separator '\\r\\n\\r\\n' not found!\n");
			fmt::print(fg(fmt::color::red), "--- DEBUG END ---\n\n");
			fmt::print(stderr, fg(fmt::color::red),
								 "Error: Could not find HTTP header/body separator.\n");
			return 1;
		}
	} catch (std::exception& e) {
		fmt::print(stderr, fg(fmt::color::red), "Error: {}\n", e.what());
		return 1;
	}
	return 0;
}

// 處理單一連線的 coroutine
awaitable<void> handle_connection(tcp::socket socket) {
	try {
		boost::asio::streambuf buf;

		// async_read_until 可以讀到特定分隔符，或用 async_read_some 讀部分資料
		size_t n = co_await socket.async_read_some(buf.prepare(4096), use_awaitable);
		buf.commit(n);

		std::string data((std::istreambuf_iterator<char>(&buf)), std::istreambuf_iterator<char>());

		fmt::print(fg(fmt::color::green), "--- Connection Received ---\n");
		fmt::print("From: {}:{}\n", socket.remote_endpoint().address().to_string(),
							 socket.remote_endpoint().port());
		fmt::print("Data ({} bytes):\n<<<\n{}\n>>>\n", data.length(), data);
		fmt::print(fg(fmt::color::green), "--- End Connection ---\n\n");

	} catch (std::exception& e) {
		fmt::print(fg(fmt::color::red), "Connection error: {}\n", e.what());
	}
}

// 主 listener coroutine - 不斷 accept 並 spawn 新任務
awaitable<void> listener(tcp::acceptor& acceptor) {
	while (true) {
		tcp::socket socket = co_await acceptor.async_accept(use_awaitable);

		fmt::print(fg(fmt::color::cyan), "[Listener] New connection from {}:{}\n",
							 socket.remote_endpoint().address().to_string(), socket.remote_endpoint().port());

		// 生成新任務處理這個連線，不阻塞 accept
		co_spawn(acceptor.get_executor(), handle_connection(std::move(socket)), detached);
	}
}

int socket_listener() {
	try {
		asio::io_context io_context;
		tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));

		fmt::print(fg(fmt::color::magenta) | fmt::emphasis::bold,
							 "╔════════════════════════════════════╗\n"
							 "║  Async Socket Listener (port 12345) ║\n"
							 "╚════════════════════════════════════╝\n");
		fmt::print("Waiting for connections...\n\n");

		// 啟動 listener coroutine
		co_spawn(io_context, listener(acceptor), detached);

		// 開始事件循環（這會阻塞直到 io_context 停止）
		io_context.run();
	} catch (std::exception& e) {
		fmt::print(stderr, fg(fmt::color::red), "Error: {}\n", e.what());
		return 1;
	}
	return 0;
}
