/*
 * Xbox Game runtime Library
 *  Xodus Interopability Layer -> IPCLayer
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

#include "ntstatus.h"
#include "robuffer.h"

#include <atomic>

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

using namespace ABI;
using namespace ABI::Xodus;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;

class ABI::Xodus::IPCLayer :
    public IIPCLayer
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
             iid == __uuidof( IIPCLayer ) )
        {
            AddRef();
            *out = static_cast<IIPCLayer *>(this);
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

    /* IIPCLayer Methods */
    HRESULT WINAPI
    InitializeSocket( LPCSTR socketPath )
    {
        TRACE("socketPath %s stub!\n", debugstr_a(socketPath));
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    SendRequestAsync( IXodusIPCPacket *packet, IAsyncOperation<IXodusIPCPacket *> **operation ) override
    {
        FIXME("packet %p, operation %p stub!\n", packet, operation);
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    add_ResponseReceived( IIPCResponseHandler *handler, EventRegistrationToken *token ) override
    {
        FIXME("handler %p, token %p stub!\n", handler, token);
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    remove_ResponseReceived( EventRegistrationToken token ) override
    {
        FIXME("token %lld stub!\n", token.value);
        return E_NOTIMPL;
    }

private:
    struct IPCHeader_CTYPE
    {
        MagicHeaderType Magic;
        UINT16 MessageType;
        UINT16 MessageLength;
    };

    static HRESULT WINAPI 
    InitializeSocketThread( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        auto iface = static_cast<IPCLayer *>( invoker );
        auto socketPath = static_cast<LPCSTR>( param );

        BYTE* messageBuffer = nullptr;
        SIZE_T offset = 0;
        HSTRING bufferClass;
        LPCWSTR bufferStr = RuntimeClass_Windows_Storage_Streams_Buffer;
        NTSTATUS status = STATUS_SUCCESS;
        POLL_SOCKET_ARGS currentPoll{};

        IBuffer *message = nullptr;
        IBufferByteAccess *messageBufferAccess = nullptr;
        IBufferFactory *bufferStatics = nullptr;

        status = WindowsCreateString( bufferStr, lstrlenW( bufferStr ), &bufferClass );
        if ( FAILED( status ) ) return status;

        status = RoGetActivationFactory( bufferClass, __uuidof( IBufferFactory ), (void **)&bufferStatics );
        WindowsDeleteString( bufferClass );
        if ( FAILED( status ) ) return status;

        // Automatically broken when the DLL is detatched. 
        while ( TRUE )
        {
            // poll_sock()
            status = __wine_unix_call( unixhandle, poll_socket, (void *)&currentPoll );
            if ( FAILED( status ) ) return HRESULT_FROM_NT( status );

            // Multiple messages may arrive at the same time.
            // Try to parse them all
            offset = 0;

            while ( TRUE )
            {
                IPCHeader_CTYPE *header;

                if ( currentPoll.curr_buffer_size - offset < sizeof(IPCHeader_CTYPE) )
                    break; //Not received the full header yet.

                header = reinterpret_cast<IPCHeader_CTYPE *>( currentPoll.curr_buffer + offset );
                
                if ( header->Magic == MagicHeaderType::Proto )
                {
                    FIXME("Proto is not yet supported!\n");
                    break;
                }
                else if ( header->Magic != MagicHeaderType::XML )
                {
                    FIXME("Invalid magic header %#x received!\n", header->Magic);
                    break;
                }

                if ( currentPoll.curr_buffer_size - offset < sizeof(IPCHeader_CTYPE) + header->MessageLength )
                    break; //We have not received the full message yet.

                status = bufferStatics->Create( header->MessageLength + 1, &message );
                if ( FAILED( status ) ) return status; //something went horribly wrong.
                status = message->QueryInterface<IBufferByteAccess>( &messageBufferAccess );
                if ( FAILED( status ) ) return status; //something went horribly wrong.
                status = messageBufferAccess->Buffer( &messageBuffer );
                if ( FAILED( status ) ) return status; //something went horribly wrong.

                if ( !messageBuffer )
                    return E_OUTOFMEMORY;

                RtlCopyMemory( (PVOID)messageBuffer, (PVOID)(currentPoll.curr_buffer + offset + sizeof(IPCHeader_CTYPE)), header->MessageLength );
                status = message->put_Length( static_cast<UINT32>(header->MessageLength) );
                offset += sizeof(IPCHeader_CTYPE) + header->MessageLength;

                /**
                 * TODO: Logic code for ResponseReceived events.
                 */
                FIXME("Received message %s\n", messageBuffer);
                // ---- //

                currentPoll = {};
                free( messageBuffer );
                messageBufferAccess->Release();
                message->Release();
            }

            if ( offset > 0 )
            {
                memmove( currentPoll.curr_buffer, currentPoll.curr_buffer + offset, currentPoll.curr_buffer_size - offset );
                currentPoll.curr_buffer_size -= offset;
            }
        }
    }

    std::atomic_long ref{ 1 };
};

static IPCLayer g_xodus_ipclayer;
IIPCLayer *xodus_ipclayer = static_cast<IIPCLayer*>(&g_xodus_ipclayer);
