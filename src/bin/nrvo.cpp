#include <iostream>
#include <utility>

using namespace std;
int bar(int& x) {
	// This function takes an int by value, which cannot benefit from NRVO.
	cout << "bar called with value x = " << x << endl;
	return x;
}

int bar(int&& x) {
	// This function takes an rvalue reference to an int,
	cout << "bar called with rvalue reference x = " << x << endl;
	return x;
}

int foo(int&& x) {
	// This function takes an rvalue reference to an int
	// and returns it by value, which can be optimized by NRVO.
	cout << "foo called with rvalue reference x = " << x << endl;
	bar(x);
	return x;
}

int main() {
	int k = 2;
	bar(foo(std::move(k)));
	bar(foo(42));
	int gan = 3;
	cout << "gan address: " << &gan << endl;
	int&& rref = std::move(gan);
	cout << "rref address: " << &rref << endl;
}
