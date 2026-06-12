#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct Tracer {
	std::string name;
	Tracer(const std::string& n) : name(n) { std::cout << name << " created.\n"; }
	~Tracer() { std::cout << name << " destroyed.\n"; }
};

// ----------------------------------------------------
// except
class PotentiallyThrowingMover {
 public:
	PotentiallyThrowingMover() = default;

	PotentiallyThrowingMover(const PotentiallyThrowingMover& other) {
		std::cout << "PotentiallyThrowingMover: COPY constructor called.\n";
	}

	PotentiallyThrowingMover(PotentiallyThrowingMover&& other) {
		std::cout << "PotentiallyThrowingMover: MOVE constructor called.\n";
	}

 private:
	Tracer t_{"Tracer in PTMover"};
};

// ----------------------------------------------------
// noexcept
class NoexceptMover {
 public:
	NoexceptMover() = default;

	NoexceptMover(const NoexceptMover& other) {
		std::cout << "NoexceptMover: COPY constructor called.\n";
	}

	NoexceptMover(NoexceptMover&& other) noexcept {
		std::cout << "NoexceptMover: MOVE constructor called.\n";
	}

 private:
	Tracer t_{"Tracer in NoexceptMover"};
};

int main() {
	std::cout << "--- Testing PotentiallyThrowingMover (no noexcept) ---\n";
	std::vector<PotentiallyThrowingMover> vec1;
	vec1.reserve(2);
	std::cout << "Pushing back first element...\n";
	vec1.push_back(PotentiallyThrowingMover());
	std::cout << "Pushing back second element...\n";
	vec1.push_back(PotentiallyThrowingMover());

	std::cout << "\n>>> Triggering reallocation by pushing a third element...\n";
	vec1.push_back(PotentiallyThrowingMover());
	std::cout << "--- Test for PotentiallyThrowingMover finished ---\n\n";

	std::cout << "\n\n\n\n\n";

	std::cout << "\n--- Testing NoexceptMover (with noexcept) ---\n";
	std::vector<NoexceptMover> vec2;
	vec2.reserve(2);
	std::cout << "Pushing back first element...\n";
	vec2.push_back(NoexceptMover());
	std::cout << "Pushing back second element...\n";
	vec2.push_back(NoexceptMover());

	std::cout << "\n>>> Triggering reallocation by pushing a third element...\n";
	vec2.push_back(NoexceptMover());
	std::cout << "--- Test for NoexceptMover finished ---\n";

	return 0;
}
