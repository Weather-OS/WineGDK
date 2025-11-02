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

    if ( XCheckBlockAndInitialize( asyncBlock ) == S_FALSE ) return E_INVALIDARG; // <-- Invalid for this call.
    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( impl->status == E_PENDING && wait )
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

    if ( XCheckBlockAndInitialize( asyncBlock ) == S_FALSE ) return E_INVALIDARG; // <-- Invalid for this call.
    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( impl->status != S_OK ) return impl->status;
    
    *bufferSize = impl->provider.data->bufferSize;

    return S_OK;
}

static HRESULT WINAPI x_threading_XAsyncCancel( IXThreadingImpl *iface, XAsyncBlock *asyncBlock )
{
    struct x_async_work *impl;

    TRACE( "iface %p, asyncBlock %p\n", iface, asyncBlock );

    if ( XCheckBlockAndInitialize( asyncBlock ) == S_FALSE ) return E_INVALIDARG; // <-- Invalid for this call.
    impl = impl_from_XAsyncBlock( asyncBlock );

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

    XCheckBlockAndInitialize( asyncBlock );
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

    if ( XCheckBlockAndInitialize( asyncBlock ) == S_FALSE ) return E_INVALIDARG; // <-- Invalid for this call.
    impl = impl_from_XAsyncBlock( asyncBlock );

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

    if ( XCheckBlockAndInitialize( asyncBlock ) == S_FALSE ) return; // <-- Invalid for this call.
    impl = impl_from_XAsyncBlock( asyncBlock );

    impl->status = result;
    impl->provider.data->bufferSize = requiredBufferSize;

    // invoke the completion routine
    if ( impl->threadBlock->callback )
        impl->threadBlock->callback( asyncBlock );

    return;
}

static HRESULT WINAPI x_threading_XAsyncGetResult( IXThreadingImpl* iface, XAsyncBlock* asyncBlock, const PVOID identity, SIZE_T bufferSize, PVOID buffer, SIZE_T* bufferUsed )
{
    struct x_async_work *impl;

    TRACE( "iface %p, asyncBlock %p, identity %p, bufferSize %llu, buffer %p, bufferUsed %p\n", iface, asyncBlock, identity, bufferSize, buffer, bufferUsed );

    if ( XCheckBlockAndInitialize( asyncBlock ) == S_FALSE ) return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED ); // <-- Invalid for this call.
    impl = impl_from_XAsyncBlock( asyncBlock );

    if ( !impl->provider.data->buffer ) return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    if ( impl->provider.data->bufferSize > bufferSize ) return E_BOUNDS;
    if ( identity != impl->provider.identity ) return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

    if ( bufferUsed )
        *bufferUsed = impl->provider.data->bufferSize;

    if( buffer )
        memcpy( buffer, impl->provider.data->buffer, impl->provider.data->bufferSize );

    return S_OK;
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
    x_threading_XAsyncGetResult
};

static struct x_threading x_threading =
{
    {&x_threading_vtbl},
    0,
};

IXThreadingImpl *x_threading_impl = &x_threading.IXThreadingImpl_iface;