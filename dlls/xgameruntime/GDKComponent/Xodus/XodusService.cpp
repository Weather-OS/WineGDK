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
    XstsTokenRequest( HSTRING url, IAsyncOperation<IXstsTokenResponse *> **operation ) override
    {
        TRACE("url %s, operation %p.\n", debugstr_hstring(url), operation);
        return AsyncOperation::Inspectable::Create( static_cast<IUnknown *>(this), 
                    static_cast<PVOID>(url), XstsTokenRequestAsync, { .operation = &__uuidof( IAsyncOperation<IXstsTokenResponse *> )}, (IAsyncOperation<IInspectable *> **)operation );
    }

private:
    static HRESULT WINAPI
    XstsTokenRequestAsync( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        auto url = static_cast<HSTRING>(param);

        INT xmlStrSize;
        BYTE *messageBuffer;
        DWORD ret;
        LPSTR xmlStr;
        UINT32 xmlStrLen;
        UINT16 messageType;
        LPCWSTR xmlStrW;
        HRESULT status;
        HSTRING bufferClass;
        HSTRING xml;

        IXodusIPCPacket *xodusPacket = nullptr;
        IBufferByteAccess *messageByteAccess = nullptr;
        IBuffer *message = nullptr;
        IBufferFactory *bufferFactory = nullptr;
        IAsyncOperation<IXodusIPCPacket *> *response;

        TRACE("invoker %p, param %p, result %p\n", invoker, param, result);

        status = WindowsCreateString( RuntimeClass_Windows_Storage_Streams_Buffer, lstrlenW( RuntimeClass_Windows_Storage_Streams_Buffer ), &bufferClass );
        if ( FAILED( status ) ) return status;

        status = RoGetActivationFactory( bufferClass, __uuidof( IBufferFactory ), (void **)&bufferFactory );
        WindowsDeleteString( bufferClass );
        if ( FAILED( status ) ) return status;

        status = xodus_xml_builder->BuildXstsTokenRequestXml( url, &xml );
        if ( FAILED( status ) ) return status;

        xmlStrW = WindowsGetStringRawBuffer( xml, &xmlStrLen );
        xmlStrSize = WideCharToMultiByte( CP_UTF8, 0, xmlStrW, xmlStrLen, nullptr, 0, nullptr, nullptr );
        xmlStr = (LPSTR)CoTaskMemAlloc( xmlStrSize );
        WideCharToMultiByte( CP_UTF8, 0, xmlStrW, xmlStrLen, xmlStr, xmlStrSize, nullptr, nullptr );

        status = bufferFactory->Create( xmlStrSize + 1, &message );
        if ( FAILED( status ) ) return status;

        status = message->QueryInterface<IBufferByteAccess>( &messageByteAccess );
        if ( FAILED( status ) ) return status;

        status = messageByteAccess->Buffer( &messageBuffer );
        messageByteAccess->Release();
        if ( FAILED( status ) ) return status;

        RtlCopyMemory( messageBuffer, xmlStr, xmlStrSize );
        status = message->put_Length( xmlStrSize );
        CoTaskMemFree( xmlStr );

        // Construct a new IPC Packet
        xodusPacket = new XodusIPCPacket(
            MagicHeaderType::XML,
            3 /* XstsTokenRequest */,
            message
        );

        xodus_ipclayer->SendRequestAsync( xodusPacket, &response );

        ret = AsyncOperationCompletedHandler<IXodusIPCPacket *>::await_AsyncOperation( response, INFINITE );
        if ( ret )
            return E_FAIL;

        xodusPacket->Release();
        message->Release();

        status = response->GetResults( &xodusPacket );
        if ( FAILED( status ) ) return status;
        response->Release();
        xodusPacket->get_MessageType( &messageType );
        xodusPacket->get_Message( &message );
        xodusPacket->Release();
        status = message->QueryInterface<IBufferByteAccess>( &messageByteAccess );
        message->Release();
        if ( FAILED( status ) ) return status;

        status = messageByteAccess->Buffer( &messageBuffer );
        messageByteAccess->Release();
        if ( FAILED( status ) ) return status;
        TRACE("Got message buffer: %s\n", debugstr_a((LPCSTR)messageBuffer));
        if ( messageType != 4 /* XstsTokenResponse */)
            return E_INVALIDARG;
        
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
        if ( FAILED( status ) ) return status;

        status = RoGetActivationFactory( bufferClass, __uuidof( IBufferFactory ), (void **)&bufferFactory );
        WindowsDeleteString( bufferClass );
        if ( FAILED( status ) ) return status;

        status = bufferFactory->Create( 1, &message );
        if ( FAILED( status ) ) return status;

        // Construct a new IPC Packet
        xodusPacket = new XodusIPCPacket(
            MagicHeaderType::XML,
            1 /* PING */,
            message
        );

        xodus_ipclayer->SendRequestAsync( xodusPacket, &response );

        ret = AsyncOperationCompletedHandler<IXodusIPCPacket *>::await_AsyncOperation( response, INFINITE );
        if ( ret )
            return E_FAIL;

        xodusPacket->Release();
        message->Release();

        // confirm that we actually PONGed
        status = response->GetResults( &xodusPacket );
        response->Release();
        if ( FAILED( status ) ) return status;
        xodusPacket->get_MessageType( &messageType );
        xodusPacket->Release();
        if ( messageType != 2 /* PONG */)
            return E_INVALIDARG;
        
        return S_OK;
    }

    std::atomic_long ref{ 1 };
};

static XodusService g_xodus_service;
IXodusService *xodus_service = static_cast<IXodusService*>(&g_xodus_service);
