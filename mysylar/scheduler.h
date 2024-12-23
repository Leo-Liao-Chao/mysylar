/**
 * @file scheduler.h
 * @brief 协程调度器封装
 * 0. 每个线程有一个t_scheduler,t_schedulerfiber（线程池里面的是根t_fiber，线程scheduler是m_rooFiber)没有使用，没有使用。
 *    使用caller，线程scheduler会有一个t_scheduler_fiber
 * 1. 构造函数，配置线程池。bool m_stopping = true;（正在停止)bool m_autoStop = false;
 * use_caller(1:使用scheduler线程，为一个协程线程；0：不使用)
 *      1.使用scheduler线程：创建t_fiber(根线程)，线程池--，指定t_scheduler,m_rootFiber加入run（抢单大厅）->t_scheduler_fiber，设置线程名，线程id加入线程     0.不使用scheduler线程，线程id=-1；
 * 2. start,m_stopping == false直接退出（正在执行）。设置m_stopping = false，任务线程池（不包含scheduler)一定为空->分配线程池。
 * ----------
 * 线程开始跑了->
 * 3.run
 * setThis() ??
 * 不是scheduler线程->创建线程（根fiber）t_scheduler_fiber,发呆线程idle_fiber,执行线程cb_fiber,ft(协程+函数+线程id)->没指定id就是任意线程都可以干，协程/函数就是任务）.
 * 清空ft，tickle_me=false,is_active=false;
 * 在协程池找活儿干->1.指定了线程，但不是我的线程->跳过，并且tickle_me = true,提醒其他线程。2.如果协程正在干，跳过。3. 有活干，取出任务，m_activeThreadCount++，is_active= true;激活线程。
 * tickle_me,有其他线程的活儿干但是没干 或者找到活儿干，就tickle_me
 * 是否提醒
 * 如果有线程要干，切入线程，m_activeThreadCount--；如果任务执行完是Ready状态，加入线程池。如果没有结束把线程状态设为Hold，清除任务。
 * 如果是函数要干，cb_fiber存在把他重置任务，不存在就创建一个cb_fiber，清除ft。执行cb_fiberm_activeThreadCount-;如果任务执行完是Ready状态，加入线程池。如果，异常或者结束，就删除；如果没有结束把线程状态设为Hold，清除任务。
 * 如果没事干，并且是激活状态，m_activeThreadCount--闲置。如果idle_fiber发呆完了，break，stop；此外，开始发呆。如果没TERM或者EXCEP，变成Hold。
 * 4. stop:m_autoStop = true
 * 4.1. 存在m_rootFiber(使用caller)，没有额外线程,状态是开始或者是结束，m_stopping = true，正在停止，没活儿。直接return
 * 4.2. 使用caller，==this；不使用caller！=this
 * 4.3. m_stopping = true;每个线程提醒。如
 * 有活儿干，加入scheduler本线程来干活儿，有活的线程就来干活儿。
 * 4.4. 释放线程，等待结束
 * 5. stopping()，设置autoStop设置停止，正在停止，//没有活干，没有激活的线程。
 * 6.idle()，如果停止了idle_fiber->hold.
 */
#ifndef __MYSYLAR_SCHEDULER_H__
#define __MYSYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include "./fiber.h"
#include "./thread.h"
#include "./noncopyable.h"

namespace mysylar
{
    /**
     * @brief 协程调度器
     * @details 封装的是N-M的协程调度器
     *          内部有一个线程池,支持协程在线程池里面切换
     */
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

        /**
         * @brief 构造函数
         * @param[in] threads 线程数量
         * @param[in] use_caller 是否使用当前调用线程
         * @param[in] name 协程调度器名称
         */
        Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");

        /**
         * @brief 析构函数，停止状态
         */
        virtual ~Scheduler();

        /**
         * @brief 返回协程调度器名称
         */
        const std::string &getName() const { return m_name; }

        /**
         * @brief 返回当前协程调度器
         */
        static Scheduler *GetThis();

        /**
         * @brief 返回当前协程调度器的调度协程
         */
        static Fiber *GetMainFiber();

        /**
         * @brief 启动协程调度器，注册线程
         */
        void start();

        /**
         * @brief 停止协程调度器
         */
        void stop();

        /**
         * @brief 调度协程
         * @param[in] fc 协程或函数
         * @param[in] thread 协程执行的线程id,-1标识任意线程,如果是新增的线程那么就要tickle
         */
        template <class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }

            if (need_tickle)
            {
                tickle();
            }
        }

        /**
         * @brief 批量调度协程
         * @param[in] begin 协程数组的开始
         * @param[in] end 协程数组的结束
         */
        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end)
                {
                    need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                    ++begin;
                }
            }
            if (need_tickle)
            {
                tickle();
            }
        }

        void switchTo(int thread = -1);
        std::ostream &dump(std::ostream &os);

    protected:
        /**
         * @brief 通知协程调度器有任务了
         */

        virtual void tickle();
        /**
         * @brief 协程调度函数
         */
        void run();

        /**
         * @brief 返回是否可以停止 m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
         *        stop()后,并且 m_autoStop,m_stopping = 1
         *        m_fibers.empty() && m_activeThreadCount == 0; 协程池为空,
         */
        virtual bool stopping();

        /**
         * @brief 协程无任务可调度时执行idle协程
         */
        virtual void idle();

        /**
         * @brief 设置当前的协程调度器
         */
        void setThis();

        /**
         * @brief 是否有空闲线程
         */
        bool hasIdleThreads() { return m_idleThreadCount > 0; }

    private:
        /**
         * @brief 协程调度启动(无锁)
         */
        template <class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread)
        {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if (ft.fiber || ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        /**
         * @brief 协程/函数/线程组 (协程/函数,线程id),id = -1表示任意线程
         */
        struct FiberAndThread
        {
            /// 协程
            Fiber::ptr fiber;
            /// 协程执行函数
            std::function<void()> cb;
            /// 线程id
            int thread;

            /**
             * @brief 构造函数
             * @param[in] f 协程
             * @param[in] thr 线程id
             */
            FiberAndThread(Fiber::ptr f, int thr)
                : fiber(f), thread(thr)
            {
            }

            /**
             * @brief 构造函数
             * @param[in] f 协程指针
             * @param[in] thr 线程id
             * @post *f = nullptr
             */
            FiberAndThread(Fiber::ptr *f, int thr)
                : thread(thr)
            {
                fiber.swap(*f);
            }

            /**
             * @brief 构造函数
             * @param[in] f 协程执行函数
             * @param[in] thr 线程id
             */
            FiberAndThread(std::function<void()> f, int thr)
                : cb(f), thread(thr)
            {
            }

            /**
             * @brief 构造函数
             * @param[in] f 协程执行函数指针
             * @param[in] thr 线程id
             * @post *f = nullptr
             */
            FiberAndThread(std::function<void()> *f, int thr)
                : thread(thr)
            {
                cb.swap(*f);
            }

            /**
             * @brief 无参构造函数
             */
            FiberAndThread()
                : thread(-1)
            {
            }

            /**
             * @brief 重置数据
             */
            void reset()
            {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };

    private:
        /// Mutex
        MutexType m_mutex;
        /// 线程池
        std::vector<Thread::ptr> m_threads;
        /// 待执行的协程队列
        std::list<FiberAndThread> m_fibers;
        /// use_caller为true时有效, 调度协程
        Fiber::ptr m_rootFiber;
        /// 协程调度器名称
        std::string m_name;

    protected:
        /// 协程下的线程id数组
        std::vector<int> m_threadIds;
        /// 线程数量
        size_t m_threadCount = 0;
        /// 工作线程数量
        std::atomic<size_t> m_activeThreadCount = {0};
        /// 空闲线程数量
        std::atomic<size_t> m_idleThreadCount = {0};
        /// 是否正在停止
        bool m_stopping = true;
        /// 是否自动停止
        bool m_autoStop = false;
        /// 主线程id(use_caller)
        int m_rootThread = 0;
    };

    class SchedulerSwitcher : public Noncopyable
    {
    public:
        SchedulerSwitcher(Scheduler *target = nullptr);
        ~SchedulerSwitcher();

    private:
        Scheduler *m_caller;
    };

}

#endif
