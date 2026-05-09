#include <iostream>
#include <memory>

#include "pg/asio_test/asio_test.hpp"
#include "pg/language_practice/some_ds.hpp"
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

template <class D>
struct Drawable {
	void draw() {	 // 靜態多型
		static_cast<D*>(this)->draw_impl();
	}
	~Drawable() { cout << "Drawable::~Drawable()" << endl; }
};

struct Circle : Drawable<Circle> {
	void draw_impl() { cout << "Circle::draw_impl()" << endl; }
	~Circle() { cout << "Circle::~Circle()" << endl; }
};

template <class D>
struct CrtpDeleter {
	void operator()(Drawable<D>* p) const noexcept { delete static_cast<D*>(p); }
};

template <typename D>
using CrtpPtr = std::unique_ptr<Drawable<D>, CrtpDeleter<D>>;

int test_class_derive() {
	B* b = new B();
	b->f();
	b->g();
	A* a = b;
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

template <class T>
void swap_nothrow(T& a, T& b) noexcept(noexcept(std::move(a)) &&	// 移動建構子
																			 noexcept(a.swap(b))				// ADL swap
);

int main() {
	using cmp = decltype([](int a, int b) { return a > b; });
	FixedBinaryHeap<int, int, 200, cmp> heap;
	heap.push(1, 10);
	heap.push(2, 20);
	shared_ptr<Drawable<Circle>> a_ptr = make_shared<Circle>();
	a_ptr->draw();
	CrtpPtr<Circle> circle_ptr{new Circle()};
	cout << "Heap size: " << heap.size() << endl;
	cout << "heap peek: " << heap.top() << endl;

	int a = 1, b = 2;
	// compiler error
	// swap_nothrow(a, b);

	return 0;
}
