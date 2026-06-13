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

#include "XTaskQueue.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

inline ITaskQueue* GetQueue(XTaskQueueHandle handle)
{
    if (handle->m_signature != TASK_QUEUE_SIGNATURE)
    {
        assert("Invalid XTaskQueueHandle");
        return nullptr;
    }

    ITaskQueue* queue = handle->m_queue;

    if (handle != queue->GetHandle())
    {
        assert(handle);
    }

    assert(queue->GetHandle()->m_signature == TASK_QUEUE_SIGNATURE);
    return queue;
}

namespace ProcessGlobals
{
    const XTaskQueueHandle g_invalidQueueHandle = (XTaskQueueHandle)(-1);
    std::atomic<XTaskQueueHandle> g_defaultProcessQueue = { g_invalidQueueHandle };
    std::atomic<XTaskQueueHandle> g_processQueue = { g_invalidQueueHandle };

#ifdef SUSPEND_API
    SuspendState g_suspendState;
#endif
}

/**
 * QueueWaitRegistry -> Mostly just a helper for XTaskQueuePort registration.
 */

HRESULT QueueWaitRegistry::Register(
    XTaskQueuePort port,
    const XTaskQueueRegistrationToken& portToken,
    XTaskQueueRegistrationToken* token
) {
    RETURN_HR_IF(E_OUTOFMEMORY, m_callbacks.capacity() == 0);

    std::lock_guard<std::mutex> lock(m_lock);

    WaitRegistration reg = { };
    reg.Port = port;
    reg.Token = ++m_nextToken;
    reg.PortToken = portToken.token;
    token->token = reg.Token;
    m_callbacks.append(reg);

    return S_OK;
}

std::pair<XTaskQueuePort, XTaskQueueRegistrationToken> QueueWaitRegistry::Unregister(
    const XTaskQueueRegistrationToken& token
) {
    XTaskQueuePort port = XTaskQueuePort::Work;
    XTaskQueueRegistrationToken portToken;
    portToken.token = 0;

    std::lock_guard<std::mutex> lock(m_lock);

    for(uint32_t idx = 0; idx < m_callbacks.count(); idx++)
    {
        if (m_callbacks[idx].Token == token.token)
        {
            port = m_callbacks[idx].Port;
            portToken.token = m_callbacks[idx].PortToken;
            m_callbacks.removeAt(idx);
            break;
        }
    }

    return std::pair<XTaskQueuePort, XTaskQueueRegistrationToken>(port, portToken);
}

/**
 * TaskQueuePortImpl -> Implements a TaskQueuePort wrapper for XTaskQueuePortObject
 */

TaskQueuePortImpl::TaskQueuePortImpl()
{
    m_header.m_signature = TASK_QUEUE_PORT_SIGNATURE;
    m_header.m_runtimeIteration = 0;
    m_header.m_port = this;
    m_header.m_queue = nullptr;
}

TaskQueuePortImpl::~TaskQueuePortImpl()
{
    m_timer.Terminate();

    EraseQueue(m_queueList.get());
    EraseQueue(m_pendingList.get());

    StaticArray<WaitRegistration*, PORT_WAIT_MAX> waits;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        waits = m_waits;
        m_waits.clear();
    }

    if (waits.count() != 0)
    {
        for (uint32_t idx = 0; idx < waits.count(); idx++)
        {
            if (waits[idx]->threadpoolWait != nullptr)
            {
                SetThreadpoolWait(waits[idx]->threadpoolWait, nullptr, nullptr);
                WaitForThreadpoolWaitCallbacks(waits[idx]->threadpoolWait, TRUE);
                CloseThreadpoolWait(waits[idx]->threadpoolWait);
            }

            delete waits[idx];
        }
    }

    m_threadPool.Terminate();

    if (m_events.count() != 0)
    {
        CloseHandle(m_events[0]);
    }
    
    m_pendingList.reset();
    m_queueList.reset();
}

HRESULT WINAPI
TaskQueuePortImpl::QueryInterface( 
    REFIID iid, 
    void **out 
) {
    TRACE( "TaskQueuePortImpl iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( TaskQueuePortImpl ) )
    {
        AddRef();
        *out = static_cast<TaskQueuePortImpl *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI
TaskQueuePortImpl::AddRef()
{
    ULONG curr = static_cast<ULONG>(++m_refs);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

ULONG WINAPI
TaskQueuePortImpl::Release()
{
    ULONG curr = static_cast<ULONG>(--m_refs);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr && m_deleting.test_and_set() == false )
    {
        delete this;
    }

    return curr;
}


HRESULT TaskQueuePortImpl::Initialize(
   XTaskQueueDispatchMode mode
) {
    m_dispatchMode = mode;

    m_queueList.reset(new (std::nothrow) LocklessQueue<QueueEntry>);
    RETURN_IF_NULL_ALLOC(m_queueList);

    m_pendingList.reset(new (std::nothrow) LocklessQueue<QueueEntry>(*m_queueList.get()));
    RETURN_IF_NULL_ALLOC(m_pendingList);

    m_terminationList.reset(new (std::nothrow) LocklessQueue<TerminationEntry*>(0));
    RETURN_IF_NULL_ALLOC(m_terminationList);

    m_pendingTerminationList.reset(new (std::nothrow) LocklessQueue<TerminationEntry*>(*m_terminationList.get()));
    RETURN_IF_NULL_ALLOC(m_pendingTerminationList);

    RETURN_IF_FAILED(m_timer.Initialize(this, [](void* context)
    {
        TaskQueuePortImpl* pthis = static_cast<TaskQueuePortImpl*>(context);
        pthis->SubmitPendingCallbacks();
    }));

    HANDLE evt = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    RETURN_LAST_ERROR_IF_NULL(evt);
    m_events.append(evt);

    switch (mode)
    {
    case XTaskQueueDispatchMode::Manual:
        // nothing
        break;

    case XTaskQueueDispatchMode::ThreadPool:
    case XTaskQueueDispatchMode::SerializedThreadPool:
        RETURN_IF_FAILED(m_threadPool.Initialize(this, [](void* context, OS::ThreadPoolActionStatus& status)
        {
            TaskQueuePortImpl* pthis = static_cast<TaskQueuePortImpl*>(context);
            pthis->ProcessThreadPoolCallback(status);
        }));
        break;
          
    case XTaskQueueDispatchMode::Immediate:
        // nothing
        break;
    }

    return S_OK;
}

HRESULT WINAPI TaskQueuePortImpl::QueueItem(
    ITaskQueuePortContext* portContext,
    uint32_t waitMs,
    void* callbackContext,
    XTaskQueueCallback* callback
) {
    RETURN_IF_FAILED(VerifyNotTerminated(portContext));

    QueueEntry entry;
    referenced_ptr<ITaskQueuePortContext> portContextHolder(portContext);

    entry.portContext = portContext;
    entry.callback = callback;
    entry.callbackContext = callbackContext;
    entry.waitRegistration = nullptr;
    entry.id = m_nextId++;

    if (waitMs == 0)
    {
        entry.enqueueTime = 0;
        RETURN_HR_IF(E_OUTOFMEMORY, !AppendEntry(entry));
    }
    else
    {
        entry.enqueueTime = m_timer.GetDueTime(waitMs);
        RETURN_HR_IF(E_OUTOFMEMORY, !m_pendingList->push_back(entry));

        while (true)
        {
            uint64_t due = m_timerDue;
            if (entry.enqueueTime < due)
            {
                if (m_timerDue.compare_exchange_weak(due, entry.enqueueTime))
                {
                    m_timer.Start(entry.enqueueTime);
                    break;
                }
            }
            else if (m_timerDue.compare_exchange_weak(due, due))
            {
                break;
            }
        }
    }

    portContextHolder.release();

    if (portContext->GetStatus() != TaskQueuePortStatus::Active)
    {
        CancelPendingEntries(portContext, true);
    }

    return S_OK;
}

HRESULT WINAPI TaskQueuePortImpl::RegisterWaitHandle(
    ITaskQueuePortContext* portContext,
    HANDLE waitHandle,
    void* callbackContext,
    XTaskQueueCallback* callback,
    XTaskQueueRegistrationToken* token)
{
    RETURN_HR_IF(E_INVALIDARG, callback == nullptr || waitHandle == nullptr || token == nullptr);
    RETURN_HR_IF(E_POINTER, token == nullptr);
    RETURN_IF_FAILED(VerifyNotTerminated(portContext));

    std::unique_ptr<WaitRegistration> waitReg(new (std::nothrow) WaitRegistration);
    RETURN_IF_NULL_ALLOC(waitReg);

    waitReg->queueEntry.portContext = portContext;
    waitReg->queueEntry.callback = callback;
    waitReg->queueEntry.callbackContext = callbackContext;
    waitReg->queueEntry.waitRegistration = waitReg.get();
    waitReg->queueEntry.id = m_nextId++;

    waitReg->waitHandle = waitHandle;
    waitReg->token = ++m_nextWaitToken;
    waitReg->port = this;
    waitReg->threadpoolWait = nullptr;
    waitReg->appended.clear();
    waitReg->refs = 1;
    waitReg->deleted = false;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        RETURN_HR_IF(E_OUTOFMEMORY, m_events.capacity() == 0);
        RETURN_HR_IF(E_OUTOFMEMORY, m_waits.capacity() == 0);

        RETURN_IF_FAILED(InitializeWaitRegistration(waitReg.get()));

        m_events.append(waitHandle);
        m_waits.append(waitReg.get());

        token->token = waitReg->token;
        waitReg.release();
    }

    SignalQueue();

    return S_OK;
}

void WINAPI TaskQueuePortImpl::UnregisterWaitHandle(
    XTaskQueueRegistrationToken token
) {
    WaitRegistration* toDelete = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        for(uint32_t idx = 0; idx < m_waits.count(); idx++)
        {
            if (m_waits[idx]->token == token.token)
            {
                toDelete = m_waits[idx];
                toDelete->deleted = true;
                m_waits.removeAt(idx);
                break;
            }
        }

        if (toDelete != nullptr)
        {
            for (uint32_t idx = 1; idx < m_events.count(); idx++)
            {
                if (m_events[idx] == toDelete->waitHandle)
                {
                    m_events.removeAt(idx);
                    break;
                }
            }
        }
    }

    if (toDelete != nullptr)
    {
        SetThreadpoolWait(toDelete->threadpoolWait, nullptr, nullptr);
        WaitForThreadpoolWaitCallbacks(toDelete->threadpoolWait, TRUE);
        CloseThreadpoolWait(toDelete->threadpoolWait);
        ReleaseWaitRegistration(toDelete);
    }

    SignalQueue();
}

HRESULT WINAPI TaskQueuePortImpl::PrepareTerminate(
    ITaskQueuePortContext* portContext,
    void* callbackContext,
    XTaskQueueTerminatedCallback* callback,
    void** token
) {
    RETURN_HR_IF(E_POINTER, token == nullptr);
    RETURN_HR_IF(E_INVALIDARG, callback == nullptr);

    std::unique_ptr<TerminationEntry> term(new (std::nothrow) TerminationEntry);
    RETURN_IF_NULL_ALLOC(term);

    {
        std::lock_guard<std::mutex> lock(m_terminationLock);
        RETURN_HR_IF(E_OUTOFMEMORY, !m_terminationList->reserve_node(term->node));
    }

    term->callbackContext = callbackContext;
    term->callback = callback;
    term->portContext = portContext;

    portContext->TrySetStatus(TaskQueuePortStatus::Active, TaskQueuePortStatus::Canceled);
    *token = term.release();

    return S_OK;
}

void WINAPI TaskQueuePortImpl::CancelTermination(
    void* token
) {
    TerminationEntry* term = static_cast<TerminationEntry*>(token);
    term->portContext->TrySetStatus(TaskQueuePortStatus::Canceled, TaskQueuePortStatus::Active);

    if (term->node != 0)
    {
        std::lock_guard<std::mutex> lock(m_terminationLock);
        m_terminationList->free_node(term->node);
    }

    delete term;
}

void WINAPI TaskQueuePortImpl::Terminate(
    void* token
) {
    TerminationEntry* term = static_cast<TerminationEntry*>(token);
    referenced_ptr<ITaskQueuePortContext> cxt(term->portContext);

    cxt->SetStatus(TaskQueuePortStatus::Terminating);

    CancelPendingEntries(cxt.get(), true);

    if (cxt->AddSuspend())
    {
        ScheduleTermination(term);
    }
    else
    {
        std::lock_guard<std::mutex> lock(m_terminationLock);
        m_pendingTerminationList->push_back(term, term->node);
        term->node = 0;
    }

    ResumeTermination(cxt.get());
}

HRESULT WINAPI TaskQueuePortImpl::Attach(
    ITaskQueuePortContext* portContext
) {
    return m_attachedContexts.Add(portContext);
}

void WINAPI TaskQueuePortImpl::Detach(
    ITaskQueuePortContext* portContext
) {
    CancelPendingEntries(portContext, false);
    m_attachedContexts.Remove([portContext](ITaskQueuePortContext* compare)
    {
        return portContext == compare;
    });
}

bool WINAPI TaskQueuePortImpl::Dispatch(
    ITaskQueuePortContext* portContext,
    uint32_t timeoutInMs
) {
    bool found = false;

    while (!found)
    {
        found = DrainOneItem();

        if (!found && !Wait(portContext, timeoutInMs))
        {
            break;
        }
    }

    return found;
}

bool TaskQueuePortImpl::DrainOneItem()
{
    struct DummyActionStatus : OS::ThreadPoolActionStatus
    {
        void Complete() override {}
        void MayRunLong() override {}
    } dummy;

    return DrainOneItem(dummy);
}

bool TaskQueuePortImpl::DrainOneItem(
    OS::ThreadPoolActionStatus& status
) {
    if (m_suspended && m_dispatchMode != XTaskQueueDispatchMode::Immediate)
    {
        return false;
    }

    m_processingCallback++;
    QueueEntry entry;
    bool popped = false;

    if (m_queueList->pop_front(entry))
    {
        popped = true;

        if (entry.portContext->GetType() == XTaskQueuePort::Work)
        {
            status.MayRunLong();
        }

        entry.callback(entry.callbackContext, IsCallCanceled(entry));
        m_processingCallback--;
        m_processingCallbackCv.notify_all();

        if (entry.waitRegistration != nullptr)
        {
            std::lock_guard<std::mutex> lock(m_lock);            
            if (!ReleaseWaitRegistration(entry.waitRegistration))
            {
                InitializeWaitRegistration(entry.waitRegistration);
            }            
        }

        entry.portContext->Release();
    }
    else
    {
        m_processingCallback--;
        m_processingCallbackCv.notify_all();
    }

    if (m_queueList->empty() && m_processingCallback.load() == 0)
    {
        SignalTerminations();
        SignalQueue();
    }

    return popped;
}

bool TaskQueuePortImpl::Wait(
    ITaskQueuePortContext* portContext,
    uint32_t timeout
) {
    while (m_suspended || (m_queueList->empty() && TerminationListEmpty()))
    {
        if (portContext->GetStatus() == TaskQueuePortStatus::Terminated)
        {
            return false;
        }

        StaticArray<HANDLE, PORT_EVENT_MAX> events;
        StaticArray<WaitRegistration*, PORT_WAIT_MAX> waits;

        {
            std::lock_guard<std::mutex> lock(m_lock);
            events = m_events;
            waits = m_waits;

            for (uint32_t idx = 0; idx < waits.count(); idx++)
            {
                waits[idx]->refs++;
            }
        }

        DWORD waitResult = WaitForMultipleObjects(events.count(), events.data(), FALSE, timeout);

        if (waitResult > WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + events.count())
        {
            for (uint32_t idx = 0; idx < waits.count(); idx++)
            {
                if (waits[idx]->waitHandle == events[waitResult - WAIT_OBJECT_0])
                {
                    if (!AppendWaitRegistrationEntry(waits[idx]))
                    {
                        LOG_IF_FAILED(InitializeWaitRegistration(waits[idx]));
                    }
                    break;
                }
            }

            for (uint32_t idx = 0; idx < waits.count(); idx++)
            {
                ReleaseWaitRegistration(waits[idx]);
            }
        }
        else if (waitResult == WAIT_TIMEOUT)
        {
            return false;
        }
        else if (waitResult == WAIT_FAILED)
        {
            FAIL_FAST_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    return true;
}

bool WINAPI TaskQueuePortImpl::IsEmpty()
{
    bool empty =
        (m_queueList->empty()) &&
        (m_pendingList->empty()) &&
        (m_processingCallback == 0);

    return empty;
}

void WINAPI TaskQueuePortImpl::WaitForUnwind()
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);

    while(m_processingCallback.load() != 0)
    {
        std::chrono::milliseconds ms{10};
        auto when = std::chrono::steady_clock::time_point(ms);
        m_processingCallbackCv.wait_until(lock, when);
    }
}

HRESULT WINAPI TaskQueuePortImpl::SuspendTermination(
    ITaskQueuePortContext* portContext
) {
    portContext->AddSuspend();
    HRESULT hr = VerifyNotTerminated(portContext);
    
    if (FAILED(hr))
    {
        ResumeTermination(portContext);
        RETURN_HR(hr);
    }

    return S_OK;
}

void WINAPI TaskQueuePortImpl::ResumeTermination(
    ITaskQueuePortContext* portContext
) {
    if (portContext->RemoveSuspend())
    {
        LocklessQueue<TerminationEntry*> entries_to_schedule(*m_pendingTerminationList.get());
        
        {
            std::lock_guard<std::mutex> lock(m_terminationLock);
            m_pendingTerminationList->remove_if([&](auto& entry, auto address)
            {
                if (entry->portContext == portContext)
                {
                    entries_to_schedule.push_back(entry, address);
                    return true;
                }

                return false;
            });
        }

        TerminationEntry* entry;
        uint64_t address;
        while (entries_to_schedule.pop_front(entry, address))
        {
            entry->node = address;
            ScheduleTermination(entry);
        }
    }
}

void WINAPI TaskQueuePortImpl::SuspendPort()
{
    m_suspended = true;
}

void WINAPI TaskQueuePortImpl::ResumePort()
{
    uint32_t notifyCount = 0;
    uint64_t address;

    QueueEntry queueEntry;
    LocklessQueue<QueueEntry> retainEntries(*(m_pendingList.get()));

    while (m_queueList->pop_front(queueEntry, address))
    {
        notifyCount++;
        retainEntries.push_back(std::move(queueEntry), address);
    }

    while (retainEntries.pop_front(queueEntry, address))
    {
        m_queueList->push_back(std::move(queueEntry), address);
    }

    {
        std::lock_guard<std::mutex> lock(m_terminationLock);
        TerminationEntry* terminationEntry;
        LocklessQueue<TerminationEntry*> retainTerminations(*(m_terminationList.get()));

        while (m_terminationList->pop_front(terminationEntry, address))
        {
            notifyCount++;
            retainTerminations.push_back(std::move(terminationEntry), address);
        }

        while (retainTerminations.pop_front(terminationEntry, address))
        {
            m_terminationList->push_back(std::move(terminationEntry), address);
        }
    }

    m_suspended = false;

    while (notifyCount)
    {
        SignalQueue();
        NotifyItemQueued();
        notifyCount--;
    }
}

HRESULT TaskQueuePortImpl::VerifyNotTerminated(
    ITaskQueuePortContext* portContext
) {
    RETURN_HR_IF(E_ABORT, portContext->GetStatus() > TaskQueuePortStatus::Canceled);
    return S_OK;
}

bool TaskQueuePortImpl::IsCallCanceled(_In_ const QueueEntry& entry)
{
    return entry.portContext->GetStatus() != TaskQueuePortStatus::Active;
}

bool TaskQueuePortImpl::AppendEntry(
    const QueueEntry& entry,
    uint64_t node
) {
    if (node != 0)
    {
        m_queueList->push_back(entry, node);
    }
    else if (!m_queueList->push_back(entry))
    {
        return false;
    }

    SignalQueue();
    NotifyItemQueued();

    return true;
}

void TaskQueuePortImpl::CancelPendingEntries(
    ITaskQueuePortContext* portContext,
    bool appendToQueue
) {
    LocklessQueue<QueueEntry> entriesToAppend(*m_queueList.get());

    m_pendingList->remove_if([&](auto& entry, auto address)
    {
        if (entry.portContext == portContext)
        {
            if (appendToQueue)
            {
                entriesToAppend.push_back(std::move(entry), address);
            }
            else
            {
                entry.portContext->Release();
                m_pendingList->free_node(address);
            }

            return true;
        }

        return false;
    });

    while (appendToQueue)
    {
        QueueEntry entry = {};
        uint64_t address = 0;
        if (!entriesToAppend.pop_front(entry, address))
        {
            break;
        }

        if (!AppendEntry(entry, address))
        {
            entry.portContext->Release();
            m_queueList->free_node(address);
        }
    }

    auto hooks = portContext->GetQueue()->GetTestHooks();
    if (hooks != nullptr)
    {
        hooks->PendingEntriesRemovedDuringTermination(portContext->GetType());
    }

    StaticArray<WaitRegistration*, PORT_WAIT_MAX> waits;

    {
        std::unique_lock<std::mutex> lock(m_lock);
        waits = m_waits;
        m_waits.clear();
    }

    for (uint32_t index = waits.count(); index > 0; index--)
    {
        uint32_t idx = index - 1;
        if (waits[idx]->queueEntry.portContext == portContext)
        {
            CloseThreadpoolWait(waits[idx]->threadpoolWait);
            waits[idx]->queueEntry.waitRegistration = nullptr;

            if (appendToQueue)
            {
                AppendWaitRegistrationEntry(waits[idx]);
            }
            
            delete waits[idx];
            waits.removeAt(idx);
        }
    }
}

void TaskQueuePortImpl::EraseQueue(
    LocklessQueue<QueueEntry>* queue
) {
    if (queue != nullptr)
    {
        QueueEntry entry;
        
        while(queue->pop_front(entry))
        {
            entry.portContext->Release();
        }
    }
}

void TaskQueuePortImpl::PromoteReadyPendingCallbacks(
    uint64_t dueTime,
    uint64_t now
) {
    for (;;)
    {
        LocklessQueue<QueueEntry> readyEntries(*m_queueList.get());

        QueueEntry nextItem = {};
        bool hasNextItem = false;

        m_pendingList->remove_if([&](auto& entry, auto address)
        {
            if (entry.enqueueTime <= now)
            {
                readyEntries.push_back(std::move(entry), address);

                return true;
            }

            if (!hasNextItem || nextItem.enqueueTime > entry.enqueueTime)
            {
                if (hasNextItem)
                {
                    nextItem.portContext->Release();
                }

                nextItem = entry;
                nextItem.portContext->AddRef();
                hasNextItem = true;
            }

            return false;
        });

        QueueEntry readyEntry = {};
        uint64_t readyEntryNode = 0;
        while (readyEntries.pop_front(readyEntry, readyEntryNode))
        {
            if (!AppendEntry(readyEntry, readyEntryNode))
            {
                readyEntry.portContext->Release();
                m_queueList->free_node(readyEntryNode);
            }
        }

        if (hasNextItem)
        {
            if (nextItem.portContext->GetStatus() == TaskQueuePortStatus::Active)
            {
                while (true)
                {
                    if (m_timerDue.compare_exchange_weak(dueTime, nextItem.enqueueTime))
                    {
                        m_timer.Start(nextItem.enqueueTime);
                        break;
                    }

                    dueTime = m_timerDue.load();

                    if (dueTime <= nextItem.enqueueTime)
                    {
                        break;
                    }
                }
            }
            else
            {
                CancelPendingEntries(nextItem.portContext, true);
            }

            nextItem.portContext->Release();
            return;
        }

        uint64_t noDueTime = UINT64_MAX;

        m_attachedContexts.Visit([&](ITaskQueuePortContext* portContext)
        {
            auto hooks = portContext->GetQueue()->GetTestHooks();
            if (hooks != nullptr)
            {
                hooks->NoNextPendingCallbackFound(
                    portContext->GetType(),
                    dueTime);
            }
        });

        if (m_timerDue.compare_exchange_strong(dueTime, noDueTime))
        {
            m_attachedContexts.Visit([&](ITaskQueuePortContext* portContext)
            {
                auto hooks = portContext->GetQueue()->GetTestHooks();
                if (hooks != nullptr)                {
                    hooks->NextPendingCallbackScheduled(
                        portContext->GetType(),
                        dueTime,
                        noDueTime);
                }
            });

            if (dueTime != noDueTime)
            {
                now = m_timer.GetCurrentTime();
                dueTime = noDueTime;
                continue;
            }
        }

        return;
    }
}

void TaskQueuePortImpl::SubmitPendingCallbacks()
{
    while (true)
    {
        uint64_t dueTime = m_timerDue.load();

        if (dueTime == UINT64_MAX)
        {
            return;
        }

        const uint64_t now = m_timer.GetCurrentTime();
        if (now < dueTime)
        {
            uint64_t expectedDueTime = dueTime;
            if (m_timerDue.compare_exchange_weak(expectedDueTime, dueTime))
            {
                m_timer.Start(dueTime);

                if (m_timerDue.load() == dueTime)
                {
                    return;
                }
            }

            continue;
        }

        PromoteReadyPendingCallbacks(dueTime, now);
        return;
    }
}

void TaskQueuePortImpl::ProcessThreadPoolCallback(
    OS::ThreadPoolActionStatus& status
) {
    if (m_dispatchMode == XTaskQueueDispatchMode::SerializedThreadPool)
    {
        uint32_t wasProcessing = m_processingSerializedTbCallback++;

        if (wasProcessing == 0)
        {
            while (DrainOneItem(status));
        }

        m_processingSerializedTbCallback--;
    }
    else
    {
        DrainOneItem(status);
    }

    status.Complete();

    Release();
}

void TaskQueuePortImpl::SignalQueue()
{
    if (!m_suspended)
    {
        SetEvent(m_events[0]);
    }
}

void TaskQueuePortImpl::NotifyItemQueued()
{
    if (!m_suspended || m_dispatchMode == XTaskQueueDispatchMode::Immediate)
    {
        switch (m_dispatchMode)
        {
        case XTaskQueueDispatchMode::Manual:
            // nothing
            break;

        case XTaskQueueDispatchMode::SerializedThreadPool:
        case XTaskQueueDispatchMode::ThreadPool:
            AddRef();
            m_threadPool.Submit();
            break;

        case XTaskQueueDispatchMode::Immediate:
            break;
        }

        m_attachedContexts.Visit([](ITaskQueuePortContext* portContext)
        {
            portContext->ItemQueued();
        });

        if (m_dispatchMode == XTaskQueueDispatchMode::Immediate)
        {
            DrainOneItem();
        }
    }
}

void TaskQueuePortImpl::SignalTerminations()
{
    LocklessQueue<TerminationEntry*> entries_to_process(*m_terminationList.get());

    {
        std::lock_guard<std::mutex> lock(m_terminationLock);
        m_terminationList->remove_if([&entries_to_process](auto& entry, auto address)
        {
            if (entry->portContext->GetStatus() >= TaskQueuePortStatus::Terminating)
            {
                entry->portContext->SetStatus(TaskQueuePortStatus::Terminated);
                entries_to_process.push_back(entry, address);
                return true;
            }

            return false;
        });
    }

    TerminationEntry* entry;
    uint64_t address;
    while (entries_to_process.pop_front(entry, address))
    {
        entry->portContext->AddRef();
        
        entry->callback(entry->callbackContext);
                
        {
            std::lock_guard<std::mutex> lock(m_terminationLock);
            m_terminationList->free_node(address);
        }

        entry->portContext->Release();
        delete entry;
    }
}

void TaskQueuePortImpl::ScheduleTermination(
    TerminationEntry* term
) {
    {
        std::lock_guard<std::mutex> lock(m_terminationLock);
        m_terminationList->push_back(term, term->node);
        term->node = 0; // now owned by the list
    }

    SignalQueue();
    NotifyItemQueued();
}

bool TaskQueuePortImpl::TerminationListEmpty()
{
    std::lock_guard<std::mutex> lock(m_terminationLock);
    return m_terminationList->empty();
}

void CALLBACK TaskQueuePortImpl::WaitCallback(
    PTP_CALLBACK_INSTANCE instance,
    void* context,
    PTP_WAIT wait,
    TP_WAIT_RESULT waitResult
) {
    UNREFERENCED_PARAMETER(instance);
    UNREFERENCED_PARAMETER(wait);

    if (waitResult == WAIT_OBJECT_0 && context != nullptr)
    {
        WaitRegistration* waitReg = static_cast<WaitRegistration*>(context);
        waitReg->port->ProcessWaitCallback(waitReg);
    }
}

HRESULT TaskQueuePortImpl::InitializeWaitRegistration(
    WaitRegistration* waitReg
) {
    waitReg->appended.clear();

    if (waitReg->threadpoolWait == nullptr)
    {
        waitReg->threadpoolWait = CreateThreadpoolWait(WaitCallback, waitReg, nullptr);
        RETURN_LAST_ERROR_IF_NULL(waitReg->threadpoolWait);
    }

    SetThreadpoolWait(waitReg->threadpoolWait, waitReg->waitHandle, nullptr);

    return S_OK;
}

bool TaskQueuePortImpl::AppendWaitRegistrationEntry(
    WaitRegistration* waitReg
) {
    QueueEntry entry = waitReg->queueEntry;
    bool success = true;

    if (waitReg->appended.test_and_set() == false)
    {
        entry.portContext->AddRef();
        waitReg->refs++;

        success = AppendEntry(entry, 0);
        if (!success)
        {
            entry.portContext->Release();
            waitReg->refs--;
        }
    }

    return success;
}

bool TaskQueuePortImpl::ReleaseWaitRegistration(
    WaitRegistration* waitReg
) {
    bool deleted = waitReg->deleted;

    if (--waitReg->refs == 0)
    {
        delete waitReg;
        return true;
    }

    return deleted;
}

void TaskQueuePortImpl::ProcessWaitCallback(
    WaitRegistration* waitReg
) {
    if (!AppendWaitRegistrationEntry(waitReg))
    {
        LOG_IF_FAILED(InitializeWaitRegistration(waitReg));
    }
}

/**
 *  TaskQueueImpl -> Implements a TaskQueue wrapper for XTaskQueueObject
 */

TaskQueueImpl::TaskQueueImpl() :
    m_callbackSubmitted(&m_header),
    m_work(this, XTaskQueuePort::Work, &m_callbackSubmitted),
    m_completion(this, XTaskQueuePort::Completion, &m_callbackSubmitted)
{
    m_header.m_signature = TASK_QUEUE_SIGNATURE;
    m_header.m_runtimeIteration = 0;
    m_header.m_queue = this;
    m_termination.terminated = false;
}

TaskQueueImpl::~TaskQueueImpl()
{
    // Zero the header so we get an early warning of using
    // a released object.
    m_header = {};
}

HRESULT WINAPI
TaskQueueImpl::QueryInterface( 
    REFIID iid, 
    void **out 
) {
    TRACE( "TaskQueueImpl iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( TaskQueueImpl ) )
    {
        AddRef();
        *out = static_cast<TaskQueueImpl *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI
TaskQueueImpl::AddRef()
{
    ULONG curr = static_cast<ULONG>(++m_refs);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

ULONG WINAPI
TaskQueueImpl::Release()
{
    ULONG curr = static_cast<ULONG>(--m_refs);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr && m_deleting.test_and_set() == false )
    {
        RundownObject();
        delete this;
    }
    return curr;
}

HRESULT TaskQueueImpl::Initialize(
    _In_ XTaskQueueDispatchMode workMode,
    _In_ XTaskQueueDispatchMode completionMode)
{
    referenced_ptr<TaskQueuePortImpl> work(new (std::nothrow) TaskQueuePortImpl);
    RETURN_IF_NULL_ALLOC(work);
    RETURN_IF_FAILED(work->Initialize(workMode));

    referenced_ptr<TaskQueuePortImpl> completion(new (std::nothrow) TaskQueuePortImpl);
    RETURN_IF_NULL_ALLOC(completion);
    RETURN_IF_FAILED(completion->Initialize(completionMode));
    
    work->GetHandle()->m_queue = this;
    completion->GetHandle()->m_queue = this;
    
    RETURN_IF_FAILED(work->QueryInterface(__uuidof( TaskQueuePortImpl ), (void**)&m_work.Port));
    RETURN_IF_FAILED(completion->QueryInterface(__uuidof( TaskQueuePortImpl ), (void**)&m_completion.Port));

    RETURN_IF_FAILED(m_work.Port->Attach(&m_work));
    RETURN_IF_FAILED(m_completion.Port->Attach(&m_completion));

#ifdef SUSPEND_API
    RETURN_IF_FAILED(m_suspendHandler.Initialize(ProcessGlobals::g_suspendState, this));
#endif

    return S_OK;
}

HRESULT TaskQueueImpl::Initialize(
    _In_ XTaskQueuePortHandle workPort,
    _In_ XTaskQueuePortHandle completionPort)
{
    RETURN_HR_IF(E_GAMERUNTIME_INVALID_HANDLE, workPort== nullptr || workPort->m_signature != TASK_QUEUE_PORT_SIGNATURE || workPort->m_runtimeIteration != 0);
    RETURN_HR_IF(E_GAMERUNTIME_INVALID_HANDLE, completionPort == nullptr || completionPort->m_signature != TASK_QUEUE_PORT_SIGNATURE || completionPort->m_runtimeIteration != 0);

    m_work.Port = referenced_ptr<ITaskQueuePort>(workPort->m_port);
    m_completion.Port = referenced_ptr<ITaskQueuePort>(completionPort->m_port);
    m_work.Source = referenced_ptr<ITaskQueue>(workPort->m_queue);
    m_completion.Source = referenced_ptr<ITaskQueue>(completionPort->m_queue);
    
    RETURN_IF_FAILED(m_work.Port->Attach(&m_work));
    RETURN_IF_FAILED(m_completion.Port->Attach(&m_completion));

#ifdef SUSPEND_API
    RETURN_IF_FAILED(m_suspendHandler.Initialize(ProcessGlobals::g_suspendState, this));
#endif

    return S_OK;
}

HRESULT WINAPI TaskQueueImpl::GetPortContext(
    _In_ XTaskQueuePort port,
    _Out_ ITaskQueuePortContext** portContext)
{
    RETURN_HR_IF(E_POINTER, portContext == nullptr);
    
    switch(port)
    {
    case XTaskQueuePort::Work:
        *portContext = &m_work;
        m_work.AddRef();
        break;
        
    case XTaskQueuePort::Completion:
        *portContext = &m_completion;
        m_completion.AddRef();
        break;
        
    default:
        RETURN_HR(E_INVALIDARG);
    }
    
    return S_OK;
}

HRESULT WINAPI TaskQueueImpl::RegisterWaitHandle(
    _In_ XTaskQueuePort port,
    _In_ HANDLE waitHandle,
    _In_opt_ void* callbackContext,
    _In_ XTaskQueueCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token)
{
    RETURN_HR_IF(E_POINTER, callback == nullptr || token == nullptr);

    XTaskQueueRegistrationToken portToken;
    referenced_ptr<ITaskQueuePortContext> portContext;

    RETURN_IF_FAILED(GetPortContext(port, portContext.address_of()));
    RETURN_IF_FAILED(portContext->GetPort()->RegisterWaitHandle(
        portContext.get(),
        waitHandle,
        callbackContext,
        callback,
        &portToken));

    HRESULT hr = m_waitRegistry.Register(port, portToken, token);
    if (FAILED(hr))
    {
        portContext->GetPort()->UnregisterWaitHandle(portToken);
    }
    RETURN_IF_FAILED(hr);

    return S_OK;
}

void WINAPI TaskQueueImpl::UnregisterWaitHandle(
    _In_ XTaskQueueRegistrationToken token)
{
    std::pair<XTaskQueuePort, XTaskQueueRegistrationToken> pair = m_waitRegistry.Unregister(token);
    if (pair.second.token != 0)
    {
        referenced_ptr<ITaskQueuePortContext> portContext;
        if (SUCCEEDED(GetPortContext(pair.first, portContext.address_of())))
        {
            portContext->GetPort()->UnregisterWaitHandle(pair.second);
        }
    }
}

HRESULT WINAPI TaskQueueImpl::RegisterSubmitCallback(
    _In_opt_ void* context,
    _In_ XTaskQueueMonitorCallback* callback,
    _Out_ XTaskQueueRegistrationToken* token)
{
    return m_callbackSubmitted.Register(context, callback, token);
}

void WINAPI TaskQueueImpl::UnregisterSubmitCallback(
    _In_ XTaskQueueRegistrationToken token)
{
    m_callbackSubmitted.Unregister(token);
}

HRESULT WINAPI TaskQueueImpl::Terminate(
    _In_ bool wait, 
    _In_opt_ void* callbackContext, 
    _In_opt_ XTaskQueueTerminatedCallback* callback)
{
    std::unique_ptr<TerminationEntry> entry(new (std::nothrow) TerminationEntry);
    RETURN_IF_NULL_ALLOC(entry);

    entry->owner = this;
    entry->level = TerminationLevel::Work;
    entry->context = callbackContext;
    entry->callback = callback;

    void* workToken;
    
    RETURN_IF_FAILED(m_work.Port->PrepareTerminate(&m_work, entry.get(), OnTerminationCallback, &workToken));

    HRESULT hr = m_completion.Port->PrepareTerminate(&m_completion, entry.get(), OnTerminationCallback, &entry->completionPortToken);
    if (FAILED(hr))
    {
        m_work.Port->CancelTermination(workToken);
        RETURN_HR(hr);
    }

    entry.release();

    AddRef();

    if (wait)
    {
        AddRef();
    }

    m_work.Port->Terminate(workToken);
    
    if (wait)
    {
        std::unique_lock<std::mutex> lock(m_termination.lock);
        while(!m_termination.terminated)
        {
            m_termination.cv.wait(lock);
        }

        m_work.Port->WaitForUnwind();
        m_completion.Port->WaitForUnwind();

        Release();
    }

    return S_OK;
}

void TaskQueueImpl::RundownObject()
{
    m_work.SetStatus(TaskQueuePortStatus::Terminated);
    m_completion.SetStatus(TaskQueuePortStatus::Terminated);

#ifdef SUSPEND_API
    m_suspendHandler.Shutdown();
#endif

    if (m_work.Port != nullptr)
    {
        m_work.Port->Detach(&m_work);
    }

    if (m_completion.Port != nullptr)
    {
        m_completion.Port->Detach(&m_completion);
    }
}

#ifdef SUSPEND_API
void TaskQueueImpl::OnSuspendResume(_In_ bool isSuspended)
{
    if (isSuspended)
    {
        m_completion.GetPort()->SuspendPort();
        m_work.GetPort()->SuspendPort();
    }
    else
    {
        m_work.GetPort()->ResumePort();
        m_completion.GetPort()->ResumePort();
    }
}
#endif

void TaskQueueImpl::OnTerminationCallback(_In_ void* context)
{
    TerminationEntry* entry = static_cast<TerminationEntry*>(context);
    switch(entry->level)
    {
        case TerminationLevel::Work:
            entry->level = TerminationLevel::Completion;
            entry->owner->m_completion.Port->Terminate(entry->completionPortToken);
            break;

        case TerminationLevel::Completion:
            if (entry->callback != nullptr)
            {
                entry->callback(entry->context);
            }

            {
                std::unique_lock<std::mutex> lock(entry->owner->m_termination.lock);
                entry->owner->m_termination.terminated = true;
                entry->owner->m_termination.cv.notify_all();
            }

            entry->owner->Release();
            delete entry;
            break;

        default:
            assert(false);
    }
}

/**
 *  TaskQueuePortContextImpl -> Implements a TaskQueuePort Context wrapper for XTaskQueuePortObject
 *  NOTE: Object ownership is by m_queue. This is a weak class.
 */

TaskQueuePortContextImpl::TaskQueuePortContextImpl(
    ITaskQueue* queue,
    XTaskQueuePort type,
    SubmitCallback* submitCallback) :
    m_queue(queue),
    m_type(type),
    m_submitCallback(submitCallback)
{
}

HRESULT WINAPI
TaskQueuePortContextImpl::QueryInterface( 
    REFIID iid, 
    void **out 
) {
    TRACE( "TaskQueuePortContextImpl iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( TaskQueuePortContextImpl ) )
    {
        AddRef();
        *out = static_cast<TaskQueuePortContextImpl *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI
TaskQueuePortContextImpl::AddRef()
{
    return m_queue->AddRef();
}

ULONG WINAPI
TaskQueuePortContextImpl::Release()
{
    return m_queue->Release();
}


XTaskQueuePort WINAPI TaskQueuePortContextImpl::GetType()
{
    return m_type;
}

TaskQueuePortStatus WINAPI TaskQueuePortContextImpl::GetStatus()
{
    return m_status;
}

ITaskQueue* WINAPI TaskQueuePortContextImpl::GetQueue()
{
    return m_queue;
}

ITaskQueuePort* WINAPI TaskQueuePortContextImpl::GetPort()
{
    return Port.get();
}

bool WINAPI TaskQueuePortContextImpl::TrySetStatus(
    TaskQueuePortStatus expectedStatus,
    TaskQueuePortStatus status
) {
    return m_status.compare_exchange_strong(expectedStatus, status);
}
    
void WINAPI TaskQueuePortContextImpl::SetStatus(
    TaskQueuePortStatus status
) {
    m_status = status;
}

void WINAPI TaskQueuePortContextImpl::ItemQueued()
{
    m_submitCallback->Invoke(m_type);
}

bool WINAPI TaskQueuePortContextImpl::AddSuspend()
{
    return (m_suspendCount.fetch_add(1) == 0);
}

bool WINAPI TaskQueuePortContextImpl::RemoveSuspend()
{
    for(;;)
    {
        uint32_t current = m_suspendCount.load();

        if (current == 0)
        {
            assert(false);
            return true;
        }

        if (m_suspendCount.compare_exchange_weak(current, current -1))
        {
            return current == 1;
        }
    }
}

/**
 *  SubmitCallback -> Handles XTaskQueueMonitorCallback registration.
 */

SubmitCallback::SubmitCallback(
    XTaskQueueHandle queue)
    : m_queue(queue)
{
    memset(m_buffer1, 0, sizeof(m_buffer1));
    memset(m_buffer2, 0, sizeof(m_buffer2));
}

HRESULT SubmitCallback::Register(
    void* context, XTaskQueueMonitorCallback* callback, 
    XTaskQueueRegistrationToken* token
) {
    RETURN_HR_IF(E_POINTER, callback == nullptr || token == nullptr);

    token->token = 0;

    std::lock_guard<std::mutex> lock(m_lock);
    uint32_t bufferReadIdx = (m_indexAndRef & 0x80000000 ? 1 : 0);
    uint32_t bufferWriteIdx = 1 - bufferReadIdx;

    for(uint32_t idx = 0; idx < ARRAYSIZE(m_buffer1); idx++)
    {
        if (token->token == 0 && m_buffers[bufferReadIdx][idx].Callback == nullptr)
        {
            token->token = ++m_nextToken;
            m_buffers[bufferWriteIdx][idx].Token = token->token;
            m_buffers[bufferWriteIdx][idx].Context = context;
            m_buffers[bufferWriteIdx][idx].Callback = callback;
        }
        else
        {
            m_buffers[bufferWriteIdx][idx] = m_buffers[bufferReadIdx][idx];
        }
    }

    RETURN_HR_IF(E_OUTOFMEMORY, token->token == 0);

    // Now spin wait to swap the active buffer.
    uint32_t expected = bufferReadIdx << 31;
    uint32_t desired = bufferWriteIdx << 31;

    for (;;)
    {
        uint32_t expectedSwap = expected;
        if (m_indexAndRef.compare_exchange_weak(expectedSwap, desired))
        {
            break;
        }
    }
    
    return S_OK;
}

void SubmitCallback::Unregister(
    XTaskQueueRegistrationToken token
) {
    std::lock_guard<std::mutex> lock(m_lock);
    uint32_t bufferReadIdx = (m_indexAndRef & 0x80000000 ? 1 : 0);
    uint32_t bufferWriteIdx = 1 - bufferReadIdx;

    for(uint32_t idx = 0; idx < ARRAYSIZE(m_buffer1); idx++)
    {
        if (m_buffers[bufferReadIdx][idx].Token == token.token)
        {
            m_buffers[bufferWriteIdx][idx].Callback = nullptr;
        }
        else
        {
            m_buffers[bufferWriteIdx][idx] = m_buffers[bufferReadIdx][idx];
        }
    }

    // Now spin wait to swap the active buffer.
    uint32_t expected = bufferReadIdx << 31;
    uint32_t desired = bufferWriteIdx << 31;

    for (;;)
    {
        uint32_t expectedSwap = expected;
        if (m_indexAndRef.compare_exchange_weak(expectedSwap, desired))
        {
            break;
        }
    }
}

void SubmitCallback::Invoke(
    XTaskQueuePort port
) {
    std::lock_guard<std::mutex> lock(m_lock);
    uint32_t indexAndRef = ++m_indexAndRef;
    uint32_t bufferIdx = (indexAndRef & 0x80000000 ? 1 : 0);

    for(uint32_t idx = 0; idx < ARRAYSIZE(m_buffer1); idx++)
    {
        if (m_buffers[bufferIdx][idx].Callback != nullptr)
        {
            m_buffers[bufferIdx][idx].Callback(
                m_buffers[bufferIdx][idx].Context,
                m_queue,
                port);
        }
    }

    m_indexAndRef--;
}

/**
 * Module methods
 */

static HRESULT CreateTaskQueueHandle(
    ITaskQueue* impl,
    XTaskQueueHandle* queue
) {
    *queue = nullptr;

    std::unique_ptr<XTaskQueueObject> q(new (std::nothrow) XTaskQueueObject);
    RETURN_IF_NULL_ALLOC(q);

    q->m_signature = TASK_QUEUE_SIGNATURE;
    q->m_runtimeIteration = 0;
    q->m_queue = impl;
    impl->AddRef();

    *queue = q.release();

    TRACE("created new handle %p\n", *queue);

    return S_OK;
}

/**
 * xgameruntime XTaskQueue methods
 */

HRESULT WINAPI XTaskQueueCreate(
    XTaskQueueDispatchMode workDispatchMode,
    XTaskQueueDispatchMode completionDispatchMode,
    XTaskQueueHandle* queue
) {
    *queue = nullptr;

    referenced_ptr<TaskQueueImpl> aq(new (std::nothrow) TaskQueueImpl);
    RETURN_IF_NULL_ALLOC(aq);

    RETURN_IF_FAILED(aq->Initialize(
        workDispatchMode, 
        completionDispatchMode));

    RETURN_IF_FAILED(CreateTaskQueueHandle(aq.get(), queue));

    TRACE("created new handle %p\n", *queue);

    return S_OK;
}

HRESULT WINAPI XTaskQueueGetPort(
    XTaskQueueHandle queue,
    XTaskQueuePort port,
    XTaskQueuePortHandle* portHandle
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_GAMERUNTIME_INVALID_HANDLE, aq == nullptr);

    referenced_ptr<ITaskQueuePortContext> portContext;
    RETURN_IF_FAILED(aq->GetPortContext(port, portContext.address_of()));
    
    *portHandle = portContext->GetPort()->GetHandle();
    
    return S_OK;
}

HRESULT WINAPI XTaskQueueCreateComposite(
    XTaskQueuePortHandle workPort,
    XTaskQueuePortHandle completionPort,
    XTaskQueueHandle* queue
) {
    *queue = nullptr;

    referenced_ptr<TaskQueueImpl> aq(new (std::nothrow) TaskQueueImpl);
    RETURN_IF_NULL_ALLOC(aq);
    RETURN_IF_FAILED(aq->Initialize(workPort, completionPort));

    RETURN_IF_FAILED(CreateTaskQueueHandle(aq.get(), queue));

    TRACE("created new handle %p\n", *queue);

    return S_OK;
}

BOOLEAN WINAPI XTaskQueueDispatch(
    XTaskQueueHandle queue,
    XTaskQueuePort port,
    UINT32 timeoutInMs
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq == nullptr)
    {
        return false;
    }

    referenced_ptr<ITaskQueuePortContext> portContext;
    if (FAILED(aq->GetPortContext(port, portContext.address_of())))
    {
        return false;
    }

    return portContext->GetPort()->Dispatch(portContext.get(), timeoutInMs);
}

void WINAPI XTaskQueueCloseHandle(
    XTaskQueueHandle queue
) {
    ITaskQueue* aq = GetQueue(queue);

    if (aq != nullptr)
    {
        if (USE_UNIQUE_HANDLES && queue != aq->GetHandle())
        {
            queue->m_signature = 0;
            queue->m_queue = nullptr;
            delete queue;
        }

        aq->Release();
    }
}

HRESULT WINAPI XTaskQueueTerminate(
    XTaskQueueHandle queue, 
    BOOLEAN wait, 
    void* callbackContext, 
    XTaskQueueTerminatedCallback* callback
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    return aq->Terminate(wait, callbackContext, callback);
}

HRESULT WINAPI XTaskQueueSubmitCallback(
    XTaskQueueHandle queue,
    XTaskQueuePort port,
    void* callbackContext,
    XTaskQueueCallback* callback
) {
    return XTaskQueueSubmitDelayedCallback(queue, port, 0, callbackContext, callback);
}

HRESULT WINAPI XTaskQueueSubmitDelayedCallback(
    XTaskQueueHandle queue,
    XTaskQueuePort port,
    uint32_t delayMs,
    void* callbackContext,
    XTaskQueueCallback* callback
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_GAMERUNTIME_INVALID_HANDLE, aq == nullptr);

    referenced_ptr<ITaskQueuePortContext> portContext;
    RETURN_IF_FAILED(aq->GetPortContext(port, portContext.address_of()));

    RETURN_IF_FAILED(portContext->GetPort()->QueueItem(portContext.get(), delayMs, callbackContext, callback));
    return S_OK;
}

HRESULT WINAPI XTaskQueueRegisterWaiter(
    XTaskQueueHandle queue,
    XTaskQueuePort port,
    HANDLE waitHandle,
    void* callbackContext,
    XTaskQueueCallback* callback,
    XTaskQueueRegistrationToken* token
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_GAMERUNTIME_INVALID_HANDLE, aq == nullptr);
    RETURN_IF_FAILED(aq->RegisterWaitHandle(port, waitHandle, callbackContext, callback, token));
    return S_OK;
}


void WINAPI XTaskQueueUnregisterWaiter(
    XTaskQueueHandle queue,
    XTaskQueueRegistrationToken token
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq != nullptr)
    {
        aq->UnregisterWaitHandle(token);
    }
}

HRESULT WINAPI XTaskQueueDuplicateHandleWithOptions(
    XTaskQueueHandle queueHandle,
    XTaskQueueDuplicateOptions options,
    XTaskQueueHandle* duplicatedHandle
) {
    RETURN_HR_IF(E_POINTER, duplicatedHandle == nullptr);

    *duplicatedHandle = nullptr;

    auto queue = GetQueue(queueHandle);
    RETURN_HR_IF(E_GAMERUNTIME_INVALID_HANDLE, queue == nullptr);

    if (USE_UNIQUE_HANDLES && (options != XTaskQueueDuplicateOptions::Reference))
    {
        RETURN_IF_FAILED(CreateTaskQueueHandle(queue, duplicatedHandle));
    }
    else
    {
        queue->AddRef();
        *duplicatedHandle = queue->GetHandle();
    }

    TRACE("created new handle %p\n", *duplicatedHandle);

    return S_OK;
}

HRESULT WINAPI XTaskQueueDuplicateHandle(
    XTaskQueueHandle queueHandle,
    XTaskQueueHandle* duplicatedHandle
) {
    return XTaskQueueDuplicateHandleWithOptions(
        queueHandle,
        XTaskQueueDuplicateOptions::None,
        duplicatedHandle);
}

HRESULT WINAPI XTaskQueueRegisterMonitor(
    XTaskQueueHandle queue,
    void* callbackContext,
    XTaskQueueMonitorCallback* callback,
    XTaskQueueRegistrationToken* token
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_GAMERUNTIME_INVALID_HANDLE, aq == nullptr);
    RETURN_IF_FAILED(aq->RegisterSubmitCallback(callbackContext, callback, token));
    return S_OK;
}

void WINAPI XTaskQueueUnregisterMonitor(
    XTaskQueueHandle queue,
    XTaskQueueRegistrationToken token
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq != nullptr)
    {
        aq->UnregisterSubmitCallback(token);
    }
}


bool WINAPI XTaskQueueGetCurrentProcessTaskQueueWithOptions(
    XTaskQueueDuplicateOptions options,
    XTaskQueueHandle* queue
) {
    *queue = nullptr;

    XTaskQueueHandle processQueue = ProcessGlobals::g_processQueue;
    if (processQueue == ProcessGlobals::g_invalidQueueHandle)
    {
        XTaskQueueHandle defaultProcessQueue = ProcessGlobals::g_defaultProcessQueue;
        if (defaultProcessQueue == ProcessGlobals::g_invalidQueueHandle)
        {
            referenced_ptr<TaskQueueImpl> aq(new (std::nothrow) TaskQueueImpl);
            if (aq != nullptr && SUCCEEDED(aq->Initialize(
                XTaskQueueDispatchMode::ThreadPool,
                XTaskQueueDispatchMode::ThreadPool)))
            {
                XTaskQueueHandle expected = ProcessGlobals::g_invalidQueueHandle;
                if (ProcessGlobals::g_defaultProcessQueue.compare_exchange_strong(
                    expected,
                    aq->GetHandle()))
                {
                    aq.release();
                }
            }

            defaultProcessQueue = ProcessGlobals::g_defaultProcessQueue;
        }

        processQueue = defaultProcessQueue;
    }

    if (processQueue == ProcessGlobals::g_invalidQueueHandle)
    {
        processQueue = nullptr;
    }

    if (processQueue != nullptr)
    {
        XTaskQueueDuplicateHandleWithOptions(processQueue, options, queue);
    }

    TRACE("created new handle %p\n", *queue);

    return (*queue) != nullptr;
}

BOOLEAN WINAPI XTaskQueueGetCurrentProcessTaskQueue(
    XTaskQueueHandle* queue
) {
    return XTaskQueueGetCurrentProcessTaskQueueWithOptions(XTaskQueueDuplicateOptions::None, queue);
}

void WINAPI XTaskQueueSetCurrentProcessTaskQueue(
    XTaskQueueHandle queue
) {
    XTaskQueueHandle newQueue = nullptr;

    if (queue != nullptr)
    {
        FAIL_FAST_IF_FAILED(XTaskQueueDuplicateHandle(queue, &newQueue));
    }

    XTaskQueueHandle previous = ProcessGlobals::g_processQueue.exchange(newQueue);
    if (previous != nullptr && previous != ProcessGlobals::g_invalidQueueHandle)
    {
        XTaskQueueCloseHandle(previous);
    }
}

HRESULT WINAPI XTaskQueueSuspendTermination( 
    XTaskQueueHandle queue
) {
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    RETURN_HR_IF(E_GAMERUNTIME_INVALID_HANDLE, aq == nullptr);

    referenced_ptr<ITaskQueuePortContext> portContext;
    RETURN_IF_FAILED(aq->GetPortContext(XTaskQueuePort::Work, portContext.address_of()));

    RETURN_IF_FAILED(portContext->GetPort()->SuspendTermination(portContext.get()));
    return S_OK;
}

void XTaskQueueResumeTermination( XTaskQueueHandle queue )
{
    referenced_ptr<ITaskQueue> aq(GetQueue(queue));
    if (aq == nullptr)
    {
        return;
    }
    
    referenced_ptr<ITaskQueuePortContext> portContext;
    if (FAILED(aq->GetPortContext(XTaskQueuePort::Work, portContext.address_of())))
    {
        return;
    }
        
    portContext->GetPort()->ResumeTermination(portContext.get());
}
