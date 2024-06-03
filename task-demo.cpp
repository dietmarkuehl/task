#include "any_scheduler.hpp"
#include "inline_scheduler.hpp"
#include "task.hpp"
#include <stdexec/execution.hpp>
#include <coroutine>
#include <iostream>

demo::task<> make_work()
{
    co_await std::suspend_always();
}

int main()
{
    auto t = make_work();
    static_assert(stdexec::sender<std::suspend_always>);
}
