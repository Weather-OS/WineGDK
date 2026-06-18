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

#include <wine/list.h>
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
    InitializeSocket()
    {
        IAsyncAction *operation;
        TRACE("\n");
        return AsyncAction::Create( static_cast<IUnknown *>(this), nullptr, InitializeSocketThread, &operation );
    }

    HRESULT WINAPI
    SendRequestAsync( IXodusIPCPacket *packet, IAsyncOperation<IXodusIPCPacket *> **operation ) override
    {
        HRESULT hr;

        packet->AddRef();        
        hr = AsyncOperation::Inspectable::Create( static_cast<IUnknown *>(this), 
                                packet, SendRequest, { .operation = &__uuidof( IAsyncOperation<IXodusIPCPacket *> ) }, (IAsyncOperation<IInspectable *> **)operation );

        return hr;
    }

    HRESULT WINAPI
    add_ResponseReceived( IIPCResponseHandler *handler, EventRegistrationToken *token ) override
    {
        response_received_callback *newCallback = new response_received_callback();
        newCallback->handler = handler;

        TRACE("handler %p, token %p.\n", handler, token);

        handler->AddRef();
        token->value = m_NextEventToken++;
        list_add_head( &m_Callbacks, &newCallback->entry );

        return S_OK;
    }

    HRESULT WINAPI
    remove_ResponseReceived( EventRegistrationToken token ) override
    {
        response_received_callback *oldCallback;

        TRACE("token %lld.\n", token.value);

        LIST_FOR_EACH_ENTRY( oldCallback, &m_Callbacks, response_received_callback, entry )
        {
            if ( oldCallback->token == token.value )
            {
                oldCallback->handler->Release();
                list_remove( &oldCallback->entry );
                return S_OK;
            }
        }

        return E_BOUNDS;
    }

private:
    struct IPCHeader_CTYPE
    {
        MagicHeaderType Magic;
        UINT16 Message_Type;
        UINT16 MessageLength;
    };

    struct IPCFrame
    {
        UINT32 frameSize;
        BYTE* frame;
    };
    
    struct SendRequestContext
    {
        HANDLE event;
        IXodusIPCPacket *response;
    };

    static HRESULT WINAPI 
    SendRequest( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        auto iface = static_cast<IPCLayer *>( invoker );
        auto packet = static_cast<IXodusIPCPacket *>( param );

        BYTE* messageBuffer;
        DWORD asyncres;
        HRESULT status = S_OK;
        NTSTATUS nts;
        IPCFrame *frame = new IPCFrame();
        IPCHeader_CTYPE header{};
        EventRegistrationToken token{};
        SendRequestContext context{ .event = CreateEventW(nullptr, TRUE, FALSE, nullptr) };
        IPCResponseHandler *handler = new IPCResponseHandler( SendRequestResponseHandler, (PVOID)&context );

        IBuffer *message;
        IBufferByteAccess *messageBufferByteAccess;

        TRACE("invoker %p, param %p, result %p\n", invoker, param, result);

        packet->get_Magic( &header.Magic );
        packet->get_MessageType( &header.Message_Type );
        packet->get_Message( &message );
        packet->Release();

        status = message->get_Length( &frame->frameSize );
        if ( FAILED( status ) ) return status;
        status = message->QueryInterface<IBufferByteAccess>( &messageBufferByteAccess );
        message->Release();
        if ( FAILED( status ) ) return status;
        status = messageBufferByteAccess->Buffer( &messageBuffer );
        messageBufferByteAccess->Release();
        if ( FAILED( status ) ) return status;

        header.MessageLength = frame->frameSize;

        frame->frameSize += sizeof(IPCHeader_CTYPE);

        frame->frame = (PBYTE)CoTaskMemAlloc( sizeof(BYTE) * frame->frameSize );
        if ( !frame->frame )
            return E_OUTOFMEMORY;

        RtlCopyMemory( frame->frame, &header.Magic, sizeof(MagicHeaderType) );
        RtlCopyMemory( frame->frame + sizeof(MagicHeaderType), &header.Message_Type, sizeof(UINT16) );
        RtlCopyMemory( frame->frame + sizeof(MagicHeaderType) + sizeof(UINT16), &header.MessageLength, sizeof(UINT16) );
        RtlCopyMemory( frame->frame + sizeof(IPCHeader_CTYPE), messageBuffer, header.MessageLength );

        status = iface->add_ResponseReceived( handler, &token );
        if ( FAILED( status ) ) return status;

        nts = __wine_unix_call( unixhandle, send_frame, (void *)frame );
        CoTaskMemFree( frame->frame );
        if ( FAILED( nts ) ) return HRESULT_FROM_NT( nts );

        asyncres = WaitForSingleObject( context.event, IPC_REQUEST_TIMEOUT_MS );
        status = iface->remove_ResponseReceived( token );
        if ( FAILED( status ) ) return status;
        if ( asyncres )
        {
            WARN("Timeout while waiting for %p to respond.\n", handler);
            return HRESULT_FROM_NT( STATUS_TIMEOUT );
        }

        result->vt = VT_UNKNOWN;
        result->punkVal = context.response;

        return S_OK;
    }

    static HRESULT WINAPI
    SendRequestResponseHandler( PVOID context, IXodusIPCPacket *packet )
    {
        auto ctx = reinterpret_cast<SendRequestContext *>( context );

        HRESULT status;
        UINT16 messageType;

        TRACE("context %p, packet %p\n", context, packet);

        status = packet->get_MessageType( &messageType );
        if ( FAILED( status ) ) return status;

        if ( messageType == 1 /* PING */ )
            return S_OK; //Skip the packet we sent.

        if ( messageType == 2 /* PONG */ )
            TRACE("Got PONGED!\n");

        ctx->response = packet;
        SetEvent( ctx->event );
        return S_OK;
    }

    static HRESULT WINAPI 
    InitializeSocketThread( IUnknown *invoker, PVOID param, PROPVARIANT *result )
    {
        auto iface = static_cast<IPCLayer *>( invoker );

        BYTE* messageBuffer = nullptr;
        SIZE_T offset = 0;
        HSTRING bufferClass;
        NTSTATUS status = STATUS_SUCCESS;
        POLL_SOCKET_ARGS currentPoll{};
        response_received_callback *currCallback;

        IBuffer *message = nullptr;
        IBufferByteAccess *messageBufferAccess = nullptr;
        IBufferFactory *bufferFactory = nullptr;
        IXodusIPCPacket *xodusPacket = nullptr;

        TRACE("invoker %p, param %p, result %p\n", invoker, param, result);

        status = WindowsCreateString( RuntimeClass_Windows_Storage_Streams_Buffer, lstrlenW( RuntimeClass_Windows_Storage_Streams_Buffer ), &bufferClass );
        if ( FAILED( status ) ) return status;

        status = RoGetActivationFactory( bufferClass, __uuidof( IBufferFactory ), (void **)&bufferFactory );
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
                    FIXME("Invalid magic header %#x received!\n", (int)header->Magic);
                    break;
                }

                if ( currentPoll.curr_buffer_size - offset < sizeof(IPCHeader_CTYPE) + header->MessageLength )
                    break; //We have not received the full message yet.
                TRACE("header->Message_Type is %d!\n", header->Message_Type);

                /**
                 * TODO: Should we ignore messages sent by ourselves?
                 * if ( header->Message_Type == MessageType::Ping ||
                 *     header->Message_Type == MessageType::XstsTokenRequest )
                 *    break;
                 */

                status = bufferFactory->Create( header->MessageLength + 1, &message );
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

                messageBuffer[header->MessageLength] = '\0';

                xodusPacket = new XodusIPCPacket( header->Magic, header->Message_Type, message );

                LIST_FOR_EACH_ENTRY( currCallback, &iface->m_Callbacks, response_received_callback, entry )
                {
                    currCallback->handler->Invoke( xodusPacket );
                }
                // ---- //

                xodusPacket->Release();
                messageBufferAccess->Release();
                message->Release();
            }

            if ( offset > 0 )
            {
                memmove( currentPoll.curr_buffer, currentPoll.curr_buffer + offset, currentPoll.curr_buffer_size - offset );
                currentPoll.curr_buffer_size -= offset;
            }
        }

        bufferFactory->Release();
    }

    struct response_received_callback
    {
        struct list entry;
        IIPCResponseHandler *handler;
        INT64 token;
    };

    struct list m_Callbacks = LIST_INIT( m_Callbacks );
    std::atomic<INT64> m_NextEventToken{ 0 };
    std::atomic_long ref{ 1 };
};

static IPCLayer g_xodus_ipclayer;
IIPCLayer *xodus_ipclayer = static_cast<IIPCLayer*>(&g_xodus_ipclayer);
