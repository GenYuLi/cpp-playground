#include <iostream>
#include <memory>

// 1) The abstract base “concept” with virtual destructor, operation, and clone
struct IShape {
	virtual ~IShape() = default;
	virtual double area() const = 0;
	virtual std::unique_ptr<IShape> clone() const = 0;
};

// 2) A templated concrete model that wraps any T providing .area()
template <typename T>
struct ShapeModel : IShape {
	T data;
	ShapeModel(T d) : data(std::move(d)) {}

	double area() const override { return data.area(); }

	std::unique_ptr<IShape> clone() const override { return std::make_unique<ShapeModel>(data); }
};

// 3) The erased-type “holder” with value semantics and runtime dispatch
class AnyShape {
	std::unique_ptr<IShape> ptr;

 public:
	// construct from any T that has double T::area() const
	template <typename T>
	AnyShape(T shape) : ptr(std::make_unique<ShapeModel<T>>(std::move(shape))) {}

	// copy-ctor implements value semantics via clone()
	AnyShape(const AnyShape& other) : ptr(other.ptr ? other.ptr->clone() : nullptr) {}

	AnyShape& operator=(AnyShape other) {
		ptr = std::move(other.ptr);
		return *this;
	}

	double area() const {
		return ptr->area();	 // runtime dispatch via vptr
	}
};

// 4) Some concrete types
struct Circle {
	double r;
	double area() const { return 3.14159 * r * r; }
};

struct Square {
	double side;
	double area() const { return side * side; }
};

int main() {
	AnyShape a = Circle{2.0};
	AnyShape b = Square{3.0};

	std::cout << "Circle area: " << a.area() << "\n";	 // 12.5664
	std::cout << "Square area: " << b.area() << "\n";	 // 9

	// Copying b still works as a value
	AnyShape c = b;
	std::cout << "Copied square area: " << c.area() << "\n";	// 9
}
