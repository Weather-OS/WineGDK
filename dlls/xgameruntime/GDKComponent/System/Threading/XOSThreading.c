/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> OS Thread Handling
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
#include "XOSThreading.h"

WINE_DEFAULT_DEBUG_CHANNEL(xtaskqueue);

static inline struct x_wait_timer *impl_from_IXWaitTimer( IXWaitTimer *iface )
{
    return CONTAINING_RECORD( iface, struct x_wait_timer, IXWaitTimer_iface );
}

static VOID CALLBACK WaitCallback( PTP_CALLBACK_INSTANCE instance , void* context, PTP_TIMER timer )
{
    if ( context != NULL )
    {
        struct x_wait_timer *impl = impl_from_IXWaitTimer( (IXWaitTimer *)context );
        impl->callback(impl->context);
    }

    return;
}

static HRESULT WINAPI x_wait_timer_QueryInterface( IXWaitTimer *iface, REFIID iid, void **out )
{
    struct x_wait_timer *impl = impl_from_IXWaitTimer( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXWaitTimer ))
    {
        *out = &impl->IXWaitTimer_iface;
        impl->IXWaitTimer_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_wait_timer_AddRef( IXWaitTimer *iface )
{
    struct x_wait_timer *impl = impl_from_IXWaitTimer( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_wait_timer_Release( IXWaitTimer *iface )
{
    struct x_wait_timer *impl = impl_from_IXWaitTimer( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_wait_timer_Initialize( IXWaitTimer *iface, PVOID context, WaitTimerCallback* callback )
{
    struct x_wait_timer *impl = impl_from_IXWaitTimer( iface );

    TRACE( "iface %p, context %p, callback %p.\n", iface, context, callback );

    impl->context = context;
    impl->callback = callback;
    impl->timer = CreateThreadpoolTimer( WaitCallback, iface, NULL );

    if ( !impl->timer ) 
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;
}

static VOID WINAPI x_wait_timer_Terminate( IXWaitTimer *iface )
{
    struct x_wait_timer *impl = impl_from_IXWaitTimer( iface );

    TRACE( "iface %p.\n", iface );

    if ( !impl->timer )
    {
        SetThreadpoolTimer( impl->timer, NULL, 0, 0 );
        WaitForThreadpoolTimerCallbacks( impl->timer, TRUE );
        CloseThreadpoolTimer( impl->timer );
        impl->timer = NULL;
    }

    return;
}

static VOID WINAPI x_wait_timer_Start( IXWaitTimer *iface, UINT64 absoluteTime )
{
    LARGE_INTEGER li;
    FILETIME ft;

    struct x_wait_timer *impl = impl_from_IXWaitTimer( iface );

    TRACE( "iface %p.\n", iface );

    li.QuadPart = (LONGLONG)absoluteTime;
    ft.dwHighDateTime = li.HighPart;
    ft.dwLowDateTime = li.LowPart;

    SetThreadpoolTimer( impl->timer, &ft, 0, 0 );

    return;
}

static VOID WINAPI x_wait_timer_Cancel( IXWaitTimer *iface )
{
    struct x_wait_timer *impl = impl_from_IXWaitTimer( iface );

    TRACE( "iface %p.\n", iface );

    SetThreadpoolTimer( impl->timer, NULL, 0, 0 );

    return;
}

static UINT64 WINAPI x_wait_timer_GetAbsoluteTime( IXWaitTimer *iface, UINT32 msFromNow )
{
    FILETIME ft;
    ULARGE_INTEGER li;
    UINT64 hundredNanosFromNow;

    TRACE( "iface %p, msFromNow %d.\n", iface, msFromNow );

    GetSystemTimeAsFileTime( &ft );

    hundredNanosFromNow = msFromNow;
    hundredNanosFromNow *= 10000ULL;

    li.HighPart = ft.dwHighDateTime;
    li.LowPart = ft.dwLowDateTime;
    li.QuadPart += hundredNanosFromNow;

    return li.QuadPart;
}

static const struct IXWaitTimerVtbl x_wait_timer_vtbl =
{
    /* IUnknown methods */
    x_wait_timer_QueryInterface,
    x_wait_timer_AddRef,
    x_wait_timer_Release,
    /* IAtomicVector methods */
    x_wait_timer_Initialize,
    x_wait_timer_Terminate,
    x_wait_timer_Start,
    x_wait_timer_Cancel,
    x_wait_timer_GetAbsoluteTime
};

HRESULT XCreateWaitTimer( IXWaitTimer **out )
{
    struct x_wait_timer *impl;

    TRACE( "out %p.\n", out );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IXWaitTimer_iface.lpVtbl = &x_wait_timer_vtbl;
    impl->ref = 1;
    
    *out = &impl->IXWaitTimer_iface;

    return S_OK;
}