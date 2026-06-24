/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XGame
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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

class XGameImpl : public IXGameImpl3
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) override
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        if (iid == __uuidof( IUnknown    ) ||
            iid == __uuidof( IXGameImpl  ) ||
            iid == __uuidof( IXGameImpl2 ) ||
            iid == __uuidof( IXGameImpl3 ))
        {
            AddRef();
            *out = static_cast<IXGameImpl3 *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI AddRef() noexcept override
    {
        ULONG ref = InterlockedIncrement( &this->ref );
        TRACE( "iface %p, increasing refcount to %lu.\n", this, ref );
        return ref;
    }

    ULONG WINAPI Release() noexcept override
    {
        ULONG ref = InterlockedDecrement( &this->ref );
        TRACE( "iface %p, decreasing refcount to %lu.\n", this, ref );
        return ref;
    }

    HRESULT WINAPI XGameGetXboxTitleId( UINT32 *value ) override
    {
        FIXME( "iface %p, value %p stub!\n", this, value );
        return E_NOTIMPL;
    }

    void WINAPI XLaunchNewGame( const char *exePath, const char *args, XUserHandle defaultUser ) override
    {
        FIXME( "iface %p, exePath %s, args %s, defaultUser %p stub!\n", this, debugstr_a( exePath ), debugstr_a( args ), defaultUser );
    }

    HRESULT WINAPI XLaunchRestartOnCrash( const char *args, UINT32 reserved ) override
    {
        FIXME( "iface %p, args %s, reserved %u stub!\n", this, debugstr_a( args ), reserved );
        return E_NOTIMPL;
    }

private:
    LONG ref = 1;
};

static XGameImpl g_x_game;

IXGameImpl *x_game = static_cast<IXGameImpl *>(&g_x_game);
