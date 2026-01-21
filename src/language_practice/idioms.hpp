// RAII
// - Resource Acquisition Is Initialization

class FooRaii {
 public:
	FooRaii() {
		// Acquire resource
	}
	~FooRaii() {
		// Release resource
	}
};

// PIMPL
// - Pointer to Implementation Idiom
// - Decouple iomplementation from interface
// - Imporve compilation time (maybe implmented in a shared library for
// different file)

class FooImpl;

class Foo {
	FooImpl* fp;
};

class FooImpl {};

// Smart Pointers
// - overload operator-> and operator* for pointer-like behavior
// - many uses in modern C++
// - Manage resource
// - Provide proxies
// - etc..
// // - std::unique_ptr, std::shared_ptr, std::weak_ptr
template <typename T>
class Ptr {
	T* p;

 public:
	Ptr(T* ptr) : p(ptr) {}
	T& operator*() { return *p; }
	T* operator->() { return p; }
	// other member functions...
};

// CRTP
// - Curiously Recurring Template Pattern
// - static polymorphism
// - Base class template takes derived class as template parameter
// - Removes virtual function overhead
// - Used for mixins, static interfaces, etc.

template <typename T>
class B {};

class D : public B<D> {};
