#ifndef INCLUDED_TASK
#define INCLUDED_TASK

#include <coroutine>
#include <optional>
#include <stdexec/execution.hpp>
#include "any_scheduler.hpp"

namespace demo
{
    template <typename RC = void> struct task;

    template <stdexec::sender Sender> struct sender_awaiter;
}

template <typename RC>
struct demo::task
{
    struct promise_type
    {
        std::optional<demo::any_scheduler> scheduler;

        constexpr std::suspend_always initial_suspend() const noexcept { return {}; }
        constexpr std::suspend_always final_suspend() const noexcept { return {}; }
        task get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        void unhandled_exception() { std::terminate(); }
        
        template <typename Awaiter>
        auto await_transform(Awaiter&& awaiter)
        {
            return std::forward<Awaiter>(awaiter);
        }
#if 0
        {
            return stdexec::as_awaitable(
                stdexec::transfer(
                    this->scheduler,
                    stdexec::to_sender(std::forward<Awaiter>(awaiter))
                ),
                *this
            );
        }
#endif
#if 0
        template <stdexec::sender Sender>
        auto await_transform(Sender&& s)
        {
            return stdexec::as_awaitable(
                stdexec::transfer(std::forward<Sender>(s), this->scheduler),
                *this
            );
        }
#endif
    };
    
    std::coroutine_handle<promise_type> handle;
};

#endif //INCLUDED_TASK