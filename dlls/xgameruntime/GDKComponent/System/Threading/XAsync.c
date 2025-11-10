/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XAsync
 * 
 * Written by Weather
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

#include "XAsync.h"
#include "XTaskQueue.h"

WINE_DEFAULT_DEBUG_CHANNEL(xtaskqueue);

static inline struct x_async_block_guard *impl_from_IXAsyncBlockInternalGuard( IXAsyncBlockInternalGuard *iface )
{
    return CONTAINING_RECORD( iface, struct x_async_block_guard, IXAsyncBlockInternalGuard_iface );
}

static inline struct async_state *impl_from_IAsyncState( IAsyncState *iface )
{
    return CONTAINING_RECORD( iface, struct async_state, IAsyncState_iface );
}

static HRESULT WINAPI selfProviderOperation( XAsyncOp op, const XAsyncProviderData* data )
{
    XAsyncWork* work;
    HRESULT hr;

    switch ( op )
    {
        case Begin:
            return XAsyncSchedule(data->async, 0);
                
        case DoWork:
            work = (XAsyncWork *)data->context;
            hr = work( data->async );
            XAsyncComplete( data->async, hr, 0 );
            break;

        case Cancel:
        case Cleanup:
        case GetResult:
            break;
    }

    return S_OK;
}

static HRESULT WINAPI async_state_QueryInterface( IAsyncState *iface, REFIID iid, void **out )
{
    struct async_state *impl = impl_from_IAsyncState( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAsyncState ))
    {
        *out = &impl->IAsyncState_iface;
        impl->IAsyncState_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI async_state_AddRef( IAsyncState *iface )
{
    struct async_state *impl = impl_from_IAsyncState( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI async_state_Release( IAsyncState *iface )
{
    struct async_state *impl = impl_from_IAsyncState( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static const struct IAsyncStateVtbl async_state_vtbl =
{
    /* IUnknown methods */
    async_state_QueryInterface,
    async_state_AddRef,
    async_state_Release
};

static HRESULT WINAPI x_async_block_guard_QueryInterface( IXAsyncBlockInternalGuard *iface, REFIID iid, void **out )
{
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXAsyncBlockInternalGuard ))
    {
        *out = &impl->IXAsyncBlockInternalGuard_iface;
        impl->IXAsyncBlockInternalGuard_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_async_block_guard_AddRef( IXAsyncBlockInternalGuard *iface )
{
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_async_block_guard_Release( IXAsyncBlockInternalGuard *iface )
{
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static IAsyncState* WINAPI x_async_block_guard_GetState( IXAsyncBlockInternalGuard *iface )
{
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );
    struct async_state *state = impl_from_IAsyncState( impl->internal->state );

    TRACE( "iface %p, state is %p.\n", iface, impl->internal->state );

    if ( impl->internal->state != NULL && state->signature != ASYNC_STATE_SIG )
    {
        return NULL;
    }

    if ( impl->internal->state != NULL )
        impl->internal->state->lpVtbl->AddRef( impl->internal->state );

    return impl->internal->state;
}

static IAsyncState* WINAPI x_async_block_guard_ExtractState( IXAsyncBlockInternalGuard *iface, BOOLEAN resultsRetrieved )
{    
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );
    struct async_state *state = impl_from_IAsyncState( impl->internal->state );

    TRACE( "iface %p, resultsRetrieved %d.\n", iface, resultsRetrieved );

    if ( impl->internal->state != NULL && state->signature != ASYNC_STATE_SIG )
    {
        return NULL;
    }

    if ( resultsRetrieved )
    {
        impl->internal->signature = ASYNC_BLOCK_RESULT_SIG;
        impl->userInternal->signature = ASYNC_BLOCK_RESULT_SIG;
    }
    else
    {
        impl->internal->signature = 0;
        impl->userInternal->signature = 0;
    }

    return impl->internal->state;
}

static HRESULT WINAPI x_async_block_guard_GetStatus( IXAsyncBlockInternalGuard *iface )
{
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );

    TRACE( "iface %p.\n", iface );

    return impl->internal->status;
}

static BOOLEAN WINAPI x_async_block_guard_GetResultsRetrieved( IXAsyncBlockInternalGuard *iface )
{
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );

    TRACE( "iface %p.\n", iface );

    return impl->internal->signature == ASYNC_BLOCK_RESULT_SIG;
}

static BOOLEAN WINAPI x_async_block_guard_TrySetTerminalStatus( IXAsyncBlockInternalGuard *iface, HRESULT status )
{
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );

    TRACE( "iface %p.\n", iface );

    if ( impl->locked && impl->internal->status == E_PENDING )
    {
        impl->userInternal->status = status;
        impl->internal->status = status;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static AsyncBlockInternal* x_async_block_guard_DoLock( XAsyncBlock* asyncBlock )
{
    AsyncBlockInternal* lockedResult;
    AsyncBlockInternal* stateAsyncBlockInternal;

    struct async_state *state = NULL;

    TRACE( "asyncBlock %p.\n", asyncBlock );

    if ( !asyncBlock )
        return NULL;

    lockedResult = (AsyncBlockInternal *)asyncBlock->internal;

    if ( lockedResult->signature != ASYNC_BLOCK_SIG )
    {
        lockedResult->state = NULL;
        return NULL;
    }

    EnterCriticalSection( &lockedResult->lock );

    state = impl_from_IAsyncState( lockedResult->state );

    if ( lockedResult->state == NULL || asyncBlock == &state->providerAsyncBlock )
        return lockedResult;

    lockedResult->state->lpVtbl->AddRef( lockedResult->state );

    LeaveCriticalSection( &lockedResult->lock );

    stateAsyncBlockInternal = (AsyncBlockInternal *)state->providerAsyncBlock.internal;
    if ( stateAsyncBlockInternal == NULL )
    {
        EnterCriticalSection( &lockedResult->lock );
        lockedResult->state->lpVtbl->Release( lockedResult->state );
        return lockedResult;
    }

    EnterCriticalSection( &stateAsyncBlockInternal->lock );

    if ( stateAsyncBlockInternal->state == NULL )
    {
        LeaveCriticalSection( &stateAsyncBlockInternal->lock );
        EnterCriticalSection( &lockedResult->lock );
        lockedResult->state->lpVtbl->Release( lockedResult->state );
        return lockedResult;
    }

    lockedResult->state->lpVtbl->Release( lockedResult->state );
    return stateAsyncBlockInternal;
}

static const struct IXAsyncBlockInternalGuardVtbl x_async_block_guard_vtbl =
{
    /* IUnknown methods */
    x_async_block_guard_QueryInterface,
    x_async_block_guard_AddRef,
    x_async_block_guard_Release,
    /* IXAsyncBlockInternalGuard methods */
    x_async_block_guard_GetState,
    x_async_block_guard_ExtractState,
    x_async_block_guard_GetStatus,
    x_async_block_guard_GetResultsRetrieved,
    x_async_block_guard_TrySetTerminalStatus,
    x_async_block_guard_DoLock
};

static VOID InitInternalGuardFromBlock( IXAsyncBlockInternalGuard *iface, XAsyncBlock* asyncBlock )
{   
    struct async_state *state = NULL;
    struct x_async_block_guard *impl = impl_from_IXAsyncBlockInternalGuard( iface );

    TRACE( "iface %p, asyncBlock %p.\n", iface, asyncBlock );

    impl->internal = iface->lpVtbl->DoLock( asyncBlock );
    impl->locked = impl->internal != NULL;

    if ( !impl->locked )
    {
        // We never locked because the block contains an invalid signature.  We still
        // need the block for access though (although that access will be read only).
        impl->internal = (AsyncBlockInternal *)asyncBlock->internal;
    }

    if ( impl->internal->state != NULL )
    {
        state = impl_from_IAsyncState( impl->internal->state );
        impl->userInternal = (AsyncBlockInternal *)state->userAsyncBlock->internal;
    }
    else
    {
        impl->userInternal = impl->internal;
    }

    // If user internal != internal, we grab its lock.  Note that
    // lock ordering here is critical.  It must always be 
    // state lock, then user lock.  If state is not available, then
    // it is just user lock.

    /*
    if ( impl->userInternal != impl->internal )
    {
        TRACE("got here!\n");
        EnterCriticalSection( &impl->userInternal->lock );
    }*/

    return;
}

static HRESULT AllocStateNoCompletion( XAsyncBlock* asyncBlock, AsyncBlockInternal* internal, size_t contextSize )
{
    struct async_state* stateImpl; 

    XTaskQueueHandle queue;

    TRACE( "asyncBlock %p, internal %p, contextSize %lld.\n", asyncBlock, internal, contextSize );

    if (!(stateImpl = calloc( 1, sizeof(*stateImpl) ))) return E_OUTOFMEMORY;

    stateImpl->IAsyncState_iface.lpVtbl = &async_state_vtbl;
    stateImpl->ref = 1;
    InitializeConditionVariable( &stateImpl->cv );
    InitializeCriticalSection( &stateImpl->cs );
    stateImpl->signature = ASYNC_STATE_SIG;
    stateImpl->providerCleanup = CleanupLocation_Destructor;
    stateImpl->valid = TRUE;

    if ( contextSize != 0 )
    {
        // User allocated additional context data.  This was allocated as extra bytes at the end of 
        // async state.
        stateImpl->providerData.context = (&stateImpl->IAsyncState_iface + 1);
    }
    
    // Addref the task queue. We duplicate with "Reference" to prevent spamming
    // the handle tracker with each async call (and to prevent a needless allocation of
    // the task queue handle wrapper).

    queue = asyncBlock->queue;
    if ( queue == NULL )
    {
        if ( IXThreadingImpl_XTaskQueueGetCurrentProcessTaskQueue( x_threading_impl, &stateImpl->queue ) == FALSE )
        {
            return HRESULT_FROM_WIN32( ERROR_NO_TASK_QUEUE );
        }
    }
    else
    {
        queue->headQueue->lpVtbl->AddRef( queue->headQueue );
        stateImpl->queue = queue;
    }

    stateImpl->userAsyncBlock = asyncBlock;
    stateImpl->providerData.async = &stateImpl->providerAsyncBlock;

    // Note: needs to be the last failable thing we do.
    //hr = XTaskQueueSuspendTermination( stateImpl->queue );

    internal->state = &stateImpl->IAsyncState_iface;

    // Duplicate the async block we've just configured
    stateImpl->providerAsyncBlock = *asyncBlock;
    stateImpl->providerAsyncBlock.queue = stateImpl->queue;

    return S_OK;
}

static HRESULT AllocState( XAsyncBlock* asyncBlock, SIZE_T contextSize )
{
    UINT32 internalIterator;
    HRESULT hr;
    AsyncBlockInternal* internal;

    TRACE( "asyncBlock %p, contextSize %lld.\n", asyncBlock, contextSize );

    if ( !asyncBlock )
        return E_INVALIDARG;

    internal = (AsyncBlockInternal *)asyncBlock->internal;

    // If the async block is already associated with another
    // call, fail.

    // We need to guard against use of an active async block.  We don't want
    // to rely on the caller zeroing memory so we check a signature
    // DWORD. This signature is cleared when the block can be reused.
    if ( internal->signature == ASYNC_BLOCK_SIG )
        return E_INVALIDARG;

    // This could be a reused async block from a prior
    // call, so zero everything.
    for ( internalIterator = 0; internalIterator < sizeof( asyncBlock->internal ); internalIterator++ )
    {
        asyncBlock->internal[internalIterator] = 0;
    }

    // Construction is inherently single threaded
    // (there is nothing we can do if the client tries to use the same
    // XAsyncBlock in 2 calls at the same time)

    internal->signature = ASYNC_BLOCK_SIG;
    InitializeCriticalSection( &internal->lock );
    
    hr = AllocStateNoCompletion( asyncBlock, internal, contextSize );

    if ( FAILED( hr ) )
    {
        internal->signature = 0;
        internal->status = hr;
    }

    return hr;
}

static void CleanupProviderForLocation( IAsyncState *state, ProviderCleanupLocation location )
{
    struct async_state *stateImpl = impl_from_IAsyncState( state );

    TRACE( "state %p, location %d.\n", state, location );
    
    if ( InterlockedCompareExchange( &stateImpl->providerCleanup, 0, 0 ) == InterlockedCompareExchange( &stateImpl->providerCleanup, location, CleanupLocation_CleanedUp ) )
    {
        stateImpl->providerCallback( Cleanup, &stateImpl->providerData );
    }

    return;
}

static BOOLEAN TrySetProviderCleanup( IAsyncState* state, ProviderCleanupLocation location )
{
    struct async_state *stateImpl = impl_from_IAsyncState( state );

    ProviderCleanupLocation expected = CleanupLocation_Destructor;

    TRACE( "state %p, location %d.\n", state, location );

    return InterlockedCompareExchange( &stateImpl->providerCleanup, 0, 0 ) == InterlockedCompareExchange( &stateImpl->providerCleanup, expected, location );
}

static VOID RevertProviderCleanup( IAsyncState* state, _In_ ProviderCleanupLocation expected )
{
    struct async_state *stateImpl = impl_from_IAsyncState( state );

    TRACE( "state %p, expected %d.\n", state, expected );
    
    InterlockedCompareExchange( &stateImpl->providerCleanup, expected, CleanupLocation_Destructor );

    return;
}

static void SignalWait( IAsyncState* state )
{
    BOOLEAN newlySatisfied;

    struct async_state *stateImpl = impl_from_IAsyncState( state );

    TRACE( "state %p.\n", state );

    EnterCriticalSection( &stateImpl->cs );
    newlySatisfied = !stateImpl->waitSatisfied;
    stateImpl->waitSatisfied = TRUE;
    WakeAllConditionVariable( &stateImpl->cv );
    LeaveCriticalSection( &stateImpl->cs );


    // We should only come in here once, but we don't want
    // to underflow task queue resumes and we already know
    // from above if we're first marking this wait as
    // satisfied, so use it.

    if ( newlySatisfied )
    {
        XTaskQueueResumeTermination( stateImpl->queue );
    }
}

static void CALLBACK CompletionCallback( void* context, BOOL canceled )
{
    IAsyncState *state = (IAsyncState *)context;
    XAsyncBlock* asyncBlock;

    struct async_state *stateImpl = impl_from_IAsyncState( state );

    TRACE( "context %p, canceled %d.\n", context, canceled );

    // We always pass the user async block into the 
    // callback, but we don't trust it -- we check
    // the callback field on our internal copy.
    asyncBlock = stateImpl->userAsyncBlock;
    if ( stateImpl->providerAsyncBlock.callback != NULL )
    {
        stateImpl->providerAsyncBlock.callback(asyncBlock);
    }

    SignalWait( state );
}

static HRESULT SignalCompletion( IAsyncState *state )
{
    HRESULT hr = S_OK;

    struct async_state *stateImpl = impl_from_IAsyncState( state );

    TRACE( "state %p.\n", state );

    if ( stateImpl->providerData.async->callback != NULL )
    {
        state->lpVtbl->AddRef( state );
        hr = XTaskQueueSubmitDelayedCallback( stateImpl->queue, Completion, 0, (PVOID)state, CompletionCallback );

        if ( SUCCEEDED( hr ) )
        {
            state->lpVtbl->Release( state );
        }
    }
    else
    {
        SignalWait( state );
    }

    return hr;
}

static void CleanupState( IAsyncState *state)
{
    struct async_state *stateImpl = impl_from_IAsyncState( state );

    TRACE( "state %p.\n", state );

    if ( state != NULL )
    {
        stateImpl->valid = FALSE;
        state->lpVtbl->Release( state );
    }
}


static void CALLBACK WorkerCallback( PVOID context, BOOL canceled )
{
    HRESULT callStatus;
    IAsyncState *state = (IAsyncState *)context;

    struct x_async_block_guard *impl;
    struct async_state *stateImpl = impl_from_IAsyncState( state );

    TRACE( "context %p, canceled %d.\n", context, canceled );

    if ( !stateImpl->valid )
        return;

    stateImpl->workScheduled = FALSE;

    // If the queue is canceling callbacks, simply cancel this work. Since no
    // new work for this call will be scheduled, if the call didn't cancel
    // immediately do it ourselves.

    if ( canceled )
    {
        XAsyncCancel( stateImpl->userAsyncBlock );

        if (!(impl = calloc( 1, sizeof(*impl) ))) return;

        impl->IXAsyncBlockInternalGuard_iface.lpVtbl = &x_async_block_guard_vtbl;
        impl->ref = 1;
        impl->locked = FALSE;

        {
            InitInternalGuardFromBlock( &impl->IXAsyncBlockInternalGuard_iface, stateImpl->userAsyncBlock );
            callStatus = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetStatus( &impl->IXAsyncBlockInternalGuard_iface );
        }

        if ( callStatus != E_ABORT )
        {
            XAsyncComplete( stateImpl->userAsyncBlock, E_ABORT, 0 );
        }

        if ( impl->locked )
        {
            LeaveCriticalSection( &impl->internal->lock );
            if ( impl->userInternal != impl->internal )
            {
                LeaveCriticalSection( &impl->userInternal->lock );
            }
        }

        free( impl );
    }
    else
    {
        callStatus = stateImpl->providerCallback( DoWork, &stateImpl->providerData );

        // Work routine can return E_PENDING if there is more work to do.  Otherwise
        // it either needs to be a failure or it should have called XAsyncComplete, which
        // would have set a new value into the status.

        if ( callStatus != E_PENDING )
        {
            if ( SUCCEEDED( callStatus ) )
            {
                callStatus = E_UNEXPECTED;
            }

            XAsyncComplete( &stateImpl->providerAsyncBlock, callStatus, 0 );
        }
    }

    // If the result of this call caused a completion with no payload, XAsyncComplete
    // will change the provider cleanup to be "AfterWork", which is here.  Cleanup
    // the provider if we need to.
    CleanupProviderForLocation( state, CleanupLocation_AfterDoWork );
}

HRESULT XAsyncGetStatus( XAsyncBlock* asyncBlock, BOOLEAN wait )
{
    HRESULT result = E_PENDING;
    IAsyncState *state;

    struct x_async_block_guard *impl;
    struct async_state *stateImpl = NULL;

    TRACE( "asyncBlock %p, wait %d.\n", asyncBlock, wait );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IXAsyncBlockInternalGuard_iface.lpVtbl = &x_async_block_guard_vtbl;
    impl->ref = 1;
    impl->locked = FALSE;

    {
        InitInternalGuardFromBlock( &impl->IXAsyncBlockInternalGuard_iface, asyncBlock );
        result = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetStatus( &impl->IXAsyncBlockInternalGuard_iface );
        state = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetState( &impl->IXAsyncBlockInternalGuard_iface );
        stateImpl = impl_from_IAsyncState( state );
    }

    // If we are being asked to wait, always check the wait state before
    // looking at the hresult.  Our wait waits until the completion runs
    // so we may need to wait past when the status is set.

    if ( wait )
    {
        if ( state == NULL )
        {
            if ( result == E_PENDING )
                return E_INVALIDARG;
        }
        else
        {
            EnterCriticalSection( &stateImpl->cs );

            if ( !stateImpl->waitSatisfied )
            {
                SleepConditionVariableCS( &stateImpl->cv, &stateImpl->cs, INFINITE );
            }
            result = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetStatus( &impl->IXAsyncBlockInternalGuard_iface );    
        }
    }

    if ( impl->locked )
    {
        LeaveCriticalSection( &impl->internal->lock );
        if ( impl->userInternal != impl->internal )
        {
            LeaveCriticalSection( &impl->userInternal->lock );
        }
    }

    free( impl );

    return result;
}

HRESULT XAsyncGetResultSize( XAsyncBlock* asyncBlock, SIZE_T* bufferSize )
{
    HRESULT result = E_PENDING;
    IAsyncState *state;

    struct x_async_block_guard *impl;
    struct async_state *stateImpl = NULL;

    TRACE( "asyncBlock %p, bufferSize %p.\n", asyncBlock, bufferSize );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IXAsyncBlockInternalGuard_iface.lpVtbl = &x_async_block_guard_vtbl;
    impl->ref = 1;
    impl->locked = FALSE;

    {
        //constructor
        InitInternalGuardFromBlock( &impl->IXAsyncBlockInternalGuard_iface, asyncBlock );
        result = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetStatus( &impl->IXAsyncBlockInternalGuard_iface );
        state = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetState( &impl->IXAsyncBlockInternalGuard_iface );
        stateImpl = impl_from_IAsyncState( state );
    }

    *bufferSize = state == NULL ? 0 : stateImpl->providerData.bufferSize;

    //destructor
    if ( impl->locked )
    {
        LeaveCriticalSection( &impl->internal->lock );
        if ( impl->userInternal != impl->internal )
        {
            LeaveCriticalSection( &impl->userInternal->lock );
        }
    }

    free( impl );

    return result;
}

VOID XAsyncCancel( XAsyncBlock* asyncBlock )
{
    IAsyncState *state;

    struct x_async_block_guard *impl;
    struct async_state *stateImpl = NULL;

    TRACE( "asyncBlock %p.\n", asyncBlock );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return;

    impl->IXAsyncBlockInternalGuard_iface.lpVtbl = &x_async_block_guard_vtbl;
    impl->ref = 1;
    impl->locked = FALSE;

    {
        InitInternalGuardFromBlock( &impl->IXAsyncBlockInternalGuard_iface, asyncBlock );
        state = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetState( &impl->IXAsyncBlockInternalGuard_iface );
        stateImpl = impl_from_IAsyncState( state );
    }

    if ( state != NULL )
    {
        // In case of cancel, failure, or success with no payload we will
        // agressively clean up the provider at the end of DoWork. This can race
        // with a cancel call. To prevent this we mark the provider cleanup as
        // "in cancel", which prevents switching it to the aggressive DoWork
        // cleanup.  We switch out of "in cancel" when done.  In the worst case this
        // will defer provider cleanup to the state destructor, which is the natural
        // place for it anyway.  Anything else here is just an optimization to get the
        // provider cleaned up sooner (the destructor location may be delayed until the
        // completion callback fires, since it's hanging on to a state object ref).

        if ( TrySetProviderCleanup( state, CleanupLocation_InCancel ) )
        {
            stateImpl->providerCallback( Cancel, &stateImpl->providerData );
            RevertProviderCleanup( state, CleanupLocation_InCancel );
        }
    }

    if ( impl->locked )
    {
        LeaveCriticalSection( &impl->internal->lock );
        if ( impl->userInternal != impl->internal )
        {
            LeaveCriticalSection( &impl->userInternal->lock );
        }
    }

    free( impl );
}

HRESULT XAsyncRun( XAsyncBlock* asyncBlock, XAsyncWork* work )
{
    HRESULT hr = S_OK;

    TRACE( "asyncBlock %p, work %p.\n", asyncBlock, work );

    hr = XAsyncBegin( asyncBlock, (PVOID)work, (PVOID)XAsyncRun, "XAsyncRun", selfProviderOperation );

    return hr;
}

HRESULT XAsyncBegin( XAsyncBlock* asyncBlock, PVOID context, PVOID identity, LPCSTR identityName, XAsyncProviderCallback* provider )
{
    HRESULT hr;
    IAsyncState *state;

    struct x_async_block_guard *impl;
    struct async_state *stateImpl = NULL;

    TRACE( "asyncBlock %p, context %p, identity %p, identityName %s, provider %p.\n", asyncBlock, context, identity, identityName, provider );

    hr = AllocState( asyncBlock, 0 );
    if ( FAILED( hr ) ) return hr;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IXAsyncBlockInternalGuard_iface.lpVtbl = &x_async_block_guard_vtbl;
    impl->ref = 1;
    impl->locked = FALSE;

    {
        InitInternalGuardFromBlock( &impl->IXAsyncBlockInternalGuard_iface, asyncBlock );
        state = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetState( &impl->IXAsyncBlockInternalGuard_iface );
        stateImpl = impl_from_IAsyncState( state );
    }

    stateImpl->providerCallback = provider;
    stateImpl->identity = identity;
    stateImpl->identityName = identityName;
    stateImpl->providerData.context = context;

    hr = stateImpl->providerCallback( Begin, &stateImpl->providerData );
    if ( FAILED( hr ) )
    {
        XAsyncComplete( asyncBlock, hr, 0 );
    }

    return S_OK;
}

HRESULT XAsyncSchedule( XAsyncBlock* asyncBlock, UINT32 delayInMs )
{
    HRESULT hr;
    HRESULT exitingStatus;
    BOOLEAN priorScheduled;
    IAsyncState *state;

    struct x_async_block_guard *impl;
    struct async_state *stateImpl = NULL;

    TRACE( "asyncBlock %p, delayInMs %d.\n", asyncBlock, delayInMs );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IXAsyncBlockInternalGuard_iface.lpVtbl = &x_async_block_guard_vtbl;
    impl->ref = 1;
    impl->locked = FALSE;

    {
        InitInternalGuardFromBlock( &impl->IXAsyncBlockInternalGuard_iface, asyncBlock );
        state = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetState( &impl->IXAsyncBlockInternalGuard_iface );
        exitingStatus = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetStatus( &impl->IXAsyncBlockInternalGuard_iface );
        stateImpl = impl_from_IAsyncState( state );
    }

    if ( FAILED( exitingStatus ) && exitingStatus != E_PENDING )
        return exitingStatus;

    if ( state == NULL )
        return E_INVALIDARG;

    priorScheduled = FALSE;
    InterlockedCompareExchange( &stateImpl->workScheduled, priorScheduled, TRUE );

    if ( priorScheduled )
    {
        return E_UNEXPECTED;
    }

    state->lpVtbl->AddRef( state );

    hr = XTaskQueueSubmitDelayedCallback( stateImpl->queue, Work, delayInMs, (PVOID)state, WorkerCallback );

    state->lpVtbl->Release( state );

    free( impl );

    return hr;
}

VOID XAsyncComplete( XAsyncBlock* asyncBlock, HRESULT result, SIZE_T requiredBufferSize )
{
    // E_PENDING is special -- if you still have work to do don't complete.
    HRESULT hr;
    BOOLEAN completedNow;
    BOOLEAN doCleanup;
    IAsyncState *state;

    struct x_async_block_guard *impl;
    struct async_state *stateImpl = NULL;

    TRACE( "asyncBlock %p, result %#lx, requiredBufferSize %lld.\n", asyncBlock, result, requiredBufferSize );

    if ( result == E_PENDING )
    {
        return;
    }

    if (!(impl = calloc( 1, sizeof(*impl) ))) return;

    impl->IXAsyncBlockInternalGuard_iface.lpVtbl = &x_async_block_guard_vtbl;
    impl->ref = 1;
    impl->locked = FALSE;

    InitInternalGuardFromBlock( &impl->IXAsyncBlockInternalGuard_iface, asyncBlock );

    completedNow = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->TrySetTerminalStatus( &impl->IXAsyncBlockInternalGuard_iface, result );

    if ( (requiredBufferSize == 0 || FAILED( result )) && completedNow )
    {
        doCleanup = TRUE;
        requiredBufferSize = 0;
        state = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->ExtractState( &impl->IXAsyncBlockInternalGuard_iface, FALSE );
    }
    else
    {
        state = impl->IXAsyncBlockInternalGuard_iface.lpVtbl->GetState( &impl->IXAsyncBlockInternalGuard_iface );
    }

    if ( !state )
    {
        WARN( "called from an invalid block!\n" );
        return;
    }

    stateImpl = impl_from_IAsyncState( state );

    if ( completedNow )
    {
        stateImpl->providerData.bufferSize = requiredBufferSize;
    }

    // Only signal / adjust needed buffer size if we were first to complete.
    if ( completedNow )
    {
        hr = SignalCompletion( state );
        if ( FAILED( hr ) ) return;
    }

    // At this point asyncBlock may be unsafe to touch. As we've cleaned up
    // state we will mark the state so that the DoWork callback calls
    // the Cleanup op on the provider.  This gets it cleaned up sooner
    // so it doesn't have to wait for the task queue to process it.

    if ( doCleanup )
    {
        TrySetProviderCleanup( state, CleanupLocation_AfterDoWork );
        CleanupState( state );
    }
}