/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XTask
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

#include "XTaskQueue.h"

WINE_DEFAULT_DEBUG_CHANNEL(xtaskqueue);

static void CALLBACK x_task_queue_port_WaitTimerOperation( void *context )
{
    IXTaskQueuePort* port = (IXTaskQueuePort *)context;
    port->lpVtbl->SubmitPendingCallback( port );
}

static BOOLEAN CALLBACK x_task_queue_port_VectorPredicateOperation( const void *element, void *context )
{
    return element == context;
}

static void CALLBACK x_task_queue_port_VectorVisitOperation( void *context )
{
    IXTaskQueuePortContext *impl = (IXTaskQueuePortContext *)context;
    impl->lpVtbl->ItemQueued( impl );
}

static inline struct x_task_queue_port_context *impl_from_IXTaskQueuePortContext( IXTaskQueuePortContext *iface )
{
    return CONTAINING_RECORD( iface, struct x_task_queue_port_context, IXTaskQueuePortContext_iface );
}

static inline struct x_task_queue_port *impl_from_IXTaskQueuePort( IXTaskQueuePort *iface )
{
    return CONTAINING_RECORD( iface, struct x_task_queue_port, IXTaskQueuePort_iface );
}

static inline struct x_task_queue_monitor_callback *impl_from_IXTaskQueueMonitorCallback( IXTaskQueueMonitorCallback *iface )
{
    return CONTAINING_RECORD( iface, struct x_task_queue_monitor_callback, IXTaskQueueMonitorCallback_iface );
}

static inline struct x_task_queue *impl_from_IXTaskQueue( IXTaskQueue *iface )
{
    return CONTAINING_RECORD( iface, struct x_task_queue, IXTaskQueue_iface );
}

static HRESULT WINAPI x_task_queue_port_context_QueryInterface( IXTaskQueuePortContext *iface, REFIID iid, void **out )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXTaskQueuePortContext ))
    {
        *out = &impl->IXTaskQueuePortContext_iface;
        impl->IXTaskQueuePortContext_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_task_queue_port_context_AddRef( IXTaskQueuePortContext *iface )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    impl->queue->lpVtbl->AddRef( impl->queue );
    return ref;
}

static ULONG WINAPI x_task_queue_port_context_Release( IXTaskQueuePortContext *iface )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    impl->queue->lpVtbl->Release( impl->queue );
    return ref;
}

static XTaskQueuePort WINAPI x_task_queue_port_context_get_Type( IXTaskQueuePortContext *iface )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p.\n", iface );

    return impl->type;
}

static XTaskQueuePortStatus WINAPI x_task_queue_port_context_get_Status( IXTaskQueuePortContext *iface )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p.\n", iface );

    return impl->status;
}

static IXTaskQueue* WINAPI x_task_queue_port_context_get_Queue( IXTaskQueuePortContext *iface )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p.\n", iface );

    impl->queue->lpVtbl->AddRef( impl->queue );

    return impl->queue;
}

static IXTaskQueuePort* WINAPI x_task_queue_port_context_get_Port( IXTaskQueuePortContext *iface )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p.\n", iface );

    impl->port->lpVtbl->AddRef( impl->port );

    return impl->port;
}

static VOID WINAPI x_task_queue_port_context_SetStatus( IXTaskQueuePortContext *iface, XTaskQueuePortStatus status )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p, status %d.\n", iface, status );

    impl->status = status;

    return;
}

static VOID WINAPI x_task_queue_port_context_ItemQueued( IXTaskQueuePortContext *iface )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p.\n", iface );

    impl->callbackSubmitted->lpVtbl->Invoke( impl->callbackSubmitted, impl->type );

    return;
}

static BOOLEAN WINAPI x_task_queue_port_context_AddSuspend( IXTaskQueuePortContext *iface )
{
    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p.\n", iface );

    return ( InterlockedExchangeAdd( &impl->suspendCount, 1 ) == 0 );
}

static BOOLEAN WINAPI x_task_queue_port_context_RemoveSuspend( IXTaskQueuePortContext *iface )
{
    UINT32 current;

    struct x_task_queue_port_context *impl = impl_from_IXTaskQueuePortContext( iface );

    TRACE( "iface %p.\n", iface );

    for(;;)
    {
        current = InterlockedCompareExchange( &impl->suspendCount, 0, 0 );

        // These should always be balanced and there is no
        // valid case where this should happen, even
        // for multiple threads racing.

        if (current == 0)
        {
            return TRUE;
        }

        if ( InterlockedCompareExchange(&impl->suspendCount, current - 1, current) == current )
        {
            return current == 1;
        }
    }

    return FALSE;
}

static const struct IXTaskQueuePortContextVtbl x_task_queue_port_context_vtbl =
{
    /* IUnknown methods */
    x_task_queue_port_context_QueryInterface,
    x_task_queue_port_context_AddRef,
    x_task_queue_port_context_Release,
    /* IXTaskQueuePortContext methods */
    x_task_queue_port_context_get_Type,
    x_task_queue_port_context_get_Status,
    x_task_queue_port_context_get_Queue,
    x_task_queue_port_context_get_Port,
    x_task_queue_port_context_SetStatus,
    x_task_queue_port_context_ItemQueued,
    x_task_queue_port_context_AddSuspend,
    x_task_queue_port_context_RemoveSuspend
};

static HRESULT WINAPI x_task_queue_monitor_callback_QueryInterface( IXTaskQueueMonitorCallback *iface, REFIID iid, void **out )
{
    struct x_task_queue_monitor_callback *impl = impl_from_IXTaskQueueMonitorCallback( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXTaskQueueMonitorCallback ))
    {
        *out = &impl->IXTaskQueueMonitorCallback_iface;
        impl->IXTaskQueueMonitorCallback_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_task_queue_monitor_callback_AddRef( IXTaskQueueMonitorCallback *iface )
{
    struct x_task_queue_monitor_callback *impl = impl_from_IXTaskQueueMonitorCallback( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_task_queue_monitor_callback_Release( IXTaskQueueMonitorCallback *iface )
{
    struct x_task_queue_monitor_callback *impl = impl_from_IXTaskQueueMonitorCallback( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_task_queue_monitor_callback_Register( IXTaskQueueMonitorCallback *iface, PVOID context, XTaskQueueMonitorCallback* callback, XTaskQueueRegistrationToken* token )
{
    UINT32 bufferReadIdx;
    UINT32 bufferWriteIdx;
    UINT32 expected;
    UINT32 desired;
    UINT32 idx;
    UINT32 expectedSwap;

    struct x_task_queue_monitor_callback *impl = impl_from_IXTaskQueueMonitorCallback( iface );

    TRACE( "iface %p, context %p, callback %p, token %p.\n", iface, context, callback, token );

    if ( !callback || !token )
        return E_POINTER;

    token->token = 0;

    EnterCriticalSection( &impl->cs );
    bufferReadIdx = ( impl->ref & 0x80000000 ? 1 : 0 );
    bufferWriteIdx = 1 - bufferReadIdx;

    for( idx = 0; idx < ARRAYSIZE( impl->monitors_buffer1 ); idx++ )
    {
        if ( token->token == 0 && impl->monitors_buffers[bufferReadIdx][idx].callback ==  NULL )
        {
            InterlockedIncrement64( &impl->nextToken );
            token->token = InterlockedCompareExchange64( &impl->nextToken, 0, 0 );
            impl->monitors_buffers[bufferWriteIdx][idx].token = token->token;
            impl->monitors_buffers[bufferWriteIdx][idx].context = context;
            impl->monitors_buffers[bufferWriteIdx][idx].callback = callback;
        }
        else
        {
            impl->monitors_buffers[bufferWriteIdx][idx] = impl->monitors_buffers[bufferReadIdx][idx];
        }
    }

    if ( token->token == 0 ) 
        return E_OUTOFMEMORY;

    // Now spin wait to swap the active buffer.
    expected = bufferReadIdx << 31;
    desired = bufferWriteIdx << 31;

    for (;;)
    {
        expectedSwap = expected;
        if ( InterlockedCompareExchange( &impl->ref, 0, 0 ) == InterlockedCompareExchange( &impl->ref, expectedSwap, desired ) )
        {
            break;
        }
    }
    
    LeaveCriticalSection( &impl->cs );

    return S_OK;
}

static VOID WINAPI x_task_queue_monitor_callback_Unregister( IXTaskQueueMonitorCallback *iface, XTaskQueueRegistrationToken token )
{
    UINT32 bufferReadIdx;
    UINT32 bufferWriteIdx;
    UINT32 idx;
    UINT32 expected;
    UINT32 desired;
    UINT32 expectedSwap;

    struct x_task_queue_monitor_callback *impl = impl_from_IXTaskQueueMonitorCallback( iface );

    TRACE( "iface %p, token %lld.\n", iface, token.token );
    
    EnterCriticalSection( &impl->cs );
    bufferReadIdx = ( impl->ref & 0x80000000 ? 1 : 0 );
    bufferWriteIdx = 1 - bufferReadIdx;

    for( idx = 0; idx < ARRAYSIZE( impl->monitors_buffer1 ); idx++ )
    {
        if ( impl->monitors_buffers[bufferReadIdx][idx].token == token.token )
        {
            impl->monitors_buffers[bufferWriteIdx][idx].callback = NULL;
        }
        else
        {
            impl->monitors_buffers[bufferWriteIdx][idx] = impl->monitors_buffers[bufferReadIdx][idx];
        }
    }

    // Now spin wait to swap the active buffer.
    expected = bufferReadIdx << 31;
    desired = bufferWriteIdx << 31;

    for (;;)
    {
        expectedSwap = expected;
        if ( InterlockedCompareExchange( &impl->ref, 0, 0 ) == InterlockedCompareExchange( &impl->ref, expectedSwap, desired ) )
        {
            break;
        }
    }

    LeaveCriticalSection( &impl->cs );
}

static VOID WINAPI x_task_queue_monitor_callback_Invoke( IXTaskQueueMonitorCallback *iface, XTaskQueuePort port )
{
    UINT32 indexAndRef;
    UINT32 bufferIdx;
    UINT32 idx = 0;

    struct x_task_queue_monitor_callback *impl = impl_from_IXTaskQueueMonitorCallback( iface );

    TRACE( "iface %p, port %d.\n", iface, port );

    InterlockedIncrement( &impl->ref );
    indexAndRef = InterlockedCompareExchange( &impl->ref, 0, 0 );
    bufferIdx = (indexAndRef & 0x80000000 ? 1 : 0);

    for( idx = 0; idx < ARRAYSIZE( impl->monitors_buffer1 ); idx++ )
    {
        if ( impl->monitors_buffers[bufferIdx][idx].callback != NULL )
        {
            impl->monitors_buffers[bufferIdx][idx].callback(
                impl->monitors_buffers[bufferIdx][idx].context,
                impl->queue,
                port);
        }
    }

    InterlockedDecrement( &impl->ref );
}

static const struct IXTaskQueueMonitorCallbackVtbl x_task_queue_monitor_callback_vtbl =
{
    /* IUnknown methods */
    x_task_queue_monitor_callback_QueryInterface,
    x_task_queue_monitor_callback_AddRef,
    x_task_queue_monitor_callback_Release,
    /* IXTaskQueueMonitorCallback methods */
    x_task_queue_monitor_callback_Register,
    x_task_queue_monitor_callback_Unregister,
    x_task_queue_monitor_callback_Invoke
};

static HRESULT WINAPI x_task_queue_port_QueryInterface( IXTaskQueuePort *iface, REFIID iid, void **out )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXTaskQueuePort ))
    {
        *out = &impl->IXTaskQueuePort_iface;
        impl->IXTaskQueuePort_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_task_queue_port_AddRef( IXTaskQueuePort *iface )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_task_queue_port_Release( IXTaskQueuePort *iface )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_task_queue_port_Initialize( IXTaskQueuePort *iface, XTaskQueueDispatchMode mode )
{
    HRESULT hr;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, mode %d.\n", iface, mode );

    impl->dispatchMode = mode;
    impl->processingCallback = 0;
    InitializeConditionVariable( &impl->cv );
    InitializeConditionVariable( &impl->cvAny );
    InitializeCriticalSection( &impl->cs );

    hr = CreateAtomicVector( &impl->attachedContexts );
    if ( FAILED( hr ) ) return hr;

    hr = XCreateWaitTimer( &impl->timer );
    if ( FAILED( hr ) ) return hr;

    hr = CreateIThreadPool( &impl->threadPool );
    if ( FAILED( hr ) ) return hr;

    impl->timer->lpVtbl->Initialize( impl->timer, iface, x_task_queue_port_WaitTimerOperation );

    return S_OK;
}

static XTaskQueuePortHandle WINAPI x_task_queue_port_GetHandle( IXTaskQueuePort *iface )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    return &impl->portHeader;
}

static HRESULT WINAPI x_task_queue_port_QueueItem( IXTaskQueuePort *iface, IXTaskQueuePortContext *portContext, UINT32 waitMs, PVOID callbackContext, XTaskQueueCallback *callback)
{
    HRESULT hr;
    
    XQueue queue;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, portContext %p, waitMs %d, callbackContext %p, callback %p.\n", iface, portContext, waitMs, callbackContext, callback );

    hr = iface->lpVtbl->VerifyNotTerminated( iface, portContext );
    if ( FAILED( hr ) ) return hr;

    queue.portContext = portContext;
    queue.callback = callback;
    queue.callbackContext = callbackContext;
    queue.id = impl->nextId;

    InterlockedIncrement( &impl->nextId );

    if ( waitMs == 0 )
    {
        queue.enqueueTime = 0;
        if( !(iface->lpVtbl->AppendEntry( iface, &queue )) ) return E_OUTOFMEMORY;
    } else
    {
        queue.enqueueTime = impl->timer->lpVtbl->GetAbsoluteTime( impl->timer, waitMs );
        if ( !impl->pendingQueueList_tail )
        {
            //queue list is empty
            impl->pendingQueueList_head = impl->pendingQueueList_tail = &queue;
        } else 
        {
            impl->pendingQueueList_tail->next = &queue;
            impl->pendingQueueList_tail = &queue;
        }

        while ( TRUE )
        {
            LONG64 due = InterlockedCompareExchange64( &impl->timerDue, 0, 0 ) ;
            if ( queue.enqueueTime < due )
            {
                if ( InterlockedCompareExchange64( &impl->timerDue, queue.enqueueTime, due ) == due )
                {
                    impl->timer->lpVtbl->Start( impl->timer, queue.enqueueTime );
                    break;
                }
            }
            else if ( InterlockedCompareExchange64( &impl->timerDue, due, due ) == due )
            {
                break;
            }
        }
    }

    // guard against race condition
    if ( portContext->lpVtbl->get_Status != PortStatus_Active )
    {
        iface->lpVtbl->CancelPendingEntries( iface, portContext, TRUE );
    }

    return S_OK;
}

static HRESULT WINAPI x_task_queue_port_RegisterWaitHandle( IXTaskQueuePort* iface, IXTaskQueuePortContext* portContext, HANDLE waitHandle, PVOID callbackContext, XTaskQueueCallback* callback, XTaskQueueRegistrationToken* token )
{
    WARN( "iface %p, portContext %p, waitHandle %p, callbackContext %p, callback %p, token %p not intended!\n", iface, portContext, waitHandle, callbackContext, callback, token );
    return E_NOTIMPL;
}

static VOID WINAPI x_task_queue_port_UnregisterWaitHandle( IXTaskQueuePort* iface, XTaskQueueRegistrationToken token )
{
    WARN( "iface %p, token %lld not intended!\n", iface, token.token );
    return;
}

static HRESULT WINAPI x_task_queue_port_PrepareTerminate( IXTaskQueuePort* iface, IXTaskQueuePortContext* portContext, PVOID callbackContext, XTaskQueueTerminatedCallback* callback, PVOID *outPrepareToken )
{
    XTerminateForPort *terminate;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, portContext %p, callbackContext %p, callback %p, outPrepareToken %p.\n", iface, portContext, callbackContext, callback, outPrepareToken );

    // Arguments
    if ( !outPrepareToken )
        return E_POINTER;
    if ( !callback )
        return E_INVALIDARG;

    if (!(terminate = calloc( 1, sizeof(*terminate) ))) return E_OUTOFMEMORY;

    if ( !impl->terminateList_tail )
    {
        //queue list is empty
        impl->terminateList_tail = impl->terminateList_head = terminate;
    } else 
    {
        impl->terminateList_tail->next = terminate;
        impl->terminateList_tail = terminate;
    }

    terminate->callbackContext = callbackContext;
    terminate->callback = callback;
    terminate->portContext = portContext;

    // Mark the port as canceled, but don't overwrite
    // terminating or terminated status.
    portContext->lpVtbl->SetStatus( portContext, PortStatus_Active );
    *outPrepareToken = (PVOID)terminate;

    TRACE( "created token %p\n", terminate );

    return S_OK;
}

static VOID WINAPI x_task_queue_port_CancelTermination( IXTaskQueuePort* iface, PVOID token )
{
    XTerminateForPort *terminate = (XTerminateForPort *)token;
    XTerminateForPort *current = NULL;
    XTerminateForPort *previous = NULL;
    XTerminateForPort *next = NULL;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, token %p.\n", iface, token );

    terminate->portContext->lpVtbl->SetStatus( terminate->portContext, PortStatus_Canceled );

    current = impl->terminateList_head;

    while ( current )
    {
        next = current->next;
        if ( current == terminate )
        {
            if ( previous )
                previous->next = next;
            else
                impl->terminateList_head = next;

            if ( impl->terminateList_tail == current )
                impl->terminateList_tail = previous;

            current->next = NULL;
            free( current );
        } else
        {
            previous = current;
        }
        current = next;
    }

    free ( terminate );
    return;
}

static VOID WINAPI x_task_queue_port_Terminate( IXTaskQueuePort* iface, PVOID token )
{
    XTerminateForPort *terminate = (XTerminateForPort *)token;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, token %p.\n", iface, token );

    terminate->portContext->lpVtbl->SetStatus( terminate->portContext, PortStatus_Terminating );

    iface->lpVtbl->CancelPendingEntries( iface, terminate->portContext, TRUE );

    // Are there existing suspends? AddSuspend returns
    // true if this is the first suspend added.
    if ( terminate->portContext->lpVtbl->AddSuspend( terminate->portContext ) )
    {
        iface->lpVtbl->ScheduleTermination( iface, terminate );
    }
    else
    {
        if ( !impl->pendingTerminateList_tail )
        {
            //queue list is empty
            impl->pendingTerminateList_tail = impl->pendingTerminateList_head = terminate;
        } else 
        {
            impl->pendingTerminateList_tail->next = terminate;
            impl->pendingTerminateList_tail = terminate;
        }
    }

    // Balance our add.  Note we must use ResumeTermination
    // here so we schedule the termination if this is the
    // last remove.
    iface->lpVtbl->ResumeTermination( iface, terminate->portContext );
}

static HRESULT WINAPI x_task_queue_port_Attach( IXTaskQueuePort *iface, IXTaskQueuePortContext* portContext )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, portContext %p.\n", iface, portContext );

    return impl->attachedContexts->lpVtbl->Add( impl->attachedContexts, (PVOID)portContext );
}

static VOID WINAPI x_task_queue_port_Detach( IXTaskQueuePort *iface, IXTaskQueuePortContext* portContext )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, portContext %p.\n", iface, portContext );

    iface->lpVtbl->CancelPendingEntries( iface, portContext, FALSE );
    impl->attachedContexts->lpVtbl->Remove( impl->attachedContexts, x_task_queue_port_VectorPredicateOperation, portContext );
    return;
}

static BOOLEAN WINAPI x_task_queue_port_Dispatch( IXTaskQueuePort *iface, IXTaskQueuePortContext* portContext, UINT32 timeoutInMs )
{
    BOOLEAN found = FALSE;

    TRACE( "iface %p, portContext %p, timeoutInMs %d.\n", iface, portContext, timeoutInMs );

    while ( !found )
    {
        found = iface->lpVtbl->DrainOneItem( iface );

        if ( !found && iface->lpVtbl->Wait( iface, portContext, timeoutInMs ) )
        {
            break;
        }
    }

    return found;
}

static BOOLEAN x_task_queue_port_DrainOneItem( IXTaskQueuePort *iface )
{
    BOOLEAN popped = FALSE;

    XQueue *front;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    if ( impl->suspended && impl->dispatchMode != Immediate )
    {
        return FALSE;
    }

    InterlockedIncrement( &impl->processingCallback );

    if ( !impl->queueList_head ) 
    {
        popped = FALSE;
    }
    else
    {
        front = impl->queueList_tail;
        impl->queueList_head = front->next;
        popped = TRUE;
    }

    if ( popped )
    {
        front->callback( front->callbackContext, iface->lpVtbl->IsCallCanceled( iface, front ) );
        InterlockedDecrement( &impl->processingCallback );
        WakeAllConditionVariable( &impl->cv );
        front->portContext->lpVtbl->Release( front->portContext );
        free( front );
    }
    else
    {
        InterlockedDecrement( &impl->processingCallback );
        WakeAllConditionVariable( &impl->cv );
    }

    if ( !impl->queueList_head )
    {
        iface->lpVtbl->SignalTerminations( iface );
        iface->lpVtbl->SignalQueue( iface );
    }

    return popped;
}

static BOOLEAN x_task_queue_port_Wait( IXTaskQueuePort *iface, IXTaskQueuePortContext* portContext, UINT32 timeout )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, portContext %p, timeout %d.\n", iface, portContext, timeout );

    while ( impl->suspended || ( !impl->queueList_head && !impl->terminateList_head ) )
    {
        if ( portContext->lpVtbl->get_Status( portContext ) == PortStatus_Terminated )
        {
            return FALSE;
        }

        EnterCriticalSection( &impl->cs );

        if ( !impl->signaled && !SleepConditionVariableCS( &impl->cvAny, &impl->cs, timeout ) )
        {
            return FALSE;
        }

        LeaveCriticalSection( &impl->cs );

        impl->signaled = FALSE;
    }

    return TRUE;
}

static BOOLEAN WINAPI x_task_queue_port_IsEmpty( IXTaskQueuePort *iface )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    return !impl->queueList_head && !impl->pendingQueueList_head && impl->processingCallback == 0;
}

static VOID WINAPI x_task_queue_port_WaitForUnwind( IXTaskQueuePort *iface )
{
    ULONGLONG ms;
    ULONGLONG now;
    CRITICAL_SECTION lock;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );
    
    InitializeCriticalSectionEx( &lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    lock.DebugInfo->Spare[0] = (DWORD_PTR)( __FILE__ ": xtaskqueue.cs" );

    EnterCriticalSection( &lock );

    while( InterlockedCompareExchange( &impl->processingCallback, 0, 0) != 0)
    {
        // wait for 10 ms.  We do not modify m_processingCallback under
        // the protection of a mutex because we don't want the hit of
        // taking a lock.  Therefore, we can't wait forever for the
        // cv here.  We could miss it due to a race. This API is only
        // called during task queue termination and therefore some polling
        // is OK.

        ms = 10; // 10 milliseconds
        now = GetTickCount64();
        SleepConditionVariableCS( &impl->cv, &lock, now + ms );
    }

    LeaveCriticalSection( &lock );
}

static HRESULT WINAPI x_task_queue_port_SuspendTermination( IXTaskQueuePort *iface, IXTaskQueuePortContext *portContext )
{
    HRESULT hr;

    TRACE( "iface %p, portContext %p.\n", iface, portContext );

    // guard against race condition
    portContext->lpVtbl->AddSuspend( portContext );
    hr = iface->lpVtbl->VerifyNotTerminated( iface, portContext );
    
    if ( FAILED( hr ) )
    {
        iface->lpVtbl->ResumeTermination( iface, portContext );
        return hr;
    }

    return S_OK;
}

static VOID WINAPI x_task_queue_port_ResumeTermination( IXTaskQueuePort *iface, IXTaskQueuePortContext *portContext )
{
    XTerminateForPort *previous = NULL;
    XTerminateForPort *current = NULL;
    XTerminateForPort *next = NULL;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, portContext %p.\n", iface, portContext );

    if ( portContext->lpVtbl->RemoveSuspend( portContext ) )
    {
        // Removed the last external callback.  Look for
        // parked terminations and reschedule them.

        if( !impl->pendingTerminateList_head )
            return;

        current = impl->pendingTerminateList_head;

        while ( current )
        {
            next = current->next;
            if ( current->portContext == portContext )
            {
                iface->lpVtbl->ScheduleTermination( iface, current );

                if ( previous )
                {
                    previous->next = next;
                } else
                {
                    impl->pendingTerminateList_head = next;
                }

                if ( current == impl->pendingTerminateList_tail )
                {
                    impl->pendingTerminateList_tail = previous;
                }

                current->next = NULL;
            }
            else 
            {
                previous = current;
            }

            current = next;
        }
    }

    return;
}

static VOID WINAPI x_task_queue_port_SuspendPort( IXTaskQueuePort *iface )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    InterlockedExchange( (LONG *)&impl->suspended, 1 );

    return;
}

static VOID WINAPI x_task_queue_port_ResumePort( IXTaskQueuePort *iface )
{    
    UINT32 notifyCount = 0;

    XQueue *queueEntry;
    XTerminateForPort *terminationEntry;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    queueEntry = impl->queueList_head;
    while ( queueEntry != NULL ) 
    {
        notifyCount++;
        queueEntry = queueEntry->next;
    }

    terminationEntry = impl->terminateList_head;
    while ( terminationEntry != NULL )
    {
        notifyCount++;
        terminationEntry = terminationEntry->next;
    }

    InterlockedExchange( (LONG *)&impl->suspended, 0 );

    while ( notifyCount )
    {
        iface->lpVtbl->SignalQueue( iface );
        iface->lpVtbl->NotifyItemQueued( iface );
        notifyCount--;
    }

    return;
}

static HRESULT x_task_queue_port_VerifyNotTerminated( IXTaskQueuePort *iface, IXTaskQueuePortContext *portContext )
{
    TRACE( "iface %p, portContext %p.\n", iface, portContext );

    if ( portContext->lpVtbl->get_Status( portContext ) > PortStatus_Canceled )
        return E_ABORT;

    return S_OK;
}

static BOOLEAN x_task_queue_port_IsCallCanceled( IXTaskQueuePort *iface, XQueue *entry )
{
    TRACE( "iface %p, entry %p.\n", iface, entry );
    return entry->portContext->lpVtbl->get_Status( entry->portContext ) != PortStatus_Active;
}

static BOOLEAN x_task_queue_port_AppendEntry( IXTaskQueuePort *iface, XQueue *entry )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, entry %p.\n", iface, entry );

    entry->next = NULL;
    if ( !impl->queueList_tail )
    {
        impl->queueList_head = impl->queueList_tail = entry;
    } else
    {
        impl->queueList_tail->next = entry;
        impl->queueList_tail = entry;
    }

    iface->lpVtbl->SignalQueue( iface );
    iface->lpVtbl->NotifyItemQueued( iface );

    return TRUE;
}

static VOID x_task_queue_port_CancelPendingEntries( IXTaskQueuePort *iface, IXTaskQueuePortContext* portContext, BOOLEAN appendToQueue )
{
    XQueue *current;
    XQueue *previous = NULL;
    XQueue *next;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, portContext %p, appendToQueue %d.\n", iface, portContext, appendToQueue );

    impl->timer->lpVtbl->Cancel( impl->timer );
    impl->timerDue = UINT64_MAX;

    current = impl->pendingQueueList_head;

    while ( current )
    {
        next = current->next;
        if ( current->portContext == portContext )
        {
            if ( previous )
            {
                previous->next = next;
            } else
            {
                impl->pendingQueueList_head = next;
            }

            if ( current == impl->pendingQueueList_tail )
            {
                impl->pendingQueueList_tail = previous;
            }

            current->next = NULL;

            if ( !appendToQueue || !iface->lpVtbl->AppendEntry( iface, current ) )
            {
                current->portContext->lpVtbl->Release( current->portContext );
                free( current );
            }
        }
        else 
        {
           previous = current;
        }
        
        current = next;
    }

    iface->lpVtbl->SubmitPendingCallback( iface );

    return;
}

static VOID x_task_queue_port_EraseQueue( IXTaskQueuePort *iface, XQueue *queueHead, XQueue *queueTail )
{
    XQueue *current;
    XQueue *next;

    TRACE( "iface %p, queueHead %p, queueTail %p.\n", iface, queueHead, queueTail );

    current = queueHead;

    while ( current )
    {
        next = current->next;
        free( current );
        current = next;
    }

    queueHead = queueTail = NULL;

    return;
}

static BOOLEAN x_task_queue_port_ScheduleNextPendingCallback( IXTaskQueuePort *iface, UINT64 dueTime, XQueue *dueEntry )
{
    XQueue *current;
    XQueue *previous = NULL;
    XQueue *nextItem = NULL;
    XQueue *next;

    BOOLEAN hasDueEntry = FALSE;
    BOOLEAN hasNextItem = FALSE;
    BOOLEAN removed = FALSE;
    UINT64 noDueTime = UINT64_MAX;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, dueTime %lld, dueEntry %p.\n", iface, dueTime, dueEntry );

    current = impl->pendingQueueList_head;

    if (!(nextItem = calloc( 1, sizeof(*nextItem) ))) return FALSE;

    while ( current )
    {
        next = current->next;
        removed = FALSE;
        if ( !hasDueEntry && current->enqueueTime == dueTime )
        {
            dueEntry = current;
            hasDueEntry = TRUE;

            if ( previous )
            {
                previous->next = next;
            } else
            {
                impl->pendingQueueList_head = next;
            }

            if ( current == impl->pendingQueueList_tail )
            {
                impl->pendingQueueList_tail = previous;
            }

            current->next = NULL;
            removed = TRUE;
        } else 
        {
            if ( !hasDueEntry || nextItem->enqueueTime > current->enqueueTime )
            {
                if ( hasNextItem )
                {
                    nextItem->portContext->lpVtbl->Release( nextItem->portContext );
                }

                nextItem = current;
                nextItem->portContext->lpVtbl->AddRef( nextItem->portContext );
                hasNextItem = TRUE;
            }
        }

        if ( !removed )
        {
            previous = current;
        }

        current = next;
    }

    if ( hasNextItem )
    {
        if ( nextItem->portContext->lpVtbl->get_Status( nextItem->portContext ) == PortStatus_Active )
        {
            while ( TRUE )
            {
                if ( InterlockedCompareExchange64( &impl->timerDue, 0, 0 ) == InterlockedCompareExchange64( &impl->timerDue, dueTime, nextItem->enqueueTime ) )
                {
                    impl->timer->lpVtbl->Start( impl->timer, nextItem->enqueueTime );
                    break;
                }

                dueTime = InterlockedCompareExchange64( &impl->timerDue, 0, 0 );

                if ( dueTime <= nextItem->enqueueTime )
                {
                    break;
                }
            }
        }
        else
        {
            // The port is no longer active. Pending entries are canceled
            // when the port is terminated, but if we were iterating above
            // it's possible that we removed an item while the termination was
            // being processed and it got missed.
            iface->lpVtbl->CancelPendingEntries( iface, nextItem->portContext, TRUE );
        }

        nextItem->portContext->lpVtbl->Release( nextItem->portContext );
    }
    else
    {
        if ( InterlockedCompareExchange64( &impl->timerDue, 0, 0 ) == InterlockedCompareExchange64( &impl->timerDue, dueTime, noDueTime ) )
        {
            impl->timer->lpVtbl->Cancel( impl->timer );
        }
    }

    return hasDueEntry;
}

static VOID x_task_queue_port_SubmitPendingCallback( IXTaskQueuePort *iface )
{
    XQueue *dueEntry = NULL;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );
    
    if ( iface->lpVtbl->ScheduleNextPendingCallback( iface, InterlockedCompareExchange64( &impl->timerDue, 0, 0 ), dueEntry ) )
    {
        if ( !iface->lpVtbl->AppendEntry( iface, dueEntry ) )
        {
            dueEntry->portContext->lpVtbl->Release( dueEntry->portContext );
            free( dueEntry );
        }
    }

    return;
}

// Must be called in a thread pool.
static VOID x_task_queue_port_ProcessThreadPoolCallback( IXTaskQueuePort *iface, ThreadPoolActionStatus *status )
{
    UINT32 wasProcessing;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    InterlockedIncrement( &impl->processingCallback );
    wasProcessing = InterlockedCompareExchange( &impl->processingCallback, 0, 0 );

    if ( impl->dispatchMode == SerializedThreadPool )
    {
        if ( wasProcessing == 0 )
        {
            while ( iface->lpVtbl->DrainOneItem( iface ) );
        }
    }
    else
    {
        iface->lpVtbl->DrainOneItem( iface );
    }

    InterlockedDecrement( &impl->processingCallback );
    WakeAllConditionVariable( &impl->cv );

    // Important that this comes before Release; otherwise
    // cleanup may deadlock.
    status->IActionStatus_iface.lpVtbl->Complete( &status->IActionStatus_iface );

    // When we submitted a request to the thread pool we
    // added a reference to ourself.  Balance it here. This
    // may be the final release.
    iface->lpVtbl->Release( iface );

    return;
}

static VOID x_task_queue_port_SignalQueue( IXTaskQueuePort *iface )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    if ( !impl->suspended )
    {
        EnterCriticalSection( &impl->cs );
        impl->signaled = TRUE;
        WakeAllConditionVariable( &impl->cvAny );
        LeaveCriticalSection( &impl->cs );
    }

    return;
}

static VOID x_task_queue_port_NotifyItemQueued( IXTaskQueuePort *iface )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    if ( !impl->suspended || impl->dispatchMode == Immediate)
    {
        switch (impl->dispatchMode)
        {
            case Manual:
                // nothing
                break;

            case SerializedThreadPool:
            case ThreadPool:
                // Addref before submitting to the thread pool in case we
                // are released while there there are outstanding threadpool
                // items. The threadpool does not cancel outstanding callbacks
                // on termination so we need to drain before releasing.
                iface->lpVtbl->AddRef( iface );
                
                impl->threadPool->lpVtbl->Submit( impl->threadPool );
                break;

            case Immediate:
                // We will handle this after we invoke
                // callback submitted.
                break;
        }

        impl->attachedContexts->lpVtbl->Visit( impl->attachedContexts, x_task_queue_port_VectorVisitOperation );
        // If the queue is immediate, drain the newly queued item
        // now.

        if (impl->dispatchMode == Immediate)
        {
            iface->lpVtbl->DrainOneItem( iface );
        }
    }
}

static VOID x_task_queue_port_SignalTerminations( IXTaskQueuePort *iface )
{
    XTerminateForPort *current;
    XTerminateForPort *previous = NULL;
    XTerminateForPort *next;

    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p.\n", iface );

    current = impl->terminateList_head;

    while ( current )
    {
        next = current->next;

        if ( current->portContext->lpVtbl->get_Status( current->portContext ) >= PortStatus_Terminating )
        {
            if ( previous )
            {
                previous->next = next;
            } else
            {
                impl->terminateList_head = next;
            }

            if ( current == impl->terminateList_tail )
            {
                impl->terminateList_tail = previous;
            }

            iface->lpVtbl->ScheduleTermination( iface, current );
            current->portContext->lpVtbl->SetStatus( current->portContext, PortStatus_Terminated );
            current->callback( current->callbackContext );

            free( current );
        } else {
            previous = current;
        }
        
        current = next;
    }
    
    return;
}

static VOID x_task_queue_port_ScheduleTermination( IXTaskQueuePort *iface, XTerminateForPort *entry )
{
    struct x_task_queue_port *impl = impl_from_IXTaskQueuePort( iface );

    TRACE( "iface %p, entry %p.\n", iface, entry );

    entry->next = NULL;                     // important: new tail points to NULL
    if ( !impl->terminateList_tail ) 
    {
        impl->terminateList_head = impl->terminateList_tail = entry;
    } else {
        impl->terminateList_tail->next = entry;
        impl->terminateList_tail = entry;
    }

    iface->lpVtbl->SignalTerminations( iface );
    iface->lpVtbl->SignalQueue( iface );

    return;
}

static const struct IXTaskQueuePortVtbl x_task_queue_port_vtbl =
{
    /* IUnknown methods */
    x_task_queue_port_QueryInterface,
    x_task_queue_port_AddRef,
    x_task_queue_port_Release,
    /* IXTaskQueuePort methods */
    x_task_queue_port_Initialize,
    x_task_queue_port_GetHandle,
    x_task_queue_port_QueueItem,
    x_task_queue_port_RegisterWaitHandle,
    x_task_queue_port_UnregisterWaitHandle,
    x_task_queue_port_PrepareTerminate,
    x_task_queue_port_CancelTermination,
    x_task_queue_port_Terminate,
    x_task_queue_port_Attach,
    x_task_queue_port_Detach,
    x_task_queue_port_Dispatch,
    x_task_queue_port_IsEmpty,
    x_task_queue_port_WaitForUnwind,
    x_task_queue_port_SuspendTermination,
    x_task_queue_port_ResumeTermination,
    x_task_queue_port_SuspendPort,
    x_task_queue_port_ResumePort,
    x_task_queue_port_VerifyNotTerminated,
    x_task_queue_port_IsCallCanceled,
    x_task_queue_port_AppendEntry,
    x_task_queue_port_CancelPendingEntries,
    x_task_queue_port_DrainOneItem,
    x_task_queue_port_Wait,
    x_task_queue_port_EraseQueue,
    x_task_queue_port_ScheduleNextPendingCallback,
    x_task_queue_port_SubmitPendingCallback,
    x_task_queue_port_SignalTerminations,
    x_task_queue_port_ScheduleTermination,
    x_task_queue_port_SignalQueue,
    x_task_queue_port_NotifyItemQueued,
    x_task_queue_port_ProcessThreadPoolCallback
};

static HRESULT WINAPI x_task_queue_QueryInterface( IXTaskQueue *iface, REFIID iid, void **out )
{
    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXTaskQueue ))
    {
        *out = &impl->IXTaskQueue_iface;
        impl->IXTaskQueue_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_task_queue_AddRef( IXTaskQueue *iface )
{
    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_task_queue_Release( IXTaskQueue *iface )
{
    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_task_queue_Initialize( IXTaskQueue *iface, XTaskQueuePortHandle workPort, XTaskQueuePortHandle completionPort )
{
    HRESULT hr; 

    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );
    struct x_task_queue_port_context *workContext = impl_from_IXTaskQueuePortContext( impl->workPort );
    struct x_task_queue_port_context *completionContext = impl_from_IXTaskQueuePortContext( impl->completionPort );

    TRACE( "iface %p, workPort %p, completionPort %p.\n", iface, workPort, completionPort );

    // Arguments
    if ( !workPort || !completionPort || workPort->signature != TASK_QUEUE_PORT_SIGNATURE || completionPort->signature != TASK_QUEUE_PORT_SIGNATURE  )
        return E_GAMERUNTIME_INVALID_HANDLE;

    workContext->port = workPort->headPort;
    completionContext->port = completionPort->headPort;
    workContext->source = workPort->headQueue;
    completionContext->source = completionPort->headQueue;
 
    impl->terminationData.allowed = TRUE;
    impl->allowClose = TRUE;

    hr = workContext->port->lpVtbl->Attach( workContext->port, &workContext->IXTaskQueuePortContext_iface );
    if ( FAILED( hr ) ) return hr;

    hr = completionContext->port->lpVtbl->Attach( completionContext->port, &completionContext->IXTaskQueuePortContext_iface );
    if ( FAILED( hr ) ) return hr;

    return S_OK;
}

static HRESULT WINAPI x_task_queue_InitializeOverloadPorts( IXTaskQueue *iface, XTaskQueueDispatchMode workDispatch, XTaskQueueDispatchMode completionDispatch, BOOLEAN allowTermination, BOOLEAN allowClose )
{
    HRESULT hr; 

    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );
    struct x_task_queue_port *workPort;
    struct x_task_queue_port *completionPort;
    struct x_task_queue_port_context *workContext = impl_from_IXTaskQueuePortContext( impl->workPort );
    struct x_task_queue_port_context *completionContext = impl_from_IXTaskQueuePortContext( impl->completionPort );

    TRACE( "iface %p, workDispatch %d, completionDispatch %d, allowTermination %d, allowClose %d.\n", iface, workDispatch, completionDispatch, allowTermination, allowClose );

    if (!(workPort = calloc( 1, sizeof(*workPort) ))) return E_OUTOFMEMORY;
    if (!(completionPort = calloc( 1, sizeof(*completionPort) ))) return E_OUTOFMEMORY;

    workPort->IXTaskQueuePort_iface.lpVtbl = &x_task_queue_port_vtbl;
    workPort->portHeader.signature = TASK_QUEUE_PORT_SIGNATURE;
    workPort->portHeader.headQueue = &impl->IXTaskQueue_iface;
    workPort->portHeader.headPort = &workPort->IXTaskQueuePort_iface;
    workPort->ref = 1;

    completionPort->IXTaskQueuePort_iface.lpVtbl = &x_task_queue_port_vtbl;
    completionPort->portHeader.signature = TASK_QUEUE_PORT_SIGNATURE;
    completionPort->portHeader.headQueue = &impl->IXTaskQueue_iface;
    completionPort->portHeader.headPort = &completionPort->IXTaskQueuePort_iface;
    completionPort->ref = 1;

    workContext->port = &workPort->IXTaskQueuePort_iface;
    completionContext->port = &completionPort->IXTaskQueuePort_iface;

    hr = workPort->IXTaskQueuePort_iface.lpVtbl->Initialize( &workPort->IXTaskQueuePort_iface, workDispatch );
    if ( FAILED( hr ) ) goto _CLEANUP;

    hr = completionPort->IXTaskQueuePort_iface.lpVtbl->Initialize( &completionPort->IXTaskQueuePort_iface, completionDispatch );
    if ( FAILED( hr ) ) goto _CLEANUP;

    workPort->IXTaskQueuePort_iface.lpVtbl->Attach( &workPort->IXTaskQueuePort_iface, impl->workPort );
    completionPort->IXTaskQueuePort_iface.lpVtbl->Attach( &completionPort->IXTaskQueuePort_iface, impl->completionPort );

    impl->terminationData.allowed = allowTermination;
    impl->allowClose = allowClose;

_CLEANUP:
    if ( FAILED( hr ) ) 
    {
        free( workPort );
        free( completionPort );
    }
    return hr;
}

static HRESULT WINAPI x_task_queue_GetPortContext( IXTaskQueue *iface, XTaskQueuePort port, IXTaskQueuePortContext **portContext )
{
    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );
    struct x_task_queue_port_context *workContext = impl_from_IXTaskQueuePortContext( impl->workPort );
    struct x_task_queue_port_context *completionContext = impl_from_IXTaskQueuePortContext( impl->completionPort );

    TRACE( "iface %p, port %d, portContext %p.\n", iface, port, portContext );

    // Arguments
    if ( !portContext )
        return E_POINTER;

    switch( port )
    {
        case Work:
            *portContext = &workContext->IXTaskQueuePortContext_iface;
            IUnknown_AddRef( *portContext );
            break;

        case Completion:
            *portContext = &completionContext->IXTaskQueuePortContext_iface;
            IUnknown_AddRef( *portContext );
            break;

        default:
            return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI x_task_queue_RegisterWaitHandle( IXTaskQueue *iface, XTaskQueuePort port, HANDLE waitHandle, PVOID callbackContext, XTaskQueueCallback* callback, XTaskQueueRegistrationToken* token )
{
    HRESULT hr;
    
    XTaskQueueRegistrationToken portToken;
    IXTaskQueuePortContext *portContext;
    IXTaskQueuePort *portQueue;

    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p, port %d, waitHandle %p, callbackContext %p, callback %p, token %p.\n", iface, port, waitHandle, callbackContext, callback, token );

    // Arguments
    if ( !callback || !token )
        return E_POINTER;

    hr = iface->lpVtbl->GetPortContext( iface, port, &portContext );
    if ( FAILED( hr ) ) return hr;

    portQueue = portContext->lpVtbl->get_Port( portContext );

    hr = portQueue->lpVtbl->RegisterWaitHandle( portQueue, portContext, waitHandle, callbackContext, callback, &portToken );
    if ( FAILED( hr ) ) return hr;

    hr = impl->waitRegistry->lpVtbl->Register( impl->waitRegistry, port, portToken, token );
    if ( FAILED( hr ) )
    {
        portQueue->lpVtbl->UnregisterWaitHandle( portQueue, portToken );
        return hr;
    }

    return S_OK;
}

static VOID WINAPI x_task_queue_UnregisterWaitHandle( IXTaskQueue *iface, XTaskQueueRegistrationToken token )
{
    HRESULT hr;

    IXTaskQueuePortContext *portContext;
    IXTaskQueuePort *queuePort;
    XTaskQueuePort port;
    XTaskQueueRegistrationToken portToken;

    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p, token %lld.\n", iface, token.token );

    hr = impl->waitRegistry->lpVtbl->Unregister( impl->waitRegistry, token, &port, &portToken );
    if ( FAILED( hr ) ) return;

    hr = iface->lpVtbl->GetPortContext( iface, port, &portContext );
    if ( FAILED( hr ) ) return;

    queuePort = portContext->lpVtbl->get_Port( portContext );
    queuePort->lpVtbl->UnregisterWaitHandle( queuePort, portToken );

    return;
}

static HRESULT WINAPI x_task_queue_RegisterSubmitCallback( IXTaskQueue *iface, PVOID context, XTaskQueueMonitorCallback* callback, XTaskQueueRegistrationToken* token )
{
    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p, context %p, callback %p, token %p.\n", iface, context, callback, token );

    return impl->callbackSubmitted->lpVtbl->Register( impl->callbackSubmitted, context, callback, token );
}

static VOID WINAPI x_task_queue_UnregisterSubmitCallback( IXTaskQueue *iface, XTaskQueueRegistrationToken token )
{
    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p, token %lld.\n", iface, token.token );

    impl->callbackSubmitted->lpVtbl->Unregister( impl->callbackSubmitted, token );
}

static BOOLEAN WINAPI x_task_queue_get_CanTerminate( IXTaskQueue *iface )
{
    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p.\n", iface );

    return impl->terminationData.allowed;
}

static BOOLEAN WINAPI x_task_queue_get_CanClose( IXTaskQueue *iface )
{
    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p.\n", iface );

    return impl->allowClose;
}

static HRESULT WINAPI x_task_queue_Terminate( IXTaskQueue* iface, BOOLEAN wait, PVOID callbackContext, XTaskQueueTerminatedCallback* callback )
{
    HRESULT hr;
    PVOID workToken;

    XTerminate *terminate;
    IXTaskQueuePort *queuePort;

    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p, wait %d, callbackContext %p, callback %p.\n", iface, wait, callbackContext, callback );

    if (!(terminate = calloc( 1, sizeof(*terminate) ))) return E_OUTOFMEMORY;

    if ( !impl->terminationData.allowed )
        return E_ACCESSDENIED;

    terminate->owner = iface;
    terminate->level = TerminationLevel_Work;
    terminate->context = callbackContext;
    terminate->callback = callback;

    queuePort = impl->workPort->lpVtbl->get_Port( impl->workPort );
    hr = queuePort->lpVtbl->PrepareTerminate( queuePort, impl->workPort, (PVOID)terminate, iface->lpVtbl->OnTerminationCallback, &workToken );
    if ( FAILED( hr ) ) goto _CLEANUP;

    queuePort = impl->completionPort->lpVtbl->get_Port( impl->completionPort );
    hr = queuePort->lpVtbl->PrepareTerminate( queuePort, impl->workPort, (PVOID)terminate, iface->lpVtbl->OnTerminationCallback, &terminate->completionPortToken );
    if ( FAILED( hr ) )
    {
        queuePort->lpVtbl->CancelTermination( queuePort, workToken );
        goto _CLEANUP;
    }

    iface->lpVtbl->AddRef( iface );
    if ( wait ) iface->lpVtbl->AddRef( iface ); // guard against de-ref
    queuePort = impl->workPort->lpVtbl->get_Port( impl->workPort );
    queuePort->lpVtbl->Terminate( queuePort, workToken );

    if ( wait )
    {
        EnterCriticalSection( &impl->terminationData.cs );
        while( !impl->terminationData.terminated )
        {
            SleepConditionVariableCS( &impl->terminationData.cv, &impl->terminationData.cs, INFINITE );
        }
        LeaveCriticalSection( &impl->terminationData.cs );

        queuePort = impl->workPort->lpVtbl->get_Port( impl->workPort );
        queuePort->lpVtbl->WaitForUnwind( queuePort );

        queuePort = impl->completionPort->lpVtbl->get_Port( impl->completionPort );
        queuePort->lpVtbl->WaitForUnwind( queuePort );

        iface->lpVtbl->Release( iface );
    }

_CLEANUP:
    if ( FAILED( hr ) )
        free( terminate );

    return hr;
}

static VOID x_task_queue_RundownObject( IXTaskQueue* iface )
{
    IXTaskQueuePort *queuePort;

    struct x_task_queue *impl = impl_from_IXTaskQueue( iface );

    TRACE( "iface %p.\n", iface );
    
    impl->workPort->lpVtbl->SetStatus( impl->workPort, PortStatus_Terminated );
    impl->completionPort->lpVtbl->SetStatus( impl->completionPort, PortStatus_Terminated );

    queuePort = impl->workPort->lpVtbl->get_Port( impl->workPort );

    if (queuePort != nullptr)
        queuePort->lpVtbl->Detach( queuePort, impl->workPort );

    queuePort = impl->workPort->lpVtbl->get_Port( impl->workPort );

    if (queuePort != nullptr)
       queuePort->lpVtbl->Detach( queuePort, impl->completionPort );

    return;
}

static VOID x_task_queue_OnTerminationCallback( PVOID context )
{
    XTerminate *terminate = (XTerminate *)context;
    IXTaskQueuePort *queuePort;

    struct x_task_queue *impl = impl_from_IXTaskQueue( terminate->owner );

    TRACE( "iface %p, context %p.\n", terminate->owner, context );

    switch ( terminate->level )
    {
        case TerminationLevel_Work:
            terminate->level = TerminationLevel_Completion;
            queuePort = impl->completionPort->lpVtbl->get_Port( impl->workPort );
            queuePort->lpVtbl->Terminate( queuePort, terminate->completionPortToken );
            break;

        case TerminationLevel_Completion:
            if ( terminate->callback != NULL )
            {
                terminate->callback( terminate->context );
            }

            EnterCriticalSection( &impl->terminationData.cs );
            impl->terminationData.terminated = true;
            WakeAllConditionVariable( &impl->terminationData.cv );
            LeaveCriticalSection( &impl->terminationData.cs );

            terminate->owner->lpVtbl->Release( terminate->owner );
            free( terminate );
            break;

        case TerminationLevel_None:
            break;
    }
}

static const struct IXTaskQueueVtbl x_task_queue_vtbl =
{
    /* IUnknown methods */
    x_task_queue_QueryInterface,
    x_task_queue_AddRef,
    x_task_queue_Release,
    /* IXTaskQueue methods */
    x_task_queue_Initialize,
    x_task_queue_InitializeOverloadPorts,
    x_task_queue_GetPortContext,
    x_task_queue_RegisterWaitHandle,
    x_task_queue_UnregisterWaitHandle,
    x_task_queue_RegisterSubmitCallback,
    x_task_queue_UnregisterSubmitCallback,
    x_task_queue_get_CanTerminate,
    x_task_queue_get_CanClose,
    x_task_queue_Terminate,
    x_task_queue_RundownObject,
    x_task_queue_OnTerminationCallback
};

static HRESULT CreateTaskQueueHandle( IXTaskQueue* impl, XTaskQueueHandle* queue )
{
    XTaskQueueObject *taskObject = NULL;

    TRACE( "impl %p, queue %p.\n", impl, queue );
    
    *queue = NULL;

    if (!(taskObject = calloc( 1, sizeof(*taskObject) ))) return E_OUTOFMEMORY;

    taskObject->signature = TASK_QUEUE_SIGNATURE;
    taskObject->headQueue = impl;

    impl->lpVtbl->AddRef( impl );

    *queue = taskObject;

    TRACE( "created taskObject %p.\n", taskObject );

    return S_OK;
}

HRESULT XTaskQueueCreate( XTaskQueueDispatchMode workDispatchMode, XTaskQueueDispatchMode completionDispatchMode, XTaskQueueHandle* queue ) 
{
    HRESULT hr;

    struct x_task_queue *impl = NULL;
    struct x_task_queue_monitor_callback *monitor_callback_impl = NULL;
    struct x_task_queue_port_context *workContext = NULL;
    struct x_task_queue_port_context *completionContext = NULL;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    if (!(monitor_callback_impl = calloc( 1, sizeof(*monitor_callback_impl) ))) return E_OUTOFMEMORY;
    if (!(workContext = calloc( 1, sizeof(*workContext) ))) return E_OUTOFMEMORY;
    if (!(completionContext = calloc( 1, sizeof(*completionContext) ))) return E_OUTOFMEMORY;

    impl->IXTaskQueue_iface.lpVtbl = &x_task_queue_vtbl;
    impl->queueHeader.signature = TASK_QUEUE_SIGNATURE;
    impl->queueHeader.headQueue = &impl->IXTaskQueue_iface;

    monitor_callback_impl->IXTaskQueueMonitorCallback_iface.lpVtbl = &x_task_queue_monitor_callback_vtbl;
    InitializeCriticalSection( &monitor_callback_impl->cs );
    monitor_callback_impl->queue = &impl->queueHeader;
    monitor_callback_impl->ref = 1;

    impl->callbackSubmitted = &monitor_callback_impl->IXTaskQueueMonitorCallback_iface;
    impl->ref = 1;

    workContext->IXTaskQueuePortContext_iface.lpVtbl = &x_task_queue_port_context_vtbl;
    workContext->callbackSubmitted = impl->callbackSubmitted;
    workContext->queue = &impl->IXTaskQueue_iface;
    workContext->type = Work;
    workContext->ref = 1;

    impl->workPort = &workContext->IXTaskQueuePortContext_iface;

    completionContext->IXTaskQueuePortContext_iface.lpVtbl = &x_task_queue_port_context_vtbl;
    completionContext->callbackSubmitted = impl->callbackSubmitted;
    completionContext->queue = &impl->IXTaskQueue_iface;
    completionContext->type = Completion;
    completionContext->ref = 1;

    impl->completionPort = &completionContext->IXTaskQueuePortContext_iface;

    *queue = NULL;

    hr = impl->IXTaskQueue_iface.lpVtbl->InitializeOverloadPorts( &impl->IXTaskQueue_iface, workDispatchMode, completionDispatchMode, TRUE, TRUE );
    if ( FAILED( hr ) ) goto _CLEANUP;

    hr = CreateTaskQueueHandle( &impl->IXTaskQueue_iface, queue );

_CLEANUP:
    if ( FAILED( hr ) ) 
        free( impl );

    return S_OK;
}