/*
 * XAsync Implementation
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

#include "../../../private.h"
#include "SpinLock.h"
#include "XTaskQueue.h"

#include <atomic>
#include <condition_variable>

// Signatures can be anything because these are not exposed to the client.
#define ASYNC_BLOCK_SIG         0x41535942 // ASYB
#define ASYNC_BLOCK_RESULT_SIG  0x41535242 // ASRB
#define ASYNC_STATE_SIG         0x41535445 // ASTE

std::atomic<uint32_t> s_AsyncLibGlobalStateCount{ 0 };

bool s_AsyncLibEnablePumpingWait = false;

enum class ProviderCleanupLocation
{
    Destructor,
    AfterDoWork,
    InCancel,
    CleanedUp
};

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

struct AsyncState
{
    uint32_t signature = ASYNC_STATE_SIG;
    std::atomic<uint32_t> refs{ 1 };
    std::atomic<ProviderCleanupLocation> providerCleanup{ ProviderCleanupLocation::Destructor };
    std::atomic<bool> workScheduled{ false };
    bool valid = true;
    XAsyncProvider* provider = nullptr;
    XAsyncProviderData providerData{ };
    XAsyncBlock providerAsyncBlock { };
    XAsyncBlock* userAsyncBlock = nullptr;
    XTaskQueueHandle queue = nullptr;
    std::mutex waitMutex;
    std::condition_variable waitCondition;
    bool waitSatisfied = false;
    HANDLE waitEvent = nullptr;

    const void* identity = nullptr;
    const char* identityName = nullptr;

    void* operator new(size_t size, size_t additional, const std::nothrow_t& tag)
    {
        return ::operator new(size + additional, tag);
    }

    void operator delete(void* ptr)
    {
        ::operator delete(ptr);
    }

    AsyncState() noexcept
    {
        ++s_AsyncLibGlobalStateCount;
    }

    void AddRef() noexcept
    {
        ++refs;
    }

    void Release() noexcept
    {
        if (--refs == 0)
        {
            delete this;
        }
    }

private:

    ~AsyncState() noexcept
    {
        if (provider != nullptr)
        {
            ProviderCleanupLocation loc = providerCleanup.exchange(ProviderCleanupLocation::CleanedUp);
            if (loc != ProviderCleanupLocation::CleanedUp)
            {
                (void)provider(XAsyncOp::Cleanup, &providerData);
            }
        }

        if (queue != nullptr)
        {
            XTaskQueueCloseHandle(queue);
        }

        if (waitEvent != nullptr)
        {
            CloseHandle(waitEvent);
        }

        --s_AsyncLibGlobalStateCount;
    }
};

struct AsyncBlockInternal
{
    AsyncState* state = nullptr;
    HRESULT status = E_PENDING;
    DWORD signature = ASYNC_BLOCK_SIG;
    std::atomic_flag lock = ATOMIC_FLAG_INIT;
};

static_assert(sizeof(AsyncBlockInternal) <= sizeof(XAsyncBlock::internal),
    "Unexpected size for AsyncBlockInternal");
static_assert(std::alignment_of<AsyncBlockInternal>::value == std::alignment_of<void*>::value,
    "Unexpected alignment for AsyncBlockInternal");
static_assert(std::is_trivially_destructible<AsyncBlockInternal>::value,
    "Unexpected nontrivial destructor for AsyncBlockInternal");

class AsyncStateRef
{
public:
    AsyncStateRef() noexcept
        : m_state{ nullptr }
    {}
    explicit AsyncStateRef(AsyncState* state) noexcept
        : m_state{ state }
    {
        if (m_state)
        {
            m_state->AddRef();
        }
    }
    AsyncStateRef(const AsyncStateRef&) = delete;
    AsyncStateRef(AsyncStateRef&& other) noexcept
        : m_state{ other.m_state }
    {
        other.m_state = nullptr;
    }
    AsyncStateRef& operator=(const AsyncStateRef&) = delete;
    AsyncStateRef& operator=(AsyncStateRef&& other) noexcept
    {
        if (&other != this) { std::swap(m_state, other.m_state); }
        return *this;
    }
    ~AsyncStateRef() noexcept
    {
        if (m_state)
        {
            m_state->Release();
        }
    }
    AsyncState* operator->() const noexcept
    {
        return m_state;
    }
    bool operator==(std::nullptr_t) const noexcept
    {
        return m_state == nullptr;
    }
    bool operator!=(std::nullptr_t) const noexcept
    {
        return m_state != nullptr;
    }
    void Attach(AsyncState* state) noexcept
    {
        assert(!m_state);
        m_state = state;
    }
    AsyncState* Detach() noexcept
    {
        AsyncState* p = m_state;
        m_state = nullptr;
        return p;
    }

    AsyncState* Get() const { return m_state; }

private:
    AsyncState* m_state;
};

class AsyncBlockInternalGuard
{
public:
    AsyncBlockInternalGuard(XAsyncBlock* asyncBlock) noexcept :
        m_internal(DoLock(asyncBlock))
    {
        m_locked = m_internal != nullptr;

        if (!m_locked)
        {
            m_internal = reinterpret_cast<AsyncBlockInternal*>(asyncBlock->internal);
        }

        if (m_internal->state != nullptr)
        {
            m_userInternal = reinterpret_cast<AsyncBlockInternal*>(m_internal->state->userAsyncBlock->internal);
        }
        else
        {
            m_userInternal = m_internal;
        }

        if (m_userInternal != m_internal)
        {
            SpinLock::Lock(m_userInternal->lock);
        }
    }

    ~AsyncBlockInternalGuard() noexcept
    {
        if (m_locked)
        {
            m_internal->lock.clear();
            if (m_userInternal != m_internal)
            {
                m_userInternal->lock.clear();
            }
        }
    }

    AsyncStateRef GetState() const noexcept
    {
        AsyncStateRef state{ m_internal->state };

        if (state != nullptr && state->signature != ASYNC_STATE_SIG)
        {
            assert(false);
            return AsyncStateRef{};
        }

        return state;
    }

    AsyncStateRef ExtractState(bool resultsRetrieved = false) const noexcept
    {
        AsyncStateRef state{ m_internal->state };
        m_internal->state = nullptr;
        m_userInternal->state = nullptr;

        if (resultsRetrieved)
        {
            m_internal->signature = ASYNC_BLOCK_RESULT_SIG;
            m_userInternal->signature = ASYNC_BLOCK_RESULT_SIG;
        }
        else
        {
            m_internal->signature = 0;
            m_userInternal->signature = 0;
        }

        if (state != nullptr && state->signature != ASYNC_STATE_SIG)
        {
            assert(false);
            return AsyncStateRef{};
        }

        return state;
    }

    HRESULT GetStatus() const noexcept
    {
        return m_internal->status;
    }

    bool GetResultsRetrieved()
    {
        return m_internal->signature == ASYNC_BLOCK_RESULT_SIG;
    }

    bool TrySetTerminalStatus(HRESULT status) noexcept
    {
        assert(m_locked || m_internal->status != E_PENDING);
        if (m_locked && m_internal->status == E_PENDING)
        {
            assert(m_userInternal->status == E_PENDING);
            m_userInternal->status = status;
            m_internal->status = status;

            return true;
        }
        else
        {
            return false;
        }
    }

private:
    AsyncBlockInternal * m_internal;
    AsyncBlockInternal * m_userInternal;
    bool m_locked = false;

    static AsyncBlockInternal* DoLock(XAsyncBlock* asyncBlock)
    {
        AsyncBlockInternal* lockedResult = reinterpret_cast<AsyncBlockInternal*>(asyncBlock->internal);

        assert(lockedResult);

        if (lockedResult->signature != ASYNC_BLOCK_SIG)
        {
            lockedResult->state = nullptr;
            return nullptr;
        }

        SpinLock::Lock(lockedResult->lock);

        if (lockedResult->state != nullptr && asyncBlock != &lockedResult->state->providerAsyncBlock)
        {
            AsyncStateRef state(lockedResult->state);
            lockedResult->lock.clear();

            AsyncBlockInternal* stateAsyncBlockInternal = reinterpret_cast<AsyncBlockInternal*>(state->providerAsyncBlock.internal);
            SpinLock::Lock(stateAsyncBlockInternal->lock);

            if (stateAsyncBlockInternal->state == nullptr)
            {
                stateAsyncBlockInternal->lock.clear();
                SpinLock::Lock(lockedResult->lock);
            }
            else
            {
                lockedResult = stateAsyncBlockInternal;
            }
        }

        return lockedResult;
    }
};

static void CALLBACK CompletionCallback(void* context, BOOLEAN canceled);
static void CALLBACK WorkerCallback(void* context, BOOLEAN canceled);
static void SignalWait(AsyncStateRef const& state);
static HRESULT SignalCompletion(AsyncStateRef const& state);
static HRESULT AllocStateNoCompletion(XAsyncBlock* asyncBlock, AsyncBlockInternal* internal, size_t contextSize);
static HRESULT AllocState(XAsyncBlock* asyncBlock, size_t contextSize);
static void CleanupState(AsyncStateRef&& state);
static void CleanupProviderForLocation(AsyncStateRef& state, ProviderCleanupLocation location);
static bool TrySetProviderCleanup(AsyncStateRef& state, ProviderCleanupLocation location);
static void RevertProviderCleanup(AsyncStateRef& state, ProviderCleanupLocation expected);

static HRESULT AllocStateNoCompletion(XAsyncBlock* asyncBlock, AsyncBlockInternal* internal, size_t contextSize)
{
    AsyncStateRef state;
    state.Attach(new (contextSize, std::nothrow) AsyncState);
    RETURN_IF_NULL_ALLOC(state);

    if (contextSize != 0)
    {
        state->providerData.context = (state.Get() + 1);
    }

    XTaskQueueHandle queue = asyncBlock->queue;
    if (queue == nullptr)
    {
        RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_NO_TASK_QUEUE), XTaskQueueGetCurrentProcessTaskQueueWithOptions(
            XTaskQueueDuplicateOptions::Reference,
            &state->queue) == false);
    }
    else
    {
        RETURN_IF_FAILED(XTaskQueueDuplicateHandleWithOptions(
            queue,
            XTaskQueueDuplicateOptions::Reference,
            &state->queue));
    }

    state->userAsyncBlock = asyncBlock;
    state->providerData.async = &state->providerAsyncBlock;

    state->waitEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    RETURN_LAST_ERROR_IF(state->waitEvent == nullptr);

    RETURN_IF_FAILED(XTaskQueueSuspendTermination(state->queue));

    internal->state = state.Detach();

    internal->state->providerAsyncBlock = *asyncBlock;
    internal->state->providerAsyncBlock.queue = internal->state->queue;

    return S_OK;
}

static HRESULT AllocState(XAsyncBlock* asyncBlock, size_t contextSize)
{
    auto internal = reinterpret_cast<AsyncBlockInternal*>(asyncBlock->internal);
    if (internal->signature == ASYNC_BLOCK_SIG)
    {
        RETURN_HR(E_INVALIDARG);
    }

    for (auto i = 0u; i < sizeof(asyncBlock->internal); ++i)
    {
        asyncBlock->internal[i] = 0;
    }

    internal = new (asyncBlock->internal) AsyncBlockInternal{};

    HRESULT hr = AllocStateNoCompletion(asyncBlock, internal, contextSize);

    if (FAILED(hr))
    {
        internal->signature = 0;
        internal->status = hr;
    }

    RETURN_IF_FAILED(hr);
    return S_OK;
}

static void CleanupState(AsyncStateRef&& state)
{
    if (state != nullptr)
    {
        // Should only cleanup state after calling ExtractState to clear it.
        assert((reinterpret_cast<AsyncBlockInternal*>(state->providerData.async->internal))->state == nullptr);

        state->valid = false;
        state->Release();
    }
}

static void CleanupProviderForLocation(AsyncStateRef& state, ProviderCleanupLocation location)
{
    if (state->providerCleanup.compare_exchange_strong(location, ProviderCleanupLocation::CleanedUp))
    {
        (void)state->provider(XAsyncOp::Cleanup, &state->providerData);
    }
}

static bool TrySetProviderCleanup(AsyncStateRef& state, ProviderCleanupLocation location)
{
    ProviderCleanupLocation expected = ProviderCleanupLocation::Destructor;
    return state->providerCleanup.compare_exchange_strong(expected, location);
}

static void RevertProviderCleanup(AsyncStateRef& state, ProviderCleanupLocation expected)
{
    state->providerCleanup.compare_exchange_strong(expected, ProviderCleanupLocation::Destructor);
}

static HRESULT SignalCompletion(AsyncStateRef const& state)
{
    HRESULT hr = S_OK;

    if (state->providerData.async->callback != nullptr)
    {
        AsyncStateRef callbackState(state.Get());
        hr = XTaskQueueSubmitCallback(
            state->queue,
            XTaskQueuePort::Completion,
            callbackState.Get(),
            CompletionCallback);

        if (SUCCEEDED(hr))
        {
            callbackState.Detach();
        }
    }
    else
    {
        SignalWait(state);
    }

    return hr;
}

static void SignalWait(AsyncStateRef const& state)
{
    bool newlySatisfied;
    {
        std::lock_guard<std::mutex> lock(state->waitMutex);
        newlySatisfied = !state->waitSatisfied;
        state->waitSatisfied = true;
        state->waitCondition.notify_all();
    }
    SetEvent(state->waitEvent);

    assert(newlySatisfied);
    if (newlySatisfied)
    {
        XTaskQueueResumeTermination(state->queue);
    }
}

static void CALLBACK CompletionCallback(
    void* context,
    BOOLEAN canceled)
{
    UNREFERENCED_PARAMETER(canceled);

    AsyncStateRef state;
    state.Attach(static_cast<AsyncState*>(context));

    XAsyncBlock* asyncBlock = state->userAsyncBlock;
    if (state->providerAsyncBlock.callback != nullptr)
    {
        state->providerAsyncBlock.callback(asyncBlock);
    }

    SignalWait(state);
}

static void CALLBACK WorkerCallback(
    void* context,
    BOOLEAN canceled)
{
    AsyncStateRef state;
    state.Attach(static_cast<AsyncState*>(context));
    state->workScheduled = false;

    if (!state->valid)
    {
        return;
    }

    if (canceled)
    {
        XAsyncCancel(state->userAsyncBlock);
        HRESULT callStatus;
        {
            AsyncBlockInternalGuard internal{ state->userAsyncBlock };
            callStatus = internal.GetStatus();
        }

        if (callStatus != E_ABORT)
        {
            XAsyncComplete(state->userAsyncBlock, E_ABORT, 0);
        }
    }
    else
    {
        HRESULT result = state->provider(XAsyncOp::DoWork, &state->providerData);

        if (result != E_PENDING)
        {
            if (SUCCEEDED(result))
            {
                result = E_UNEXPECTED;
            }

            XAsyncComplete(&state->providerAsyncBlock, result, 0);
        }
    }

    CleanupProviderForLocation(state, ProviderCleanupLocation::AfterDoWork);
}


HRESULT WINAPI XAsyncGetStatus(
    XAsyncBlock *asyncBlock,
    BOOLEAN wait
) {
    HRESULT result = E_PENDING;
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        result = internal.GetStatus();
        state = internal.GetState();
    }

    if (wait)
    {
        if (state == nullptr)
        {
            assert(result != E_PENDING);
            RETURN_HR_IF(E_INVALIDARG, result == E_PENDING);
        }
        else
        {
            if (s_AsyncLibEnablePumpingWait)
            {
                APTTYPE aptType;
                APTTYPEQUALIFIER aptQualifier;
                if (SUCCEEDED(CoGetApartmentType(&aptType, &aptQualifier)) && aptType != APTTYPE_MTA && aptType != APTTYPE_NA)
                {
                    DWORD idx;
                    CoWaitForMultipleHandles(COWAIT_DEFAULT, INFINITE, 1, &state->waitEvent, &idx);
                }
            }
            {
                std::unique_lock<std::mutex> lock(state->waitMutex);

                if (!state->waitSatisfied)
                {
                    AsyncState* s = state.Get();
                    state->waitCondition.wait(lock, [s] { return s->waitSatisfied; });
                }
            }

            {
                AsyncBlockInternalGuard internal{ asyncBlock };
                result = internal.GetStatus();
            }        
        }
    }

    return result;
}

HRESULT WINAPI XAsyncGetResultSize(
    XAsyncBlock* asyncBlock,
    size_t* bufferSize
) {
    HRESULT result = E_PENDING;
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        result = internal.GetStatus();
        state = internal.GetState();
    }

    *bufferSize = state == nullptr ? 0 : state->providerData.bufferSize;

    return result;
}

void WINAPI XAsyncCancel(
    XAsyncBlock* asyncBlock
) {
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        state = internal.GetState();
    }

    if (state != nullptr)
    {
        if (TrySetProviderCleanup(state, ProviderCleanupLocation::InCancel))
        {
            state->provider(XAsyncOp::Cancel, &state->providerData);
            RevertProviderCleanup(state, ProviderCleanupLocation::InCancel);
        }
    }
}

HRESULT WINAPI XAsyncRun(
    XAsyncBlock* asyncBlock,
    XAsyncWork* work
) {
    RETURN_IF_FAILED(XAsyncBegin(
        asyncBlock,
        reinterpret_cast<void*>(work),
        reinterpret_cast<void*>(XAsyncRun),
        __FUNCTION__,
        [](XAsyncOp op, const XAsyncProviderData* data)
    {
        switch (op)
        {
            case XAsyncOp::Begin:
                return XAsyncSchedule(data->async, 0);
                
            case XAsyncOp::DoWork:
                {
                    XAsyncWork* work = reinterpret_cast<XAsyncWork*>(data->context);
                    HRESULT hr = work(data->async);
                    XAsyncComplete(data->async, hr, 0);
                }
                break;

            case XAsyncOp::Cancel:
            case XAsyncOp::Cleanup:
            case XAsyncOp::GetResult:
                break;

        }

        return S_OK;
    }));

    return S_OK;
}

HRESULT WINAPI XAsyncBegin(
    XAsyncBlock* asyncBlock,
    void* context,
    const void* identity,
    const char* identityName,
    XAsyncProvider* provider
) {
    RETURN_IF_FAILED(AllocState(asyncBlock, 0));

    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        state = internal.GetState();
    }

    state->provider = provider;
    state->providerData.context = context;
    state->identity = identity;
    state->identityName = identityName;

    HRESULT hr = provider(XAsyncOp::Begin, &state->providerData);
    if (FAILED(hr))
    {
        XAsyncComplete(asyncBlock, hr, 0);
    }

    return S_OK;
}

HRESULT WINAPI XAsyncBeginAlloc(
    XAsyncBlock* asyncBlock,
    const void* identity,
    const char* identityName,
    XAsyncProvider* provider,
    size_t contextSize,
    size_t parameterBlockSize,
    void* parameterBlock
) {
    RETURN_HR_IF(E_INVALIDARG, contextSize == 0);

    if (parameterBlockSize != 0)
    {
        RETURN_HR_IF(E_INVALIDARG, parameterBlock == nullptr || parameterBlockSize > contextSize);
    }
    else
    {
        RETURN_HR_IF(E_INVALIDARG, parameterBlock != nullptr);
    }

    RETURN_IF_FAILED(AllocState(asyncBlock, contextSize));

    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        state = internal.GetState();
    }

    assert(state != nullptr);

    state->provider = provider;
    state->identity = identity;
    state->identityName = identityName;

    assert(state->providerData.context != nullptr);
    memset(state->providerData.context, 0, contextSize);

    if (parameterBlockSize != 0)
    {
        memcpy(state->providerData.context, parameterBlock, parameterBlockSize);
    }

    HRESULT hr = provider(XAsyncOp::Begin, &state->providerData);

    if (FAILED(hr))
    {
        XAsyncComplete(asyncBlock, hr, 0);
    }

    return S_OK;
}

HRESULT WINAPI XAsyncSchedule(
    XAsyncBlock* asyncBlock,
    uint32_t delayInMs
) {
    HRESULT existingStatus;
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        state = internal.GetState();
        existingStatus = internal.GetStatus();
    }

    RETURN_HR_IF(existingStatus, FAILED(existingStatus) && existingStatus != E_PENDING);

    RETURN_HR_IF(E_INVALIDARG, state == nullptr);

    bool priorScheduled = false;
    state->workScheduled.compare_exchange_strong(priorScheduled, true);

    if (priorScheduled)
    {
        RETURN_HR(E_UNEXPECTED);
    }

    TRACE("state->queue here is %p\n", state->queue);

    RETURN_IF_FAILED(XTaskQueueSubmitDelayedCallback(
        state->queue,
        XTaskQueuePort::Work,
        delayInMs,
        state.Get(),
        WorkerCallback));

    state.Detach();

    TRACE("Before completion, queue here was %p\n", state->queue);

    return S_OK;
}

void WINAPI XAsyncComplete(
    XAsyncBlock* asyncBlock,
    HRESULT result,
    size_t requiredBufferSize
) {
    if (result == E_PENDING)
    {
        return;
    }

    bool completedNow = false;
    bool doCleanup = false;
    AsyncStateRef state;
    {
        AsyncBlockInternalGuard internal{ asyncBlock };

        completedNow = internal.TrySetTerminalStatus(result);

        if ((requiredBufferSize == 0 || FAILED(result)) && completedNow)
        {
            doCleanup = true;
            requiredBufferSize = 0;
            state = internal.ExtractState();
        }
        else
        {
            state = internal.GetState();
        }

        if (completedNow)
        {
            state->providerData.bufferSize = requiredBufferSize;
        }
    }

    if (completedNow)
    {
        FAIL_FAST_IF_FAILED(SignalCompletion(state));
    }

    if (doCleanup)
    {
        TrySetProviderCleanup(state, ProviderCleanupLocation::AfterDoWork);
        CleanupState(std::move(state));
    }
}

HRESULT WINAPI XAsyncGetResult(
    XAsyncBlock* asyncBlock,
    const void* identity,
    size_t bufferSize,
    void* buffer,
    size_t* bufferUsed
) {
    HRESULT result = E_PENDING;
    AsyncStateRef state;
    bool resultsAlreadyReturned;

    {
        AsyncBlockInternalGuard internal{ asyncBlock };
        result = internal.GetStatus();
        state = internal.GetState();
        resultsAlreadyReturned = internal.GetResultsRetrieved();
    }

    if (SUCCEEDED(result))
    {
        RETURN_HR_IF(E_ILLEGAL_METHOD_CALL, resultsAlreadyReturned);

        if (state == nullptr)
        {
            if (bufferUsed != nullptr)
            {
                *bufferUsed = 0;
            }
        }
        else if (identity != state->identity)
        {
            char buf[100];
            int sprintfResult;
            if (state->identityName != nullptr)
            {
                sprintfResult = snprintf(
                    buf,
                    sizeof(buf),
                    "Call/Result mismatch.  This XAsyncBlock was initiated by '%s'.\r\n",
                    state->identityName);
            }
            else
            {
                sprintfResult = snprintf(buf, sizeof(buf), "Call/Result mismatch\r\n");
            }

            result = E_INVALIDARG;
            TRACE(buf);
            assert(false);
            assert(sprintfResult > 0);
        }
        else if (state->providerData.bufferSize == 0)
        {
            result = E_NOTIMPL;
        }
        else if (buffer == nullptr)
        {
            result = E_INVALIDARG;
        }
        else if (bufferSize < state->providerData.bufferSize)
        {
            result = E_NOT_SUFFICIENT_BUFFER;
        }
        else
        {
            if (bufferUsed != nullptr)
            {
                *bufferUsed = state->providerData.bufferSize;
            }

            state->providerData.bufferSize = bufferSize;
            state->providerData.buffer = buffer;
            result = state->provider(XAsyncOp::GetResult, &state->providerData);
        }
    }

    if (result != E_PENDING && state != nullptr)
    {
        {
            AsyncBlockInternalGuard internal{ asyncBlock };
            internal.ExtractState(true);
        }

        CleanupState(std::move(state));
    }

    return result;
}
