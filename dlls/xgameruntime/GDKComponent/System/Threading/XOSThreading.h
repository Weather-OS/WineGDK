/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> OS Thread Handling
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

#ifndef XOSTHREADING_H
#define XOSTHREADING_H

#include "../../../private.h"

typedef void CALLBACK WaitTimerCallback(void* context);

typedef struct IXWaitTimer IXWaitTimer;

typedef struct IXWaitTimerVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IXWaitTimer* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IXWaitTimer* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IXWaitTimer* This);

    /* IXWaitTimer methods */
    HRESULT (STDMETHODCALLTYPE *Initialize)(
        IXWaitTimer* This,
        PVOID context,
        WaitTimerCallback* callback);

    VOID    (STDMETHODCALLTYPE *Terminate)(
        IXWaitTimer* This);

    VOID    (STDMETHODCALLTYPE *Start)(
        IXWaitTimer* This,
        UINT64 absoluteTime);

    VOID    (STDMETHODCALLTYPE *Cancel)(
        IXWaitTimer* This);

    UINT64  (STDMETHODCALLTYPE *GetAbsoluteTime)(
        IXWaitTimer* This,
        UINT32 msFromNow);
} IXWaitTimerVtbl;

struct IXWaitTimer {
    const IXWaitTimerVtbl* lpVtbl;
};

struct x_wait_timer {
    IXWaitTimer IXWaitTimer_iface;
    PTP_TIMER timer;
    PVOID context;
    WaitTimerCallback *callback;
    LONG ref;
};

HRESULT XCreateWaitTimer( IXWaitTimer **out );

#endif