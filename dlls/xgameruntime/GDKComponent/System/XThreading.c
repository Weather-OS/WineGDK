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
        FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XAsyncGetResultSize( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, SIZE_T *bufferSize )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XAsyncCancel( IXThreadingImpl *iface, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XAsyncRun( IXThreadingImpl *iface, XAsyncBlock *asyncBlock, XAsyncWork *work )
{
    FIXME( "iface %p, asyncBlock %p stub!\n", iface, asyncBlock );
    return E_NOTIMPL;
}

/* --- XAsyncProvider --- */

static HRESULT WINAPI x_threading_XAsyncBegin(IXThreadingImpl* iface, XAsyncBlock* asyncBlock, PVOID context, const PVOID identity, LPCSTR identityName, XAsyncProviderCallback* provider)
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI __PADDING__( IXThreadingImpl* iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_threading_XAsyncSchedule( IXThreadingImpl* iface, XAsyncBlock* asyncBlock, UINT32 delayInMs )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static VOID WINAPI x_threading_XAsyncComplete( IXThreadingImpl* iface, XAsyncBlock* asyncBlock, HRESULT result, SIZE_T requiredBufferSize )
{
    FIXME( "iface %p stub!\n", iface );
    return;
}

static HRESULT WINAPI x_threading_XAsyncGetResult( IXThreadingImpl* iface, XAsyncBlock* asyncBlock, const PVOID identity, SIZE_T bufferSize, PVOID buffer, SIZE_T* bufferUsed )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}


/* --- XTaskQueue --- */

static HRESULT WINAPI x_threading_XTaskQueueCreate( IXThreadingImpl* iface, XTaskQueueDispatchMode workDispatchMode, XTaskQueueDispatchMode completionDispatchMode, XTaskQueueHandle* queue )
{
    TRACE( "iface %p, workDispatchMode %d, completionDispatchMode %d, queue %p.\n", iface, workDispatchMode, completionDispatchMode, queue );
    return XTaskQueueCreate( workDispatchMode, completionDispatchMode, queue );
}

static HRESULT WINAPI x_threading_XTaskQueueCreateComposite( IXThreadingImpl* iface, XTaskQueuePortHandle workPort, XTaskQueuePortHandle completionPort, XTaskQueueHandle* queue )
{
    TRACE( "iface %p, workPort %p, completionPort %p, queue %p.\n", iface, workPort, completionPort, queue );
    return XTaskQueueCreateComposite( workPort, completionPort, queue );
}

static HRESULT WINAPI x_threading_XTaskQueueGetPort( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, XTaskQueuePortHandle* portHandle )
{
    TRACE( "iface %p, queue %p, port %d, portHandle %p.\n", iface, queue, port, portHandle );
    return XTaskQueueGetPort( queue, port, portHandle );
}

static HRESULT WINAPI x_threading_XTaskQueueDuplicateHandle( IXThreadingImpl* iface, XTaskQueueHandle queueHandle, XTaskQueueHandle* duplicatedHandle )
{
    TRACE( "iface %p, queueHandle %p, duplicatedHandle %p.\n", iface, queueHandle, duplicatedHandle );
    return XTaskQueueDuplicateHandle( queueHandle, duplicatedHandle );
}

static BOOLEAN WINAPI x_threading_XTaskQueueDispatch( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, uint32_t timeoutInMs )
{
    TRACE( "iface %p, queue %p, port %d, timeoutInMs %d.\n", iface, queue, port, timeoutInMs );
    return XTaskQueueDispatch( queue, port, timeoutInMs );
}

static VOID WINAPI x_threading_XTaskQueueCloseHandle( IXThreadingImpl* iface, XTaskQueueHandle queue )
{
    TRACE( "iface %p, queue %p.\n", iface, queue );
    XTaskQueueCloseHandle( queue );
    /* no-op return */
}

static HRESULT WINAPI x_threading_XTaskQueueSubmitCallback( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, PVOID callbackContext, XTaskQueueCallback* callback )
{
    TRACE( "iface %p, queue %p, port %d, callbackContext %p, callback %p.\n", iface, queue, port, callbackContext, callback );
    return XTaskQueueSubmitDelayedCallback( queue, port, 0, callback, callbackContext );
}

static HRESULT WINAPI x_threading_XTaskQueueSubmitDelayedCallback( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueuePort port, uint32_t delayMs, PVOID callbackContext, XTaskQueueCallback* callback )
{
    TRACE( "iface %p, queue %p, port %d, delayMs %d, callbackContext %p, callback %p.\n", iface, queue, port, delayMs, callbackContext, callback );
    return XTaskQueueSubmitDelayedCallback( queue, port, delayMs, callback, callbackContext );
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
    TRACE( "iface %p, queue %p, wait %d, callbackContext %p, callback %p.\n", iface, queue, wait, callbackContext, callback );
    return XTaskQueueTerminate( queue, wait, callbackContext, callback );
}

static HRESULT WINAPI x_threading_XTaskQueueRegisterMonitor( IXThreadingImpl* iface, XTaskQueueHandle queue, PVOID callbackContext, XTaskQueueMonitorCallback* callback, XTaskQueueRegistrationToken* token )
{
    TRACE( "iface %p, queue %p, callbackContext %p, callback %p, token %p.\n", iface, queue, callbackContext, callback, token );
    return XTaskQueueRegisterMonitor( queue, callbackContext, callback, token );
}

static VOID WINAPI x_threading_XTaskQueueUnregisterMonitor( IXThreadingImpl* iface, XTaskQueueHandle queue, XTaskQueueRegistrationToken token )
{
    FIXME( "iface %p, queue %p, token %p stub!\n", iface, queue, &token );
    /* no-op return */
}

static BOOLEAN WINAPI x_threading_XTaskQueueGetCurrentProcessTaskQueue( IXThreadingImpl* iface, XTaskQueueHandle* queue )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    TRACE( "iface %p, queue %p.\n", iface, queue );
    if ( impl->currentProcessTaskQueue )
    {
        *queue = impl->currentProcessTaskQueue;
        return TRUE;
    }
    return FALSE;
}

static VOID WINAPI x_threading_XTaskQueueSetCurrentProcessTaskQueue( IXThreadingImpl* iface, XTaskQueueHandle queue )
{
    struct x_threading *impl = impl_from_IXThreadingImpl( iface );
    TRACE( "iface %p, queue %p.\n", iface, queue );
    impl->currentProcessTaskQueue = queue;
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
    NULL,
    0,
    0,
};

IXThreadingImpl *x_threading_impl = &x_threading.IXThreadingImpl_iface;