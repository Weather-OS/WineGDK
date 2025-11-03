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

#include <xasyncprovider.h>

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
    HRESULT (*XTaskQueueCreate)(IXThreadingImpl* This, XTaskQueueDispatchMode workDispatchMode, XTaskQueueDispatchMode completionDispatchMode, XTaskQueueHandle* queue);
    HRESULT (*XTaskQueueCreateComposite)(IXThreadingImpl* This, XTaskQueuePortHandle workPort, XTaskQueuePortHandle completionPort, XTaskQueueHandle* queue);
    HRESULT (*XTaskQueueGetPort)(IXThreadingImpl* This, XTaskQueueHandle queue, XTaskQueuePort port, XTaskQueuePortHandle* portHandle);
    HRESULT (*XTaskQueueDuplicateHandle)(IXThreadingImpl* This, XTaskQueueHandle queueHandle, XTaskQueueHandle* duplicatedHandle);
    BOOLEAN (*XTaskQueueDispatch)(IXThreadingImpl* This, XTaskQueueHandle queue, XTaskQueuePort port, uint32_t timeoutInMs);
    VOID    (*XTaskQueueCloseHandle)(IXThreadingImpl* This, XTaskQueueHandle queue);
    HRESULT (*XTaskQueueSubmitCallback)(IXThreadingImpl* This, XTaskQueueHandle queue, XTaskQueuePort port, PVOID callbackContext, XTaskQueueCallback* callback);
    HRESULT (*XTaskQueueSubmitDelayedCallback)(IXThreadingImpl* This, XTaskQueueHandle queue, XTaskQueuePort port, uint32_t delayMs, PVOID callbackContext, XTaskQueueCallback* callback);
    HRESULT (*XTaskQueueRegisterWaiter)(IXThreadingImpl* This, XTaskQueueHandle queue, XTaskQueuePort port, HANDLE waitHandle, PVOID callbackContext, XTaskQueueCallback* callback, XTaskQueueRegistrationToken* token);
    VOID    (*XTaskQueueUnregisterWaiter)(IXThreadingImpl* This, XTaskQueueHandle queue, XTaskQueueRegistrationToken token);
    HRESULT (*XTaskQueueTerminate)(IXThreadingImpl* This, XTaskQueueHandle queue, BOOLEAN wait, PVOID callbackContext, XTaskQueueTerminatedCallback* callback);
    HRESULT (*XTaskQueueRegisterMonitor)(IXThreadingImpl* This, XTaskQueueHandle queue, PVOID callbackContext, XTaskQueueMonitorCallback* callback, XTaskQueueRegistrationToken* token);
    VOID    (*XTaskQueueUnregisterMonitor)(IXThreadingImpl* This, XTaskQueueHandle queue, XTaskQueueRegistrationToken token);
    BOOLEAN (*XTaskQueueGetCurrentProcessTaskQueue)(IXThreadingImpl* This, XTaskQueueHandle* queue);
    VOID    (*XTaskQueueSetCurrentProcessTaskQueue)(IXThreadingImpl* This, XTaskQueueHandle queue);
    HRESULT (*XThreadSetTimeSensitive)(IXThreadingImpl* This, BOOLEAN isTimeSensitiveThread);
    HRESULT (*__PADDING_2__)(IXThreadingImpl* This);
    VOID    (*XThreadAssertNotTimeSensitive)(IXThreadingImpl* This);
    BOOLEAN (*XThreadIsTimeSensitive)(IXThreadingImpl* This);
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
#define IXThreadingImpl_XTaskQueueCreate(This,workDispatchMode,completionDispatchMode,queue) (This)->lpVtbl->XTaskQueueCreate(This,workDispatchMode,completionDispatchMode,queue)
#define IXThreadingImpl_XTaskQueueCreateComposite(This,workPort,completionPort,queue) (This)->lpVtbl->XTaskQueueCreateComposite(This,workPort,completionPort,queue)
#define IXThreadingImpl_XTaskQueueGetPort(This,queue,port,portHandle) (This)->lpVtbl->XTaskQueueGetPort(This,queue,port,portHandle)
#define IXThreadingImpl_XTaskQueueDuplicateHandle(This,queueHandle,duplicatedHandle) (This)->lpVtbl->XTaskQueueDuplicateHandle(This,queueHandle,duplicatedHandle)
#define IXThreadingImpl_XTaskQueueDispatch(This,queue,port,timeoutInMs) (This)->lpVtbl->XTaskQueueDispatch(This,queue,port,timeoutInMs)
#define IXThreadingImpl_XTaskQueueCloseHandle(This,queue) (This)->lpVtbl->XTaskQueueCloseHandle(This,queue)
#define IXThreadingImpl_XTaskQueueSubmitCallback(This,queue,port,callbackContext,callback) (This)->lpVtbl->XTaskQueueSubmitCallback(This,queue,port,callbackContext,callback)
#define IXThreadingImpl_XTaskQueueSubmitDelayedCallback(This,queue,port,delayMs,callbackContext,callback) (This)->lpVtbl->XTaskQueueSubmitDelayedCallback(This,queue,port,delayMs,callbackContext,callback)
#define IXThreadingImpl_XTaskQueueRegisterWaiter(This,queue,port,waitHandle,callbackContext,callback,token) (This)->lpVtbl->XTaskQueueRegisterWaiter(This,queue,port,waitHandle,callbackContext,callback,token)
#define IXThreadingImpl_XTaskQueueUnregisterWaiter(This,queue,token) (This)->lpVtbl->XTaskQueueUnregisterWaiter(This,queue,token)
#define IXThreadingImpl_XTaskQueueTerminate(This,queue,wait,callbackContext,callback) (This)->lpVtbl->XTaskQueueTerminate(This,queue,wait,callbackContext,callback)
#define IXThreadingImpl_XTaskQueueRegisterMonitor(This,queue,callbackContext,callback,token) (This)->lpVtbl->XTaskQueueRegisterMonitor(This,queue,callbackContext,callback,token)
#define IXThreadingImpl_XTaskQueueUnregisterMonitor(This,queue,token) (This)->lpVtbl->XTaskQueueUnregisterMonitor(This,queue,token)
#define IXThreadingImpl_XTaskQueueGetCurrentProcessTaskQueue(This,queue) (This)->lpVtbl->XTaskQueueGetCurrentProcessTaskQueue(This,queue)
#define IXThreadingImpl_XTaskQueueSetCurrentProcessTaskQueue(This,queue) (This)->lpVtbl->XTaskQueueSetCurrentProcessTaskQueue(This,queue)
#define IXThreadingImpl_XThreadSetTimeSensitive(This,isTimeSensitiveThread) (This)->lpVtbl->XThreadSetTimeSensitive(This,isTimeSensitiveThread)
#define IXThreadingImpl_XThreadAssertNotTimeSensitive(This) (This)->lpVtbl->XThreadAssertNotTimeSensitive(This)
#define IXThreadingImpl_XThreadIsTimeSensitive(This) (This)->lpVtbl->XThreadIsTimeSensitive(This)

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
    This->lpVtbl->XAsyncComplete(This,asyncBlock,result,requiredBufferSize);
}
static inline HRESULT IXThreadingImpl_XAsyncGetResult(IXThreadingImpl* This,XAsyncBlock* asyncBlock,const PVOID identity,SIZE_T bufferSize,PVOID buffer,SIZE_T* bufferUsed) {
    return This->lpVtbl->XAsyncGetResult(This,asyncBlock,identity,bufferSize,buffer,bufferUsed);
}
static inline HRESULT IXThreadingImpl_XTaskQueueCreate(IXThreadingImpl* This,XTaskQueueDispatchMode workDispatchMode,XTaskQueueDispatchMode completionDispatchMode,XTaskQueueHandle* queue) {
    return This->lpVtbl->XTaskQueueCreate(This,workDispatchMode,completionDispatchMode,queue);
}
static inline HRESULT IXThreadingImpl_XTaskQueueCreateComposite(IXThreadingImpl* This,XTaskQueuePortHandle workPort,XTaskQueuePortHandle completionPort,XTaskQueueHandle* queue) {
    return This->lpVtbl->XTaskQueueCreateComposite(This,workPort,completionPort,queue);
}
static inline HRESULT IXThreadingImpl_XTaskQueueGetPort(IXThreadingImpl* This,XTaskQueueHandle queue,XTaskQueuePort port,XTaskQueuePortHandle* portHandle) {
    return This->lpVtbl->XTaskQueueGetPort(This,queue,port,portHandle);
}
static inline HRESULT IXThreadingImpl_XTaskQueueDuplicateHandle(IXThreadingImpl* This,XTaskQueueHandle queueHandle,XTaskQueueHandle* duplicatedHandle) {
    return This->lpVtbl->XTaskQueueDuplicateHandle(This,queueHandle,duplicatedHandle);
}
static inline BOOLEAN IXThreadingImpl_XTaskQueueDispatch(IXThreadingImpl* This,XTaskQueueHandle queue,XTaskQueuePort port,uint32_t timeoutInMs) {
    return This->lpVtbl->XTaskQueueDispatch(This,queue,port,timeoutInMs);
}
static inline VOID IXThreadingImpl_XTaskQueueCloseHandle(IXThreadingImpl* This,XTaskQueueHandle queue) {
    This->lpVtbl->XTaskQueueCloseHandle(This,queue);
}
static inline HRESULT IXThreadingImpl_XTaskQueueSubmitCallback(IXThreadingImpl* This,XTaskQueueHandle queue,XTaskQueuePort port,PVOID callbackContext,XTaskQueueCallback* callback) {
    return This->lpVtbl->XTaskQueueSubmitCallback(This,queue,port,callbackContext,callback);
}
static inline HRESULT IXThreadingImpl_XTaskQueueSubmitDelayedCallback(IXThreadingImpl* This,XTaskQueueHandle queue,XTaskQueuePort port,uint32_t delayMs,PVOID callbackContext,XTaskQueueCallback* callback) {
    return This->lpVtbl->XTaskQueueSubmitDelayedCallback(This,queue,port,delayMs,callbackContext,callback);
}
static inline HRESULT IXThreadingImpl_XTaskQueueRegisterWaiter(IXThreadingImpl* This,XTaskQueueHandle queue,XTaskQueuePort port,HANDLE waitHandle,PVOID callbackContext,XTaskQueueCallback* callback,XTaskQueueRegistrationToken* token) {
    return This->lpVtbl->XTaskQueueRegisterWaiter(This,queue,port,waitHandle,callbackContext,callback,token);
}
static inline VOID IXThreadingImpl_XTaskQueueUnregisterWaiter(IXThreadingImpl* This,XTaskQueueHandle queue,XTaskQueueRegistrationToken token) {
    This->lpVtbl->XTaskQueueUnregisterWaiter(This,queue,token);
}
static inline HRESULT IXThreadingImpl_XTaskQueueTerminate(IXThreadingImpl* This,XTaskQueueHandle queue,BOOLEAN wait,PVOID callbackContext,XTaskQueueTerminatedCallback* callback) {
    return This->lpVtbl->XTaskQueueTerminate(This,queue,wait,callbackContext,callback);
}
static inline HRESULT IXThreadingImpl_XTaskQueueRegisterMonitor(IXThreadingImpl* This,XTaskQueueHandle queue,PVOID callbackContext,XTaskQueueMonitorCallback* callback,XTaskQueueRegistrationToken* token) {
    return This->lpVtbl->XTaskQueueRegisterMonitor(This,queue,callbackContext,callback,token);
}
static inline VOID IXThreadingImpl_XTaskQueueUnregisterMonitor(IXThreadingImpl* This,XTaskQueueHandle queue,XTaskQueueRegistrationToken token) {
    This->lpVtbl->XTaskQueueUnregisterMonitor(This,queue,token);
}
static inline BOOLEAN IXThreadingImpl_XTaskQueueGetCurrentProcessTaskQueue(IXThreadingImpl* This,XTaskQueueHandle* queue) {
    return This->lpVtbl->XTaskQueueGetCurrentProcessTaskQueue(This,queue);
}
static inline VOID IXThreadingImpl_XTaskQueueSetCurrentProcessTaskQueue(IXThreadingImpl* This,XTaskQueueHandle queue) {
    This->lpVtbl->XTaskQueueSetCurrentProcessTaskQueue(This,queue);
}
static inline HRESULT IXThreadingImpl_XThreadSetTimeSensitive(IXThreadingImpl* This,BOOLEAN isTimeSensitiveThread) {
    return This->lpVtbl->XThreadSetTimeSensitive(This,isTimeSensitiveThread);
}
static inline VOID IXThreadingImpl_XThreadAssertNotTimeSensitive(IXThreadingImpl* This) {
    This->lpVtbl->XThreadAssertNotTimeSensitive(This);
}
static inline BOOLEAN IXThreadingImpl_XThreadIsTimeSensitive(IXThreadingImpl* This) {
    return This->lpVtbl->XThreadIsTimeSensitive(This);
}
#endif
#endif

// 073b7dcb-1fcf-4030-94be-e3c9eb623428

DEFINE_GUID(CLSID_XThreadingImpl, 0x073b7dcb, 0x1fcf, 0x4030, 0x94,0xbe, 0xe3,0xc9,0xeb,0x62,0x34,0x28);
DEFINE_GUID(IID_IXThreadingImpl, 0x073b7dcb, 0x1fcf, 0x4030, 0x94,0xbe, 0xe3,0xc9,0xeb,0x62,0x34,0x28);

#endif