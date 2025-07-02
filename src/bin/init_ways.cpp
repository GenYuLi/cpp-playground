#include <array>
#include <iostream>
#include <vector>
using namespace std;

struct base1 {
  int value;
  base1(int v) : value(v) {
    std::cout << "base1 constructor called with value: " << value << std::endl;
  }
};

struct base2 {
  string value;
  base2(string v) : value(v) {
    std::cout << "base2 constructor called with value: " << value << std::endl;
  }
};

struct only_int {
  int value;
};

struct self_construct : base2, base1 {
  int value;
  only_int oi;  // Member of type 'only_int'
  self_construct(int v, int oi)
      : value(v),
        oi(oi),
        base1(2),
        base2("abc") {}  // Constructor to initialize 'value'
  self_construct(int v, int oi, base1&& b1, base2&& b2)
      : value(v),
        oi(oi),
        base1(b1),
        base2(b2) {}  // Constructor to initialize 'value'
};

int test_list(vector<int> test_list) {
  for (const auto& item : test_list) {
    std::cout << item << " ";
  }
  std::cout << std::endl;
  return 0;
}

struct S {
  static const int c;
};

const int d =
    10 * S::c;       // not a constant expression: S::c has no preceding
                     // initializer, this initialization happens after const
const int S::c = 5;  // constant initialization, guaranteed to happen first

struct fuck {
  int their;
  int life;
};

fuck you{1, 2};

int main() {
  only_int oi{42};
  // oi.value = 100;  // This line would be an error if 'value' were const
  std::cout << "The value is: " << oi.value << std::endl;

  self_construct sc{100, 20, {30}, {"fuck"}};
  vector test_fuck{"2"};
  test_list({1, 2, 3, 4, 5});
  std::cout << "d = " << d << '\n';
  std::array<int, S::c> a1;  // OK: S::c is a constant expression
  //  std::array<int, d> a2;    // error: d is not a constant expression
  return 0;
}
