#include <iostream>
#include <vector>

using namespace std;

template <typename T>
void gogo(std::vector<T>&& par);

template <typename T>
class A {
 public:
  void gogo(int&& c);
};

template <typename T>
class B {
 public:
  void gogo(T&& c);
};

template <typename A>
class C {
 public:
  template <typename B>
  void gogo(B&& par);
};

// only gogo in B is universally reference

void k(int&) { cout << " k(int&) called\n"; }
void k(int&&) { cout << " k(int&&) called\n"; }

template <typename T>
void perfect_forwarding(T&& t) {
  // T is deduced as int&
  k(std::forward<T>(t));  // perfect forwarding
}

/*
 * int& &
 * int&& &
 * int& &&
 * int&& &&
 * only int && && is rvalue reference
 */

int main() {}
