/*
 * Xbox Game runtime Library
 *  Xodus Interopability Layer -> Struct types
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

#include <atomic>

#ifndef __XODUS_STRUCTS__
#define __XODUS_STRUCTS__

using namespace ABI;
using namespace ABI::Xodus;

typedef HRESULT (CALLBACK *IPCResponseHandlerCallback)( PVOID context, IXodusIPCPacket *response );

struct XodusIPCPacket :
    public IXodusIPCPacket
{
public:
    XodusIPCPacket() = default;
    virtual ~XodusIPCPacket() = default;

    XodusIPCPacket( const XodusIPCPacket& ) = delete;
    XodusIPCPacket& operator=( const XodusIPCPacket& ) = delete;

    XodusIPCPacket( 
        MagicHeaderType type, 
        UINT16 messageType, 
        Windows::Storage::Streams::IBuffer *message );

    /* IUnknown Methods */
    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) noexcept override;
    ULONG WINAPI AddRef() noexcept override;
    ULONG WINAPI Release() noexcept override;

    /* IXodusIPCPacket Methods */
    HRESULT WINAPI get_Magic( MagicHeaderType *out ) override;
    HRESULT WINAPI get_MessageType( UINT16 *out ) override;
    HRESULT WINAPI get_Message( Windows::Storage::Streams::IBuffer **out ) override;

private:
    MagicHeaderType Magic;
    UINT16 Message_Type;
    Windows::Storage::Streams::IBuffer *Message;
    std::atomic_long ref{ 1 };
};

struct IPCResponseHandler :
    public IIPCResponseHandler
{
    IPCResponseHandler() = default;
    virtual ~IPCResponseHandler() = default;

    IPCResponseHandler( const IPCResponseHandler& ) = delete;
    IPCResponseHandler& operator=( const IPCResponseHandler& ) = delete;

    IPCResponseHandler( 
        IPCResponseHandlerCallback callback,
        PVOID context );

    /* IUnknown Methods */
    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) noexcept override;
    ULONG WINAPI AddRef() noexcept override;
    ULONG WINAPI Release() noexcept override;

    /* IXstsTokenResponse Methods */
    HRESULT WINAPI Invoke( IXodusIPCPacket *response ) override;

private:
    IPCResponseHandlerCallback m_callback;
    PVOID m_context;
    std::atomic_long ref{ 1 };
};

struct MsaTokenResponse :
    public IMsaTokenResponse
{
    MsaTokenResponse() = default;
    virtual ~MsaTokenResponse() = default;

    MsaTokenResponse( const MsaTokenResponse& ) = delete;
    MsaTokenResponse& operator=( const MsaTokenResponse& ) = delete;

    /* IUnknown Methods */
    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) noexcept override;
    ULONG WINAPI AddRef() noexcept override;
    ULONG WINAPI Release() noexcept override;

    /* IMsaTokenResponse Methods */
    HRESULT WINAPI get_Token( HSTRING *out );
    HRESULT WINAPI get_Expiry( ABI::Windows::Foundation::DateTime *out );

private:
    std::atomic_long ref{ 1 };
};

#endif