#include <memory>

template <typename> struct any_receiver_virtual;
template <typename... A>
struct any_receiver_virtual<set_value(A...)>
{
    virtual void do_set_value(A...) = 0;
};
template <typename E>
struct any_receiver_virtual<set_value(E)>
{
    virtual void do_set_error(E) = 0;
};
struct any_receiver_virtual<set_stopped()>
{
    virtual void do_set_stopped() = 0;
};

template <typename> struct any_receiver_base;
template <typename... Sigs>
struct any_receiver_base<completion_signatures<Sigs...>>
    : any_receiver_base_virtual<Sigs>...>
{
};

struct any_receiver_concrete<Re

template <typename Completion>
struct any_sender
{
    struct state_base
    {
        virtual void do_start() = 0;
    };
    template <typename State>
    struct state_concrete
    {
        void do_start() override
        {
            start(this->state);
        }
    };
    struct base
    {
        virtual std::unique_ptr<state_base> do_connect() = 0;
    };
    template <typemame Sender>
    struct concrete
        : base
    {
        Sender sender;
        std::unique_ptr<state_base> do_connect(receiver_base* receiver) override
        { 
        }
    };  
};