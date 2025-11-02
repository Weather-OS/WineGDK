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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_async_work *impl_from_IWineAsyncWorkImpl( IWineAsyncWorkImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_async_work, IWineAsyncWorkImpl_iface );
}

/* static object inheritence */
struct x_async_work *impl_from_XAsyncBlock( XAsyncBlock *block )
{
    return CONTAINING_RECORD( block, struct x_async_work, threadBlock );
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