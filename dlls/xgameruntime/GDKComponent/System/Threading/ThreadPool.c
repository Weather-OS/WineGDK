/*
 * Thread Pool Implementation
 *  From https://github.com/microsoft/libHttpClient
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

#include "ThreadPool.h"
#include <winternl.h>

WINE_DEFAULT_DEBUG_CHANNEL(xtaskqueue);

static inline struct thread_pool *impl_from_IThreadPool( IThreadPool *iface )
{
    return CONTAINING_RECORD( iface, struct thread_pool, IThreadPool_iface );
}

static inline struct ThreadPoolActionStatus *impl_from_IActionStatus( IActionStatus *iface )
{
    return CONTAINING_RECORD( iface, struct ThreadPoolActionStatus, IActionStatus_iface );
}

static VOID WINAPI ThreadPoolActionStatus_Complete( IActionStatus *iface )
{
    struct ThreadPoolActionStatus *impl = impl_from_IActionStatus( iface );

    TRACE( "iface %p.\n", iface );

    impl->IsComplete = TRUE;
    DisassociateCurrentThreadFromCallback( impl->instance );

    return;
}

static VOID WINAPI ThreadPoolActionStatus_MayRunLong( IActionStatus *iface )
{
    struct ThreadPoolActionStatus *impl = impl_from_IActionStatus( iface );

    TRACE( "iface %p.\n", iface );

    if ( !impl->longRunning )
    {
        impl->longRunning = TRUE;
        TpCallbackMayRunLong( impl->instance );
    }

    return;
}

const struct IActionStatusVtbl ThreadPoolActionStatus_vtbl =
{
    /* IActionStatus methods */
    ThreadPoolActionStatus_Complete,
    ThreadPoolActionStatus_MayRunLong
};

static void CALLBACK TPCallback( PTP_CALLBACK_INSTANCE instance, void* context, PTP_WORK work ) 
{
    struct thread_pool *impl = impl_from_IThreadPool( (IThreadPool *)context );
    struct ThreadPoolActionStatus *status;

    // ActionStatus offers a way for the call to
    // provide the threadpool of its status. It
    // can mark the call complete to indicate 
    // all portions of the call have finished
    // and it is safe to release the
    // thread pool, even if the callback has
    // not totally unwound.  This is neccessary
    // to allow users to close a task queue from
    // within a callback.  Task queue guards with an 
    // extra ref to ensure a safe point where 
    // member state is no longer accessed, but the
    // final release does need to wait on outstanding
    // calls.
    //
    // A call can also mark itself that it may run
    // long, which is often the case for work callbacks
    // in an async call.  This guides the thread pool
    // to allow more threads to be created if needed.

    //ActionStatusImpl status(pthis, instance);
    if (!(status = calloc( 1, sizeof(*status) ))) return;

    status->IActionStatus_iface.lpVtbl = &ThreadPoolActionStatus_vtbl;
    status->owner = (IThreadPool *)context;
    status->instance = instance;

    impl->IThreadPool_iface.lpVtbl->AddRef( &impl->IThreadPool_iface );
    impl->callback( impl->context, status );

    if ( !status->IsComplete )
    {
        status->IActionStatus_iface.lpVtbl->Complete( &status->IActionStatus_iface );
    }

    impl->IThreadPool_iface.lpVtbl->Release( &impl->IThreadPool_iface ); // May delete this
}

static HRESULT WINAPI thread_pool_QueryInterface( IThreadPool *iface, REFIID iid, void **out )
{
    struct thread_pool *impl = impl_from_IThreadPool( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IThreadPool ))
    {
        *out = &impl->IThreadPool_iface;
        impl->IThreadPool_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI thread_pool_AddRef( IThreadPool *iface )
{
    struct thread_pool *impl = impl_from_IThreadPool( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    if ( !ref )
        free( impl );
    return ref;
}

static ULONG WINAPI thread_pool_Release( IThreadPool *iface )
{
    struct thread_pool *impl = impl_from_IThreadPool( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI thread_pool_Initialize( IThreadPool *iface, PVOID context, ThreadPoolCallback callback )
{
    struct thread_pool *impl = impl_from_IThreadPool( iface );

    TRACE( "iface %p, context %p, callback %p.\n", iface, context, callback );

    impl->context = context;
    impl->callback = callback;
    impl->work = CreateThreadpoolWork( TPCallback, iface, NULL );

    if ( !impl->work )
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;
}

static VOID WINAPI thread_pool_Terminate( IThreadPool *iface )
{
    struct thread_pool *impl = impl_from_IThreadPool( iface );

    TRACE( "iface %p.\n", iface );

    if ( !impl->work )
    {
        WaitForThreadpoolWorkCallbacks( impl->work, FALSE );
        CloseThreadpoolWork( impl->work );
        impl->work = NULL;
    }

    iface->lpVtbl->Release( iface );

    return;
}

static VOID WINAPI thread_pool_Submit( IThreadPool *iface )
{
    struct thread_pool *impl = impl_from_IThreadPool( iface );

    TRACE( "iface %p.\n", iface );

    SubmitThreadpoolWork( impl->work );

    return;
}

static const struct IThreadPoolVtbl thread_pool_vtbl =
{
    /* IUnknown methods */
    thread_pool_QueryInterface,
    thread_pool_AddRef,
    thread_pool_Release,
    thread_pool_Initialize,
    thread_pool_Terminate,
    thread_pool_Submit
};

HRESULT CreateIThreadPool( IThreadPool **out )
{
    struct thread_pool *impl;

    TRACE( "out %p.\n", out );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IThreadPool_iface.lpVtbl = &thread_pool_vtbl;
    impl->ref = 1;
    
    *out = &impl->IThreadPool_iface;
    return S_OK;
}