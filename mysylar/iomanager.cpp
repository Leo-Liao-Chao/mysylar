#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>

namespace mysylar
{

    static mysylar::Logger::ptr g_logger = MYSYLAR_LOG_NAME("system");

    enum EpollCtlOp
    {
    };

    static std::ostream &operator<<(std::ostream &os, const EpollCtlOp &op)
    {
        switch ((int)op)
        {
#define XX(ctl) \
    case ctl:   \
        return os << #ctl;
            XX(EPOLL_CTL_ADD);
            XX(EPOLL_CTL_MOD);
            XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
        }
#undef XX
    }

    static std::ostream &operator<<(std::ostream &os, EPOLL_EVENTS events)
    {
        if (!events)
        {
            return os << "0";
        }
        bool first = true;
#define XX(E)          \
    if (events & E)    \
    {                  \
        if (!first)    \
        {              \
            os << "|"; \
        }              \
        os << #E;      \
        first = false; \
    }
        XX(EPOLLIN);
        XX(EPOLLPRI);
        XX(EPOLLOUT);
        XX(EPOLLRDNORM);
        XX(EPOLLRDBAND);
        XX(EPOLLWRNORM);
        XX(EPOLLWRBAND);
        XX(EPOLLMSG);
        XX(EPOLLERR);
        XX(EPOLLHUP);
        XX(EPOLLRDHUP);
        XX(EPOLLONESHOT);
        XX(EPOLLET);
#undef XX
        return os;
    }

    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event)
    {
        switch (event)
        {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            MYSYLAR_ASSERT2(false, "getContext");
        }
        throw std::invalid_argument("getContext invalid event");
    }

    void IOManager::FdContext::resetContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(IOManager::Event event)
    {
        // MYSYLAR_LOG_INFO(g_logger) << "fd=" << fd
        //     << " triggerEvent event=" << event
        //     << " events=" << events;
        MYSYLAR_ASSERT(events & event);
        // if(MYSYLAR_UNLIKELY(!(event & event))) {
        //     return;
        // }
        events = (Event)(events & ~event);
        EventContext &ctx = getContext(event);
        if (ctx.cb)
        {
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler = nullptr;
        return;
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
    {
        // epoll_create 函数用于创建一个epoll实例，5000 是指定的初始事件队列大小。m_epfd 是存储创建的epoll文件描述符的成员变量。MYSYLAR_ASSERT 宏用于确保 m_epfd 是一个有效的文件描述符（大于0）。
        m_epfd = epoll_create(5000);
        MYSYLAR_ASSERT(m_epfd > 0);
        // pipe 函数用于创建一个管道，m_tickleFds 是一个包含两个文件描述符的数组，用于在 IOManager 需要被“唤醒”时发送信号。MYSYLAR_ASSERT 确保管道创建成功（返回值为0）。
        int rt = pipe(m_tickleFds);
        MYSYLAR_ASSERT(!rt);
        // 这段代码初始化一个 epoll_event 结构体，设置事件类型为 EPOLLIN（可读事件）和 EPOLLET（边缘触发模式）。event.data.fd 设置为管道的 读 端文件描述符。
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];
        // fcntl 函数用于设置管道读端的文件状态标志，将其设置为非阻塞模式。MYSYLAR_ASSERT 确保操作成功。
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        MYSYLAR_ASSERT(!rt);
        // epoll_ctl 函数用于将管道 读 端添加到epoll实例中，以便可以监控其可读事件。MYSYLAR_ASSERT 确保事件添加成功。
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        MYSYLAR_ASSERT(!rt);
        // contextResize 函数可能用于调整内部上下文或缓存的大小，这里设置为32。
        contextResize(32);
        // 处理
        start();
    }

    IOManager::~IOManager()
    {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i])
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext *fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        }
        else
        {
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        // 事件添加重复。
        if (MYSYLAR_UNLIKELY(fd_ctx->events & event))
        {
            MYSYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                        << " event=" << (EPOLL_EVENTS)event
                                        << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
            MYSYLAR_ASSERT(!(fd_ctx->events & event));
        }
        // 判断事件是否存在，存在就是修改，不存在就是ADD
        // 边缘触发、两个事件
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx;

        // epoll_ctl 函数用于控制 epoll 实例，m_epfd 是 epoll 文件描述符。
        // op 是要执行的操作类型，可以是 EPOLL_CTL_ADD（添加事件）、EPOLL_CTL_MOD（修改事件）或 EPOLL_CTL_DEL（删除事件）。
        // fd 是要监控的文件描述符。
        // &epevent 是指向 epoll_event 结构体的指针，包含了事件类型和相关数据。
        // rt 是 epoll_ctl 函数的返回值，如果操作成功，返回值为0；如果失败，返回值为-1，并设置 errno 以指示错误原因。
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            MYSYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                        << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                        << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                        << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
        }

        ++m_pendingEventCount;
        fd_ctx->events = (Event)(fd_ctx->events | event);
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        MYSYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        // 加入回调。
        if (cb)
        {
            event_ctx.cb.swap(cb);
        }
        // 加入协程。
        else
        {
            event_ctx.fiber = Fiber::GetThis();
            MYSYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, "state=" << event_ctx.fiber->getState());
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (MYSYLAR_UNLIKELY(!(fd_ctx->events & event)))
        {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            MYSYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                        << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                        << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events = new_events;
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (MYSYLAR_UNLIKELY(!(fd_ctx->events & event)))
        {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            MYSYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                        << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                        << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!fd_ctx->events)
        {
            return false;
        }

        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            MYSYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                        << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                        << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        if (fd_ctx->events & READ)
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if (fd_ctx->events & WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        MYSYLAR_ASSERT(fd_ctx->events == 0);
        return true;
    }

    IOManager *IOManager::GetThis()
    {
        // 派生类转换，转换失败返回nullptr。
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    bool IOManager::stopping(uint64_t &timeout)
    {
        timeout = getNextTimer();
        return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    bool IOManager::stopping()
    {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    void IOManager::tickle()
    {
        if (!hasIdleThreads())
        {
            return;
        }
        // 其目的是向一个管道的写端发送一个信号。
        int rt = write(m_tickleFds[1], "T", 1);
        MYSYLAR_ASSERT(rt == 1);
    }
    // 正在停止，没事干

    void IOManager::idle()
    {
        MYSYLAR_LOG_DEBUG(g_logger) << "idle";
        // 定义了一个最大事件数 MAX_EVENTS 为 256，并创建了一个 epoll_event 数组来存储 epoll 事件。同时，使用 std::shared_ptr 来管理这个数组的内存，确保在 idle 方法结束时能够正确释放内存。
        const uint64_t MAX_EVNETS = 256;
        epoll_event *events = new epoll_event[MAX_EVNETS]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr)
                                                   { delete[] ptr; });

        while (true)
        {
            uint64_t next_timeout = 0;
            // 如果 IOManager 正在停止，记录日志并退出循环。
            if (MYSYLAR_UNLIKELY(stopping(next_timeout)))
            {
                MYSYLAR_LOG_INFO(g_logger) << "name=" << getName()
                                           << " idle stopping exit";
                break;
            }

            int rt = 0;
            do
            {
                // 最大超时时间 MAX_TIMEOUT 为 3000 毫秒，并根据 next_timeout 的值来决定实际的超时时间。
                static const int MAX_TIMEOUT = 3000;
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > MAX_TIMEOUT
                                       ? MAX_TIMEOUT
                                       : next_timeout;
                }
                else
                {
                    next_timeout = MAX_TIMEOUT;
                }
                // 使用 epoll_wait 等待事件，m_epfd 是 epoll 实例的文件描述符，events 是存储事件的数组，MAX_EVNETS 是最大事件数，next_timeout 是超时时间。
                rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
                // 如果 epoll_wait 被中断（例如，由于信号），则忽略并重新等待。
                if (rt < 0 && errno == EINTR)
                {
                }
                else
                {
                    break;
                }
            } while (true);
            // 这里获取所有已过期的定时器回调，并在 cbs 向量中存储。
            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if (!cbs.empty())
            {
                // MYSYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
                // 所有定时器回调调度到事件循环中执行
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            // if(MYSYLAR_UNLIKELY(rt == MAX_EVNETS)) {
            //     MYSYLAR_LOG_INFO(g_logger) << "epoll wait events=" << rt;
            // }
            // 遍历所有 epoll_wait 返回的事件，并根据事件类型（读、写、错误等）进行处理。
            for (int i = 0; i < rt; ++i)
            {
                epoll_event &event = events[i];
                // 如果事件是管道事件，则读取管道数据以清除事件。0-read
                if (event.data.fd == m_tickleFds[0])
                {
                    uint8_t dummy[256];
                    while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
                        ;
                    continue;
                }
                // 获取与文件描述符关联的上下文，并根据事件类型触发相应的事件处理函数。
                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                // 如果事件包含错误（EPOLLERR）或挂起（EPOLLHUP），则将这些事件标记为读（EPOLLIN）或写（EPOLLOUT）事件，如果它们已经在 FdContext 中被设置。
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                }
                // epoll_event 是否包含读（EPOLLIN）或写（EPOLLOUT）事件，并设置 real_events 变量以反映实际发生的事件。
                int real_events = NONE;
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }
                // 如果 FdContext 中没有设置任何实际发生的事件，则跳过当前事件，不进行进一步处理。
                if ((fd_ctx->events & real_events) == NONE)
                {
                    continue;
                }
                // 这段代码计算剩余的事件（即在 FdContext 中设置但未在当前事件中处理的事件）。根据剩余事件的数量，决定是修改（EPOLL_CTL_MOD）还是删除（EPOLL_CTL_DEL）epoll 实例中的事件。同时，设置 event.events 为边缘触发模式（EPOLLET）和剩余事件的组合。
                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                // 注册回fd
                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                // 如果 epoll_ctl 调用失败，则记录错误日志。
                if (rt2)
                {
                    MYSYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                                << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                                                << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                // SYLAR_LOG_INFO(g_logger) << " fd=" << fd_ctx->fd << " events=" << fd_ctx->events
                //                          << " real_events=" << real_events;
                if (real_events & READ)
                {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE)
                {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
            // 在事件循环的末尾，交换出当前协程，以便其他协程可以运行。
            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();

            raw_ptr->swapOut();
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }

}
