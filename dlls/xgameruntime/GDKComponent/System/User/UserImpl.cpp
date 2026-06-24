/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XNetworking
 * 
 * Written by Weather
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

#include "UserImpl.h"

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

UserImpl::UserImpl( HSTRING token )
: m_token(token)
{

}

HRESULT WINAPI
UserImpl::QueryInterface( REFIID iid, void **out ) noexcept
{
    TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( IUser ) )
    {
        AddRef();
        *out = static_cast<IUser *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI 
UserImpl::AddRef() noexcept
{
    ULONG curr = static_cast<ULONG>(++ref);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

ULONG WINAPI 
UserImpl::Release() noexcept
{
    ULONG curr = static_cast<ULONG>(--ref);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr )
    {
        WindowsDeleteString( m_token );
        delete this;
    }
    return curr;
}

HRESULT WINAPI
UserImpl::GetMsaToken( HSTRING *out )
{
    TRACE( "iface %p, out %p\n", this, out );
    WindowsDuplicateString( m_token, out );
    return S_OK;
}
