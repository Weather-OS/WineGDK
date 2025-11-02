/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XSystem
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

#include "XSystem.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_system *impl_from_IXSystemImpl( IXSystemImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_system, IXSystemImpl_iface );
}

static HRESULT WINAPI x_system_QueryInterface( IXSystemImpl *iface, REFIID iid, void **out )
{
    struct x_system *impl = impl_from_IXSystemImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXSystemImpl ))
    {
        *out = &impl->IXSystemImpl_iface;
        impl->IXSystemImpl_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_system_AddRef( IXSystemImpl *iface )
{
    struct x_system *impl = impl_from_IXSystemImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_system_Release( IXSystemImpl *iface )
{
    struct x_system *impl = impl_from_IXSystemImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_system_XSystemGetConsoleId( IXSystemImpl *iface, INT32 consoleIdSize, LPSTR consoleId, SIZE_T *consoleIdUsed )
{    
    // For Windows, Console ID is always `00000000.00000000.00000000.00000000.00
    LPCSTR Id = "00000000.00000000.00000000.00000000.00";

    TRACE( "iface %p, consoleIdSize %d, consoleId %p, consoleIdUsed %p\n", iface, consoleIdSize, consoleId, consoleIdUsed );

    if ( !consoleId || !consoleIdUsed )
        return E_POINTER;

    if ( consoleIdSize < XSystemConsoleIdBytes )
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );

    strcpy_s( consoleId, consoleIdSize, Id );
    *consoleIdUsed = strlen( Id ) + 1;
    return S_OK;
}

static HRESULT WINAPI x_system_XSystemGetXboxLiveSandboxId( IXSystemImpl *iface, INT32 sandboxIdSize, LPSTR sandboxId, SIZE_T *sandboxIdUsed )
{    
    // Always assume RETAIL environment for Wine
    LPCSTR Id = "RETAIL";

    TRACE( "iface %p, sandboxIdSize %d, sandboxId %p, sandboxIdUsed %p\n", iface, sandboxIdSize, sandboxId, sandboxIdUsed );

    if ( !sandboxId || !sandboxIdUsed )
        return E_POINTER;

    if ( sandboxIdSize < XSystemXboxLiveSandboxIdMaxBytes )
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );

    strcpy_s( sandboxId, sandboxIdSize, Id );
    *sandboxIdUsed = strlen( Id ) + 1;
    return S_OK;
}

static HRESULT WINAPI x_system_XSystemGetAppSpecificDeviceId( IXSystemImpl *iface, INT32 appSpecificDeviceIdSize, LPSTR appSpecificDeviceId, SIZE_T *appSpecificDeviceIdUsed )
{    
    FIXME( "iface %p, appSpecificDeviceIdSize %d, appSpecificDeviceId %p, appSpecificDeviceIdUsed %p stub!\n", iface, appSpecificDeviceIdSize, appSpecificDeviceId, appSpecificDeviceIdUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_system_XSystemHandleTrack( IXSystemImpl *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_system_XSystemIsHandleValid( IXSystemImpl *iface )
{
    // always assume it's valid.
    FIXME( "iface %p stub!\n", iface );
    return TRUE;
}

static HRESULT WINAPI x_system_XSystemAllowFullDownloadBandwidth( IXSystemImpl *iface, boolean enable )
{
    FIXME( "iface %p, enable %d stub!\n", iface, enable );
    return E_NOTIMPL;
}

static const struct IXSystemImplVtbl x_system_vtbl =
{
    x_system_QueryInterface,
    x_system_AddRef,
    x_system_Release,
    /* IXSystemImpl methods */
    x_system_XSystemGetConsoleId,
    x_system_XSystemGetXboxLiveSandboxId,
    x_system_XSystemGetAppSpecificDeviceId,
    x_system_XSystemHandleTrack,
    x_system_XSystemIsHandleValid,
    x_system_XSystemAllowFullDownloadBandwidth
};

static struct x_system x_system =
{
    {&x_system_vtbl},
    0,
};

IXSystemImpl *x_system_impl = &x_system.IXSystemImpl_iface;