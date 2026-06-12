#include <cstdarg>
#include <iostream>
#include <ostream>
#include <vector>

using namespace std;

template <class... Ts>
void foo() {
	// print the type names of all template parameters without endl
	(cout << ... << typeid(Ts).name()) << endl;
	((cout << typeid(Ts).name() << " "), ...) << endl;
}

template <class... Arg>
void foo(Arg... args) {
	((cout << "args: " << args << " "), ...) << endl;
}

template <typename T, typename... Ts>
std::vector<T> make_vector_left(T first, Ts... rest) {
	std::vector<T> v;
	v.reserve(1 + sizeof...(rest));
	// 逗號運算子左折疊：先依序執行 v.emplace_back(rest)
	(v.emplace_back(rest), ..., v.emplace_back(first));
	return v;
}

template <typename T, typename... Ts>
std::vector<T> make_vector_right(T first, Ts... rest) {
	std::vector<T> v;
	v.reserve(1 + sizeof...(rest));
	(v.emplace_back(first), ..., v.emplace_back(rest));
	return v;
}

template <typename... Ts, int... N>
void g(Ts (&... arr)[N]) {}

int n[1];

void print_ints(int args, ...) {
	va_list ap;
	va_start(ap, args);
	cout << "print_ints: ";
	for (size_t i = 0; i < args; ++i) {
		int v = va_arg(ap, int);
		cout << v << " ";
	}
	va_end(ap);
	cout << "\n\n";
}

int printx(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	int printed = 0;
	for (const char* p = fmt; *p != '\0'; ++p) {
		if (*p == '%' && *(p + 1)) {
			++p;
			switch (*p) {
				case 'd': {
					int v = va_arg(ap, int);
					printed += printf("%d", v);
					break;
				}
				case 's': {
					const char* s = va_arg(ap, const char*);
					printed += printf("%s", s);
					break;
				}
			}
		} else {
			putchar(*p);
			++printed;
			cout.flush();
		}
	}

	va_end(ap);
	return printed;
}
// 1. The general, empty template
template <typename T>
struct C {};

// 2. A specialization for 'int'
template <>
struct C<int> {
	void say_hi(int) { puts("int hi"); }
};

// 3. A specialization for 'double'
template <>
struct C<double> {
	void say_hi(double) { puts("double hi"); }
};

// 4. A specialization for 'char'
template <>
struct C<char> {
	void say_hi(char) { puts("char hi"); }
};

// invalid
// struct C<string> {
// void say_hi(string) { puts("string hi"); }
// }
// // because <string> cannot use directly in template specialization
// template<> is the syntax for explicit specialization.

int main() {
	foo<int, double, string>();
	foo(1, 2.5, "hello");
	vector<int> left_vector = make_vector_left(1, 2, 3, 4, 5);
	cout << "make_vector_left: ";
	for (const auto& v : left_vector) {
		cout << v << " ";
	}
	cout << endl;
	vector<int> right_vector = make_vector_right(1, 2, 3, 4, 5);
	cout << "make_vector_right: ";
	for (const auto& v : right_vector) {
		cout << v << " ";
	}
	cout << endl;
	g<const char, int>("a", n);	 // Ts (&...arr)[N] expands to
															 // const char (&)[2], int(&)[1]
	print_ints(3, 1, 2, 3);
	printx("Hello %s, your score is %d\n\0", "Alice", 95);
	return 0;
}
