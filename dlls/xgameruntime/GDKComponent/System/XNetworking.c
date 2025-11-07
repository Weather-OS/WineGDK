/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XSystem
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

#include "XNetworking.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_networking *impl_from_IXNetworkingImpl( IXNetworkingImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_networking, IXNetworkingImpl_iface );
}

static HRESULT WINAPI x_networking_QueryInterface( IXNetworkingImpl *iface, REFIID iid, void **out )
{
    struct x_networking *impl = impl_from_IXNetworkingImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IXNetworkingImpl ))
    {
        *out = &impl->IXNetworkingImpl_iface;
        impl->IXNetworkingImpl_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_networking_AddRef( IXNetworkingImpl *iface )
{
    struct x_networking *impl = impl_from_IXNetworkingImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_networking_Release( IXNetworkingImpl *iface )
{
    struct x_networking *impl = impl_from_IXNetworkingImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPort( IXNetworkingImpl *iface, UINT16 *preferredLocalUdpMultiplayerPort )
{
    FIXME( "iface %p, preferredLocalUdpMultiplayerPort %p stub!\n", iface, preferredLocalUdpMultiplayerPort );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync( IXNetworkingImpl *iface, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, asyncBlock %p stub!\n", iface, asyncBlock );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult( IXNetworkingImpl *iface, XAsyncBlock *asyncBlock, UINT16 *preferredLocalUdpMultiplayerPort )
{
    FIXME( "iface %p, asyncBlock %p, preferredLocalUdpMultiplayerPort %p stub!\n", iface, asyncBlock, preferredLocalUdpMultiplayerPort );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged( IXNetworkingImpl *iface, XTaskQueueHandle queue, PVOID context, XNetworkingPreferredLocalUdpMultiplayerPortChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_networking_XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged( IXNetworkingImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return FALSE;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlAsync( IXNetworkingImpl *iface, LPCSTR url, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, url %p, asyncBlock %p stub!\n", iface, url, asyncBlock );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlAsyncResultSize( IXNetworkingImpl *iface, XAsyncBlock *asyncBlock, SIZE_T *securityInformationBufferByteCount )
{
    FIXME( "iface %p, asyncBlock %p, securityInformationBufferByteCount %p stub!\n", iface, asyncBlock, securityInformationBufferByteCount );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlAsyncResult( IXNetworkingImpl *iface, XAsyncBlock *asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T *securityInformationBufferByteCountUsed, UINT8 *securityInformationBuffer, XNetworkingSecurityInformation **securityInformation )
{
    FIXME( "iface %p, asyncBlock %p, securityInformationBufferByteCount %lld, securityInformationBufferByteCountUsed %p, securityInformationBuffer %p, securityInformation %p stub!\n", iface, asyncBlock, securityInformationBufferByteCount, securityInformationBufferByteCountUsed, securityInformationBuffer, securityInformation );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlUtf16Async( IXNetworkingImpl *iface, LPCSTR url, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, url %p, asyncBlock %p stub!\n", iface, url, asyncBlock );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize( IXNetworkingImpl *iface, XAsyncBlock *asyncBlock, SIZE_T *securityInformationBufferByteCount )
{
    FIXME( "iface %p, asyncBlock %p, securityInformationBufferByteCount %p stub!\n", iface, asyncBlock, securityInformationBufferByteCount );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult( IXNetworkingImpl *iface, XAsyncBlock *asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T *securityInformationBufferByteCountUsed, UINT8 *securityInformationBuffer, XNetworkingSecurityInformation **securityInformation )
{
    FIXME( "iface %p, asyncBlock %p, securityInformationBufferByteCount %lld, securityInformationBufferByteCountUsed %p, securityInformationBuffer %p, securityInformation %p stub!\n", iface, asyncBlock, securityInformationBufferByteCount, securityInformationBufferByteCountUsed, securityInformationBuffer, securityInformation );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingVerifyServerCertificate( IXNetworkingImpl *iface, PVOID requestHandle, const XNetworkingSecurityInformation *securityInformation )
{
    FIXME( "iface %p, requestHandle %p, securityInformation %p stub!\n", iface, requestHandle, securityInformation );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingGetConnectivityHint( IXNetworkingImpl *iface, XNetworkingConnectivityHint *connectivityHint )
{
    FIXME( "iface %p, connectivityHint %p stub!\n", iface, connectivityHint );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingRegisterConnectivityHintChanged( IXNetworkingImpl *iface, XTaskQueueHandle queue, PVOID context, XNetworkingConnectivityHintChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_networking_XNetworkingUnregisterConnectivityHintChanged( IXNetworkingImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return FALSE;
}

static HRESULT WINAPI x_networking_XNetworkingQueryConfigurationSetting( IXNetworkingImpl *iface, XNetworkingConfigurationSetting configurationSetting, UINT64 *value )
{
    FIXME( "iface %p, configurationSetting %d, value %p stub!\n", iface, configurationSetting, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingSetConfigurationSetting( IXNetworkingImpl *iface, XNetworkingConfigurationSetting configurationParameter, UINT64 value )
{
    FIXME( "iface %p, configurationParameter %d, value %llu stub!\n", iface, configurationParameter, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_networking_XNetworkingQueryStatistics( IXNetworkingImpl *iface, XNetworkingStatisticsBuffer *statisticsBuffer )
{
    FIXME( "iface %p, statisticsBuffer %p stub!\n", iface, statisticsBuffer );
    return E_NOTIMPL;
}

static const struct IXNetworkingImplVtbl x_networking_vtbl =
{
    x_networking_QueryInterface,
    x_networking_AddRef,
    x_networking_Release,
    /* IXNetworkingImpl methods */
    x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPort,
    x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync,
    x_networking_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult,
    x_networking_XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged,
    x_networking_XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged,
    x_networking_XNetworkingQuerySecurityInformationForUrlAsync,
    x_networking_XNetworkingQuerySecurityInformationForUrlAsyncResultSize,
    x_networking_XNetworkingQuerySecurityInformationForUrlAsyncResult,
    x_networking_XNetworkingQuerySecurityInformationForUrlUtf16Async,
    x_networking_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize,
    x_networking_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult,
    x_networking_XNetworkingVerifyServerCertificate,
    x_networking_XNetworkingGetConnectivityHint,
    x_networking_XNetworkingRegisterConnectivityHintChanged,
    x_networking_XNetworkingUnregisterConnectivityHintChanged,
    x_networking_XNetworkingQueryConfigurationSetting,
    x_networking_XNetworkingSetConfigurationSetting,
    x_networking_XNetworkingQueryStatistics,
};

static struct x_networking x_networking =
{
    {&x_networking_vtbl},
    0,
};

IXNetworkingImpl *x_networking_impl = &x_networking.IXNetworkingImpl_iface;