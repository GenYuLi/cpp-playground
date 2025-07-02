#include <iostream>
#include <string>
#include <string_view>

using namespace std;

constexpr char* const RAII_EXAMPLE_NO_CONST = "RAII Example";
constexpr const char* const RAII_EXAMPLE = "RAII Example";
constexpr std::string_view RAII_EXAMPLE_VIEW = "RAII Example View";

int mut_int = 10;
constexpr int* const const_expr_int = &mut_int;

int main() {
  // RAII_EXAMPLE_NO_CONST[2] = 'X'; -> it segfaults because RAII_EXAMPLE_NO_CONST is a pointer to a string literal, which is immutable.
  // Using constexpr char* const
  cout << RAII_EXAMPLE_NO_CONST << endl;
  // Using constexpr const char* const
  cout << RAII_EXAMPLE << endl;
  // Using std::string_view
  cout << RAII_EXAMPLE_VIEW << endl;
  // Demonstrating that constexpr char* const is not a string literal
  // This will cause a compilation error if uncommented
  // constexpr char* const NOT_A_STRING_LITERAL = "This is not a string literal";

  *const_expr_int = 20;
  return 0;
}
