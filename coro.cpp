#include <condition_variable>
#include <coroutine>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <unordered_map>

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

struct none {};

template <typename X>
struct promise_type_base
{
    template <typename R>
    void return_value(R&& r) { result.emplace(std::forward<R>(r)); }
    std::optional<X> result;
};

template <>
struct promise_type_base<void>
{
    void return_void() {}
    std::optional<none> result;
};

// ----------------------------------------------------------------------------

template <typename T = void>
struct task
{
    using type = T;
    using aux = std::conditional_t<std::is_same_v<T, void>, none, T>;
    struct promise_type
        : promise_type_base<T>
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
        return std::move(this->handle);
    }
    T await_resume()
    {
        if constexpr (not std::same_as<T, void>)
        {
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
    using aux = std::conditional_t<std::is_same_v<T, void>, none, T>;

    struct promise_type
        : promise_type_base<T>
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

struct coroutine_scope
{
    struct work
    {
        struct promise_type
        {
            std::suspend_never initial_suspend() const noexcept { return {}; }
            std::suspend_never final_suspend() const noexcept { return {}; }
            void unhandled_exception() { std::terminate(); }
            work get_return_object() { return {}; }
        };
    };

    std::atomic<std::size_t> count{};
    std::function<void()>    on_empty{[]{}};

    template <typename Task>
    void spawn(Task&& task)
    {
        ++this->count;
        std::invoke(
            [](Task task, auto& scope)->work{
                co_await task;
                if (0u == --scope.count)
                {
                    scope.on_empty();
                }
            },
            std::forward<Task>(task),
            *this
        );
    }
    template <typename Fun>
    void when_empty(Fun&& fun)
    {
        this->on_empty = std::forward<Fun>(fun);
    }
};

// ----------------------------------------------------------------------------

struct context
{
    struct outstanding_request
    {
        std::coroutine_handle<> handle;
        std::optional<std::string> result;
    };
    std::unordered_map<int, outstanding_request> work;

    struct awaiter
    {
        std::unordered_map<int, outstanding_request>*          work;
        std::unordered_map<int, outstanding_request>::iterator request;

        bool await_ready() const noexcept { return this->request->second.result.has_value(); }
        void await_suspend(std::coroutine_handle<> handle)
        {
            this->request->second.handle = handle;
        }
        std::string await_resume()
        {
            std::string rc(std::move(*this->request->second.result));
            this->work->erase(this->request);
            return rc;
        }
    };

    bool empty() const { return this->work.empty(); }
    awaiter request(int id)
    {
        return { &this->work, this->work.insert(std::pair(id, outstanding_request{})).first };
    }
    void complete(int id, std::string value)
    {
        auto& outstanding{this->work[id]};
        outstanding.result.emplace(std::move(value));
        if (outstanding.handle)
        {
            outstanding.handle.resume();
        }
    }
};

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
    std::cout << "\n";

    context         ctxt;
    coroutine_scope scope;
    scope.when_empty([&ctxt]{
            std::cout << "scope is done: ctxt.empty()==" << std::boolalpha << ctxt.empty() << "\n";
        });
    scope.spawn(std::invoke([](context& ctxt)->task<>{
            auto res1{co_await ctxt.request(1)};
            std::cout << "task 1 res1=" << res1 << "\n";
            auto res2{co_await ctxt.request(2)};
            std::cout << "task 1 res2=" << res2 << "\n";
        },
        ctxt));
    scope.spawn(std::invoke([](context& ctxt)->task<>{
            auto res1{co_await ctxt.request(3)};
            std::cout << "task 2 res1=" << res1 << "\n";
            auto res2{co_await ctxt.request(4)};
            std::cout << "task 2 res2=" << res2 << "\n";
        },
        ctxt));
    std::pair<int, std::string> results[]{
        { 3, "value 3" },
        { 2, "value 2" },
        { 1, "value 1" },
        { 4, "value 4" }
    };
    for (auto[id, value]: results)
    {
        ctxt.complete(id, value);
    }
}