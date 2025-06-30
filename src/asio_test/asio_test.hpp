#include <fmt/color.h>
#include <fmt/core.h>

// close for clang-format
// clang-format off
#include <utility>
// clang-format on

#include <boost/asio.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

using boost::asio::ip::tcp;
using json = nlohmann::json;

int asio_test();
