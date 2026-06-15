/*
 * Xbox Game runtime Library
 *  Xodus Interopability Layer -> XodusIPCPacket
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

#include "Structs.h"

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

/**
 * XodusIPCPacket: Wraps IPC Packets sent to and received from Xodus.
 */

XodusIPCPacket::XodusIPCPacket( 
    MagicHeaderType type, 
    UINT16 messageType, 
    Windows::Storage::Streams::IBuffer *message )
: Magic(type),
  MessageType(messageType)
{
    Message = message;
    message->AddRef();
}

HRESULT WINAPI
XodusIPCPacket::QueryInterface( REFIID iid, void **out ) noexcept
{
    TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( IXodusIPCPacket ) )
    {
        AddRef();
        *out = static_cast<IXodusIPCPacket *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI 
XodusIPCPacket::AddRef() noexcept
{
    ULONG curr = static_cast<ULONG>(++ref);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

ULONG WINAPI 
XodusIPCPacket::Release() noexcept
{
    ULONG curr = static_cast<ULONG>(--ref);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr )
    {
        Message->Release();
        delete this;
    }

    return curr;
}

HRESULT WINAPI
XodusIPCPacket::get_Magic( MagicHeaderType *out )
{
    FIXME("iface %p, out %p stub!\n", this, out);
    return E_NOTIMPL;
}

HRESULT WINAPI
XodusIPCPacket::get_MessageType( UINT16 *out )
{
    FIXME("iface %p, out %p stub!\n", this, out);
    return E_NOTIMPL;
}

HRESULT WINAPI
XodusIPCPacket::get_Message( Windows::Storage::Streams::IBuffer **out )
{
    FIXME("iface %p, out %p stub!\n", this, out);
    return E_NOTIMPL;
}

/**
 * XstsTokenResponse: Wraps XstsTokenResponse from Xodus. 
 */

XstsTokenResponse::XstsTokenResponse( 
    HSTRING userToken,
    Windows::Foundation::DateTime expiry,
    HSTRING relyingParty,
    TitleMgtSignaturePolicy signaturePolicy )
: UserToken(userToken),
  Expiry(expiry),
  RelyingParty(relyingParty),
  SignaturePolicy(signaturePolicy)
{
}

HRESULT WINAPI
XstsTokenResponse::QueryInterface( REFIID iid, void **out ) noexcept
{
    TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( IXstsTokenResponse ) )
    {
        AddRef();
        *out = static_cast<IXstsTokenResponse *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI 
XstsTokenResponse::AddRef() noexcept
{
    ULONG curr = static_cast<ULONG>(++ref);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

ULONG WINAPI 
XstsTokenResponse::Release() noexcept
{
    ULONG curr = static_cast<ULONG>(--ref);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr )
    {
        delete this;
    }

    return curr;
}

HRESULT WINAPI
XstsTokenResponse::get_UserToken( HSTRING *out )
{
    FIXME("iface %p, type %p stub!\n", this, out);
    return E_NOTIMPL;
}

HRESULT WINAPI
XstsTokenResponse::get_Expiry( Windows::Foundation::DateTime *out )
{
    FIXME("iface %p, type %p stub!\n", this, out);
    return E_NOTIMPL;
}

HRESULT WINAPI
XstsTokenResponse::get_RelyingParty( HSTRING *out )
{
    FIXME("iface %p, type %p stub!\n", this, out);
    return E_NOTIMPL;
}

HRESULT WINAPI
XstsTokenResponse::get_SignaturePolicy( TitleMgtSignaturePolicy *out )
{
    FIXME("iface %p, type %p stub!\n", this, out);
    return E_NOTIMPL;
}