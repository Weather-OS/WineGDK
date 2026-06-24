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

#include "../../private.h"

#include <cstring>
#include <atomic>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

const INT32 XSystemConsoleIdBytes = 39;
const INT32 XSystemXboxLiveSandboxIdMaxBytes = 16;
const INT32 XSystemAppSpecificDeviceIdBytes = 45;

class XSystemImpl : 
    public IXSystemImpl
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out )
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXSystemImpl ) )
        {
            AddRef();
            *out = static_cast<IXSystemImpl *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI 
    AddRef() noexcept override
    {
        ULONG curr = static_cast<ULONG>(++ref);
        TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
        return curr;
    }

    ULONG WINAPI 
    Release() noexcept override
    {
        ULONG curr = static_cast<ULONG>(--ref);
        TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

        // Polymorphic classes should not be deleted.
        /*
        if ( !curr )
            delete this;
        */

        return curr;
    }

    HRESULT WINAPI XSystemGetConsoleId( INT32 consoleIdSize, LPSTR consoleId, SIZE_T *consoleIdUsed ) override
    {    
        // For Windows, Console ID is always `00000000.00000000.00000000.00000000.00
        LPCSTR Id = "00000000.00000000.00000000.00000000.00";

        TRACE( "consoleIdSize %d, consoleId %p, consoleIdUsed %p\n", consoleIdSize, consoleId, consoleIdUsed );

        if ( consoleIdSize < XSystemConsoleIdBytes )
            return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );

        if ( consoleIdUsed )
            *consoleIdUsed = std::strlen( Id ) + 1;
        if ( consoleId )
            lstrcpynA( consoleId, Id, consoleIdSize );
            
        return S_OK;
    }

    HRESULT WINAPI XSystemGetXboxLiveSandboxId( INT32 sandboxIdSize, LPSTR sandboxId, SIZE_T *sandboxIdUsed ) override
    {    
        // Always assume RETAIL environment for Wine
        LPCSTR Id = "RETAIL";

        TRACE( "sandboxIdSize %d, sandboxId %p, sandboxIdUsed %p\n", sandboxIdSize, sandboxId, sandboxIdUsed );

        if ( sandboxIdSize < XSystemXboxLiveSandboxIdMaxBytes )
            return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );

        if ( sandboxIdUsed )
            *sandboxIdUsed = std::strlen( Id ) + 1;
        if ( sandboxId )
            lstrcpynA( sandboxId, Id, sandboxIdSize );

        return S_OK;
    }

    HRESULT WINAPI XSystemGetAppSpecificDeviceId( INT32 appSpecificDeviceIdSize, LPSTR appSpecificDeviceId, SIZE_T *appSpecificDeviceIdUsed ) override
    {    
        FIXME( "appSpecificDeviceIdSize %d, appSpecificDeviceId %p, appSpecificDeviceIdUsed %p stub!\n", appSpecificDeviceIdSize, appSpecificDeviceId, appSpecificDeviceIdUsed );
        return E_NOTIMPL;
    }

private:
    std::atomic_long ref{ 1 };
};

static XSystemImpl g_x_system;

IXSystemImpl *x_system = static_cast<IXSystemImpl*>(&g_x_system);