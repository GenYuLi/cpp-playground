
#include <iostream>
#include <memory>
#include <tuple>
#include <utility>

using namespace std;

class Obj {
 public:
  Obj() { cout << "Obj constructed\n"; }
  Obj(const Obj&) { cout << "Obj copy-constructed\n"; }
  Obj(Obj&& obj) noexcept {
    cout << "obj address in move constructor: " << &obj << endl;
    cout << "Obj move-constructed\n";
  }
  ~Obj() {
    cout << "obj address: " << this << endl;
    cout << "Obj destructed\n";
  }

  Obj& operator=(const Obj&) {
    cout << "Obj copy-assigned\n";
    return *this;
  }

  Obj& operator=(Obj&&) noexcept {
    cout << "Obj move-assigned\n";
    return *this;
  }

  friend ostream& operator<<(ostream& os, const Obj&) {
    return os << "Obj instance";
  }
};

void g(Obj& k) {
  cout << "g(int& k) called with k = " << k << endl;
  cout << "k address: " << &k << endl << endl;
}
void g(Obj&& s) {
  cout << "g(int&& s) called with s = " << s << endl;
  cout << "s address: " << &s << endl << endl;
}

void f(Obj&& r) {
  // fuck still produce a new item
  Obj fuck = std::forward<Obj>(r);
  // Obj& you = r;
  g(std::forward<Obj>(r));  // Forwarding r to g
  g(r);
  cout << " yo\n";
}

struct FishData {
  static inline string l_val_name = "Left fish";
  static inline string r_val_name = "Right fish";

  string& Name() & { return l_val_name; }
  string Name() && { return r_val_name; }
};

struct Fish {
  Fish(const FishData&) { cout << "fish copy conversopm ctor\n"; }
  Fish(FishData&&) { cout << "fish move conversion ctor\n"; }
  Fish(const string& name) { cout << "copy conversion: " << name << endl; }
  Fish(string&& name) { cout << "move conversion: " << name << endl; }
};

template <typename T>
Fish MakeFish(T&& fd) {
  return Fish{std::forward<T>(fd)};
}

enum class FishType : uint { shark, salmon };

template <typename... Args>
shared_ptr<Fish> MakeFishPtr(FishType type, Args&&... args) {
  if (type == FishType::salmon) {
    return make_shared<Fish>(std::forward<Args>(args)...);
  }
  return nullptr;
}

template <typename T>
Fish MakeFish2(T&& fd) {
  // return
  // Fish{std::forward<decltype(std::forward<T>(fd).Name())>(std::forward<T>(fd).Name())};
  // equals to
  return Fish{
      std::forward<decltype(declval<T>().Name())>(std::forward<T>(fd).Name())};
}

int main() {
  Obj obj;
  cout << "obj address:" << &obj << endl;
  f(std::move(obj));  // Pass k as an rvalue reference
  FishData fd;
  cout << "\n\n\n";
  MakeFish2(fd);
  MakeFish2(FishData{});
  cout << "\n\n\n";
  return 0;
}
