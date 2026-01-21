#include "asio_test.hpp"

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
