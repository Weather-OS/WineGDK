/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XLauncher
 *
 * Copyright 2026 Olivia Ryan
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

#include "private.h"
#include <shellapi.h>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

class XLauncherImpl : public IXLauncherImpl
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) override
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (iid == __uuidof( IUnknown       ) ||
            iid == __uuidof( IXLauncherImpl ))
        {
            AddRef();
            *out = static_cast<IXLauncherImpl *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI AddRef() override
    {
        ULONG ref = InterlockedIncrement( &this->ref );
        TRACE( "iface %p increasing refcount to %lu.\n", this, ref );
        return ref;
    }

    ULONG WINAPI Release() override
    {
        ULONG ref = InterlockedDecrement( &this->ref );
        TRACE( "iface %p decreasing refcount to %lu.\n", this, ref );
        return ref;
    }

    HRESULT WINAPI XLaunchUri( XUserHandle user, const char *uri ) override
    {
        TRACE( "iface %p, user %p uri %s.\n", this, user, debugstr_a( uri ) );
        return (SIZE_T)ShellExecuteA( NULL, "open", uri, NULL, NULL, SW_SHOW ) > 32 ? S_OK : E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER;
    }

    HRESULT WINAPI XDisplayAcquireTimeoutDeferral( XDisplayTimeoutDeferralHandle *handle ) override
    {
        FIXME( "iface %p, handle %p stub!\n", this, handle );
        return E_NOTIMPL;
    }

    void WINAPI XDisplayCloseTimeoutDeferralHandle( XDisplayTimeoutDeferralHandle handle ) override
    {
        FIXME( "iface %p, handle %p stub!\n", this, handle );
    }

private:
    LONG ref = 1;
};

static XLauncherImpl g_x_launcher;

IXLauncherImpl *x_launcher = static_cast<IXLauncherImpl *>(&g_x_launcher);
