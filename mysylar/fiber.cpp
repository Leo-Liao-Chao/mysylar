#include "./fiber.h"
#include "./config.h"
#include "./macro.h"
#include "./log.h"
#include "./scheduler.h"
#include <atomic>

namespace mysylar
{

    static Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");
    // 全局静态变量，std::atomic原子变量，可以实现线程安全。
    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};
    // 线程局部存储变量，用于存储当前线程的协程指针。
    static thread_local Fiber *t_fiber = nullptr;
    static thread_local Fiber::ptr t_threadFiber = nullptr;
    // 一个配置变量，用于设置协程栈的大小。
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
        Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

    class MallocStackAllocator
    {
    public:
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }

        static void Dealloc(void *vp, size_t size)
        {
            return free(vp);
        }
    };

    using StackAllocator = MallocStackAllocator;

    uint64_t Fiber::GetFiberId()
    {
        if (t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }

    Fiber::Fiber()
    {
        m_state = EXEC;
        SetThis(this);
        // 调用 getcontext 函数，并将结果存储在 m_ctx 变量中，m_ctx 是 Fiber 类的一个成员变量，类型为 ucontext_t。这个结构体包含了恢复执行所需的所有信息，比如程序计数器、寄存器集合、信号屏蔽字等。
        // 如果 getcontext 成功执行，它返回 0；如果失败，返回 -1 并设置 errno 以指示错误原因。
        // 捕获当前上下文：在创建一个新的协程时，getcontext 可以捕获调用它的线程的当前执行上下文，这样协程就可以在以后某个时刻恢复执行。
        // 初始化协程上下文：getcontext 可以用于初始化一个新的上下文结构体，使其成为协程的起始点。在这个上下文中，可以设置协程的入口函数和栈空间等。
        if (getcontext(&m_ctx))
        {
            MYSYLAR_ASSERT2(false, "getcontext");
        }

        ++s_fiber_count;

        MYSYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main: s_fiber_count:"<<s_fiber_count;
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
        : m_id(++s_fiber_id), m_cb(cb)
    {
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

        m_stack = StackAllocator::Alloc(m_stacksize);
        if (getcontext(&m_ctx))
        {
            MYSYLAR_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        if (!use_caller)
        {
            // makecontext 是一个 POSIX 标准函数，它用于设置 ucontext_t 结构体中的信息，以便在之后通过 setcontext 或 swapcontext 函数恢复执行时，能够跳转到指定的函数或代码块执行。
            // 配置上下文->准备执行函数cb
            makecontext(&m_ctx, &Fiber::MainFunc, 0);
        }
        else
        {
            // 配置上下文->准备执行函数cb
            makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        }

        MYSYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id<<"s_fiber_count: "<<s_fiber_count;
    }

    Fiber::~Fiber()
    {
        --s_fiber_count;
        if (m_stack)
        {
            MYSYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);

            StackAllocator::Dealloc(m_stack, m_stacksize);
        }
        else
        {
            MYSYLAR_ASSERT(!m_cb);
            MYSYLAR_ASSERT(m_state == EXEC);

            Fiber *cur = t_fiber;
            if (cur == this)
            {
                SetThis(nullptr);
            }
        }
        MYSYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                                    << " total=" << s_fiber_count;
    }

    // 重置协程函数，并重置状态
    // INIT，TERM, EXCEPT ->INIT
    void Fiber::reset(std::function<void()> cb)
    {
        MYSYLAR_ASSERT(m_stack);
        MYSYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        m_cb = cb;
        if (getcontext(&m_ctx))
        {
            MYSYLAR_ASSERT2(false, "getcontext");
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;
    }

    void Fiber::call()
    {
        SetThis(this);
        m_state = EXEC;
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx))
        {
            MYSYLAR_ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::back()
    {
        SetThis(t_threadFiber.get());
        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx))
        {
            MYSYLAR_ASSERT2(false, "swapcontext");
        }
    }

    // 切换到当前协程执行
    void Fiber::swapIn()
    {
        SetThis(this);
        MYSYLAR_ASSERT(m_state != EXEC);
        m_state = EXEC;
        if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx))
        {
            MYSYLAR_ASSERT2(false, "swapcontext");
        }
    }

    // 切换到后台执行
    void Fiber::swapOut()
    {
        SetThis(Scheduler::GetMainFiber());
        if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx))
        {
            MYSYLAR_ASSERT2(false, "swapcontext");
        }
    }

    // 设置当前协程
    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }

    // 返回当前协程
    Fiber::ptr Fiber::GetThis()
    {
        if (t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        // 无参数构造
        Fiber::ptr main_fiber(new Fiber);
        // 确认是默认构造函数SetThis，当前fiber是主fiber
        MYSYLAR_ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    // 协程切换到后台，并且设置为Ready状态
    void Fiber::YieldToReady()
    {
        Fiber::ptr cur = GetThis();
        MYSYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = READY;
        // Test fiber
        cur->back();

        // cur->swapOut();
    }

    // 协程切换到后台，并且设置为Hold状态
    void Fiber::YieldToHold()
    {
        Fiber::ptr cur = GetThis();
        MYSYLAR_ASSERT(cur->m_state == EXEC);
        // Changed By Leo 1
        // cur->m_state = HOLD;
        // Test fiber
        cur->back();

        // cur->swapOut();
    }

    // 总协程数
    uint64_t Fiber::TotalFibers()
    {
        return s_fiber_count;
    }

    void Fiber::MainFunc()
    {
        Fiber::ptr cur = GetThis();
        MYSYLAR_ASSERT(cur);
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception &ex)
        {
            cur->m_state = EXCEPT;
            MYSYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                                        << " fiber_id=" << cur->getId()
                                        << std::endl
                                        << mysylar::BacktraceToString();
        }
        catch (...)
        {
            cur->m_state = EXCEPT;
            MYSYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                                        << " fiber_id=" << cur->getId()
                                        << std::endl
                                        << mysylar::BacktraceToString();
        }

        auto raw_ptr = cur.get();
        cur.reset();
        // Test Fiber
        raw_ptr->back();

        // raw_ptr->swapOut();

        MYSYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
    }
    void Fiber::CallerMainFunc()
    {
        Fiber::ptr cur = GetThis();
        MYSYLAR_ASSERT(cur);
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception &ex)
        {
            cur->m_state = EXCEPT;
            MYSYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                                        << " fiber_id=" << cur->getId()
                                        << std::endl
                                        << mysylar::BacktraceToString();
        }
        catch (...)
        {
            cur->m_state = EXCEPT;
            MYSYLAR_LOG_ERROR(g_logger) << "Fiber Except"
                                        << " fiber_id=" << cur->getId()
                                        << std::endl
                                        << mysylar::BacktraceToString();
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->back();
        MYSYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
    }

}
