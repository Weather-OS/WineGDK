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

#include "xasync.h"

typedef struct IXThreadingImpl IXThreadingImpl;

typedef struct IXThreadingImplVtbl {
    HRESULT (*QueryInterface)(IXThreadingImpl* This, REFIID riid, void** ppvObject);
    ULONG (*AddRef)(IXThreadingImpl* This);
    ULONG (*Release)(IXThreadingImpl* This);
    
    HRESULT (*XAsyncGetStatus)(IXThreadingImpl* This, XAsyncBlock* asyncBlock, boolean wait);
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
#define IXThreadingImpl_XAsyncGetStatus(This,feature) (This)->lpVtbl->XAsyncGetStatus(This,asyncBlock,wait)
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
static inline BOOLEAN IXThreadingImpl_XAsyncGetStatus(IXThreadingImpl* This,XAsyncBlock* asyncBlock,boolean wait) {
    return This->lpVtbl->XAsyncGetStatus(This,asyncBlock,wait);
}
#endif
#endif

// 073b7dcb-1fcf-4030-94be-e3c9eb623428

DEFINE_GUID(CLSID_XThreadingImpl, 0x073b7dcb, 0x1fcf, 0x4030, 0x94,0xbe, 0xe3,0xc9,0xeb,0x64,0x34,0x28);
DEFINE_GUID(IID_IXThreadingImpl, 0x073b7dcb, 0x1fcf, 0x4030, 0x94,0xbe, 0xe3,0xc9,0xeb,0x64,0x34,0x28);

#endif