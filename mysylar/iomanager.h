/**
 * @file iomanager.h
 * @brief 基于Epoll的IO协程调度器
 * 管道用于线程间通信。addEvent，...用于idle 线程去处理。trigger就是做事。
 * IOManager:
 *      Event : NONE:0x00 READ:0x01 WRITE:0x04
 *      FdContext: Eventcontext (read,write),fd,events（可能是独写。）
 *          EvenetConetxt:schduler fiber cb
 *          EventContext& getContext(Event event);返回Context（read,write)
 *          void resetContext(EventContext& ctx);清空EventContext.
 *          void triggerEvent(Event event);判断events更新-去event（代表执行完成，cb加入scheduler，ctx.scheduler = nullptr.
 *      IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
 *          创建epoll实列（m_epfd实例的描述符），用于告诉内核这个 epoll 实例预期会监控的文件描述符数量。内核会根据这个提示来优化内部数据结构，以提高效率。
            pipe 系统调用创建一个新的管道，并尝试将管道的读端和写端文件描述符fd存储在 m_tickleFds 数组中。
            创建一个epoll_event结构体，配置为，刻度，边缘触发，指定管道读端fd。
            fcntl 被用来设置文件描述符的属性，具体来说是将文件描述符设置为非阻塞模式。
            epoll_ctl,m_epfd实例监听添加(ADD)了,m_tickleFds[0]这个实例，并且支队他的读(IN)感兴趣。
            m_fdContexts.resize(size);分配fd上下文大小。
            开始scheduler的start();
        ~IOManager():
            stop,关闭监听，清空m_fdContextqs。
            stopping():正在停止，如果timer处理完了m_pendingEventCount没了就正在停止。
            tickle():如果没有空闲的就不tickle了，有空闲的，就向管道写端发一个T信号。
            idle():创建epoll_event events数组。while
                    如果正在停止就break，没有停止的化，设置next的时间，相对时间。
                        监听epoll_wait，看管道读端有没有读到东西，设置了最大读的事件数，最大的等待事件。
                        如果没有听到东西就继续等。
                        听到东西开始干活儿。
                    找到到期的timer，开始干活儿。
                    处理监听到的事件，如果事件读端的事件，循环会一直执行，直到 read 返回 0 或 -1，这样可以确保管道中的数据被完全读取并清除，避免管道缓冲区满导致写端阻塞。
                    处理事件，（add,delete,cancel,cancelAll)
                        如果event的事件包含错误（EPOLLERR）或挂起（EPOLLHUP），event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                        real_events,event.events包含读事件real_events+=READ,
                                    event.events包含写事件real_events+=WRITE
                        fd_ctx->events & real_events同时都有事干。

                        left_events = fd_ctx->events - real_events;
                        判断是剩余事件，如果有，就是修改，没有就是delte了。
                        配置event.events剩余事件。
                        继续监听这个fd。

                        触发事件，--m_pendingEventCount。

                        遍历完事件，idle协程结束。
                addEvent,根据fd，获取FdContex，配置epoll_event事件。操作（修改/添加）fd。++m_pendingEventCount;获取EventContext是read还是write配置event的event_ctx(scheduler,cb,fiber)[注意：fd对应事件是epevent,epevent->fd_ctx,fd_ctx->(schduler,cb,fiber)]
                delEvent,操作(修改或者删除)fd,event删除，--pendingEventCount,重置event_ctx.
                cancelEvent,操作(修改或者删除)fd,把event删除，触发任务，--pendingEventCount。
                cancelAllEvent,,操作(删除)fd,把event删除，触发任务，--pendingEventCount。
 *      
 * 
 */
#ifndef __MYSYLAR_IOMANAGER_H__
#define __MYSYLAR_IOMANAGER_H__

#include "./scheduler.h"
#include "./timer.h"

namespace mysylar {

/**
 * @brief 基于Epoll的IO协程调度器
 */
class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    /**
     * @brief IO事件(NONE,READ,WRITE)
     */
    enum Event {
        /// 无事件
        NONE    = 0x0,
        /// 读事件(EPOLLIN)
        READ    = 0x1,
        /// 写事件(EPOLLOUT)
        WRITE   = 0x4,
    };
private:
    /**
     * @brief Socket事件上线文类
     */
    struct FdContext {
        typedef Mutex MutexType;
        /**
         * @brief 事件上线文类
         */
        struct EventContext {
            /// 事件执行的调度器
            Scheduler* scheduler = nullptr;
            /// 事件协程
            Fiber::ptr fiber;
            /// 事件的回调函数
            std::function<void()> cb;
        };

        /**
         * @brief 获取事件上下文类
         * @param[in] event 事件类型
         * @return 返回对应事件的上线文
         */
        EventContext& getContext(Event event);

        /**
         * @brief 重置事件上下文
         * @param[in, out] ctx 待重置的上下文类
         */
        void resetContext(EventContext& ctx);

        /**
         * @brief 触发事件
         * @param[in] event 事件类型
         */
        void triggerEvent(Event event);

        /// 读事件上下文
        EventContext read;
        /// 写事件上下文
        EventContext write;
        /// 事件关联的句柄
        int fd = 0;
        /// 当前的事件(NONE,READ,WRITE)
        Event events = NONE;
        /// 事件的Mutex
        MutexType mutex;
    };

public:
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否将调用线程包含进去
     * @param[in] name 调度器的名称
     */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    /**
     * @brief 析构函数
     */
    ~IOManager();

    /**
     * @brief 添加事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @param[in] cb 事件回调函数
     * @return 添加成功返回0,失败返回-1
     */
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @attention 不会触发事件
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 取消事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @attention 如果事件存在则触发事件
     */
    bool cancelEvent(int fd, Event event);

    /**
     * @brief 取消所有事件
     * @param[in] fd socket句柄
     */
    bool cancelAll(int fd);

    /**
     * @brief 返回当前的IOManager
     */
    static IOManager* GetThis();
protected:
    /**
     * @brief 没有空闲线程return，有空闲线程，写端发一个信号。
     */
    void tickle() override;
    /**
     * @brief 正在停止
     */
    bool stopping() override;
    /**
     * @brief 设定了一个events来监听事件序列
     * 进入while循环，
     * 判断是否正在停止，停止就结束。
     * 进入第二个监听，每次监听一定时间，监听到内容就跳出循环，没有就继续循环。监听到的内容给events。
     * 处理到期的事件，加入scheduler，清空。
     * 遍历监听事件
     *  获取上下文，如果是错误或者挂起，且事件存在，那么就标志为读写事件。
     * 获取真实事件real_events.
     * 判断剩余事件left_events.注册fd
     * 根据真实事件，触发事件。
     * 触发完成切除线程。
     */
    void idle() override;
    /**
     * @brief 
     */
    void onTimerInsertedAtFront() override;

    /**
     * @brief 重置socket句柄上下文的容器大小
     * @param[in] size 容量大小
     */
    void contextResize(size_t size);

    /**
     * @brief 判断是否可以停止
     * @param[out] timeout 最近要出发的定时器事件间隔
     * @return 返回是否可以停止
     */
    bool stopping(uint64_t& timeout);
private:
    /// epoll 文件句柄
    int m_epfd = 0;
    /// pipe 文件句柄
    int m_tickleFds[2];
    /// 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    /// IOManager的Mutex
    RWMutexType m_mutex;
    /// socket事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};

}

#endif
