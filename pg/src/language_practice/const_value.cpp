// #include <pg/language_practice/const_value.hpp>
//
// const char* const appName = "Language Practice";
//
enum class status {
	BURNOUT,
	BUSYING,
	DEBUGING,
	PENDING,
	READY,
	ON_VACATION,
};

bool wait_for_coworker(status& coworker_status) {
	while (coworker_status == status::BUSYING)
		;
	return true;
}

struct task {
	int tid;
	bool success;
};

void set_success(task t) {
	t.success = true;
}


