#ifndef INCLUDED_ANY_SCHEDULER
#define INCLUDED_ANY_SCHEDULER

#include <stdexec/execution.hpp>
#include <memory>

namespace demo
{
    class any_scheduler_receiver;
    class any_scheduler_state_receiver;
    template <typename Receiver>
    class any_scheduler_state;
    class any_scheduler_state_base;
    template <typename State>
    class any_scheduler_state_concrete;
    class any_scheduler_sender;
    class any_scheduler_base;
    class any_scheduler;
}

class demo::any_scheduler_receiver
{
public:
    virtual void do_set_value() = 0;
    virtual void do_set_stopped() = 0;
};

struct demo::any_scheduler_state_receiver
{
    using is_receiver = void;

    demo::any_scheduler_receiver* state;
    void set_value() noexcept { state->do_set_value(); }
    void set_stopped() noexcept { state->do_set_stopped(); }
};

struct demo::any_scheduler_state_base
{
    virtual ~any_scheduler_state_base() = default;
    virtual void start() = 0;
};

struct demo::any_scheduler_base
{
    virtual ~any_scheduler_base() = default;
    virtual any_scheduler_sender do_schedule(std::shared_ptr<demo::any_scheduler_base> const&) = 0;
};

class demo::any_scheduler_sender
{
public:
    struct env;
    struct base
    {
        virtual ~base() = default;
        virtual std::unique_ptr<demo::any_scheduler_state_base>
        do_connect(demo::any_scheduler_state_receiver) = 0;
    };
    template <typename Sender>
    struct concrete
        : base
    {
        Sender sender;

        template <typename S>
        concrete(S&& sender)
            : sender(std::forward<S>(sender))
        {
        }
        std::unique_ptr<any_scheduler_state_base>
        do_connect(any_scheduler_state_receiver) override;
    };
    std::shared_ptr<demo::any_scheduler_base> scheduler;
    std::unique_ptr<base> sender;

public:
    using is_sender = void;
    using completion_signatures = stdexec::completion_signatures<
        //stdexec::set_stopped_t(),
        stdexec::set_value_t()
    >;

    template <typename Sender>
    any_scheduler_sender(std::shared_ptr<demo::any_scheduler_base> const& scheduler, Sender&& sender)
        : scheduler(scheduler)
        , sender(new concrete<std::remove_cvref_t<Sender>>(std::forward<Sender>(sender)))
    {
        static_assert(stdexec::sender<Sender>);
    }

    template <typename Receiver>
    demo::any_scheduler_state<Receiver> connect(Receiver&&) const;
    template <typename Receiver>
    friend demo::any_scheduler_state<Receiver> tag_invoke(
        stdexec::connect_t,
        any_scheduler_sender const& self,
        Receiver&& receiver)
    {
        return self.connect(std::forward<Receiver>(receiver));
    }

    env get_env() const noexcept;
};

template <typename State>
struct demo::any_scheduler_state_concrete
    : demo::any_scheduler_state_base
{
    std::remove_cvref_t<State> state;
    template <typename Sender>
    any_scheduler_state_concrete(Sender&& sender, demo::any_scheduler_state_receiver r)
        : state(stdexec::connect(std::forward<Sender>(sender), r))
    {
    }
    void start() override
    {
        stdexec::start(this->state);
    }
};

template <typename Receiver>
class demo::any_scheduler_state
    : public demo::any_scheduler_receiver
{
public:
    std::remove_cvref_t<Receiver>                   rec;
    std::unique_ptr<demo::any_scheduler_state_base> state;

    void do_set_value() override { stdexec::set_value(std::move(this->rec)); }
    void do_set_stopped() override { stdexec::set_stopped(std::move(this->rec)); }

    template <typename R>
    any_scheduler_state(demo::any_scheduler_sender::base* sender, R&& rec)
        : rec(std::forward<R>(rec))
        , state(sender->do_connect(any_scheduler_state_receiver{this}))
    {
    }
    any_scheduler_state(any_scheduler_state&&) = delete;

    void start() noexcept
    {
        this->state->start();
    }
};

template <typename Receiver>
demo::any_scheduler_state<Receiver> demo::any_scheduler_sender::connect(Receiver&& receiver) const
{
    return demo::any_scheduler_state<Receiver>(this->sender.get(), std::forward<Receiver>(receiver));
}

template <typename Sender>
std::unique_ptr<demo::any_scheduler_state_base>
demo::any_scheduler_sender::concrete<Sender>::do_connect(demo::any_scheduler_state_receiver receiver)
{
    using State = decltype(stdexec::connect(this->sender, receiver));
    return std::unique_ptr<demo::any_scheduler_state_base>(
        new demo::any_scheduler_state_concrete<State>(this->sender, receiver));
}

class demo::any_scheduler
{
private:
    template <typename Scheduler>
    struct concrete
        : demo::any_scheduler_base
    {
        Scheduler sched;
        template <typename S>
        concrete(S&& sched): sched(std::forward<S>(sched)) {}
        any_scheduler_sender do_schedule(std::shared_ptr<demo::any_scheduler_base> const& self) override
        {
            return any_scheduler_sender(self, stdexec::schedule(this->sched));
        }
    };

    std::shared_ptr<demo::any_scheduler_base> sched;

public:
    template <typename Scheduler>
    any_scheduler(Scheduler sched)
        : sched(new concrete<Scheduler>{sched})
    {
    }
    any_scheduler(std::shared_ptr<demo::any_scheduler_base> sched)
        : sched(sched)
    {
    }

    bool operator== (any_scheduler const&) const = default;

    any_scheduler_sender schedule() {
        return this->sched->do_schedule(this->sched);
    }
};

struct demo::any_scheduler_sender::env
{
    std::shared_ptr<demo::any_scheduler_base> scheduler;
    demo::any_scheduler get_completion_scheduler() const
    {
        return demo::any_scheduler(this->scheduler);
    }
    friend demo::any_scheduler tag_invoke(stdexec::get_completion_scheduler_t<stdexec::set_value_t>,
                                          env const& self)
    {
        return demo::any_scheduler(self.scheduler);
    }
};

demo::any_scheduler_sender::env demo::any_scheduler_sender::get_env() const noexcept
{
    return demo::any_scheduler_sender::env{ this->scheduler };
}

#endif // INCLUDED_ANY_SCHEDULER