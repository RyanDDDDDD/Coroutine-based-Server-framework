#ifndef __SERVER_FIBER_HPP__
#define __SERVER_FIBER_HPP__

#include <ucontext.h>
#include <memory>
#include <functional>

#include "thread.hpp"

namespace Server {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    using ptr = std::shared_ptr<Fiber>;

    enum class State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };

private:
    // constructor for main coroutine
    Fiber();

public:
    // constructor for sub coroutine
    Fiber(std::function<void()> cb, size_t stackSize = 0);

    ~Fiber();
    
    // reset state of a sub coroutine after it finishes
    void reset(std::function<void()> cb);

    // suspend main thread(coroutine) and switch into sub coroutine
    void swapIn();

    // switch back to main coroutine
    void swapOut();

    // get coroutine Id
    uint64_t getId() const { return m_id; };

public:
    // setup current coroutine
    static void setThis(Fiber* f);

    // get current coroutine
    static Fiber::ptr getThis();
    
    // switch out from sub coroutine and set sub coroutine to ready state
    static void yieldToReady();

    // switch out from sub coroutine and set sub coroutine to hold state
    static void yieldToHold();

    // total number of coroutines
    static uint64_t totalFibers();

    // callback in coroutine
    static void mainFunc();

    static uint64_t getFiberId();
private:
    // coroutine id
    uint64_t m_id = 0;
    
    // coroutine size
    uint32_t m_stackSize = 0;

    // coroutine state
    State m_state = State::INIT;

    ucontext_t m_ctx;

    void* m_stack = nullptr;

    // called method in coroutine
    std::function<void()> m_cb;
};

}

#endif