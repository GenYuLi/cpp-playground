#include <utility>
#include <new>
#include <type_traits>

using namespace std;

// 1. 定義概念（Concept）介面：所有型別都必須實作 `call()`。
struct CallableConcept {
    virtual ~CallableConcept() = default;
    virtual void call() = 0;
    virtual CallableConcept* clone(void* buffer) const = 0;
    virtual CallableConcept* heapClone() const = 0;
};

// 2. 具體模型（Model）模板：將任意符合 callable 介面的型別包裝起來。
template<typename F>
struct CallableModel : CallableConcept {
    F func;
    explicit CallableModel(F f) : func(std::move(f)) {}

    // 執行實際呼叫
    void call() override {
        func();
    }

    // 將自己複製到 buffer（SBO 用）
    CallableConcept* clone(void* buffer) const override {
        return new (buffer) CallableModel<F>(func);
    }

    // 動態配置到 heap
    CallableConcept* heapClone() const override {
        return new CallableModel<F>(func);
    }
};

// 3. 包裝者（Wrapper）
class AnyCallable {
    // 小型緩衝區（SBO），避免經常 heap allocation
    static constexpr size_t BufferSize = 3 * sizeof(void*);
    alignas(alignof(void*)) unsigned char buffer[BufferSize];

    // 指向實際的概念物件
    CallableConcept* ptr;

    // 判斷物件是否放在 buffer 內
    bool in_sbo;

public:
    // 建構：接受任何 callable 物件
    template<typename F>
    AnyCallable(F f) {
        using Model = CallableModel<std::decay_t<F>>;
        if (sizeof(Model) <= BufferSize) {
            ptr = Model(std::move(f)).clone(buffer);
            in_sbo = true;
        } else {
            ptr = new Model(std::move(f));
            in_sbo = false;
        }
    }

    // 複製建構
    AnyCallable(const AnyCallable& other) {
        if (other.in_sbo) {
            ptr = other.ptr->clone(buffer);
            in_sbo = true;
        } else {
            ptr = other.ptr->heapClone();
            in_sbo = false;
        }
    }

    // 移動建構
    AnyCallable(AnyCallable&& other) noexcept {
        if (other.in_sbo) {
            ptr = other.ptr->clone(buffer);
            in_sbo = true;
        } else {
            ptr = other.ptr;
            in_sbo = false;
        }
        other.ptr = nullptr;
    }

    // 解構：呼叫析構並釋放
    ~AnyCallable() {
        if (!ptr) return;
        ptr->~CallableConcept();
        if (!in_sbo) delete ptr;
    }

    // 呼叫介面
    void operator()() {
        ptr->call();
    }
};

