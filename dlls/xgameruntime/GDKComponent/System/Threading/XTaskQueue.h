/*
 * XTaskQueue Implementation
 *  From https://github.com/microsoft/libHttpClient
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _XTASKQUEUE_H_
#define _XTASKQUEUE_H_

#include "../../../private.h"
#include "StaticArray.h"
#include "referenced_ptr.h"
#include "ThreadPool.h"
#include "AtomicVector.h"
#include "WaitTimer.h"
#include "LocklessQueue.h"

#include <mutex>

// For debugging purposes
#define USE_UNIQUE_HANDLES true

// DO NOT USE SUSPEND_API. ITS FOR NON-WINDOWS TARGETS!
// #define SUSPEND_API 1

#define PORT_WAIT_MAX 60
#define PORT_EVENT_MAX (PORT_WAIT_MAX + 1)
#define QUEUE_WAIT_MAX (PORT_WAIT_MAX * 2)

static uint32_t const SUBMIT_CALLBACK_MAX = 32;

enum class TaskQueuePortStatus
{
    Active,
    Canceled,
    Terminating,
    Terminated
};

enum class XTaskQueueDuplicateOptions
{
    None,
    Reference
};

struct ITaskQueue;
struct ITaskQueuePort;
struct ITaskQueuePortContext;

struct XTaskQueueObject
{
    uint32_t m_signature;
    uint32_t m_runtimeIteration;
    ITaskQueue* m_queue;
};

struct XTaskQueuePortObject
{
    uint32_t m_signature;
    uint32_t m_runtimeIteration;
    ITaskQueuePort* m_port;
    ITaskQueue* m_queue;
};

struct XTaskQueueTestHooks
{
    virtual void PendingEntriesRemovedDuringTermination(
        XTaskQueuePort port)
    {
        UNREFERENCED_PARAMETER(port);
    }

    virtual void NoNextPendingCallbackFound(
        XTaskQueuePort port,
        uint64_t dueTime)
    {
        UNREFERENCED_PARAMETER(port);
        UNREFERENCED_PARAMETER(dueTime);
    }

    virtual void NextPendingCallbackScheduled(
        XTaskQueuePort port,
        uint64_t lastDueTime,
        uint64_t nextDueTime)
    {
        UNREFERENCED_PARAMETER(port);
        UNREFERENCED_PARAMETER(lastDueTime);
        UNREFERENCED_PARAMETER(nextDueTime);
    }
};

struct ITaskQueuePort : IUnknown
{
    virtual XTaskQueuePortHandle WINAPI GetHandle() = 0;

    virtual HRESULT WINAPI QueueItem(
        ITaskQueuePortContext* portContext,
        uint32_t waitMs,
        void* callbackContext,
        XTaskQueueCallback* callback) = 0;

    virtual HRESULT WINAPI RegisterWaitHandle(
        ITaskQueuePortContext* portContext,
        HANDLE waitHandle,
        void* callbackContext,
        XTaskQueueCallback* callback,
        XTaskQueueRegistrationToken* token) = 0;

    virtual void WINAPI UnregisterWaitHandle(
        XTaskQueueRegistrationToken token) = 0;

    virtual HRESULT WINAPI PrepareTerminate(
        ITaskQueuePortContext* portContext,
        void* callbackContext,
        XTaskQueueTerminatedCallback* callback,
        void** token) = 0;

    virtual void WINAPI CancelTermination(
        void* token) = 0;

    virtual void WINAPI Terminate(
        void* token) = 0;

    virtual HRESULT WINAPI Attach(
        ITaskQueuePortContext* portContext) = 0;
    
    virtual void WINAPI Detach(
        ITaskQueuePortContext* portContext) = 0;

    virtual bool WINAPI Dispatch(
        ITaskQueuePortContext* portContext,
        uint32_t timeoutInMs) = 0;

    virtual bool WINAPI IsEmpty() = 0;

    virtual void WINAPI WaitForUnwind() = 0;

    virtual HRESULT WINAPI SuspendTermination(
        ITaskQueuePortContext* portContext) = 0;

    virtual void WINAPI ResumeTermination(
        ITaskQueuePortContext* portContext) = 0;

    virtual void WINAPI SuspendPort() = 0;
    virtual void WINAPI ResumePort() = 0;

    virtual void WINAPI SubmitPendingCallbacks() = 0;
};

struct ITaskQueuePortContext : IUnknown
{
    virtual XTaskQueuePort WINAPI GetType() = 0;
    virtual TaskQueuePortStatus WINAPI GetStatus() = 0;
    virtual ITaskQueue* WINAPI GetQueue() = 0;
    virtual ITaskQueuePort* WINAPI GetPort() = 0;
    
    virtual bool WINAPI TrySetStatus(
        TaskQueuePortStatus expectedStatus,
        TaskQueuePortStatus status) = 0;
    
    virtual void WINAPI SetStatus(
        TaskQueuePortStatus status) = 0;

    virtual void WINAPI ItemQueued() = 0;

    virtual bool WINAPI AddSuspend() = 0;
    virtual bool WINAPI RemoveSuspend() = 0;
};

struct ITaskQueue : IUnknown
{
    virtual XTaskQueueHandle WINAPI GetHandle() = 0;
    virtual XTaskQueueTestHooks* WINAPI GetTestHooks() = 0;
    virtual void WINAPI SetTestHooks(XTaskQueueTestHooks* testHooks) = 0;
    
    virtual HRESULT WINAPI GetPortContext(
        XTaskQueuePort port,
        ITaskQueuePortContext** portContext) = 0;
    
    virtual HRESULT WINAPI RegisterWaitHandle(
        XTaskQueuePort port,
        HANDLE waitHandle,
        void* callbackContext,
        XTaskQueueCallback* callback,
        XTaskQueueRegistrationToken* token) = 0;

    virtual void WINAPI UnregisterWaitHandle(
        XTaskQueueRegistrationToken token) = 0;

    virtual HRESULT WINAPI RegisterSubmitCallback(
        void* context,
        XTaskQueueMonitorCallback* callback,
        XTaskQueueRegistrationToken* token) = 0;

    virtual void WINAPI UnregisterSubmitCallback(
        XTaskQueueRegistrationToken token) = 0;

    virtual HRESULT WINAPI Terminate(
        bool wait, 
        void* callbackContext, 
        XTaskQueueTerminatedCallback* callback) = 0;         
};

class SubmitCallback
{
public:

    SubmitCallback(XTaskQueueHandle queue);

    HRESULT Register(void* context, XTaskQueueMonitorCallback* callback, XTaskQueueRegistrationToken* token);
    void Unregister(XTaskQueueRegistrationToken token);
    void Invoke(XTaskQueuePort port);

private:

    struct CallbackRegistration
    {
        uint64_t Token;
        void* Context;
        XTaskQueueMonitorCallback* Callback;
    };

    std::atomic<uint64_t> m_nextToken{ 0 };
    std::mutex m_lock;
    CallbackRegistration m_buffer1[SUBMIT_CALLBACK_MAX];
    CallbackRegistration m_buffer2[SUBMIT_CALLBACK_MAX];
    CallbackRegistration* m_buffers[2]= { m_buffer1, m_buffer2 };
    std::atomic<uint32_t> m_indexAndRef { 0 };
    XTaskQueueHandle m_queue;
};

class QueueWaitRegistry
{
public:

    HRESULT Register(
        XTaskQueuePort port,
        const XTaskQueueRegistrationToken& portToken,
        XTaskQueueRegistrationToken* token);

    std::pair<XTaskQueuePort, XTaskQueueRegistrationToken> Unregister(
        const XTaskQueueRegistrationToken& token);

private:

    struct WaitRegistration
    {
        uint64_t Token;
        uint64_t PortToken;
        XTaskQueuePort Port;
    };

    std::atomic<uint64_t> m_nextToken{ 0 };
    StaticArray<WaitRegistration, QUEUE_WAIT_MAX> m_callbacks;
    std::mutex m_lock;
};


class TaskQueuePortImpl : public ITaskQueuePort
{
public:

    TaskQueuePortImpl();
    virtual ~TaskQueuePortImpl();

    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) override;
    ULONG WINAPI AddRef() override;
    ULONG WINAPI Release() override;

    HRESULT Initialize(
        XTaskQueueDispatchMode mode);

    XTaskQueuePortHandle WINAPI GetHandle() { return &m_header; }

    HRESULT WINAPI QueueItem(
        ITaskQueuePortContext* portContext,
        uint32_t waitMs,
        void* callbackContext,
        XTaskQueueCallback* callback);

    HRESULT WINAPI RegisterWaitHandle(
        ITaskQueuePortContext* portContext,
        HANDLE waitHandle,
        void* callbackContext,
        XTaskQueueCallback* callback,
        XTaskQueueRegistrationToken* token);

    void WINAPI UnregisterWaitHandle(
        XTaskQueueRegistrationToken token);

    HRESULT WINAPI PrepareTerminate(
        ITaskQueuePortContext* portContext,
        void* callbackContext,
        XTaskQueueTerminatedCallback* callback,
        void** token);

    void WINAPI CancelTermination(
        void* token);

    void WINAPI Terminate(
        void* token);

    virtual HRESULT WINAPI Attach(
        ITaskQueuePortContext* portContext);

    void WINAPI Detach(
        ITaskQueuePortContext* portContext);

    bool WINAPI Dispatch(
        ITaskQueuePortContext* portContext,
        uint32_t timeoutInMs);

    bool WINAPI IsEmpty();

    void WINAPI WaitForUnwind();

    HRESULT WINAPI SuspendTermination(
        ITaskQueuePortContext* portContext);

    void WINAPI ResumeTermination(
        ITaskQueuePortContext* portContext);

    void WINAPI SuspendPort();
    void WINAPI ResumePort();

    void WINAPI SubmitPendingCallbacks();

private:

    struct WaitRegistration;

    struct QueueEntry
    {
        ITaskQueuePortContext* portContext;
        void* callbackContext;
        XTaskQueueCallback* callback;
        WaitRegistration* waitRegistration;
        uint64_t enqueueTime;
        uint64_t id;
    };

    struct TerminationEntry
    {
        ITaskQueuePortContext* portContext;
        void* callbackContext;
        XTaskQueueTerminatedCallback* callback;
        std::uint64_t node;
    };

    struct WaitRegistration
    {
        uint64_t token;
        HANDLE waitHandle;
        PTP_WAIT threadpoolWait;
        TaskQueuePortImpl* port;
        QueueEntry queueEntry;
        std::atomic_flag appended;
        std::atomic<uint32_t> refs;
        bool deleted;
    };

    XTaskQueuePortObject m_header = { };
    XTaskQueueDispatchMode m_dispatchMode = XTaskQueueDispatchMode::Manual;
    AtomicVector<ITaskQueuePortContext*> m_attachedContexts;
    std::atomic<uint32_t> m_processingSerializedTbCallback{ 0 };
    std::atomic<uint32_t> m_processingCallback{ 0 };
    std::condition_variable m_processingCallbackCv;
    std::mutex m_lock;
    std::unique_ptr<LocklessQueue<QueueEntry>> m_queueList;
    std::unique_ptr<LocklessQueue<QueueEntry>> m_pendingList;
    std::unique_ptr<LocklessQueue<TerminationEntry*>> m_terminationList;
    std::unique_ptr<LocklessQueue<TerminationEntry*>> m_pendingTerminationList;
    std::mutex m_terminationLock;
    OS::WaitTimer m_timer;
    OS::ThreadPool m_threadPool;
    std::atomic<uint64_t> m_timerDue = { UINT64_MAX };
    std::atomic<uint64_t> m_nextId = { 0 };
    std::atomic<bool> m_suspended = { false };
    std::atomic<ULONG> m_refs{ 0 };
    std::atomic_flag m_deleting;

    StaticArray<WaitRegistration*, PORT_WAIT_MAX> m_waits;
    StaticArray<HANDLE, PORT_EVENT_MAX> m_events;
    uint64_t m_nextWaitToken = 0;

    HRESULT VerifyNotTerminated(ITaskQueuePortContext* portContext);

    bool IsCallCanceled(const QueueEntry& entry);

    // Appends the given entry to the active queue.  The entry should already
    // be add-refd.
    bool AppendEntry(
        const QueueEntry& entry,
        uint64_t node = 0);

    void CancelPendingEntries(
        ITaskQueuePortContext* portContext,
        bool appendToQueue);

    bool DrainOneItem();

    bool DrainOneItem(
        OS::ThreadPoolActionStatus& status);

    bool Wait(
        ITaskQueuePortContext* portContext,
        uint32_t timeout);

    static void EraseQueue(
        LocklessQueue<QueueEntry>* queue);

    void PromoteReadyPendingCallbacks(
        uint64_t dueTime,
        uint64_t now);

    void SignalTerminations();
    void ScheduleTermination(TerminationEntry* term);
    bool TerminationListEmpty();

    void SignalQueue();
    void NotifyItemQueued();

    void ProcessThreadPoolCallback(OS::ThreadPoolActionStatus& status);

    HRESULT InitializeWaitRegistration(
        WaitRegistration* waitReg);

    bool AppendWaitRegistrationEntry(
        WaitRegistration* waitReg);

    bool ReleaseWaitRegistration(
        WaitRegistration* waitReg);

    void ProcessWaitCallback(
        WaitRegistration* waitReg);

    static void CALLBACK WaitCallback(
        PTP_CALLBACK_INSTANCE instance,
        void* context,
        PTP_WAIT wait,
        TP_WAIT_RESULT waitResult);
};
//4529CADF-1BA0-4038-A854-2C0CA26396CB
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(TaskQueuePortImpl, 0x4529CADF, 0x1BA0, 0x4038, 0xA8,0x54, 0x2C,0x0C,0xA2,0x63,0x96,0xCB)
#endif

class TaskQueuePortContextImpl : public ITaskQueuePortContext
{
public:

    TaskQueuePortContextImpl(
        ITaskQueue* queue,
        XTaskQueuePort type,
        SubmitCallback* submitCallback);

    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) override;
    ULONG WINAPI AddRef() override;
    ULONG WINAPI Release() override;

    XTaskQueuePort WINAPI GetType() override;
    TaskQueuePortStatus WINAPI GetStatus() override;
    ITaskQueue* WINAPI GetQueue() override;
    ITaskQueuePort* WINAPI GetPort() override;

    bool WINAPI TrySetStatus(
        TaskQueuePortStatus expectedStatus,
        TaskQueuePortStatus status) override;

    void WINAPI SetStatus(
        TaskQueuePortStatus status) override;

    void WINAPI ItemQueued() override;

    bool WINAPI AddSuspend() override;
    bool WINAPI RemoveSuspend() override;

    referenced_ptr<ITaskQueuePort> Port;
    referenced_ptr<ITaskQueue> Source;

private:

    ITaskQueue* m_queue = nullptr;
    XTaskQueuePort m_type = XTaskQueuePort::Work;
    SubmitCallback* m_submitCallback = nullptr;
    std::atomic<TaskQueuePortStatus> m_status = { TaskQueuePortStatus::Active };
    std::atomic<uint32_t> m_suspendCount = { 0 };
};
//AFAF0302-AEBB-491A-946F-BFB283496E7A
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(TaskQueuePortContextImpl, 0xAFAF0302, 0xAEBB, 0x491A, 0x94,0x6F, 0xBF,0xB2,0x83,0x49,0x6E,0x7A)
#endif

class TaskQueueImpl : public ITaskQueue
#ifdef SUSPEND_API
    , public ISuspendResumeCallback
#endif
{
public:
    TaskQueueImpl();
    virtual ~TaskQueueImpl();

    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) override;
    ULONG WINAPI AddRef() override;
    ULONG WINAPI Release() override;

    HRESULT Initialize(
        XTaskQueueDispatchMode workMode,
        XTaskQueueDispatchMode completionMode);

    HRESULT Initialize(
        XTaskQueuePortHandle workPort,
        XTaskQueuePortHandle completionPort);
    
    XTaskQueueHandle WINAPI GetHandle() override { return &m_header; }
    XTaskQueueTestHooks* WINAPI GetTestHooks() override { return m_testHooks; }
    void WINAPI SetTestHooks(XTaskQueueTestHooks* testHooks) override { m_testHooks = testHooks; }

    HRESULT WINAPI GetPortContext(
        XTaskQueuePort port,
        ITaskQueuePortContext** portContext) override;

    HRESULT WINAPI RegisterWaitHandle(
        XTaskQueuePort port,
        HANDLE waitHandle,
        void* callbackContext,
        XTaskQueueCallback* callback,
        XTaskQueueRegistrationToken* token) override;

    void WINAPI UnregisterWaitHandle(
        XTaskQueueRegistrationToken token) override;

    HRESULT WINAPI RegisterSubmitCallback(
        void* context,
        XTaskQueueMonitorCallback* callback,
        XTaskQueueRegistrationToken* token) override;

    void WINAPI UnregisterSubmitCallback(
        XTaskQueueRegistrationToken token) override;

    HRESULT WINAPI Terminate(
        bool wait, 
        void* callbackContext, 
        XTaskQueueTerminatedCallback* callback) override;

protected:
    void RundownObject();

private:

#ifdef SUSPEND_API
    void OnSuspendResume(bool isSuspended) override;
#endif

    static void CALLBACK OnTerminationCallback(void* context);

private:

    enum class TerminationLevel
    {
        None,
        Work,
        Completion
    };

    struct TerminationEntry
    {
        TaskQueueImpl* owner;
        TerminationLevel level;
        void* completionPortToken;
        void* context;
        XTaskQueueTerminatedCallback* callback;
    };

    struct TerminationData
    {
        bool terminated;
        std::mutex lock;
        std::condition_variable cv;
    };

    XTaskQueueObject m_header = { };
    SubmitCallback m_callbackSubmitted;
    QueueWaitRegistry m_waitRegistry;
    TerminationData m_termination;
    TaskQueuePortContextImpl m_work;
    TaskQueuePortContextImpl m_completion;
    XTaskQueueTestHooks* m_testHooks = nullptr;
    std::atomic<ULONG> m_refs{ 0 };
    std::atomic_flag m_deleting;

#ifdef SUSPEND_API
    SuspendResumeHandler m_suspendHandler;
#endif
};
//D63140D8-9A27-4951-B446-FB8639490A9B
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(TaskQueueImpl, 0xD63140D8, 0x9A27, 0x4951, 0xB4,0x46, 0xFB,0x86,0x39,0x49,0x0A,0x9B)
#endif

// Signatures can be anything because these are not exposed to the client.
static uint32_t const TASK_QUEUE_SIGNATURE = 0x41515545;
static uint32_t const TASK_QUEUE_PORT_SIGNATURE = 0x41515553;

HRESULT XTaskQueueSuspendTermination( XTaskQueueHandle queue );
bool XTaskQueueGetCurrentProcessTaskQueueWithOptions( XTaskQueueDuplicateOptions options, XTaskQueueHandle* queue );
HRESULT XTaskQueueDuplicateHandleWithOptions( XTaskQueueHandle queueHandle, XTaskQueueDuplicateOptions options, XTaskQueueHandle *duplicatedHandle );
void XTaskQueueResumeTermination( XTaskQueueHandle queue );

#endif