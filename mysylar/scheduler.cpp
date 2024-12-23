#include "./scheduler.h"
#include "./log.h"
#include "./macro.h"
// Changed by Leo 1
// #include "./hook.h"

namespace mysylar
{

    static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

    static thread_local Scheduler *t_scheduler = nullptr;
    static thread_local Fiber *t_scheduler_fiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
        : m_name(name)
    {
        MYSYLAR_ASSERT(threads > 0);

        if (use_caller)
        {
            std::cout << "Use caller" << std::endl;
            // 构建主fiber
            mysylar::Fiber::GetThis();
            --threads;

            MYSYLAR_ASSERT(GetThis() == nullptr);
            // 创建scheduler
            t_scheduler = this;
            // 构造fiber
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            t_scheduler_fiber = m_rootFiber.get();

            mysylar::Thread::SetName(m_name);
            m_rootThread = mysylar::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler()
    {
        MYSYLAR_ASSERT(m_stopping);
        if (GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }
    void Scheduler::setThis()
    {
        t_scheduler = this;
    }

    Fiber *Scheduler::GetMainFiber()
    {
        return t_scheduler_fiber;
    }

    void Scheduler::start()
    {
        MutexType::Lock lock(m_mutex);
        if (!m_stopping)
        {
            return;
        }
        m_stopping = false;
        // 分配线程
        MYSYLAR_ASSERT(m_threads.empty());
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        lock.unlock();

        // if(m_rootFiber) {
        //     //m_rootFiber->swapIn();
        //     m_rootFiber->call();
        //     MYSYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
        // }
    }
    bool Scheduler::stopping()
    {
        MutexType::Lock lock(m_mutex);
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }
    void Scheduler::stop()
    {
        m_autoStop = true;
        if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
        {
            MYSYLAR_LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if (stopping())
            {
                return;
            }
        }

        // bool exit_on_this_fiber = false;
        if (m_rootThread != -1)
        {
            MYSYLAR_ASSERT(GetThis() == this);
        }
        else
        {
            MYSYLAR_ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            // while(!stopping()) {
            //     if(m_rootFiber->getState() == Fiber::TERM
            //             || m_rootFiber->getState() == Fiber::EXCEPT) {
            //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            //         MYSYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
            //         t_fiber = m_rootFiber.get();
            //     }
            //     m_rootFiber->call();
            // }
            if (!stopping())
            {
                std::cout << "m_rootFiber->call()" << std::endl;
                m_rootFiber->call();
            }
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for (auto &i : thrs)
        {
            i->join();
        }
        // if(exit_on_this_fiber) {
        // }
    }

    void Scheduler::run()
    {
        MYSYLAR_LOG_DEBUG(g_logger) << m_name << " run";
        // Changed by Leo 1
        //  set_hook_enable(true);
        setThis();
        if (mysylar::GetThreadId() != m_rootThread)
        {
            t_scheduler_fiber = Fiber::GetThis().get();
        }

        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cb_fiber;

        FiberAndThread ft;
        while (true)
        {
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                // 判断是否有协程要做
                while (it != m_fibers.end())
                {
                    // 如果当前协程,!=-1（指定线程）并且当前线程不是指定线程，continue，设置有活儿干，提醒一下。
                    if (it->thread != -1 && it->thread != mysylar::GetThreadId())
                    {
                        ++it;
                        tickle_me = true;
                        // std::cout<<"Exception fiber 1"<<std::endl;
                        continue;
                    }
                    // 确认有活儿干
                    MYSYLAR_ASSERT(it->fiber || it->cb);
                    // 如果协程，存在且它在执行，continue
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC)
                    {
                        ++it;
                        // std::cout<<"Exception fiber 2"<<std::endl;
                        continue;
                    }
                    // 如果协程没在执行,取出来执行,激活线程，删除协程
                    ft = *it;
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    is_active = true;
                    // std::cout<<"Exception fiber 3"<<std::endl;
                    break;
                }
                // 需要提醒提醒，找到活儿干也提醒。
                tickle_me |= it != m_fibers.end();
            }
            // 有活儿干就提醒，
            if (tickle_me)
            {
                tickle();
            }
            // 如果有本线程的活儿干，是协程，状态正常
            if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT))
            {
                // 执行协程
                ft.fiber->swapIn();
                --m_activeThreadCount;

                if (ft.fiber->getState() == Fiber::READY)
                {
                    // 就位就加入线程池
                    schedule(ft.fiber);
                }
                else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
                {
                    // 没有结束/异常,就Hold
                    ft.fiber->m_state = Fiber::HOLD;
                }
                // 清除ft
                ft.reset();
            }
            // 如果有本线程的活儿干，是函数
            else if (ft.cb)
            {
                if (cb_fiber)
                {
                    // 如果存在cb_fiber重置成INIT
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    // 如果不存在,就新一个协程
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                // 清除ft
                ft.reset();
                // 加入执行
                cb_fiber->swapIn();
                --m_activeThreadCount;

                if (cb_fiber->getState() == Fiber::READY)
                {
                    // 加入m_fibers
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
                {
                    cb_fiber->reset(nullptr);
                }
                else
                { // if(cb_fiber->getState() != Fiber::TERM) {
                    // 变成Hold状态
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            }
            else
            {
                // 没有协程要做.
                // 如果是激活的,--,认为结束了
                if (is_active)
                {
                    --m_activeThreadCount;
                    continue;
                }
                // 如果是发完呆了,结束循环,意味着线程结束.
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    MYSYLAR_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }
                // 发呆的发呆++
                ++m_idleThreadCount;
                // 执行idle
                idle_fiber->swapIn();
                // 为什么只执行一次
                // 发呆--
                --m_idleThreadCount;
                // 变成Hold状态
                if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
                {
                    idle_fiber->m_state = Fiber::HOLD;
                }
            }
        }
    }

    void Scheduler::tickle()
    {
        MYSYLAR_LOG_INFO(g_logger) << "tickle";
    }

    void Scheduler::idle()
    {
        MYSYLAR_LOG_INFO(g_logger) << "idle";
        while (!stopping())
        {
            mysylar::Fiber::YieldToHold();
        }
    }

    void Scheduler::switchTo(int thread)
    {
        MYSYLAR_ASSERT(Scheduler::GetThis() != nullptr);
        if (Scheduler::GetThis() == this)
        {
            if (thread == -1 || thread == mysylar::GetThreadId())
            {
                return;
            }
        }
        schedule(Fiber::GetThis(), thread);
        Fiber::YieldToHold();
    }

    std::ostream &Scheduler::dump(std::ostream &os)
    {
        os << "[Scheduler name=" << m_name
           << " size=" << m_threadCount
           << " active_count=" << m_activeThreadCount
           << " idle_count=" << m_idleThreadCount
           << " stopping=" << m_stopping
           << " ]" << std::endl
           << "    ";
        for (size_t i = 0; i < m_threadIds.size(); ++i)
        {
            if (i)
            {
                os << ", ";
            }
            os << m_threadIds[i];
        }
        return os;
    }

    SchedulerSwitcher::SchedulerSwitcher(Scheduler *target)
    {
        m_caller = Scheduler::GetThis();
        if (target)
        {
            target->switchTo();
        }
    }

    SchedulerSwitcher::~SchedulerSwitcher()
    {
        if (m_caller)
        {
            m_caller->switchTo();
        }
    }

}
