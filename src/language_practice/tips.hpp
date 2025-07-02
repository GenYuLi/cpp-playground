#include <memory>
#include <numeric>  // for std::accumulate
#include <string>
#include <utility>
#include <vector>

// mutable
// - used to allow modification of a member variable even in a const member
// function.
// // - Useful for caching, lazy initialization, thread-safe operations, or
// shared_from_this idiom.

// shared from this
template <typename T>
class MutEx {
  mutable std::weak_ptr<T> this_weak_;  // 可以在 const 成員函式中修改

 public:
  std::shared_ptr<T> shared_from_this() const {
    if (auto sp = this_weak_.lock())  // lock() 可以改變 this_weak_
                                      // 的內部狀態（例如延伸生命週期）
      return sp;
    auto sp = std::make_shared<T>(*this);
    this_weak_ = sp;  // 即使此方法被宣告為 const，也能修改 this_weak_
    return sp;
  }
};

// cache

class DataProcessor {
 private:
  std::vector<int> data_;
  mutable bool cached_{false};
  mutable int sum_cache_{0};

 public:
  DataProcessor(std::vector<int> d) : data_(std::move(d)) {}

  // const 函式，對外行為不變，但內部可更新 cache
  int sum() const {
    if (!cached_) {
      // 第一次呼叫時計算並快取
      sum_cache_ = std::accumulate(data_.begin(), data_.end(), 0);
      cached_ = true;
    }
    return sum_cache_;
  }
};

// lazy initialization

class ConfigLoader {
 private:
  std::string file_path_;
  mutable std::shared_ptr<std::string> contents_;

 public:
  explicit ConfigLoader(std::string path) : file_path_(std::move(path)) {}

  // const 函式讀取設定檔，第一次才載入檔案
  const std::string& getContents() const {
    if (!contents_) {
      // 第一次呼叫才讀檔
      auto buf = std::make_shared<std::string>();
      // 假裝從檔案讀取到 buf->...
      *buf = "loaded config data from " + file_path_;
      contents_ = buf;
    }
    return *contents_;
  }
};

// noexcept
// - 用於標記函式不拋出異常
// // - 提高性能，因為編譯器可以優化異常處理
// // - 使程式碼更清晰，表明函式的行為預期

template <class T>
void swap_nothrow(T& a, T& b) noexcept(
    noexcept(std::move(a)) &&                                    // 移動建構子
    noexcept(std::swap(std::declval<T&>(), std::declval<T&>()))  // ADL swap
) {
  using std::swap;
  swap(a, b);
}

void test_noexcept() {
  int a = 1, b = 2;
  swap_nothrow(a, b);  // 使用 noexcept 的 swap 函式
}
