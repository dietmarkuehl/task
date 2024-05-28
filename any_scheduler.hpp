#include <any_sender.hpp>
#include <memory>

struct any_scheduler
{
    struct base
    {
        virtual any_sender<set_value()> do_schedule() = 0;
    };
    template <typename Scheduler>
    struct concrete
        : base
    {
        Scheduler sched;
        any_sender<set_value()> do_schedule() override
        {
            return any_sender<set_value()>(schedule(this->sched));
        }
    };
    
    std::unique_ptr<base> sched;
    template <typename Scheduler>
    any_scheduler(Scheduler sched): sched(new concrete(sched)) {}

    any_sender<set_value()> schedule() {
        return any_sender<set_value()>>(this->sched->do_schedule());
    }
};