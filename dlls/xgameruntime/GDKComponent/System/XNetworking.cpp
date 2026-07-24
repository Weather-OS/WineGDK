/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XNetworking
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

#include <cstring>
#include <atomic>
#include <winhttp.h>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static HRESULT WINAPI security_information_provider( XAsyncOp op, const XAsyncProviderData *data )
{
    IXThreadingImpl *threading;
    HRESULT hr;

    TRACE( "op %u, asyncBlock %p, bufferSize %Iu, buffer %p.\n",
            static_cast<unsigned int>(op), data->async, data->bufferSize, data->buffer );
    if (FAILED(hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&threading )))
        return hr;

    switch (op)
    {
        case XAsyncOp::Begin:
            hr = threading->XAsyncSchedule( data->async, 0 );
            break;
        case XAsyncOp::DoWork:
            threading->XAsyncComplete( data->async, S_OK, sizeof(XNetworkingSecurityInformation) );
            hr = S_OK;
            break;
        case XAsyncOp::GetResult:
        {
            auto *securityInformation = static_cast<XNetworkingSecurityInformation *>(data->buffer);
            securityInformation->enabledHttpSecurityProtocolFlags = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1
                    | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
            securityInformation->thumbprintCount = 0;
            securityInformation->thumbprints = nullptr;
            hr = S_OK;
            break;
        }
        case XAsyncOp::Cancel:
        case XAsyncOp::Cleanup:
            hr = S_OK;
            break;
    }

    threading->Release();
    return hr;
}


class XNetworkingImpl : 
    public IXNetworkingImpl
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out )
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXNetworkingImpl ) ||
             iid == __uuidof( IXNetworkingImpl2 ) )
        {
            AddRef();
            *out = static_cast<IXNetworkingImpl *>(this);
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

    HRESULT WINAPI XNetworkingQueryPreferredLocalUdpMultiplayerPort( UINT16 *preferredLocalUdpMultiplayerPort ) override
    {
        FIXME( "preferredLocalUdpMultiplayerPort %p stub!\n", preferredLocalUdpMultiplayerPort );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync( XAsyncBlock *asyncBlock ) override
    {
        FIXME( "asyncBlock %p stub!\n", asyncBlock );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult( XAsyncBlock *asyncBlock, UINT16 *preferredLocalUdpMultiplayerPort ) override
    {
        FIXME( "asyncBlock %p, preferredLocalUdpMultiplayerPort %p stub!\n", asyncBlock, preferredLocalUdpMultiplayerPort );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged( XTaskQueueHandle queue, PVOID context, XNetworkingPreferredLocalUdpMultiplayerPortChangedCallback *callback, XTaskQueueRegistrationToken *token ) override
    {
        FIXME( "queue %p, context %p, callback %p, token %p stub!\n", queue, context, callback, token );
        return E_NOTIMPL;
    }

    BOOLEAN WINAPI XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged( XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        FIXME( "token %p, wait %d stub!\n", &token, wait );
        return FALSE;
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlAsync( LPCSTR url, XAsyncBlock *asyncBlock ) override
    {
        IXThreadingImpl *threading;
        HRESULT hr;

        TRACE( "url %s, asyncBlock %p.\n", debugstr_a( url ), asyncBlock );
        if (!url || !asyncBlock) return E_POINTER;
        if (FAILED(hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&threading )))
            return hr;
        hr = threading->XAsyncBegin( asyncBlock, nullptr, nullptr,
                "XNetworkingQuerySecurityInformationForUrlAsync", security_information_provider );
        threading->Release();
        return hr;
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlAsyncResultSize( XAsyncBlock *asyncBlock, SIZE_T *securityInformationBufferByteCount ) override
    {
        IXThreadingImpl *threading;
        HRESULT hr;

        TRACE( "asyncBlock %p, securityInformationBufferByteCount %p.\n", asyncBlock, securityInformationBufferByteCount );
        if (!asyncBlock || !securityInformationBufferByteCount) return E_POINTER;
        if (FAILED(hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&threading )))
            return hr;
        hr = threading->XAsyncGetResultSize( asyncBlock, securityInformationBufferByteCount );
        threading->Release();
        return hr;
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlAsyncResult( XAsyncBlock *asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T *securityInformationBufferByteCountUsed, UINT8 *securityInformationBuffer, XNetworkingSecurityInformation **securityInformation ) override
    {
        IXThreadingImpl *threading;
        HRESULT hr;

        TRACE( "asyncBlock %p, securityInformationBufferByteCount %Iu, securityInformationBufferByteCountUsed %p, securityInformationBuffer %p, securityInformation %p.\n",
                asyncBlock, securityInformationBufferByteCount, securityInformationBufferByteCountUsed, securityInformationBuffer, securityInformation );
        if (!asyncBlock || !securityInformationBuffer || !securityInformation) return E_POINTER;
        if (FAILED(hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&threading )))
            return hr;
        if (SUCCEEDED(hr = threading->XAsyncGetResult( asyncBlock, nullptr, securityInformationBufferByteCount,
                securityInformationBuffer, securityInformationBufferByteCountUsed )))
            *securityInformation = reinterpret_cast<XNetworkingSecurityInformation *>(securityInformationBuffer);
        threading->Release();
        return hr;
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlUtf16Async( LPCWSTR url, XAsyncBlock *asyncBlock ) override
    {
        IXThreadingImpl *threading;
        HRESULT hr;

        TRACE( "url %s, asyncBlock %p.\n", debugstr_w( url ), asyncBlock );
        if (!url || !asyncBlock) return E_POINTER;
        if (FAILED(hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&threading )))
            return hr;
        hr = threading->XAsyncBegin( asyncBlock, nullptr, nullptr,
                "XNetworkingQuerySecurityInformationForUrlUtf16Async", security_information_provider );
        threading->Release();
        return hr;
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize( XAsyncBlock *asyncBlock, SIZE_T *securityInformationBufferByteCount ) override
    {
        return XNetworkingQuerySecurityInformationForUrlAsyncResultSize( asyncBlock, securityInformationBufferByteCount );
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult( XAsyncBlock *asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T *securityInformationBufferByteCountUsed, UINT8 *securityInformationBuffer, XNetworkingSecurityInformation **securityInformation ) override
    {
        return XNetworkingQuerySecurityInformationForUrlAsyncResult( asyncBlock, securityInformationBufferByteCount,
                securityInformationBufferByteCountUsed, securityInformationBuffer, securityInformation );
    }

    HRESULT WINAPI XNetworkingVerifyServerCertificate( PVOID requestHandle, const XNetworkingSecurityInformation *securityInformation ) override
    {
        FIXME( "requestHandle %p, securityInformation %p stub!\n", requestHandle, securityInformation );
        return S_OK;
    }

    HRESULT WINAPI XNetworkingGetConnectivityHint( XNetworkingConnectivityHint *connectivityHint ) override
    {
        XNetworkingConnectivityHint hint;

        TRACE( "connectivityHint %p\n", connectivityHint );

        hint.ianaInterfaceType = 0; // There's no direct way to get NDIS interface type in userspace.
        hint.roaming = FALSE;
        hint.overDataLimit = FALSE;
        hint.networkInitialized = TRUE;
        hint.approachingDataLimit = FALSE;
        hint.connectivityLevel = XNetworkingConnectivityLevelHint::InternetAccess;
        hint.connectivityCost = XNetworkingConnectivityCostHint::Unrestricted;

        *connectivityHint = hint;

        return S_OK;
    }

    HRESULT WINAPI XNetworkingRegisterConnectivityHintChanged( XTaskQueueHandle queue, PVOID context, XNetworkingConnectivityHintChangedCallback *callback, XTaskQueueRegistrationToken *token ) override
    {
        XNetworkingConnectivityHint hint;
        FIXME( "queue %p, context %p, callback %p, token %p stub!\n", queue, context, callback, token );
        XNetworkingGetConnectivityHint( &hint );
        callback( context, &hint );
        return S_OK;
    }

    BOOLEAN WINAPI XNetworkingUnregisterConnectivityHintChanged( XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        FIXME( "token %p, wait %d stub!\n", &token, wait );
        return FALSE;
    }

private:
    std::atomic_long ref{ 1 };
};

static XNetworkingImpl g_x_networking;

IXNetworkingImpl *x_networking = static_cast<IXNetworkingImpl*>(&g_x_networking);