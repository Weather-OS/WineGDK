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

#ifndef __XTHREAD_H
#define __XTHREAD_H

#include "xasyncprovider.h"

typedef struct IXThreadingImpl IXThreadingImpl;

typedef struct IXThreadingImplVtbl {
    HRESULT (*QueryInterface)(IXThreadingImpl* This, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IXThreadingImpl* This);
    ULONG (*Release)(IXThreadingImpl* This);
    
    HRESULT (*XAsyncGetStatus)(IXThreadingImpl* This, XAsyncBlock* asyncBlock, boolean wait);
    HRESULT (*XAsyncGetResultSize)(IXThreadingImpl* This, XAsyncBlock* asyncBlock, SIZE_T *bufferSize);
    HRESULT (*XAsyncCancel)(IXThreadingImpl* This, XAsyncBlock* asyncBlock);
    HRESULT (*XAsyncRun)(IXThreadingImpl* This, XAsyncBlock* asyncBlock, XAsyncWork* work);
    HRESULT (*XAsyncBegin)(IXThreadingImpl* This, XAsyncBlock* asyncBlock, PVOID context, const PVOID identity, LPCSTR identityName, XAsyncProviderCallback* provider);
    HRESULT (*__PADDING__)(IXThreadingImpl* This);
    HRESULT (*XAsyncSchedule)(IXThreadingImpl* This, XAsyncBlock* asyncBlock, UINT32 delayInMs);
    VOID    (*XAsyncComplete)(IXThreadingImpl* This, XAsyncBlock* asyncBlock, HRESULT result, SIZE_T requiredBufferSize);
    HRESULT (*XAsyncGetResult)(IXThreadingImpl* This, XAsyncBlock* asyncBlock, const PVOID identity, SIZE_T bufferSize, PVOID buffer, SIZE_T* bufferUsed);
} IXThreadingImplVtbl;

struct IXThreadingImpl {
    const IXThreadingImplVtbl* lpVtbl;
};

#ifdef COBJMACROS
#ifndef WIDL_C_INLINE_WRAPPERS
/*** IUnknown methods ***/
#define IXThreadingImpl_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define IXThreadingImpl_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IXThreadingImpl_Release(This) (This)->lpVtbl->Release(This)
/*** IXThreadingImpl methods ***/
#define IXThreadingImpl_XAsyncGetStatus(This,asyncBlock,wait) (This)->lpVtbl->XAsyncGetStatus(This,asyncBlock,wait)
#define IXThreadingImpl_XAsyncGetResultSize(This,asyncBlock,bufferSize) (This)->lpVtbl->XAsyncGetResultSize(This,asyncBlock,bufferSize)
#define IXThreadingImpl_XAsyncCancel(This,asyncBlock) (This)->lpVtbl->XAsyncCancel(This,asyncBlock)
#define IXThreadingImpl_XAsyncRun(This,asyncBlock,work) (This)->lpVtbl->XAsyncRun(This,asyncBlock,work)
#define IXThreadingImpl_XAsyncBegin(This,asyncBlock,context,identity,identityName,provider) (This)->lpVtbl->XAsyncBegin(This,asyncBlock,context,identity,identityName,provider)
#define IXThreadingImpl_XAsyncSchedule(This,asyncBlock,delayInMs) (This)->lpVtbl->XAsyncSchedule(This,asyncBlock,delayInMs)
#define IXThreadingImpl_XAsyncComplete(This,asyncBlock,result,requiredBufferSize) (This)->lpVtbl->XAsyncComplete(This,asyncBlock,result,requiredBufferSize)
#define IXThreadingImpl_XAsyncGetResult(This,asyncBlock,identity,bufferSize,buffer,bufferUsed) (This)->lpVtbl->XAsyncGetResult(This,asyncBlock,identity,bufferSize,buffer,bufferUsed)
#else
/*** IUnknown methods ***/
static inline HRESULT IXThreadingImpl_QueryInterface(IXThreadingImpl* This,REFIID riid,void **ppvObject) {
    return This->lpVtbl->QueryInterface(This,riid,ppvObject);
}
static inline ULONG IXThreadingImpl_AddRef(IXThreadingImpl* This) {
    return This->lpVtbl->AddRef(This);
}
static inline ULONG IXThreadingImpl_Release(IXThreadingImpl* This) {
    return This->lpVtbl->Release(This);
}
/*** IXGameRuntimeFeatureImpl methods ***/
static inline HRESULT IXThreadingImpl_XAsyncGetStatus(IXThreadingImpl* This,XAsyncBlock* asyncBlock,boolean wait) {
    return This->lpVtbl->XAsyncGetStatus(This,asyncBlock,wait);
}
static inline HRESULT IXThreadingImpl_XAsyncGetResultSize(IXThreadingImpl* This,XAsyncBlock* asyncBlock,SIZE_T *bufferSize) {
    return This->lpVtbl->XAsyncGetResultSize(This,asyncBlock,bufferSize);
}
static inline HRESULT IXThreadingImpl_XAsyncCancel(IXThreadingImpl* This,XAsyncBlock* asyncBlock) {
    return This->lpVtbl->XAsyncCancel(This,asyncBlock);
}
static inline HRESULT IXThreadingImpl_XAsyncRun(IXThreadingImpl* This,XAsyncBlock* asyncBlock,XAsyncWork* work) {
    return This->lpVtbl->XAsyncRun(This,asyncBlock,work);
}
static inline HRESULT IXThreadingImpl_XAsyncBegin(IXThreadingImpl* This,XAsyncBlock* asyncBlock,PVOID context,const PVOID identity,LPCSTR identityName,XAsyncProviderCallback* provider) {
    return This->lpVtbl->XAsyncBegin(This,asyncBlock,context,identity,identityName,provider);
}
static inline HRESULT IXThreadingImpl_XAsyncSchedule(IXThreadingImpl* This,XAsyncBlock* asyncBlock,UINT32 delayInMs) {
    return This->lpVtbl->XAsyncSchedule(This,asyncBlock,delayInMs);
}
static inline VOID IXThreadingImpl_XAsyncComplete(IXThreadingImpl* This,XAsyncBlock* asyncBlock,HRESULT result,SIZE_T requiredBufferSize) {
    return This->lpVtbl->XAsyncComplete(This,asyncBlock,result,requiredBufferSize);
}
static inline VOID IXThreadingImpl_XAsyncGetResult(IXThreadingImpl* This,XAsyncBlock* asyncBlock,const PVOID identity,SIZE_T bufferSize,PVOID buffer,SIZE_T* bufferUsed) {
    This->lpVtbl->XAsyncGetResult(This,asyncBlock,identity,bufferSize,buffer,bufferUsed);
}
#endif
#endif

// 073b7dcb-1fcf-4030-94be-e3c9eb623428

DEFINE_GUID(CLSID_XThreadingImpl, 0x073b7dcb, 0x1fcf, 0x4030, 0x94,0xbe, 0xe3,0xc9,0xeb,0x62,0x34,0x28);
DEFINE_GUID(IID_IXThreadingImpl, 0x073b7dcb, 0x1fcf, 0x4030, 0x94,0xbe, 0xe3,0xc9,0xeb,0x62,0x34,0x28);

#endif