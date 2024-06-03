#ifndef INLCUDED_INLINE_SCHELUDER
#define INLCUDED_INLINE_SCHELUDER

#include <stdexec/execution.hpp>

namespace demo
{
    struct inline_scheduler;
    struct inline_sender;
    template <typename Receiver> struct inline_state;
}

template <typename Receiver>
struct demo::inline_state
{
    std::remove_cvref_t<Receiver> receiver;
    void start() noexcept { stdexec::set_value(std::move(receiver)); }
};

struct demo::inline_sender
{
    using is_sender = void;
    struct env;

    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t()
    >;

    template <typename Receiver>
    demo::inline_state<Receiver> connect(Receiver&& receiver)
    {
        return demo::inline_state<Receiver>{ std::forward<Receiver>(receiver) };
    }
    template <typename Receiver>
    friend demo::inline_state<Receiver> tag_invoke(
        stdexec::connect_t,
        inline_sender const&,
        Receiver&& receiver)
    {
        return demo::inline_state<Receiver>{ std::forward<Receiver>(receiver) };
    }
    env get_env() const noexcept;
};

struct demo::inline_scheduler
{
    auto schedule() { return demo::inline_sender(); }
    bool operator== (inline_scheduler const&) const = default;
};

struct demo::inline_sender::env
{
    friend demo::inline_scheduler tag_invoke(stdexec::get_completion_scheduler_t<stdexec::set_value_t>, env const&)
    {
        return {};
    }
};

demo::inline_sender::env demo::inline_sender::get_env() const noexcept
{
    return {};
}

#endif // INLCUDED_INLINE_SCHELUDER