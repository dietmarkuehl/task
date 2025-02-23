#include "any_scheduler.hpp"
#include "inline_scheduler.hpp"
#include "task.hpp"
#include <stdexec/execution.hpp>
#include <iostream>

namespace test
{
    struct receiver
    {
        struct env {};
        env get_env() const noexcept { return {}; }

        using is_receiver = void;
        void set_value() noexcept {}
        void set_stopped() noexcept {}
    };
}

int main()
{
    static_assert(stdexec::scheduler<demo::inline_scheduler>);
    demo::inline_scheduler inline_sched;
    auto state = stdexec::connect(stdexec::schedule(inline_sched), test::receiver());
    static_assert(stdexec::operation_state<decltype(state)>);
    stdexec::start(state);

    demo::any_scheduler scheduler(demo::inline_scheduler{});
    static_assert(stdexec::scheduler<demo::any_scheduler>);

    auto s = stdexec::schedule(scheduler);
    static_assert(stdexec::sender<decltype(s)>);
    static_assert(stdexec::receiver<test::receiver>);

    auto o = stdexec::connect(std::move(s), test::receiver());
    stdexec::start(o);
}
