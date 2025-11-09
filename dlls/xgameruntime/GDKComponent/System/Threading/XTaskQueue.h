/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XTask
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

#ifndef XTASKQUEUE_H
#define XTASKQUEUE_H

#include "../../../private.h"

#include "XOSThreading.h"
#include "AtomicVector.h"
#include "ThreadPool.h"

typedef struct IXTaskQueue IXTaskQueue;
typedef struct IXTaskQueuePort IXTaskQueuePort;
typedef struct IXTaskQueueMonitorCallback IXTaskQueueMonitorCallback;
typedef struct IXTaskQueuePortContext IXTaskQueuePortContext;
typedef struct IXTaskQueueWaitCallback IXTaskQueueWaitCallback;

static const UINT32 TASK_QUEUE_SIGNATURE = 0x41515545;
static const UINT32 TASK_QUEUE_PORT_SIGNATURE = 0x41515553;

typedef enum TerminationLevel
{
    TerminationLevel_None,
    TerminationLevel_Work,
    TerminationLevel_Completion
} TerminationLevel;

typedef struct XMonitor
{
    PVOID context;
    UINT64 token;
    XTaskQueueMonitorCallback *callback;
} XMonitor;

typedef struct XQueue
{
    IXTaskQueuePortContext* portContext;
    PVOID callbackContext;
    XTaskQueueCallback* callback;
    UINT64 enqueueTime;
    UINT64 id;
    struct XQueue* next;
} XQueue;

typedef struct XTerminateForPort
{
    IXTaskQueuePortContext* portContext;
    PVOID callbackContext;
    XTaskQueueTerminatedCallback* callback;
    UINT64 node;
    struct XTerminateForPort* next;
} XTerminateForPort;

typedef struct XTerminate
{
    IXTaskQueue* owner;
    TerminationLevel level;
    PVOID completionPortToken;
    PVOID context;
    XTaskQueueTerminatedCallback* callback;
    struct XTerminate* next;
} XTerminate;

typedef struct XWait
{
    UINT64 Token;
    UINT64 PortToken;
    XTaskQueuePort Port; 
} XWait;

typedef struct XTerminateData
{
    BOOLEAN allowed;
    BOOLEAN terminated;
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;
} XTerminateData;

typedef struct IXTaskQueuePortVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IXTaskQueuePort* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IXTaskQueuePort* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IXTaskQueuePort* This);

    /* IXTaskQueuePort methods */
    HRESULT (STDMETHODCALLTYPE *Initialize)(
        IXTaskQueuePort* This,
        XTaskQueueDispatchMode mode);

    XTaskQueuePortHandle (STDMETHODCALLTYPE *GetHandle)(
        IXTaskQueuePort* This);

    HRESULT (STDMETHODCALLTYPE *QueueItem)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext,
        UINT32 waitMs,
        PVOID callbackContext,
        XTaskQueueCallback* callback);

    HRESULT (STDMETHODCALLTYPE *RegisterWaitHandle)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext,
        HANDLE waitHandle,
        PVOID callbackContext,
        XTaskQueueCallback* callback,
        XTaskQueueRegistrationToken* token);

    VOID   (STDMETHODCALLTYPE *UnregisterWaitHandle)(
        IXTaskQueuePort* This,
        XTaskQueueRegistrationToken token);

    HRESULT (STDMETHODCALLTYPE *PrepareTerminate)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext,
        PVOID callbackContext,
        XTaskQueueTerminatedCallback* callback,
        PVOID *outPrepareToken);

    VOID    (STDMETHODCALLTYPE *CancelTermination)(
        IXTaskQueuePort* This,
        PVOID prepareToken);

    VOID    (STDMETHODCALLTYPE *Terminate)(
        IXTaskQueuePort* This,
        PVOID prepareToken);

    HRESULT (STDMETHODCALLTYPE *Attach)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext);

    VOID    (STDMETHODCALLTYPE *Detach)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext);

    BOOLEAN (STDMETHODCALLTYPE *Dispatch)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext,
        UINT32 timeoutInMs);

    BOOLEAN (STDMETHODCALLTYPE *IsEmpty)(
        IXTaskQueuePort* This);

    VOID    (STDMETHODCALLTYPE *WaitForUnwind)(
        IXTaskQueuePort* This);

    HRESULT (STDMETHODCALLTYPE *SuspendTermination)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext);

    VOID    (STDMETHODCALLTYPE *ResumeTermination)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext);

    VOID    (STDMETHODCALLTYPE *SuspendPort)(
        IXTaskQueuePort* This);

    VOID    (STDMETHODCALLTYPE *ResumePort)(
        IXTaskQueuePort* This);

    HRESULT (*VerifyNotTerminated)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext);

    BOOLEAN (*IsCallCanceled)(
        IXTaskQueuePort* This,
        XQueue *entry);

    BOOLEAN (*AppendEntry)(
        IXTaskQueuePort* This,
        XQueue *entry);

    VOID    (*CancelPendingEntries)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext,
        BOOLEAN appendToQueue);

    BOOLEAN (*DrainOneItem)(
        IXTaskQueuePort* This);

    BOOLEAN (*Wait)(
        IXTaskQueuePort* This,
        IXTaskQueuePortContext* portContext,
        UINT32 timeout);

    VOID    (*EraseQueue)(
        IXTaskQueuePort* This,
        XQueue* queueHead,
        XQueue* queueTail);

    BOOLEAN (*ScheduleNextPendingCallback)(
        IXTaskQueuePort* This,
        UINT64 dueTime,
        XQueue *dueEntry);

    VOID    (*SubmitPendingCallback)(
        IXTaskQueuePort* This);

    VOID    (*SignalTerminations)(
        IXTaskQueuePort* This);

    VOID    (*ScheduleTermination)(
        IXTaskQueuePort* This,
        XTerminateForPort* terminate);

    VOID    (*SignalQueue)(
        IXTaskQueuePort* This);

    VOID    (*NotifyItemQueued)(
        IXTaskQueuePort* This);

    VOID    (*ProcessThreadPoolCallback)(
        IXTaskQueuePort* This,
        ThreadPoolActionStatus *status);
} IXTaskQueuePortVtbl;

typedef struct IXTaskQueuePortContextVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IXTaskQueuePortContext* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IXTaskQueuePortContext* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IXTaskQueuePortContext* This);

    /* IXTaskQueuePortContext methods */
    XTaskQueuePort (STDMETHODCALLTYPE *get_Type)(
        IXTaskQueuePortContext* This);

    XTaskQueuePortStatus (STDMETHODCALLTYPE *get_Status)(
        IXTaskQueuePortContext* This);

    IXTaskQueue* (STDMETHODCALLTYPE *get_PartentQueue)(
        IXTaskQueuePortContext* This);

    IXTaskQueuePort* (STDMETHODCALLTYPE *get_Port)(
        IXTaskQueuePortContext* This);

    VOID    (STDMETHODCALLTYPE *SetStatus)(
        IXTaskQueuePortContext* This,
        XTaskQueuePortStatus status);

    VOID    (STDMETHODCALLTYPE *ItemQueued)(
        IXTaskQueuePortContext* This);

    BOOLEAN (STDMETHODCALLTYPE *AddSuspend)(
        IXTaskQueuePortContext* This);

    BOOLEAN (STDMETHODCALLTYPE *RemoveSuspend)(
        IXTaskQueuePortContext* This);

} IXTaskQueuePortContextVtbl;

typedef struct IXTaskQueueVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IXTaskQueue* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IXTaskQueue* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IXTaskQueue* This);

    /* IXTaskQueue methods */
    HRESULT (STDMETHODCALLTYPE *Initialize)(
        IXTaskQueue* This, 
        XTaskQueuePortHandle workPort, 
        XTaskQueuePortHandle completionPort);

    HRESULT (STDMETHODCALLTYPE *InitializeOverloadPorts)(
        IXTaskQueue* This, 
        XTaskQueueDispatchMode workDispatch, 
        XTaskQueueDispatchMode completionDispatch,
        BOOLEAN allowTermination,
        BOOLEAN allowClose);
    
    HRESULT (STDMETHODCALLTYPE *GetPortContext)(
        IXTaskQueue* This,
        XTaskQueuePort port,
        IXTaskQueuePortContext** portContext);

    HRESULT (STDMETHODCALLTYPE *RegisterWaitHandle)(
        IXTaskQueue* This,
        XTaskQueuePort port,
        HANDLE waitHandle,
        PVOID callbackContext,
        XTaskQueueCallback* callback,
        XTaskQueueRegistrationToken* token);

    VOID    (STDMETHODCALLTYPE *UnregisterWaitHandle)(
        IXTaskQueue* This,
        XTaskQueueRegistrationToken token);

    HRESULT (STDMETHODCALLTYPE *RegisterSubmitCallback)(
        IXTaskQueue* This,
        PVOID context,
        XTaskQueueMonitorCallback* callback,
        XTaskQueueRegistrationToken* token);

    VOID    (STDMETHODCALLTYPE *UnregisterSubmitCallback)(
        IXTaskQueue* This,
        XTaskQueueRegistrationToken token);

    BOOLEAN (STDMETHODCALLTYPE *get_CanTerminate)(
        IXTaskQueue* This);

    BOOLEAN (STDMETHODCALLTYPE *get_CanClose)(
        IXTaskQueue* This);

    HRESULT (STDMETHODCALLTYPE *Terminate)(
        IXTaskQueue* This,
        BOOLEAN wait,
        PVOID callbackContext,
        XTaskQueueTerminatedCallback* callback);

    VOID    (*RundownObject)(
        IXTaskQueue* This);

    VOID    (*OnTerminationCallback)(
        PVOID context);
} IXTaskQueueVtbl;

typedef struct IXTaskQueueMonitorCallbackVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IXTaskQueueMonitorCallback* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IXTaskQueueMonitorCallback* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IXTaskQueueMonitorCallback* This);
        
    /* IXTaskQueueMonitorCallback methods */
    HRESULT (STDMETHODCALLTYPE *Register)(
        IXTaskQueueMonitorCallback* This,
        PVOID context,
        XTaskQueueMonitorCallback* callback,
        XTaskQueueRegistrationToken *token
    );

    VOID    (STDMETHODCALLTYPE *Unregister)(
        IXTaskQueueMonitorCallback* This,
        XTaskQueueRegistrationToken token
    );

    VOID    (STDMETHODCALLTYPE *Invoke)(
        IXTaskQueueMonitorCallback* This,
        XTaskQueuePort port
    );
    
} IXTaskQueueMonitorCallbackVtbl;

typedef struct IXTaskQueueWaitCallbackVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IXTaskQueueWaitCallback* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IXTaskQueueWaitCallback* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IXTaskQueueWaitCallback* This);

    /* IXTaskQueueWaitCallback methods */
    HRESULT (STDMETHODCALLTYPE *Register)(
        IXTaskQueueWaitCallback* This,
        XTaskQueuePort port,
        XTaskQueueRegistrationToken portToken,
        XTaskQueueRegistrationToken *token);
    
    HRESULT (STDMETHODCALLTYPE *Unregister)(
        IXTaskQueueWaitCallback* This,
        XTaskQueueRegistrationToken token,
        XTaskQueuePort *outPort,
        XTaskQueueRegistrationToken *outPortToken
    );
} IXTaskQueueWaitCallbackVtbl;

struct IXTaskQueue {
    const IXTaskQueueVtbl* lpVtbl;
};

struct IXTaskQueuePortContext {
    const IXTaskQueuePortContextVtbl* lpVtbl;
};

struct IXTaskQueueMonitorCallback {
    const IXTaskQueueMonitorCallbackVtbl* lpVtbl;
};

struct IXTaskQueuePort {
    const IXTaskQueuePortVtbl* lpVtbl;
};

struct IXTaskQueueWaitCallback {
    const IXTaskQueueWaitCallbackVtbl* lpVtbl;
};

// Backwards decleration of XTaskQueueObject
typedef struct XTaskQueueObject
{
    UINT32 signature;
    IXTaskQueue *headQueue;
} XTaskQueueObject;

// Backwards decleration of XTaskQueuePortObject
typedef struct XTaskQueuePortObject
{
    UINT32 signature;
    IXTaskQueuePort *headPort;
    IXTaskQueue *headQueue;
} XTaskQueuePortObject;

struct x_task_queue_monitor_callback
{
    IXTaskQueueMonitorCallback IXTaskQueueMonitorCallback_iface;
    INT64 nextToken;
    CRITICAL_SECTION cs;
    XMonitor monitors_buffer1[32];
    XMonitor monitors_buffer2[32];
    XMonitor *monitors_buffers[2];
    XTaskQueueHandle queue;
    LONG ref;
};

struct x_task_queue_port
{
    IXTaskQueuePort IXTaskQueuePort_iface;
    XTaskQueuePortObject portHeader;
    XTaskQueueDispatchMode dispatchMode;
    IAtomicVector *attachedContexts;
    LONG processingCallback;
    CONDITION_VARIABLE cv;
    CONDITION_VARIABLE cvAny;
    CRITICAL_SECTION cs;
    XQueue *queueList_tail, *queueList_head;
    XQueue *pendingQueueList_tail, *pendingQueueList_head;
    XTerminateForPort *terminateList_tail, *terminateList_head;
    XTerminateForPort *pendingTerminateList_tail, *pendingTerminateList_head;
    IXWaitTimer *timer;
    IThreadPool *threadPool;
    LONG64 timerDue;
    LONG nextId;
    BOOLEAN signaled;
    BOOL suspended;
    LONG ref;
};

struct x_task_queue_port_context
{
    IXTaskQueuePortContext IXTaskQueuePortContext_iface;
    IXTaskQueuePort *port;
    IXTaskQueue *source;

    XTaskQueuePort type;
    XTaskQueuePortStatus status;
    IXTaskQueue *queue;
    IXTaskQueueMonitorCallback *callbackSubmitted;
    LONG suspendCount;
    LONG ref;
};

struct x_task_queue
{
    IXTaskQueue IXTaskQueue_iface;

    XTaskQueueObject queueHeader;
    IXTaskQueueMonitorCallback *callbackSubmitted;
    IXTaskQueueWaitCallback *waitRegistry;
    XTerminateData terminationData;
    IXTaskQueuePortContext *workPort;
    IXTaskQueuePortContext *completionPort;
    BOOLEAN allowClose;
    LONG ref;
};

struct x_task_queue_wait_callback
{
    IXTaskQueueWaitCallback IXTaskQueueWaitCallback_iface;
    UINT64 nextToken;
    XWait callbacks[120];
    CRITICAL_SECTION cs;
    LONG ref;
};

HRESULT XTaskQueueCreate( XTaskQueueDispatchMode workDispatchMode, XTaskQueueDispatchMode completionDispatchMode, XTaskQueueHandle* queue );

#endif