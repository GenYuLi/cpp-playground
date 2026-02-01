#pragma once

// pragma once or include guards
// #ifndef LP_GLOBAL_HPP
// #define LP_GLOBAL_HPP
#include <string_view>
// C++ 11 or earlier
extern const char* const appName;

// C++ 11 static (multiple independent copies, but no ODR)
static const char* const appVersion = "1.0.0\n";

// C++ 14 constexpr (multiple independent copies, but no ODR)
constexpr const char* const authorName = "Your Name\n";

// C++ 17 inline (one definition across translation units, no ODR) use those
inline constexpr const char* const appDescription = "A language practice application.\n";
inline constexpr std::string_view appDescriptionView = "A language practice application.\n";

class CostEstimate {
 private:
	// C++ 11 static (one independent copies, ODR)
	static const double pi;
	// C++ 14 constexpr (one independent copies, no ODR)
	static constexpr double radius = 5.0;
	// C++ 17 inline (one definition across translation units, no ODR)
	inline static const double diameter = 2.0 * radius;
	// C++ 20 constexpr
	inline static constexpr double circumference = 2.0 * 3.14;

	// after g++13, g++12 does not support inline static constexpr(C++20 P0784R7)
	// inline constexpr int multiple = 2;
};

class Rect {
 private:
	/* ────────── 古早寫法：匿名 enum 整數常量 ────────── */
	enum { pi = 5 };	// 型別為 int，無 storage

	/* ────────── 現代替代：inline static constexpr ────────── */
	inline static constexpr int kPiModern = 5;
};

const double CostEstimate::pi = 3.14159;

// #endif // LP_GLOBAL_HPP
