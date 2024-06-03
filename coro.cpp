#include <coroutine>
#include <iostream>
#include <optional>
#include <mutex>
#include <condition_variable>

// ----------------------------------------------------------------------------

template <typename Msg>
struct track
{
    Msg msg;
    track() { std::cout << "default ctor(" << &msg << "): " << msg() << '\n'; }
    track(track const&) { std::cout << "copy ctor(" << &msg << "): " << msg() << '\n'; }
    track(track&&) { std::cout << "move ctor(" << &msg << "): " << msg() << '\n'; }
    ~track() { std::cout << "dtor(" << &msg << "): " << msg() << '\n'; }
};

// ----------------------------------------------------------------------------

template <typename T = void>
struct task
{
    using type = T;

    struct none {};
    using aux = std::conditional_t<std::is_same_v<T, void>, none, T>;
    template <typename X>
    struct promise_type_base
    {
        template <typename R>
        void return_value(R&& r) { result.emplace(std::forward<R>(r)); }
        std::optional<aux> result;
    };
    template <>
    struct promise_type_base<void>
    {
        void return_void() {}
        std::optional<aux> result;
    };

    struct promise_type
        : track<decltype([]()->char const*{ return "task"; })>
        , promise_type_base<T>
    {
        struct final_awaiter
        {
            promise_type* promise;
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept
            {
                return std::exchange(promise->handle, std::move(handle));
            }
            void await_resume() noexcept {}
        };
        std::suspend_always initial_suspend() { return {}; }
        final_awaiter final_suspend() noexcept { return { this }; }
        void unhandled_exception() { std::terminate(); }
        task get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }

        std::coroutine_handle<> handle;
    };

    bool await_ready() const { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> outer)
    {
        this->handle.promise().handle = outer;
        this->result = &this->handle.promise().result;
        std::cout << "promise=" << &this->handle.promise().msg << " result-addr=" << this->result << "\n";
        return std::move(this->handle);
    }
    T await_resume()
    {
        if constexpr (not std::same_as<T, void>)
        {
            std::cout << "await_resume: addr=" << this->result << "\n";
            return **this->result;
        }
    }

    std::coroutine_handle<promise_type> handle;
    std::optional<aux>*                 result;
    task(std::coroutine_handle<promise_type> handle): handle(handle) {}
    task(task&& other): handle(std::exchange(other.handle, std::coroutine_handle<promise_type>())) {}
    ~task() { if (this->handle) this->handle.destroy(); }
};

// ----------------------------------------------------------------------------

template <typename T>
struct starter
{
    struct none {};
    using aux = std::conditional_t<std::is_same_v<T, void>, none, T>;

    template <typename R>
    struct promise_type_base
    {
        std::optional<aux> result;
        template <typename X>
        void return_value(X&& x) { result.emplace(std::forward<X>(x)); }
    };
    template <>
    struct promise_type_base<void>
    {
        std::optional<aux> result;
        void return_void() { result.emplace(none{}); }
    };

    struct promise_type
        : track<decltype([]()->char const*{ return "starter"; })>
        , promise_type_base<T>
    {
        struct final_awaiter
        {
            bool await_ready() noexcept
            {
                this->promise->condition.notify_one();
                return false;
            }
            void await_suspend(std::coroutine_handle<>) noexcept { }
            void await_resume() noexcept {}
            promise_type* promise;
        };
        std::mutex               mutex;
        std::condition_variable condition;

        std::suspend_always initial_suspend() noexcept { return {}; }
        final_awaiter final_suspend() noexcept { return {this}; }
        starter<T> get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;
    ~starter() { if (handle) handle.destroy(); }

    void run() { this->handle.resume(); }
};

template <typename Task>
typename Task::type coroutine_wait(Task task)
{
    auto s = std::invoke([](Task&& task)->starter<typename Task::type> {
        co_return co_await std::move(task);
        },
        std::move(task));
    s.run();
    std::unique_lock kerberos(s.handle.promise().mutex);
    s.handle.promise().condition.wait(kerberos, [&s]{ return s.handle.promise().result; });
    if constexpr (not std::is_same_v<typename Task::type, void>)
    {
        return *s.handle.promise().result;
    }
}

// ----------------------------------------------------------------------------

task<int> make_work()
{
    std::cout << "make work 1\n";
    auto res = (co_await std::invoke([]() -> task<int> {
        co_return 42;
        }));
    std::cout << "nested=" <<  res << "\n";
    std::cout << "make work 2\n";
    co_return 17;
}

int main()
{
    auto t = make_work();
    auto res = coroutine_wait(std::move(t));
    std::cout << "coroutine result=" << res << "\n";
}