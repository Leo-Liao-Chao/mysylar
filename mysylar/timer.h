/**
 * @file timer.h
 * @brief 定时器,定时器管理器
 */
#ifndef __MYSYLAR_TIMER_H__
#define __MYSYLAR_TIMER_H__

#include "thread.h"
#include <memory>
#include <set>
#include <vector>

namespace mysylar
{
    // 提前声明给Timer
    class TimerManager;
    /**
     * @brief 定时器
     */
    class Timer : public std::enable_shared_from_this<Timer>
    {
        friend class TimerManager;

    public:
        typedef std::shared_ptr<Timer> ptr;
        /**
         * @brief 取消定时器，成功返回true，失败返回false
         */
        bool cancel();
        /**
         * @brief 刷新定时器，如果存在（cb，timer），更新timer，成功返回true，失败返回false
         */
        bool refresh();
        /**
         * @brief
         * 重置定时器，如果没改变直接返回成功，如果不存在返回失败，如果存在，删除后加入m_manager
         * @param[in] ms 延时
         * @param[in] from_now 是否从当前开始
         */
        bool reset(uint64_t ms, bool from_now);

    private:
        /**
         * @brief 构造函数，周期定时器
         * @param[in] ms 延迟时间
         * @param[in] cb 回调函数
         * @param[in] recurring 是否周期
         * @param[in] manager 定时器管理器
         */
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager *manager);
        /**
         * @brief 构造函数，单次定时器
         * @param[in] next 下次触发时间
         */
        Timer(uint64_t next);

    private:
        // 是否周期恢复
        bool m_recurring = false;
        // 定时时间ms
        uint64_t m_ms = 0;
        // 下一次触发时间ms
        uint64_t m_next = 0;
        // 定时器的回调函数
        std::function<void()> m_cb;
        // 定时器管理器，1.提供管理器更新。2.保护timer线程安全。
        TimerManager *m_manager = nullptr;

    private:
        /**
         * @brief 比较器，比较两个定时器下次触发时间大小，用于set：m_timers
         */
        struct Comparator
        {
            bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
        };
    };

    class TimerManager
    {
        friend class Timer;

    public:
        typedef RWMutex RWMutexType;

        /**
         * @brief 构造函数，m_previouseTime = 当前时间
         */
        TimerManager();
        /**
         * @brief 析构函数
         */
        virtual ~TimerManager();
        /**
         * @brief 添加定时器
         * @param[in] ms 延时
         * @param[in] cb 回调函数
         * @param[in] recurring 是否周期，默认不是周期
         */
        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
        /**
         * @brief 添加条件定时器
         * @param[in] ms 延时
         * @param[in] cb 回调函数
         * @param[in] weak_cond 条件弱指针（用于判断条件是否成立）
         * @param[in] recurring 是否周期，默认不是周期
         */
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                     std::weak_ptr<void> weak_cond, bool recurring = false);
        /**
         * @brief 获取下个定时器的相对于现在的触发时间
         */
        uint64_t getNextTimer();
        /**
         * @brief 获取过期的回调
         * @param[in out] cbs 过期的回调函数
         */
        void listExpiredCb(std::vector<std::function<void()>> &cbs);
        /**
         * @brief 判断是否有定时器
         */
        bool hasTimer();

    protected:
        /**
         * @brief 虚函数用于实现onTimerInsertedAtFront
         */
        virtual void onTimerInsertedAtFront() = 0;
        /**
         * @brief 添加定时器，如果是第一个定时器，则开始定时 Todo:onTimerInsertedAtFront
         * @param[in] val 定时器
         * @param[in] lock 互斥锁
         */
        void addTimer(Timer::ptr val, RWMutexType::WriteLock &lock);

    private:
        /**
         * @brief 用于判断是否发生时间回滚，更新时间
         * @param[in] now_ms 当前时间，更新m_previouseTime
         */
        bool detectClockRollover(uint64_t now_ms);

    private:
        // 互斥锁
        RWMutexType m_mutex;
        // 定时器集合
        std::set<Timer::ptr, Timer::Comparator> m_timers;
        // 是否定时
        bool m_tickled = false;
        // 管理器设置时间
        uint64_t m_previouseTime = 0;
    };

}

#endif
