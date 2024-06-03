#include "inline_scheduler.hpp"
#include <stdexec/execution.hpp>
#include <cassert>

int main()
{
    static_assert(stdexec::scheduler<demo::inline_scheduler>);
    demo::inline_scheduler inline_sched;
    auto x = stdexec::sync_wait(
        stdexec::schedule(inline_sched)
        | stdexec::then([]{ return 42; })
        );
    assert(x == std::tuple(42));
}
