/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XThread, XAsync and XTask
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

#include "XThreading.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_threading *impl_from_IXThreadingImpl( IXThreadingImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_threading, IXThreadingImpl_iface );
}

static HRESULT WINAPI x_threading_QueryInterface( IXThreadingImpl *iface, REFIID iid, void **out )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXThreadingImpl ))
    {
        *out = &impl->IXThreadingImpl_iface;
        impl->IXThreadingImpl_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_threading_AddRef( IXThreadingImpl *iface )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_threading_Release( IXThreadingImpl *iface )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

/* --- XAsync --- */

static HRESULT WINAPI x_threading_XAsyncGetStatus( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, boolean wait )
{
    struct x_async_work *impl;

    TRACE( "iface %p, asyncBlock %p, wait %d\n", iface, asyncBlock, wait );

    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( impl == NULL )
    {
        ERR( "called from an invalid block!\n" );
        return E_INVALIDARG;
    }

    if ( wait )
    {
        WaitForThreadpoolWorkCallbacks( impl->async_run_work, FALSE );
        /* impl->status will be set by the thread */
    }
    return impl->status;
}

static HRESULT WINAPI x_threading_XAsyncGetResultSize( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, SIZE_T *bufferSize )
{
    struct x_async_work *impl;

    TRACE( "iface %p, asyncBlock %p, bufferSize %p\n", iface, asyncBlock, bufferSize );

    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( impl->status != S_OK ) return impl->status;
    
    *bufferSize = impl->provider.data->bufferSize;

    return S_OK;
}

static HRESULT WINAPI x_threading_XAsyncCancel( IXThreadingImpl *iface, XAsyncBlock *asyncBlock )
{
    struct x_async_work *impl;

    TRACE( "iface %p, asyncBlock %p\n", iface, asyncBlock );

    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( impl == NULL )
    {
        ERR( "called from an invalid block!\n" );
        return E_INVALIDARG;
    }

    if ( impl->status == S_OK ) return S_OK;
    impl->provider.operation = Cancel;

    if (!( impl->async_run_work = CreateThreadpoolWork( XTPCallback, &impl->IWineAsyncWorkImpl_iface, NULL ) ))
        return HRESULT_FROM_WIN32( GetLastError() );

    InitializeCriticalSectionEx( &impl->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)( __FILE__ ": xasync.cs" );

    impl->status = E_PENDING;

    SubmitThreadpoolWork( impl->async_run_work );

    return S_OK;
}

static HRESULT WINAPI x_threading_XAsyncRun( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, XAsyncWork *work )
{
    FIXME( "iface %p, asyncBlock %p stub!\n", iface, asyncBlock );
    return E_NOTIMPL;
}

/* --- XAsyncProvider --- */

static HRESULT WINAPI x_threading_XAsyncBegin(IXThreadingImpl* iface, XAsyncBlock* asyncBlock, PVOID context, const PVOID identity, LPCSTR identityName, XAsyncProviderCallback* provider)
{
    struct x_async_work *impl;
    struct x_async_provider newProvider;

    TRACE( "iface %p, context %p, identity %p, identityName %s, provider %p\n", iface, context, identity, identityName, provider );

    XInitializeBlock( asyncBlock );
    impl = impl_from_XAsyncBlock( asyncBlock );

    if (!(newProvider.data = calloc( 1, sizeof(*newProvider.data) ))) return E_OUTOFMEMORY;

    newProvider.callback = provider;
    newProvider.data->async = asyncBlock;
    newProvider.data->context = context;
    newProvider.identity = identity;
    newProvider.identityName = identityName;
    newProvider.operation = Begin;

    impl->provider = newProvider;

    if (!( impl->async_run_work = CreateThreadpoolWork( XTPCallback, &impl->IWineAsyncWorkImpl_iface, NULL ) ))
    {
        free( newProvider.data );
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    InitializeCriticalSectionEx( &impl->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)( __FILE__ ": xasync.cs" );

    SubmitThreadpoolWork( impl->async_run_work );

    return S_OK;
}

static HRESULT WINAPI __PADDING__( IXThreadingImpl* iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XAsyncSchedule( IXThreadingImpl* iface, XAsyncBlock* asyncBlock, UINT32 delayInMs )
{
    struct x_async_work *impl;

    TRACE( "iface %p, asyncBlock %p, delayInMs %d\n", iface, asyncBlock, delayInMs );

    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( impl == NULL )
    {
        ERR( "called from an invalid block!\n" );
        return E_INVALIDARG;
    }

    if ( FAILED( impl->status ) ) return impl->status;
    impl->provider.operation = DoWork;
    impl->provider.workDelay = delayInMs;

    if (!( impl->async_run_work = CreateThreadpoolWork( XTPCallback, &impl->IWineAsyncWorkImpl_iface, NULL ) ))
        return HRESULT_FROM_WIN32( GetLastError() );

    InitializeCriticalSectionEx( &impl->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)( __FILE__ ": xasync.cs" );

    SubmitThreadpoolWork( impl->async_run_work );

    return S_OK;
}

static VOID WINAPI x_threading_XAsyncComplete( IXThreadingImpl* iface, XAsyncBlock* asyncBlock, HRESULT result, SIZE_T requiredBufferSize )
{
    struct x_async_work *impl;

    TRACE( "iface %p, asyncBlock %p, result %#lx, requiredBufferSize %llu\n", iface, asyncBlock, result, requiredBufferSize );

    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( impl == NULL )
    {
        ERR( "called from an invalid block!\n" );
        return;
    }

    impl->status = result;
    impl->provider.data->bufferSize = requiredBufferSize;

    if ( SUCCEEDED( result ) )
        impl->provider.data->buffer = malloc( requiredBufferSize );

    // invoke the completion routine
    if ( impl->threadBlock->callback )
        impl->threadBlock->callback( asyncBlock );


    return;
}

static HRESULT WINAPI x_threading_XAsyncGetResult( IXThreadingImpl* iface, XAsyncBlock* asyncBlock, const PVOID identity, SIZE_T bufferSize, PVOID buffer, SIZE_T* bufferUsed )
{
    struct x_async_work *impl;

    HRESULT hr;

    TRACE( "iface %p, asyncBlock %p, identity %p, bufferSize %llu, buffer %p, bufferUsed %p\n", iface, asyncBlock, identity, bufferSize, buffer, bufferUsed );

    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( impl == NULL )
    {
        ERR( "called from an invalid block!\n" );
        return E_INVALIDARG;
    }

    if ( FAILED( impl->status ) ) return impl->status;

    if ( !impl->provider.data->buffer ) return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    if ( impl->provider.data->bufferSize > bufferSize ) return E_BOUNDS;
    if ( identity != impl->provider.identity ) return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

    if ( bufferUsed )
        *bufferUsed = impl->provider.data->bufferSize;
    
    impl->provider.operation = GetResult;

    // GetResult is not asynchronous
    hr = impl->provider.callback( GetResult, impl->provider.data );
    if ( FAILED( hr ) ) return hr;

    if( buffer )
        memcpy( buffer, impl->provider.data->buffer, impl->provider.data->bufferSize );

    // Microsoft is very vague about when Cleanup is invoked. 
    // We'll invoke it here just to be safe.

    hr = impl->provider.callback( Cleanup, impl->provider.data );

    return hr;
}

/* --- XTaskQueue --- */

static HRESULT WINAPI x_threading_XTaskQueueCreate( IXThreadingImpl* iface, XTaskQueueDispatchMode workDispatchMode, XTaskQueueDispatchMode completionDispatchMode, XTaskQueueHandle* queue )
{
    FIXME( "iface %p, workDispatchMode %d, completionDispatchMode %d, queue %p stub!\n", iface, workDispatchMode, completionDispatchMode, queue );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueCreateComposite( IXThreadingImpl* iface, XTaskQueuePortHandle workPort, XTaskQueuePortHandle completionPort, XTaskQueueHandle* queue )
{
    FIXME( "iface %p, workPort %p, completionPort %p, queue %p stub!\n", iface, workPort, completionPort, queue );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueGetPort( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, XTaskQueuePortHandle* portHandle )
{
    FIXME( "iface %p, queue %p, port %d, portHandle %p stub!\n", iface, queue, port, portHandle );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueDuplicateHandle( IXThreadingImpl* iface, XTaskQueueHandle queueHandle, XTaskQueueHandle* duplicatedHandle )
{
    FIXME( "iface %p, queueHandle %p, duplicatedHandle %p stub!\n", iface, queueHandle, duplicatedHandle );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_threading_XTaskQueueDispatch( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, uint32_t timeoutInMs )
{
    FIXME( "iface %p, queue %p, port %d, timeoutInMs %d stub!\n", iface, queue, port, timeoutInMs );
    return FALSE;
}

static VOID WINAPI x_threading_XTaskQueueCloseHandle( IXThreadingImpl* iface, XTaskQueueHandle queue )
{
    FIXME( "iface %p, queue %p stub!\n", iface, queue );
    /* no-op return */
}

static HRESULT WINAPI x_threading_XTaskQueueSubmitCallback( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, PVOID callbackContext, XTaskQueueCallback* callback )
{
    FIXME( "iface %p, queue %p, port %d, callbackContext %p, callback %p stub!\n", iface, queue, port, callbackContext, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueSubmitDelayedCallback( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, uint32_t delayMs, PVOID callbackContext, XTaskQueueCallback* callback )
{
    FIXME( "iface %p, queue %p, port %d, delayMs %d, callbackContext %p, callback %p stub!\n", iface, queue, port, delayMs, callbackContext, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueRegisterWaiter( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, HANDLE waitHandle, PVOID callbackContext, XTaskQueueCallback* callback, XTaskQueueRegistrationToken* token )
{
    FIXME( "iface %p, queue %p, port %d, waitHandle %p, callbackContext %p, callback %p, token %p stub!\n", iface, queue, port, waitHandle, callbackContext, callback, token );
    return E_NOTIMPL;
}

static VOID WINAPI x_threading_XTaskQueueUnregisterWaiter( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueueRegistrationToken token )
{
    FIXME( "iface %p, queue %p, token %p stub!\n", iface, queue, &token );
    /* no-op return */
}

static HRESULT WINAPI x_threading_XTaskQueueTerminate( IXThreadingImpl* iface, XTaskQueueHandle queue, BOOLEAN wait, PVOID callbackContext, XTaskQueueTerminatedCallback* callback )
{
    FIXME( "iface %p, queue %p, wait %d, callbackContext %p, callback %p stub!\n", iface, queue, wait, callbackContext, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XTaskQueueRegisterMonitor( IXThreadingImpl* iface, XTaskQueueHandle queue, PVOID callbackContext, XTaskQueueMonitorCallback* callback, XTaskQueueRegistrationToken* token )
{
    FIXME( "iface %p, queue %p, callbackContext %p, callback %p, token %p stub!\n", iface, queue, callbackContext, callback, token );
    return E_NOTIMPL;
}

static VOID WINAPI x_threading_XTaskQueueUnregisterMonitor( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueueRegistrationToken token )
{
    FIXME( "iface %p, queue %p, token %p stub!\n", iface, queue, &token );
    /* no-op return */
}

static BOOLEAN WINAPI x_threading_XTaskQueueGetCurrentProcessTaskQueue( IXThreadingImpl* iface, XTaskQueueHandle* queue )
{
    FIXME( "iface %p, queue %p stub!\n", iface, queue );
    return FALSE;
}

static VOID WINAPI x_threading_XTaskQueueSetCurrentProcessTaskQueue( IXThreadingImpl* iface, XTaskQueueHandle queue )
{
    FIXME( "iface %p, queue %p stub!\n", iface, queue );
    /* no-op return */
}

static HRESULT WINAPI x_threading_XThreadSetTimeSensitive( IXThreadingImpl* iface, BOOLEAN isTimeSensitiveThread )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    TRACE( "iface %p, isTimeSensitiveThread %d.\n", iface, isTimeSensitiveThread );
    impl->isTimeSensitiveThread = isTimeSensitiveThread;
    return S_OK;
}

static VOID WINAPI x_threading_XThreadAssertNotTimeSensitive( IXThreadingImpl* iface )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    TRACE( "iface %p.\n", iface );
    if ( impl->isTimeSensitiveThread && IsDebuggerPresent() )
        DebugBreak();
    /* no-op return */
}

static BOOLEAN WINAPI x_threading_XThreadIsTimeSensitive( IXThreadingImpl* iface )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    TRACE( "iface %p.\n", iface );
    return impl->isTimeSensitiveThread;
}

static const struct IXThreadingImplVtbl x_threading_vtbl =
{
    x_threading_QueryInterface,
    x_threading_AddRef,
    x_threading_Release,
    /* IXThreadingImpl methods */
    x_threading_XAsyncGetStatus,
    x_threading_XAsyncGetResultSize,
    x_threading_XAsyncCancel,
    x_threading_XAsyncRun,
    x_threading_XAsyncBegin,
    __PADDING__,
    x_threading_XAsyncSchedule,
    x_threading_XAsyncComplete,
    x_threading_XAsyncGetResult,
    x_threading_XTaskQueueCreate,
    x_threading_XTaskQueueCreateComposite,
    x_threading_XTaskQueueGetPort,
    x_threading_XTaskQueueDuplicateHandle,
    x_threading_XTaskQueueDispatch,
    x_threading_XTaskQueueCloseHandle,
    x_threading_XTaskQueueSubmitCallback,
    x_threading_XTaskQueueSubmitDelayedCallback,
    x_threading_XTaskQueueRegisterWaiter,
    x_threading_XTaskQueueUnregisterWaiter,
    x_threading_XTaskQueueTerminate,
    x_threading_XTaskQueueRegisterMonitor,
    x_threading_XTaskQueueUnregisterMonitor,
    x_threading_XTaskQueueGetCurrentProcessTaskQueue,
    x_threading_XTaskQueueSetCurrentProcessTaskQueue,
    x_threading_XThreadSetTimeSensitive,
    __PADDING__,
    x_threading_XThreadAssertNotTimeSensitive,
    x_threading_XThreadIsTimeSensitive
};

static struct x_threading x_threading =
{
    {&x_threading_vtbl},
    0,
};

IXThreadingImpl *x_threading_impl = &x_threading.IXThreadingImpl_iface;