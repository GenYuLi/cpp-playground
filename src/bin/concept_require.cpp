template <typename T>
concept A = T::value || true;

template <typename U>
concept B = A<U*>;	// OK: normalized to the disjunction of
										// - T::value (with mapping T -> U*) and
										// - true (with an empty mapping).
										// No invalid type in mapping even though
										// T::value is ill-formed for all pointer types

template <typename V>
concept C = B<V&>;	// Normalizes to the disjunction of
										// - T::value (with mapping T-> V&*) and
										// - true (with an empty mapping).
										// Invalid type V&* formed in mapping => ill-formed NDR

template <typename S>
concept D = A<S&> || A<S>;

template <typename T>
	requires A<T>
void testhere(T&& t) {}

int main() {
	int k = 2;
	int& t = k;
	testhere(t);
}
