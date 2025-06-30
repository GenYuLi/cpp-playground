#include <iostream>
#include <memory>

using namespace std;

class A {
 public:
  ~A() { cout << "A::~A()\n\n" << endl; }
  virtual void f() { cout << "A::f()" << endl; }
};

class B : public A {
 public:
  ~B() { cout << "B::~B()\n\n" << endl; }
  void f() override { cout << "B::f()" << endl; }
  void g() { cout << "B::g()" << endl; }
};

// vtable only for overriden functions
//  B object
//  dataframe: {
//  * vptr: B::vtable
//    B::vtable: {
//      B::f -> override A::f
//    }
//  }
//
//
//
//

int main() {
  B *b = new B();
  b->f();
  A *a = b;
  delete a;
  shared_ptr<A> s_a = std::make_shared<B>();
  return 0;
}
