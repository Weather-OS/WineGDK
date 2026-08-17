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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

/* Semi-stub for the security information query: we have no pinned certificates to
 * report, so complete the operation with an empty XNetworkingSecurityInformation.
 * Callers wait on the XAsyncBlock, so failing synchronously would leave it pending
 * forever and the caller would spin. */
static HRESULT __stdcall security_information_provider( XAsyncOp op, const XAsyncProviderData *data )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    TRACE( "op %d, data %p.\n", (int)op, data );

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    switch (op)
    {
        case XAsyncOp::Begin:
            return xthreading->XAsyncSchedule( data->async, 0 );

        case XAsyncOp::DoWork:
            /* Succeed with an empty thumbprint set: nothing is pinned, so ordinary
             * certificate validation applies. Failing here instead aborts the caller's
             * transfer half-way through - XCurl then runs its completion routine on a
             * handle it never attached to a multi and dereferences null. */
            xthreading->XAsyncComplete( data->async, S_OK, sizeof(XNetworkingSecurityInformation) );
            /* Having completed the operation ourselves, the framework must not
             * complete it a second time - that is what E_PENDING means here. */
            return E_PENDING;

        case XAsyncOp::GetResult:
        {
            XNetworkingSecurityInformation *info = (XNetworkingSecurityInformation *)data->buffer;

            info->enabledHttpSecurityProtocolFlags = 0;
            info->thumbprintCount = 0;
            info->thumbprints = nullptr;
            return S_OK;
        }

        default:
            break;
    }

    return S_OK;
}

/* The provider address doubles as the async identity, so the ANSI and UTF-16
 * entry points share one implementation. */
static HRESULT begin_security_information_query( XAsyncBlock *asyncBlock )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    return xthreading->XAsyncBegin( asyncBlock, nullptr, (void *)security_information_provider,
                                    "XNetworkingQuerySecurityInformationForUrl",
                                    security_information_provider );
}

static HRESULT security_information_result_size( XAsyncBlock *asyncBlock, SIZE_T *byteCount )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    return xthreading->XAsyncGetResultSize( asyncBlock, byteCount );
}

static HRESULT security_information_result( XAsyncBlock *asyncBlock, SIZE_T byteCount, SIZE_T *byteCountUsed,
                                            UINT8 *buffer, XNetworkingSecurityInformation **securityInformation )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    hr = xthreading->XAsyncGetResult( asyncBlock, (void *)security_information_provider,
                                      byteCount, buffer, byteCountUsed );
    if ( SUCCEEDED( hr ) && securityInformation )
        *securityInformation = (XNetworkingSecurityInformation *)buffer;

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
        FIXME( "url %p, asyncBlock %p semi-stub.\n", url, asyncBlock );
        return begin_security_information_query( asyncBlock );
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlAsyncResultSize( XAsyncBlock *asyncBlock, SIZE_T *securityInformationBufferByteCount ) override
    {
        TRACE( "asyncBlock %p, securityInformationBufferByteCount %p.\n", asyncBlock, securityInformationBufferByteCount );
        return security_information_result_size( asyncBlock, securityInformationBufferByteCount );
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlAsyncResult( XAsyncBlock *asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T *securityInformationBufferByteCountUsed, UINT8 *securityInformationBuffer, XNetworkingSecurityInformation **securityInformation ) override
    {
        TRACE( "asyncBlock %p, securityInformationBufferByteCount %lld, securityInformationBufferByteCountUsed %p, securityInformationBuffer %p, securityInformation %p\n", asyncBlock, securityInformationBufferByteCount, securityInformationBufferByteCountUsed, securityInformationBuffer, securityInformation );
        return security_information_result( asyncBlock, securityInformationBufferByteCount,
                                            securityInformationBufferByteCountUsed,
                                            securityInformationBuffer, securityInformation );
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlUtf16Async( LPCWSTR url, XAsyncBlock *asyncBlock ) override
    {
        FIXME( "url %p, asyncBlock %p semi-stub.\n", url, asyncBlock );
        return begin_security_information_query( asyncBlock );
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize( XAsyncBlock *asyncBlock, SIZE_T *securityInformationBufferByteCount ) override
    {
        TRACE( "asyncBlock %p, securityInformationBufferByteCount %p.\n", asyncBlock, securityInformationBufferByteCount );
        return security_information_result_size( asyncBlock, securityInformationBufferByteCount );
    }

    HRESULT WINAPI XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult( XAsyncBlock *asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T *securityInformationBufferByteCountUsed, UINT8 *securityInformationBuffer, XNetworkingSecurityInformation **securityInformation ) override
    {
        TRACE( "asyncBlock %p, securityInformationBufferByteCount %lld, securityInformationBufferByteCountUsed %p, securityInformationBuffer %p, securityInformation %p\n", asyncBlock, securityInformationBufferByteCount, securityInformationBufferByteCountUsed, securityInformationBuffer, securityInformation );
        return security_information_result( asyncBlock, securityInformationBufferByteCount,
                                            securityInformationBufferByteCountUsed,
                                            securityInformationBuffer, securityInformation );
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

        FIXME( "queue %p, context %p, callback %p semi-stub: connectivity never changes.\n", queue, context, callback );

        /* The caller keeps this token and unregisters with it later, so it has to be
         * written even though no further change will ever be reported. */
        if ( token ) token->token = ++m_NextHintToken;

        if ( callback )
        {
            XNetworkingGetConnectivityHint( &hint );
            callback( context, &hint );
        }
        return S_OK;
    }

    BOOLEAN WINAPI XNetworkingUnregisterConnectivityHintChanged( XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        TRACE( "token %llu, wait %d.\n", (UINT64)token.token, wait );
        return TRUE;
    }

private:
    std::atomic_long ref{ 1 };
    std::atomic<UINT64> m_NextHintToken{ 0 };
};

static XNetworkingImpl g_x_networking;

IXNetworkingImpl *x_networking = static_cast<IXNetworkingImpl*>(&g_x_networking);