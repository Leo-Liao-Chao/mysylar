/**
 * @file fiber.h
 * @brief 协程封装
 * @details 
 * 1. 每个线程有一个Fiber t_fiber（用于执行任务）,Fiber::ptr t_threadFiber指向（根t_fiber）的智能指针
 * 2. 通过GetThis创建main_fiber->t_threadFiber,私有无参构造函数构造t_fiber(一次),状态为EXEC
 * 3. 有参构造函数，构造任务协程指定任务函数，栈大小，是否使用caller(0->Scheduler,1->无Scheduler)，并且绑定m_ctx，用于执行任务，任务执行完设置TERM状态。
 * 4. swapIn,call()先切换t_fiber为任务fiber，fiber设为EXEC状态，切换任务进行点
 * 5. back,swapOut()先切换为（根fiber），切换回去。
 * 6. SetThis,指定t_fiber;GetThis,创建t_threadFiber，t_fiber/返回t_fiber;
 * 7. reset(cb),重置协程任务cb,INIT。schedule才能重置
 * 8. YieldTohold(No Hold),YieldToReady(Ready)，把运行状态的(根t_fiber),swapOut()，重置状态。
 * 9. TotalFibers(),return s_fiber_count;
 * ---------------------
 * Question 1:为什么有参Fiber，不设初始状态；
 * Question 2:析构函数，为什么t_threadFiber构造了三个根t_fiber,s_fiber_count只加了一次。
 */
#ifndef __MYSYLAR_FIBER_H__
#define __MYSYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

namespace mysylar
{

    class Scheduler;

    /**
     * @brief 协程类
     */
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
        friend class Scheduler;

    public:
        typedef std::shared_ptr<Fiber> ptr;

        /**
         * @brief 协程状态
         */
        enum State
        {
            /// 初始化状态
            INIT,
            /// 暂停状态
            HOLD,
            /// 执行中状态
            EXEC,
            /// 结束状态
            TERM,
            /// 可执行状态
            READY,
            /// 异常状态
            EXCEPT
        };

    private:
        /**
         * @brief 无参构造函数，s_fiber_count++
         * @attention 每个线程第一个协程的构造
         */
        Fiber();

    public:
        /**
         * @brief 构造函数，s_fiber_count++
         * @param[in] cb 协程执行的函数
         * @param[in] stacksize 协程栈大小
         * @param[in] use_caller 是否在MainFiber（t_threadFiber）上调度
         */
        Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);

        /**
         * @brief 析构函数，非EXEC析构，主协程析构
         */
        ~Fiber();

        /**
         * @brief 重置协程执行函数,并设置状态
         * @pre getState() 为 INIT, TERM, EXCEPT
         * @post getState() = INIT
         */
        void reset(std::function<void()> cb);

        /**
         * @brief 将当前协程切换到运行状态
         * @pre getState() != EXEC
         * @post getState() = EXEC
         */
        void swapIn();

        /**
         * @brief 将当前协程切换到后台
         */
        void swapOut();

        /**
         * @brief 将当前线程切换到执行状态 t_fiber = this
         * @pre 执行的为当前线程的主协程
         */
        void call();

        /**
         * @brief 将当前线程切换到后台，t_fiber = t_threadFiber.get()
         * @pre 执行的为该协程
         * @post 返回到线程的主协程
         */
        void back();

        /**
         * @brief 返回协程id
         */
        uint64_t getId() const { return m_id; }

        /**
         * @brief 返回协程状态
         */
        State getState() const { return m_state; }

    public:
        /**
         * @brief 设置当前线程的运行协程t_fiber
         * @param[in] f 运行协程
         */
        static void SetThis(Fiber *f);

        /**
         * @brief 返回当前所在的协程t_fiber
         */
        static Fiber::ptr GetThis();

        /**
         * @brief 将当前协程切换到后台,并设置为READY状态->swapOut()
         * @post getState() = READY
         */
        static void YieldToReady();

        /**
         * @brief 将当前协程切换到后台,并设置为HOLD状态->swapOut() EXEC=HOLD
         * @post getState() = HOLD
         */
        static void YieldToHold();

        /**
         * @brief 返回当前协程的总数量
         */
        static uint64_t TotalFibers();

        /**
         * @brief 协程执行函数->swapOut()->Scheduler::GetMainFiber()
         * @post 执行完成返回到线程主协程
         */
        static void MainFunc();

        /**
         * @brief 协程执行函数->back()->t_threadFiber.fiber线程主fiber
         * @post 执行完成返回到线程调度协程
         */
        static void CallerMainFunc();

        /**
         * @brief 获取当前协程的id
         */
        static uint64_t GetFiberId();

    private:
        /// 协程id
        uint64_t m_id = 0;
        /// 协程运行栈大小
        uint32_t m_stacksize = 0;
        /// 协程状态
        State m_state = INIT;
        /// 协程上下文
        ucontext_t m_ctx;
        /// 协程运行栈指针
        void *m_stack = nullptr;
        /// 协程运行函数
        std::function<void()> m_cb;
    };

}

#endif
