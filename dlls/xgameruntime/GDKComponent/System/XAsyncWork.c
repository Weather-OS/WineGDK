/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XThread, XAsync and XTask Workloads
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

#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_async_work *impl_from_IWineAsyncWorkImpl( IWineAsyncWorkImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_async_work, IWineAsyncWorkImpl_iface );
}

static inline BOOLEAN is_valid_read_ptr( const void *ptr, size_t size )
{
    MEMORY_BASIC_INFORMATION mbi;
    if ( !ptr ) return FALSE;
    if ( !VirtualQuery( ptr, &mbi, sizeof(mbi) ) ) return FALSE;
    return !( mbi.Protect & ( PAGE_NOACCESS | PAGE_GUARD ) );
}

/* static object inheritence */
struct x_async_work *impl_from_XAsyncBlock( XAsyncBlock *block )
{
    PVOID p;
    struct x_async_work *w;

    if ( !block )
        return NULL;

    memcpy( &p, block->internal, sizeof(p) );
    w = (struct x_async_work *)p;

    if ( !is_valid_read_ptr( w, sizeof(*w) ) )
        return NULL;

    if ( w->magic != X_ASYNC_WORK_MAGIC )
        return NULL;

    return w;
}

static HRESULT WINAPI x_async_work_QueryInterface( IWineAsyncWorkImpl *iface, REFIID iid, void **out )
{
    struct x_async_work *impl = impl_from_IWineAsyncWorkImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IWineAsyncWorkImpl ))
    {
        *out = &impl->IWineAsyncWorkImpl_iface;
        impl->IWineAsyncWorkImpl_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_async_work_AddRef( IWineAsyncWorkImpl *iface )
{
    struct x_async_work *impl = impl_from_IWineAsyncWorkImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_async_work_Release( IWineAsyncWorkImpl *iface )
{
    struct x_async_work *impl = impl_from_IWineAsyncWorkImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static const struct IWineAsyncWorkImplVtbl x_async_work_vtbl =
{
    x_async_work_QueryInterface,
    x_async_work_AddRef,
    x_async_work_Release
};

HRESULT WINAPI XInitializeBlock( XAsyncBlock* asyncBlock )
{
    struct x_async_work *newImpl;
    PVOID p;
    
    TRACE( "asyncBlock %p.\n", asyncBlock );

    if (!(newImpl = calloc( 1, sizeof(*newImpl) ))) return E_OUTOFMEMORY;

    newImpl->IWineAsyncWorkImpl_iface.lpVtbl = &x_async_work_vtbl;
    newImpl->threadBlock = asyncBlock;
    newImpl->status = S_OK;
    newImpl->ref = 1;
    newImpl->magic = X_ASYNC_WORK_MAGIC;
    newImpl->provider.data = NULL;

    p = (PVOID)newImpl;
    memcpy( (PVOID *)asyncBlock->internal, &p, sizeof(p) );

    return S_OK;
}

VOID CALLBACK XTPCallback( TP_CALLBACK_INSTANCE *instance, void *iface, TP_WORK *work )
{
    struct x_async_work *impl = impl_from_IWineAsyncWorkImpl( (IWineAsyncWorkImpl *)iface );

    TRACE( "instance %p, iface %p, work %p.\n", instance, iface, work );

    EnterCriticalSection( &impl->cs );
    if ( impl->provider.operation == DoWork )
        Sleep( impl->provider.workDelay );
    impl->provider.callback( impl->provider.operation, impl->provider.data );
    /* It's the client's job to call XAsyncComplete and set the status, such as E_PENDING, E_ABORT or S_OK.*/
    LeaveCriticalSection( &impl->cs );

    return;
}