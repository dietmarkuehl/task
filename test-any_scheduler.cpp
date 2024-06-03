#include "any_scheduler.hpp"
#include "inline_scheduler.hpp"
#include <stdexec/execution.hpp>
#include <cassert>

int main()
{
    static_assert(stdexec::scheduler<demo::any_scheduler>);

    demo::any_scheduler scheduler(demo::inline_scheduler{});
    auto x = stdexec::sync_wait(
        stdexec::schedule(scheduler)
        | stdexec::then([]{ return 43; })
        );
    assert(*x == std::tuple<int>(42));
}
