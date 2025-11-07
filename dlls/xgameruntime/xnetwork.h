/*
 * xgameruntime.dll implementation
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

#ifndef __XNETWORK_H
#define __XNETWORK_H

#include <xnetworking.h>

typedef struct IXNetworkingImpl IXNetworkingImpl;

typedef struct IXNetworkingImplVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IXNetworkingImpl* This, REFIID riid, void** ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IXNetworkingImpl* This);
    ULONG   (STDMETHODCALLTYPE *Release)(IXNetworkingImpl* This);

    HRESULT (STDMETHODCALLTYPE *XNetworkingQueryPreferredLocalUdpMultiplayerPort)(IXNetworkingImpl* This, UINT16* preferredLocalUdpMultiplayerPort);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync)(IXNetworkingImpl* This, XAsyncBlock* asyncBlock);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult)(IXNetworkingImpl* This, XAsyncBlock* asyncBlock, UINT16* preferredLocalUdpMultiplayerPort);
    HRESULT (STDMETHODCALLTYPE *XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged)(IXNetworkingImpl* This, XTaskQueueHandle queue, PVOID context, XNetworkingPreferredLocalUdpMultiplayerPortChangedCallback* callback, XTaskQueueRegistrationToken* token);
    BOOLEAN (STDMETHODCALLTYPE *XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged)(IXNetworkingImpl* This, XTaskQueueRegistrationToken token, BOOLEAN wait);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQuerySecurityInformationForUrlAsync)(IXNetworkingImpl* This, LPCSTR url, XAsyncBlock* asyncBlock);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQuerySecurityInformationForUrlAsyncResultSize)(IXNetworkingImpl *This, XAsyncBlock* asyncBlock, SIZE_T* securityInformationBufferByteCount);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQuerySecurityInformationForUrlAsyncResult)(IXNetworkingImpl *This, XAsyncBlock* asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T* securityInformationBufferByteCountUsed, UINT8* securityInformationBuffer, XNetworkingSecurityInformation** securityInformation);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQuerySecurityInformationForUrlUtf16Async)(IXNetworkingImpl* This, LPCSTR url, XAsyncBlock* asyncBlock);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize)(IXNetworkingImpl *This, XAsyncBlock* asyncBlock, SIZE_T* securityInformationBufferByteCount);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult)(IXNetworkingImpl *This, XAsyncBlock* asyncBlock, SIZE_T securityInformationBufferByteCount, SIZE_T* securityInformationBufferByteCountUsed, UINT8* securityInformationBuffer, XNetworkingSecurityInformation** securityInformation);
    HRESULT (STDMETHODCALLTYPE *XNetworkingVerifyServerCertificate)(IXNetworkingImpl *This, PVOID requestHandle, const XNetworkingSecurityInformation* securityInformation);
    HRESULT (STDMETHODCALLTYPE *XNetworkingGetConnectivityHint)(IXNetworkingImpl *This, XNetworkingConnectivityHint* connectivityHint);
    HRESULT (STDMETHODCALLTYPE *XNetworkingRegisterConnectivityHintChanged)(IXNetworkingImpl *This, XTaskQueueHandle queue, PVOID context, XNetworkingConnectivityHintChangedCallback* callback, XTaskQueueRegistrationToken* token);
    BOOLEAN (STDMETHODCALLTYPE *XNetworkingUnregisterConnectivityHintChanged)(IXNetworkingImpl* This, XTaskQueueRegistrationToken token, BOOLEAN wait);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQueryConfigurationSetting)(IXNetworkingImpl* This, XNetworkingConfigurationSetting configurationSetting, UINT64* value);
    HRESULT (STDMETHODCALLTYPE *XNetworkingSetConfigurationSetting)(IXNetworkingImpl* This, XNetworkingConfigurationSetting configurationParameter, UINT64 value);
    HRESULT (STDMETHODCALLTYPE *XNetworkingQueryStatistics)(IXNetworkingImpl* This, XNetworkingStatisticsBuffer* statisticsBuffer);
} IXNetworkingImplVtbl;

struct IXNetworkingImpl {
    const IXNetworkingImplVtbl* lpVtbl;
};

#ifdef COBJMACROS
#ifndef WIDL_C_INLINE_WRAPPERS
/*** IUnknown methods ***/
#define IXNetworkingImpl_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IXNetworkingImpl_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IXNetworkingImpl_Release(This) (This)->lpVtbl->Release(This)
/*** IXNetworkingImpl methods ***/
#define IXNetworkingImpl_XNetworkingQueryPreferredLocalUdpMultiplayerPort(This,preferredLocalUdpMultiplayerPort) (This)->lpVtbl->XNetworkingQueryPreferredLocalUdpMultiplayerPort(This,preferredLocalUdpMultiplayerPort)
#define IXNetworkingImpl_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync(This,asyncBlock) (This)->lpVtbl->XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync(This,asyncBlock)
#define IXNetworkingImpl_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult(This,asyncBlock,preferredLocalUdpMultiplayerPort) (This)->lpVtbl->XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult(This,asyncBlock,preferredLocalUdpMultiplayerPort)
#define IXNetworkingImpl_XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged(This,queue,context,callback,token) (This)->lpVtbl->XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged(This,queue,context,callback,token)
#define IXNetworkingImpl_XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged(This,token,wait) (This)->lpVtbl->XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged(This,token,wait)
#define IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsync(This,url,asyncBlock) (This)->lpVtbl->XNetworkingQuerySecurityInformationForUrlAsync(This,url,asyncBlock)
#define IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsyncResultSize(This,asyncBlock,securityInformationBufferByteCount) (This)->lpVtbl->XNetworkingQuerySecurityInformationForUrlAsyncResultSize(This,asyncBlock,securityInformationBufferByteCount)
#define IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsyncResult(This,asyncBlock,securityInformationBufferByteCount,securityInformationBufferByteCountUsed,securityInformationBuffer,securityInformation) (This)->lpVtbl->XNetworkingQuerySecurityInformationForUrlAsyncResult(This,asyncBlock,securityInformationBufferByteCount,securityInformationBufferByteCountUsed,securityInformationBuffer,securityInformation)
#define IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16Async(This,url,asyncBlock) (This)->lpVtbl->XNetworkingQuerySecurityInformationForUrlUtf16Async(This,url,asyncBlock)
#define IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize(This,asyncBlock,securityInformationBufferByteCount) (This)->lpVtbl->XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize(This,asyncBlock,securityInformationBufferByteCount)
#define IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult(This,asyncBlock,securityInformationBufferByteCount,securityInformationBufferByteCountUsed,securityInformationBuffer,securityInformation) (This)->lpVtbl->XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult(This,asyncBlock,securityInformationBufferByteCount,securityInformationBufferByteCountUsed,securityInformationBuffer,securityInformation)
#define IXNetworkingImpl_XNetworkingVerifyServerCertificate(This,requestHandle,securityInformation) (This)->lpVtbl->XNetworkingVerifyServerCertificate(This,requestHandle,securityInformation)
#define IXNetworkingImpl_XNetworkingGetConnectivityHint(This,connectivityHint) (This)->lpVtbl->XNetworkingGetConnectivityHint(This,connectivityHint)
#define IXNetworkingImpl_XNetworkingRegisterConnectivityHintChanged(This,queue,context,callback,token) (This)->lpVtbl->XNetworkingRegisterConnectivityHintChanged(This,queue,context,callback,token)
#define IXNetworkingImpl_XNetworkingUnregisterConnectivityHintChanged(This,token,wait) (This)->lpVtbl->XNetworkingUnregisterConnectivityHintChanged(This,token,wait)
#define IXNetworkingImpl_XNetworkingQueryConfigurationSetting(This,configurationSetting,value) (This)->lpVtbl->XNetworkingQueryConfigurationSetting(This,configurationSetting,value)
#define IXNetworkingImpl_XNetworkingSetConfigurationSetting(This,configurationParameter,value) (This)->lpVtbl->XNetworkingSetConfigurationSetting(This,configurationParameter,value)
#define IXNetworkingImpl_XNetworkingQueryStatistics(This,statisticsBuffer) (This)->lpVtbl->XNetworkingQueryStatistics(This,statisticsBuffer)
#else
/*** IUnknown methods ***/
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_QueryInterface(IXNetworkingImpl* This,REFIID riid,void **ppvObject) {
    return This->lpVtbl->QueryInterface(This,riid,ppvObject);
}
static inline ULONG STDMETHODCALLTYPE IXNetworkingImpl_AddRef(IXNetworkingImpl* This) {
    return This->lpVtbl->AddRef(This);
}
static inline ULONG STDMETHODCALLTYPE IXNetworkingImpl_Release(IXNetworkingImpl* This) {
    return This->lpVtbl->Release(This);
}
/*** IXNetworkingImpl methods ***/
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQueryPreferredLocalUdpMultiplayerPort(IXNetworkingImpl* This,UINT16* preferredLocalUdpMultiplayerPort) {
    return This->lpVtbl->XNetworkingQueryPreferredLocalUdpMultiplayerPort(This,preferredLocalUdpMultiplayerPort);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync(IXNetworkingImpl* This,XAsyncBlock* asyncBlock) {
    return This->lpVtbl->XNetworkingQueryPreferredLocalUdpMultiplayerPortAsync(This,asyncBlock);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult(IXNetworkingImpl* This,XAsyncBlock* asyncBlock,UINT16* preferredLocalUdpMultiplayerPort) {
    return This->lpVtbl->XNetworkingQueryPreferredLocalUdpMultiplayerPortAsyncResult(This,asyncBlock,preferredLocalUdpMultiplayerPort);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged(IXNetworkingImpl* This,XTaskQueueHandle queue,PVOID context,XNetworkingPreferredLocalUdpMultiplayerPortChangedCallback* callback,XTaskQueueRegistrationToken* token) {
    return This->lpVtbl->XNetworkingRegisterPreferredLocalUdpMultiplayerPortChanged(This,queue,context,callback,token);
}
static inline BOOLEAN STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged(IXNetworkingImpl* This,XTaskQueueRegistrationToken token,BOOLEAN wait) {
    return This->lpVtbl->XNetworkingUnregisterPreferredLocalUdpMultiplayerPortChanged(This,token,wait);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsync(IXNetworkingImpl* This,LPCSTR url,XAsyncBlock* asyncBlock) {
    return This->lpVtbl->XNetworkingQuerySecurityInformationForUrlAsync(This,url,asyncBlock);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsyncResultSize(IXNetworkingImpl *This,XAsyncBlock* asyncBlock,SIZE_T* securityInformationBufferByteCount) {
    return This->lpVtbl->XNetworkingQuerySecurityInformationForUrlAsyncResultSize(This,asyncBlock,securityInformationBufferByteCount);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsyncResult(IXNetworkingImpl *This,XAsyncBlock* asyncBlock,SIZE_T securityInformationBufferByteCount,SIZE_T* securityInformationBufferByteCountUsed,UINT8* securityInformationBuffer,XNetworkingSecurityInformation** securityInformation) {
    return This->lpVtbl->XNetworkingQuerySecurityInformationForUrlAsyncResult(This,asyncBlock,securityInformationBufferByteCount,securityInformationBufferByteCountUsed,securityInformationBuffer,securityInformation);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16Async(IXNetworkingImpl* This,LPCSTR url,XAsyncBlock* asyncBlock) {
    return This->lpVtbl->XNetworkingQuerySecurityInformationForUrlUtf16Async(This,url,asyncBlock);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize(IXNetworkingImpl *This,XAsyncBlock* asyncBlock,SIZE_T* securityInformationBufferByteCount) {
    return This->lpVtbl->XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize(This,asyncBlock,securityInformationBufferByteCount);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult(IXNetworkingImpl *This,XAsyncBlock* asyncBlock,SIZE_T securityInformationBufferByteCount,SIZE_T* securityInformationBufferByteCountUsed,UINT8* securityInformationBuffer,XNetworkingSecurityInformation** securityInformation) {
    return This->lpVtbl->XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult(This,asyncBlock,securityInformationBufferByteCount,securityInformationBufferByteCountUsed,securityInformationBuffer,securityInformation);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingVerifyServerCertificate(IXNetworkingImpl *This,PVOID requestHandle,const XNetworkingSecurityInformation* securityInformation) {
    return This->lpVtbl->XNetworkingVerifyServerCertificate(This,requestHandle,securityInformation);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingGetConnectivityHint(IXNetworkingImpl *This,XNetworkingConnectivityHint* connectivityHint) {
    return This->lpVtbl->XNetworkingGetConnectivityHint(This,connectivityHint);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingRegisterConnectivityHintChanged(IXNetworkingImpl *This,XTaskQueueHandle queue,PVOID context,XNetworkingConnectivityHintChangedCallback* callback,XTaskQueueRegistrationToken* token) {
    return This->lpVtbl->XNetworkingRegisterConnectivityHintChanged(This,queue,context,callback,token);
}
static inline BOOLEAN STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingUnregisterConnectivityHintChanged(IXNetworkingImpl* This,XTaskQueueRegistrationToken token,BOOLEAN wait) {
    return This->lpVtbl->XNetworkingUnregisterConnectivityHintChanged(This,token,wait);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQueryConfigurationSetting(IXNetworkingImpl* This,XNetworkingConfigurationSetting configurationSetting,UINT64* value) {
    return This->lpVtbl->XNetworkingQueryConfigurationSetting(This,configurationSetting,value);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingSetConfigurationSetting(IXNetworkingImpl* This,XNetworkingConfigurationSetting configurationParameter,UINT64 value) {
    return This->lpVtbl->XNetworkingSetConfigurationSetting(This,configurationParameter,value);
}
static inline HRESULT STDMETHODCALLTYPE IXNetworkingImpl_XNetworkingQueryStatistics(IXNetworkingImpl* This,XNetworkingStatisticsBuffer* statisticsBuffer) {
    return This->lpVtbl->XNetworkingQueryStatistics(This,statisticsBuffer);
}
#endif
#endif

// 37e56907-2f10-41e8-b72f-36edb185331a
DEFINE_GUID(CLSID_XNetworkingImpl, 0x37e56907, 0x2f10, 0x41e8, 0xb7,0x2f, 0x36,0xed,0xb1,0x85,0x33,0x1a);

// bf2346b2-39af-4658-b5ea-44713c7e83b3
DEFINE_GUID(IID_IXNetworkingImpl, 0xbf2346b2, 0x39af, 0x4658, 0xb5,0xea, 0x44,0x71,0x3c,0x7e,0x83,0xb3);

#endif