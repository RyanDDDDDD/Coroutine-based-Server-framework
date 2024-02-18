#include "fiber.hpp"
#include "config.hpp"
#include "macro.hpp"
#include "log.hpp"
#include "scheduler.hpp"

#include <atomic>

namespace Server {

static Logger::ptr g_logger = SERVER_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id { 0 };
static std::atomic<uint64_t> s_fiber_count { 0 };

// pointer to current main/sub coroutine
static thread_local Fiber* t_fiber{ nullptr };

// pointer to current main coroutine
static thread_local Fiber::ptr t_threadFiber{ nullptr }; 

static ConfigArg<uint32_t>::ptr gFiberStackSize = 
    ConfigMgr::lookUp<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::getFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    
    return 0;
}   

Fiber::Fiber() {
    m_state = State::EXEC;
    setThis(this);

    if (getcontext(&m_ctx)) {
        SERVER_ASSERT_INFO(false, "getcontext failed");
    }

    ++s_fiber_count;
    SERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stackSize, bool useCaller)
    : m_id(++s_fiber_id), m_cb(cb) {

    ++s_fiber_count;
    m_stackSize = stackSize ? stackSize : gFiberStackSize->getValue();

    m_stack = StackAllocator::Alloc(m_stackSize);
    // get current state of thread
    if (getcontext(&m_ctx)) {
        SERVER_ASSERT_INFO(false, "getcontext failed");
    }
    // points to the context that will be resumed when the current context terminates
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;

    if (useCaller) {
        makecontext(&m_ctx, &Fiber::callerMainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::mainFunc, 0);
    }
    SERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    
    if (m_stack) {
        // sub coroutine
        SERVER_ASSERT(m_state == State::TERM 
                    || m_state == State::EXCEPT
                    || m_state == State::INIT);

        StackAllocator::Dealloc(m_stack, m_stackSize);
    } else {
        // main coroutine
        SERVER_ASSERT(!m_cb);
        SERVER_ASSERT(m_state == State::EXEC);

        Fiber* cur = t_fiber;
        if (cur == this) {
            setThis(nullptr);
        }
    }

    SERVER_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
}

void Fiber::reset(std::function<void()> cb) {
    SERVER_ASSERT(m_stack);
    // the coroutine should not be in execution
    SERVER_ASSERT(m_state == State::TERM 
                || m_state == State::EXCEPT
                || m_state == State::INIT);

    m_cb = cb;
    if (getcontext(&m_ctx)){
        SERVER_ASSERT_INFO(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;

    makecontext(&m_ctx, &Fiber::mainFunc, 0);
    m_state = State::INIT;
}

void Fiber::call() {
    setThis(this);
    m_state = State::EXEC;
    SERVER_LOG_ERROR(g_logger) << getId();
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SERVER_ASSERT_INFO(false, "call: swapcontext failed");
    }
}

void Fiber::back() {
    setThis(t_threadFiber.get());
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SERVER_ASSERT_INFO(false, "swapout: swapcontext failed");
    }
}

void Fiber::swapIn() {
    setThis(this);
    SERVER_ASSERT(m_state != State::EXEC);
    m_state = State::EXEC;

    if (swapcontext(&Scheduler::getMainFiber()->m_ctx, &m_ctx)) {
        SERVER_ASSERT_INFO(false, "swapin: swapcontext failed");
    }
}

void Fiber::swapOut() {
    setThis(Scheduler::getMainFiber());
    if (swapcontext(&m_ctx, &Scheduler::getMainFiber()->m_ctx)) {
        SERVER_ASSERT_INFO(false, "swapout: swapcontext failed");
    }
}

void Fiber::setThis(Fiber* f) {
    t_fiber = f;
}

Fiber::ptr Fiber::getThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    // using default constructor to initializ coroutine
    Fiber::ptr main_fiber(new Fiber);
    SERVER_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;

    return t_fiber->shared_from_this();
}

void Fiber::yieldToReady() {
    SERVER_ASSERT(t_fiber != t_threadFiber.get());
    Fiber::ptr cur = getThis();
    cur->m_state = State::READY;
    cur->swapOut();
}

void Fiber::yieldToHold() {
    SERVER_ASSERT(t_fiber != t_threadFiber.get());
    Fiber::ptr cur = getThis();
    cur->m_state = State::HOLD;
    cur->swapOut();
}

uint64_t Fiber::totalFibers() {
    return s_fiber_count;
}

void Fiber::mainFunc() {
    Fiber::ptr cur = getThis();
    SERVER_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = State::TERM;;
    } catch (std::exception& e) {
        cur->m_state = State::EXCEPT;
        SERVER_LOG_ERROR(g_logger) << "Fiber Exception: " << e.what()
                                << " fiber id = " << cur->getId()
                                << std::endl
                                << Server::BacktraceToString();
    } catch (...) {
        cur->m_state = State::EXCEPT;
        SERVER_LOG_ERROR(g_logger) << "Fiber Exception"
                            << " fiber id = " << cur->getId()
                            << std::endl
                            << Server::BacktraceToString();
    }

    auto r_ptr = cur.get();

    // release ownership of shared_ptr
    cur.reset();

    // this is a dangling pointer!!!
    r_ptr->swapOut();

    SERVER_ASSERT_INFO(false, "never reach fiber_id = " + std::to_string(r_ptr->getId()));
}

void Fiber::callerMainFunc() {
    Fiber::ptr cur = getThis();
    SERVER_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = State::TERM;;
    } catch (std::exception& e) {
        cur->m_state = State::EXCEPT;
        SERVER_LOG_ERROR(g_logger) << "Fiber Exception: " << e.what()
                                << " fiber id = " << cur->getId()
                                << std::endl
                                << Server::BacktraceToString();
    } catch (...) {
        cur->m_state = State::EXCEPT;
        SERVER_LOG_ERROR(g_logger) << "Fiber Exception"
                            << " fiber id = " << cur->getId()
                            << std::endl
                            << Server::BacktraceToString();
    }

    auto r_ptr = cur.get();

    // release ownership of shared_ptr
    cur.reset();

    // this is a dangling pointer!!!
    r_ptr->back();

    SERVER_ASSERT_INFO(false, "never reach fiber_id = " + std::to_string(r_ptr->getId()));
}

}