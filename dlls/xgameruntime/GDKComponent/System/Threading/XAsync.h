/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XAsync
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

#ifndef XASYNC_H
#define XASYNC_H

#include "../../../private.h"

#define ASYNC_BLOCK_SIG         0x41535942 // ASYB
#define ASYNC_BLOCK_RESULT_SIG  0x41535242 // ASRB
#define ASYNC_STATE_SIG         0x41535445 // ASTE

typedef struct IXAsyncBlockInternalGuard IXAsyncBlockInternalGuard;
typedef struct IAsyncState IAsyncState;

typedef enum ProviderCleanupLocation
{
    CleanupLocation_Destructor,
    CleanupLocation_AfterDoWork,
    CleanupLocation_InCancel,
    CleanupLocation_CleanedUp
} ProviderCleanupLocation;

typedef struct AsyncBlockInternal
{
    IAsyncState* state;
    HRESULT status;
    DWORD signature;
    CRITICAL_SECTION lock;
} AsyncBlockInternal;

typedef struct IXAsyncBlockInternalGuardVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IXAsyncBlockInternalGuard* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IXAsyncBlockInternalGuard* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IXAsyncBlockInternalGuard* This);

    IAsyncState* (STDMETHODCALLTYPE *GetState)(
        IXAsyncBlockInternalGuard* This);

    IAsyncState* (STDMETHODCALLTYPE *ExtractState)(
        IXAsyncBlockInternalGuard* This,
        BOOLEAN resultsRetrieved);

    HRESULT (STDMETHODCALLTYPE *GetStatus)(
        IXAsyncBlockInternalGuard* This);

    BOOLEAN (STDMETHODCALLTYPE *GetResultsRetrieved)(
        IXAsyncBlockInternalGuard* This);

    BOOLEAN (STDMETHODCALLTYPE *TrySetTerminalStatus)(
        IXAsyncBlockInternalGuard* This,
        HRESULT status);

    AsyncBlockInternal* (*DoLock)(
        XAsyncBlock* asyncBlock);
} IXAsyncBlockInternalGuardVtbl;

typedef struct IAsyncStateVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IAsyncState* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IAsyncState* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IAsyncState* This);
} IAsyncStateVtbl;

struct IAsyncState {
    const IAsyncStateVtbl* lpVtbl;
};

struct IXAsyncBlockInternalGuard {
    const IXAsyncBlockInternalGuardVtbl* lpVtbl;
};

struct async_state
{
    IAsyncState IAsyncState_iface;
    UINT32 signature;
    LONG ref;
    LONG /* ProviderCleanupLocation */ providerCleanup;
    LONG workScheduled;
    BOOLEAN valid;
    XAsyncProviderCallback *providerCallback;
    XAsyncProviderData providerData;
    XAsyncBlock providerAsyncBlock;
    XAsyncBlock* userAsyncBlock;
    XTaskQueueHandle queue;
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;
    BOOLEAN waitSatisfied;
    LPCSTR identity;
    LPCSTR identityName;
};

struct x_async_block_guard
{
    IXAsyncBlockInternalGuard IXAsyncBlockInternalGuard_iface;
    AsyncBlockInternal *internal;
    AsyncBlockInternal *userInternal;
    BOOLEAN locked;
    LONG ref;
};

HRESULT XAsyncGetStatus( XAsyncBlock* asyncBlock, BOOLEAN wait );
HRESULT XAsyncGetResultSize( XAsyncBlock* asyncBlock, SIZE_T* bufferSize );
VOID XAsyncCancel( XAsyncBlock* asyncBlock );
HRESULT XAsyncRun( XAsyncBlock* asyncBlock, XAsyncWork* work );
HRESULT XAsyncBegin( XAsyncBlock* asyncBlock, PVOID context, PVOID identity, LPCSTR identityName, XAsyncProviderCallback* provider );
HRESULT XAsyncSchedule( XAsyncBlock* asyncBlock, UINT32 delayInMs );
VOID XAsyncComplete( XAsyncBlock* asyncBlock, HRESULT result, SIZE_T requiredBufferSize );

#endif