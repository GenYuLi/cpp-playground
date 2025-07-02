#include <iostream>
#include <memory>
#include <mutex>

// RAII

void do_work() {
  std::mutex mt;

  mt.lock();

  // do stuff

  mt.unlock();
}

class Lock {
  std::mutex& mt_;

 public:
  explicit Lock(std::mutex& mt) : mt_(mt) {
    std::cout << "Locking mutex\n";
    mt_.lock();
  }
  ~Lock() {
    std::cout << "Unlocking mutex\n";
    mt_.unlock();
  }
};

void do_raii_work() {
  std::mutex mt;
  try {
    Lock lock(mt);
    // do stuff
  }  // lock goes out of scope and unlocks the mutex
  catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << '\n';
    // lock is still released by RAII
  }
}

// STATE
// - Represent changing object state
// - Object states are classes
// - Remove if-else or switch-case statements

// this need lots of if-else and switch-case statements
class OldMember {
  enum Membership { FREE, BASIC, PREMIUM };
  Membership membership_;

  int current_downloads;

  std::string name;

 public:
  OldMember(const std::string& n)
      : name(n), membership_(FREE), current_downloads(0) {}
  void upgrade() {
    switch (membership_) {
      case FREE:
        std::cout << "Upgrading to basic membership.\n";
        membership_ = BASIC;
        break;
      case BASIC:
        std::cout << "Upgrading to premium membership.\n";
        membership_ = PREMIUM;
        break;
      case PREMIUM:
        std::cout << "Already a premium member.\n";
        break;
    }
  }

  bool can_download() {
    switch (membership_) {
      case FREE:
        return current_downloads < 1;
      case BASIC:
        return current_downloads < 5;
      case PREMIUM:
        return true;  // Premium members can download unlimited
    }
  }

  bool download() {
    if (can_download()) {
      ++current_downloads;
      std::cout << name << " downloaded a file. Current downloads: "
                << current_downloads << '\n';
      return true;
    } else {
      std::cout << name << " cannot download more files.\n";
      return false;
    }
  }
};

class Membership : public std::enable_shared_from_this<Membership> {
  std::string tier_;

 public:
  explicit Membership(std::string name) : tier_(std::move(name)) {}
  virtual ~Membership() = default;
  virtual int downloads_per_day() const = 0;
  virtual std::shared_ptr<const Membership> upgrade() const = 0;
  const std::string& tier_name() const { return tier_; }
};

class PremiumMember : public Membership {
 public:
  explicit PremiumMember() : Membership("Premium") {}
  int downloads_per_day() const override {
    return 100;  // Premium members can download unlimited files
  }
  virtual std::shared_ptr<const Membership> upgrade() const override {
    // return std::make_shared<PremiumMember>();
    return shared_from_this();  // No further upgrade
  }  // No further upgrade
};

class BasicMember : public Membership {
 public:
  explicit BasicMember() : Membership("Basic") {}
  int downloads_per_day() const override {
    return 5;  // Basic members can download 5 files per day
  }
  virtual std::shared_ptr<const Membership> upgrade() const override {
    return std::make_shared<PremiumMember>();
  }
};

class FreeTier : public Membership {
 public:
  explicit FreeTier() : Membership("Free") {}

  int downloads_per_day() const override {
    return 1;  // Free members can download 1 file per day
  }

  virtual std::shared_ptr<const Membership> upgrade() const override {
    return std::make_shared<BasicMember>();
  }
};

class Member {
  std::shared_ptr<const Membership> _role;
  std::string _name;
  int current_downloads;

 public:
  Member(const std::string& n)
      : _name(n), current_downloads(0), _role(std::make_shared<FreeTier>()) {}

  const std::string& name() { return _name; }

  bool can_download() const {
    return _role->downloads_per_day() > current_downloads;
  }

  bool download() {
    if (can_download()) {
      ++current_downloads;
      return true;
    }
    return false;
  }

  void upgrade() { _role = _role->upgrade(); }
};
