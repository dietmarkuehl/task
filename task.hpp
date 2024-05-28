#include <coroutines>

template <typename RC = void>
struct task
{
    struct promise_type
    {
        constexpr std::suspend_always initial_suspend() const noexcept { return {}; }
        task get_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        
        template <typename Awaiter>
        auto await_transform(Awaiter&& awaiter)
        {
            return as_awaiter(transfer(this->scheduler, to_sender(std::forward<Awaiter>(awaiter)));
        }
        template <sender Sender>
        auto await_transform(Sender&& s)
        {
            return as_awaiter(transfer(this->scheduler, std::forward<Sender>(s));
        }
    };
    
    std::coroutine_handle<promise_type> handle;
};