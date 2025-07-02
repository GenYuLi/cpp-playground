#include <iostream>

using namespace std;

template <class... Ts>
void foo() {
  // print the type names of all template parameters without endl
  (cout << ... << typeid(Ts).name()) << endl;
  ((cout << typeid(Ts).name() << " "), ...) << endl;
}

template <class... Arg>
void foo(Arg ... args) {

  ((cout << "args: " << args << " "), ...) << endl;
}

int main() {
  foo<int, double, string>();
  foo(1, 2.5, "hello");
  return 0;
}
