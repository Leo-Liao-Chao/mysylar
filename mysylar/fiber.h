#ifndef __MYSYLAR_FIBER_H__
#define _MYSYLAR_FIBER_H__

#include <memory>
#include "thread.h"
#include <ucontext.h>
#include <functional>

namespace mysylar
{
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        typedef std::shared_ptr<Fiber> ptr;

        enum State
        {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };

    private:
        Fiber();

    public:
        Fiber(std::function<void()> cb, size_t stacksize = 0);
        ~Fiber();
        // 重置协程函数，并重置状态
        // INIT，TERM
        void reset(std::function<void()> cb);
        // 切换到当前协程执行
        void swapIn();
        // 切换到后台执行
        void swapOut();
        uint64_t getId() const{return m_id;}
    public:
        static void SetThis(Fiber *f);
        // 返回当前协程
        static Fiber::ptr GetThis();
        // 协程切换到后台，并且设置为ready状态
        static void YieldToReady();
        // 协程切换到后台，并且设置为hold状态
        static void YieldToHold();
        // Total fiber number
        static uint64_t TotalFibers();

        static void MainFunc();
        static uint64_t GetFiberId();

    private:
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        State m_state = INIT;

        ucontext_t m_ctx;
        void *m_stack = nullptr;

        std::function<void()> m_cb;
    };

}
#endif