#pragma once
#include <iostream>
#include <type_traits>
#include <vector>

template <typename T>
void print_all(T&& t) {
	for (auto&& elem : t) {
		std::cout << elem << ' ';
	}
	std::cout << "\n\n";
}

template <typename T, typename = void>
struct has_iter_methods : std::false_type {};	 // 預設為 false

template <typename T>
struct has_iter_methods<T, std::void_t<decltype(std::declval<T>().begin()),
																			 std::void_t<decltype(std::declval<T>().end())>>>
		: std::true_type {};

template <typename T>
inline constexpr bool has_iter_methods_v = has_iter_methods<std::decay_t<T>>::value;

template <typename T>
auto make_vector(T& t) {
	if constexpr (std::is_array_v<T>) {
		using ELEM_T = std::remove_extent_t<T>;
		if constexpr (std::is_same_v<ELEM_T, const char*>) {
			// return std::vector<std::string>(t, t + std::size(t));
			return std::vector<std::string>(std::begin(t), std::end(t));
		} else {
			return std::vector<ELEM_T>(std::begin(t), std::end(t));
		}
	} else if constexpr (has_iter_methods<T>::value) {
		return std::vector<typename T::value_type>(std::begin(t), std::end(t));
	} else {
		return std::vector<T>{t};	 // Not an iterable, return single-element vector
	}
}
