#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <unordered_map>
#include <pg/language_practice/utils.hpp>

class CClass {
 public:
	constexpr explicit CClass(int n) : n_(n) {}
	constexpr CClass operator*(const CClass& rhs) const { return CClass(n_ * rhs.n_); }
	int n_;
};
constexpr int sq(int n) {
	return n * n;
}
constexpr CClass cube(const CClass& c) {
	return c * c * c;
}

constexpr int fib(int n) {
	if (n <= 0) {
		return 1;
	}
	return fib(n - 1) + fib(n - 2);
}

enum FIB_ENUM {
	a = fib(20),
	b = fib(23),
	c = fib(24),
};

template <typename T>
void int_type(std::conditional_t<std::is_integral_v<T>, int, void>* = nullptr) {}

template <typename T, typename = void>
struct has_size_member : std::false_type {};	// 預設為 false

template <typename T>
struct has_size_member<T, std::void_t<decltype(std::declval<T>().size())>> : std::true_type {};

template <typename T>
inline constexpr bool has_size_member_v = has_size_member<std::decay_t<T>>::value;

template <typename T>
auto get_len(T&& t) {
	// 使用 std::decay_t 移除引用和 cv 限定符，取得純粹的型別
	using PureT = std::decay_t<T>;
	if constexpr (std::is_array_v<PureT>) {
		return sizeof(t) / sizeof(t[0]);
	} else if constexpr (std::is_same_v<PureT, const char*>) {
		return strlen(t);
	} else if constexpr (has_size_member_v<PureT>) {
		return t.size();
	} else {
		return 1;	 // Not an array, return length as 1
	}
}

template <typename T>
constexpr bool is_int_or_double_v = std::is_same_v<T, int> || std::is_same_v<T, double>;

// before C++ 17, we cannot use `if constexpr`
template <typename T>
void func(T t, std::enable_if_t<is_int_or_double_v<T>>* = nullptr) {
	std::cout << "func called with integral type\n";
}

// after C++ 17

template <typename T>
void func_new(T t) {
	if constexpr (is_int_or_double_v<T>) {
		std::cout << "func_new called with integral type\n";
	} else {
		std::cout << "func_new called with non-integral type\n";
	}
}

template <typename... Types>
struct type_list {};

template <>
struct type_list<> {
	template <typename>
	constexpr static bool has() {
		return false;
	}
};

template <typename T, typename... Types>
struct type_list<T, Types...> {
	template <typename U>
	constexpr static bool has() {
		return std::is_same_v<T, U> || type_list<Types...>::template has<U>();
	}
};

int main() {
	if constexpr (std::is_same_v<int, int>) {
		std::cout << "int is same as int\n";
	} else {
		std::cout << "int is not same as int\n";
	}
	constexpr CClass c1(sq(2));
	constexpr CClass c2_6 = cube(c1);
	constexpr CClass c3_6 = cube(CClass(sq(3)));
	CClass c3_3 = cube(CClass(3));
	static_assert(c2_6.n_ == 64, "!!!");
	static_assert(c3_6.n_ == 729, "!!!");
	static_assert(cube(CClass(2)).n_ == 8, "@");
	if constexpr (c3_6.n_ == 729) {
		std::cout << "c3_6 is 729\n";
	} else {
		std::cout << "c3_6 is not 729\n";
	}

	std::cout << "arr{1, 2, 3} len:" << get_len(std::array<int, 3>{1, 2, 3}) << std::endl;

	std::cout << "str abcd len: " << get_len("abcd") << std::endl;
	std::unordered_map<int, int> m{{1, 2}, {3, 4}};
	auto m_vec = make_vector(m);
	// print_all(m_vec);
	for (auto& pair : m_vec) {
		std::cout << "key: " << pair.first << ", value: " << pair.second << '\n';
	}

	// it cannot use static assert on non-constexpr expression
	// clang: Static assertion expression is not an integral constant expression
	// [static_assert_expression_is_not_constant]
	//
	// static_assert(c3_3.n_ == 27, "shit man");
}
