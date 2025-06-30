#include <iostream>
#include <memory>

using namespace std;

class A {
 public:
  ~A() { cout << "A::~A()\n\n" << endl; }
  virtual void f() { cout << "A::f()" << endl; }
  void g() { cout << "A::g()" << endl; }
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
  b->g();
  A *a = b;
  // delete without shared_ptr
  cout << "delete without shared_ptr" << endl;
  delete a;
  // delete with shared_ptr
  cout << "delete with shared_ptr" << endl;
  shared_ptr<A> s_a = std::make_shared<B>();
  s_a->f();
  s_a->g();
  return 0;
}
