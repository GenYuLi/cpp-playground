#include <iostream>
#include <memory>
#include <string>

//--------------------------------------------------
// 抽象基底
//--------------------------------------------------
class Membership : public std::enable_shared_from_this<Membership> {
	std::string tier_;

 protected:
	explicit Membership(std::string name) : tier_(std::move(name)) {}

 public:
	virtual ~Membership() = default;
	virtual int downloads_per_day() const = 0;
	virtual std::shared_ptr<const Membership> upgrade() const = 0;
	const std::string& tier_name() const { return tier_; }
};

//--------------------------------------------------
// Premium - Singleton
//--------------------------------------------------
class PremiumMember : public Membership {
	PremiumMember() : Membership("Premium") {}	// 私有建構
 public:
	static std::shared_ptr<const Membership> instance()	 // Meyers Singleton
	{
		static std::shared_ptr<const Membership> inst(
				std::shared_ptr<PremiumMember>(new PremiumMember()));
		return inst;
	}

	int downloads_per_day() const override { return 100; }

	std::shared_ptr<const Membership> upgrade() const override {
		return instance();	// 已最高等級，回傳自己
	}
};

//--------------------------------------------------
// Basic - Singleton
//--------------------------------------------------
class BasicMember : public Membership {
	BasicMember() : Membership("Basic") {}

 public:
	static std::shared_ptr<const Membership> instance() {
		static std::shared_ptr<const Membership> inst(std::shared_ptr<BasicMember>(new BasicMember()));
		return inst;
	}

	int downloads_per_day() const override { return 5; }

	std::shared_ptr<const Membership> upgrade() const override { return PremiumMember::instance(); }
};

//--------------------------------------------------
// Free - Singleton
//--------------------------------------------------
class FreeTier : public Membership {
	FreeTier() : Membership("Free") {}

 public:
	static std::shared_ptr<const Membership> instance() {
		static std::shared_ptr<const Membership> inst(std::shared_ptr<FreeTier>(new FreeTier()));
		return inst;
	}

	int downloads_per_day() const override { return 1; }

	std::shared_ptr<const Membership> upgrade() const override { return BasicMember::instance(); }
};

//--------------------------------------------------
// 使用者
//--------------------------------------------------
class Member {
	std::shared_ptr<const Membership> role_ = FreeTier::instance();
	std::string name_;
	int downloaded_ = 0;

 public:
	explicit Member(std::string n) : name_(std::move(n)) {}

	const std::string& name() const { return name_; }

	bool can_download() const { return downloaded_ < role_->downloads_per_day(); }

	bool download() {
		if (!can_download())
			return false;
		++downloaded_;
		return true;
	}

	void upgrade() { role_ = role_->upgrade(); }

	void show() const {
		std::cout << name_ << " → " << role_->tier_name()
							<< " (limit/day = " << role_->downloads_per_day() << ")\n";
	}
};

//--------------------------------------------------
// 小測試
//--------------------------------------------------
int test_signleton() {
	Member alice("Alice");
	alice.show();			// Free
	alice.upgrade();	// → Basic
	alice.show();
	alice.upgrade();	// → Premium
	alice.show();
	return 0;
}
