/*
 * Thread Pool Implementation
 *  From https://github.com/microsoft/libHttpClient
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

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "../../../private.h"

typedef struct IThreadPool IThreadPool;
typedef struct IActionStatus IActionStatus;
typedef struct IActionStatus IActionStatus;
typedef struct ThreadPoolActionStatus ThreadPoolActionStatus;

typedef void CALLBACK (*ThreadPoolCallback)(void* context, ThreadPoolActionStatus *status);

typedef struct IActionStatusVtbl {
    VOID (STDMETHODCALLTYPE *Complete)(IActionStatus *This);
    VOID (STDMETHODCALLTYPE *MayRunLong)(IActionStatus *This);
} IActionStatusVtbl;

typedef struct IThreadPoolVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IThreadPool* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IThreadPool* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IThreadPool* This);

    /* IThreadPool Methods */
    HRESULT (STDMETHODCALLTYPE *Initialize)(
        IThreadPool* This,
        PVOID context,
        ThreadPoolCallback callback);

    VOID    (STDMETHODCALLTYPE *Terminate)(
        IThreadPool* This);

    VOID    (STDMETHODCALLTYPE *Submit)(
        IThreadPool* This);
} IThreadPoolVtbl;

struct IThreadPool {
    const IThreadPoolVtbl* lpVtbl;
};

struct IActionStatus {
    const IActionStatusVtbl* lpVtbl;
};

struct ThreadPoolActionStatus {
    IActionStatus IActionStatus_iface;
    
    IThreadPool* owner;
    PTP_CALLBACK_INSTANCE instance;
    BOOLEAN longRunning;
    BOOLEAN IsComplete;
};

struct thread_pool {
    IThreadPool IThreadPool_iface;

    LONG ref;
    PTP_WORK work;
    PVOID context;
    ThreadPoolCallback callback;
};

HRESULT CreateIThreadPool( IThreadPool **out );

#endif