#include <iostream>
#include <thread>

struct S {
  ~S() {
    if (t.joinable()) t.join();
  }
  std::thread t;
};

void routine1() { std::cout << "Routine 1 running\n"; }

int main() {}
