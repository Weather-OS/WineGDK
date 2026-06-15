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

#include "../../private.h"

#include <atomic>

using namespace ABI;
using namespace ABI::Xodus;

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
    UINT16 MessageType;
    Windows::Storage::Streams::IBuffer *Message;
    std::atomic_long ref{ 1 };
};

struct XstsTokenResponse :
    public IXstsTokenResponse
{
    XstsTokenResponse() = default;
    virtual ~XstsTokenResponse() = default;

    XstsTokenResponse( const XstsTokenResponse& ) = delete;
    XstsTokenResponse& operator=( const XstsTokenResponse& ) = delete;

    XstsTokenResponse( 
        HSTRING userToken,
        Windows::Foundation::DateTime expiry,
        HSTRING relyingParty,
        TitleMgtSignaturePolicy signaturePolicy );

    /* IUnknown Methods */
    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) noexcept override;
    ULONG WINAPI AddRef() noexcept override;
    ULONG WINAPI Release() noexcept override;

    /* IXstsTokenResponse Methods */
    HRESULT WINAPI get_UserToken( HSTRING *out ) override;
    HRESULT WINAPI get_Expiry( Windows::Foundation::DateTime *out ) override;
    HRESULT WINAPI get_RelyingParty( HSTRING *out ) override;
    HRESULT WINAPI get_SignaturePolicy( TitleMgtSignaturePolicy *out ) override;

private:
    HSTRING UserToken;
    Windows::Foundation::DateTime Expiry;
    HSTRING RelyingParty;
    TitleMgtSignaturePolicy SignaturePolicy;
    std::atomic_long ref{ 1 };
};