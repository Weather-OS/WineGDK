/*
 * Xbox Game runtime Library
 *  Xodus Interopability Layer -> XodusService
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
#include "../../WineCoreUAP/Foundation/IWineAsync.hpp"
#include "Structs.h"

#include "robuffer.h"
#include <atomic>

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

using namespace ABI;
using namespace ABI::Xodus;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;

class ABI::Xodus::XodusService :
    public IXodusService
{
public:
    /* IUnknown Methods */
    HRESULT WINAPI 
    QueryInterface( REFIID iid, void **out )
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXodusService ) )
        {
            AddRef();
            *out = static_cast<IXodusService *>(this);
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

    /* IInspectable Methods */
    HRESULT WINAPI
    GetIids( ULONG *iidCount, IID **iids ) override
    {
        FIXME("iface %p, iidCount %p, iids %p stub!\n", this, iidCount, iids);
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetRuntimeClassName( HSTRING *className ) override 
    {
        FIXME("iface %p, className %p stub!\n", this, className);
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetTrustLevel( TrustLevel *trustLevel ) override
    {
        FIXME("iface %p, trustLevel %p stub!\n", this, trustLevel);
        return E_NOTIMPL;
    }

    /* IXodusService Methods */
    HRESULT WINAPI
    Ping( IAsyncAction **operation ) override
    {
        TRACE("operation %p.\n", operation);
        return AsyncAction::Create( static_cast<IUnknown *>(this), nullptr, PingAsync, operation );
    }

    HRESULT WINAPI
    MsaTokenRequest( HSTRING clientId, IAsyncOperation<IMsaTokenResponse *> **operation ) override
    {
        HRESULT hr;
        HSTRING clientIdCopy;

        TRACE("clientId %s, operation %p.\n", debugstr_hstring(clientId), operation);

        hr = WindowsDuplicateString( clientId, &clientIdCopy );
        if ( FAILED( hr ) ) return hr;
    
        return AsyncOperation<IMsaTokenResponse *>::Create( static_cast<IUnknown *>(this), 
                    static_cast<PVOID>(clientIdCopy), MsaTokenRequestAsync, operation );
    }

private:
    static HRESULT WINAPI
    MsaTokenRequestAsync( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        auto clientId = static_cast<HSTRING>(param);

        BYTE *messageBuffer;
        DWORD ret;
        LPSTR xmlStr;
        UINT16 messageType;
        HRESULT status = S_OK;
        HSTRING bufferClass;

        IMsaTokenResponse *tokenResponse = nullptr;
        IXodusIPCPacket *xodusPacket = nullptr;
        IBufferByteAccess *messageByteAccess = nullptr;
        IBuffer *message = nullptr;
        IBufferFactory *bufferFactory = nullptr;
        IAsyncOperation<IXodusIPCPacket *> *response;

        TRACE("invoker %p, param %p, result %p\n", invoker, param, result);

        status = WindowsCreateString( RuntimeClass_Windows_Storage_Streams_Buffer, lstrlenW( RuntimeClass_Windows_Storage_Streams_Buffer ), &bufferClass );
        if ( FAILED( status ) ) goto _CLEANUP;

        status = RoGetActivationFactory( bufferClass, __uuidof( IBufferFactory ), (void **)&bufferFactory );
        if ( FAILED( status ) ) goto _CLEANUP;

        // FIXME: Probably need to do HSTRING on xmlStr as doing manual CoTaskMemFree on xmlStr is janky.
        status = xodus_xml_builder->BuildMsaTokenRequestXml( clientId, &xmlStr );
        if ( FAILED( status ) ) goto _CLEANUP;

        status = bufferFactory->Create( lstrlenA( xmlStr ) + 1, &message );
        if ( FAILED( status ) ) goto _CLEANUP;

        status = message->QueryInterface<IBufferByteAccess>( &messageByteAccess );
        if ( FAILED( status ) ) goto _CLEANUP;

        status = messageByteAccess->Buffer( &messageBuffer );
        messageByteAccess->Release();
        messageByteAccess = nullptr;
        if ( FAILED( status ) ) goto _CLEANUP;

        RtlCopyMemory( messageBuffer, xmlStr, lstrlenA( xmlStr ) + 1 );
        status = message->put_Length( lstrlenA( xmlStr ) + 1 );

        // Construct a new IPC Packet
        xodusPacket = new XodusIPCPacket(
            MagicHeaderType::XML,
            3 /* MsaTokenRequest */,
            message
        );

        xodus_ipclayer->SendRequestAsync( xodusPacket, &response );

        ret = AsyncOperationCompletedHandler<IXodusIPCPacket *>::await_AsyncOperation( response, INFINITE );
        if ( ret )
        {
            status = E_FAIL;
            goto _CLEANUP;
        }

        xodusPacket->Release();
        xodusPacket = nullptr;

        message->Release();
        message = nullptr;

        status = response->GetResults( &xodusPacket );
        if ( FAILED( status ) ) goto _CLEANUP;

        xodusPacket->get_MessageType( &messageType );
        xodusPacket->get_Message( &message );
        status = message->QueryInterface<IBufferByteAccess>( &messageByteAccess );
        if ( FAILED( status ) ) goto _CLEANUP;

        status = messageByteAccess->Buffer( &messageBuffer );
        if ( FAILED( status ) ) goto _CLEANUP;

        status = xodus_xml_builder->FromMsaTokenResponseXml( reinterpret_cast<LPCSTR>(messageBuffer), &tokenResponse );

        if ( SUCCEEDED( status ) )
        {
            result->vt = VT_UNKNOWN;
            result->punkVal = tokenResponse;

            if ( messageType != 4 /* MsaTokenResponse */ )
                status = E_INVALIDARG;
        }
        
_CLEANUP:
        if ( bufferClass ) WindowsDeleteString( bufferClass );
        if ( bufferFactory ) bufferFactory->Release();
        if ( xmlStr ) CoTaskMemFree( xmlStr );
        if ( message ) message->Release();
        if ( messageByteAccess ) messageByteAccess->Release();
        if ( xodusPacket ) xodusPacket->Release();
        if ( response ) response->Release();
        if ( clientId ) WindowsDeleteString( clientId );
        return S_OK;
    }

    static HRESULT WINAPI
    PingAsync( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        DWORD ret;
        UINT16 messageType;
        HRESULT status;
        HSTRING bufferClass;

        IXodusIPCPacket *xodusPacket = nullptr;
        IBuffer *message = nullptr;
        IBufferFactory *bufferFactory = nullptr;
        IAsyncOperation<IXodusIPCPacket *> *response;

        TRACE("invoker %p, param %p, result %p\n", invoker, param, result);

        status = WindowsCreateString( RuntimeClass_Windows_Storage_Streams_Buffer, lstrlenW( RuntimeClass_Windows_Storage_Streams_Buffer ), &bufferClass );
        if ( FAILED( status ) ) goto _CLEANUP;

        status = RoGetActivationFactory( bufferClass, __uuidof( IBufferFactory ), (void **)&bufferFactory );
        if ( FAILED( status ) ) goto _CLEANUP;

        status = bufferFactory->Create( 1, &message );
        if ( FAILED( status ) ) goto _CLEANUP;

        // Construct a new IPC Packet
        xodusPacket = new XodusIPCPacket(
            MagicHeaderType::XML,
            1 /* PING */,
            message
        );

        xodus_ipclayer->SendRequestAsync( xodusPacket, &response );

        ret = AsyncOperationCompletedHandler<IXodusIPCPacket *>::await_AsyncOperation( response, INFINITE );
        if ( ret )
        {
            status = E_FAIL;
            goto _CLEANUP;
        }

        xodusPacket->Release();
        xodusPacket = nullptr;

        message->Release();
        message = nullptr;

        // confirm that we actually PONGed
        status = response->GetResults( &xodusPacket );
        if ( FAILED( status ) ) return status;
        xodusPacket->get_MessageType( &messageType );
        if ( messageType != 2 /* PONG */)
            return E_INVALIDARG;
        
_CLEANUP:
        if ( bufferClass ) WindowsDeleteString( bufferClass );
        if ( bufferFactory ) bufferFactory->Release();
        if ( message ) message->Release();
        if ( response ) response->Release();
        if ( xodusPacket ) xodusPacket->Release();

        return status;
    }

    std::atomic_long ref{ 1 };
};

static XodusService g_xodus_service;
IXodusService *xodus_service = static_cast<IXodusService*>(&g_xodus_service);
