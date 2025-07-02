#include <iostream>

using namespace std;
int main() {

  // Allocate memory for an int but do not initialize it
  void* buffer = operator new(sizeof(int));
  // Use placement new to construct an int in the allocated memory
  int* p = new (buffer) int(100);
  for (int i = 0 ; i < 10; i++) {
    p[i] = i + 200;
  }
  cout << "After placement new: " << *p << endl;
  // Clean up: call destructor and free memory
  operator delete(buffer);
  return 0;
}
