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

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

using namespace ABI;
using namespace ABI::Xodus;

class ABI::Xodus::XodusIPCPacket :
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
        Windows::Storage::Streams::IBuffer *message )
    : m_type(type),
      m_messageType(messageType)
    {
        m_message = message;
        message->AddRef();
    }

    /* IUnknown Methods */
    HRESULT WINAPI QueryInterface( REFIID iid, void **out )
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

        if ( !curr )
        {
            m_message->Release();
            delete this;
        }

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

    /* IXodusIPCPacket Methods */
    HRESULT WINAPI
    get_Magic( MagicHeaderType *type ) override
    {
        FIXME("iface %p, type %p stub!\n", this, type);
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    get_MessageType( UINT16 *messageType ) override
    {
        FIXME("iface %p, messageType %p stub!\n", this, messageType);
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    get_Message( Windows::Storage::Streams::IBuffer **message ) override
    {
        FIXME("iface %p, message %p stub!\n", this, message);
        return E_NOTIMPL;
    }

private:
    MagicHeaderType m_type;
    UINT16 m_messageType;
    Windows::Storage::Streams::IBuffer *m_message;
    std::atomic_long ref{ 1 };
};