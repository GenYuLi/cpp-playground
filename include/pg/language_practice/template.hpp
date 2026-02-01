
#pragma once
#include <iostream>
template <class T>
void show(T x) {
	std::cout << x << '\n';
}

// keep template instance declaration in header file
// to prevent .o file bloat
// extern template void show<int>(int x);
// you should add declaration in cpp file which you would link to
// this is to keep every translation unit from instantiating its own

// However this is not necessary, because the compiler will
// delete the redundant instantiation in the final binary
// But if there is some template specialization that is little different somehow,
// then it would become a problem
