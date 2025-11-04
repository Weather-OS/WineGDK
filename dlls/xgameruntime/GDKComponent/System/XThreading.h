/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XThread, XAsync and XTask
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

#ifndef XTHREADING_H
#define XTHREADING_H

#define X_ASYNC_WORK_MAGIC 0x58A55A01u

#include "../../private.h"

#include <string.h>

struct x_threading
{
    IXThreadingImpl IXThreadingImpl_iface;
    XTaskQueueHandle currentProcessTaskQueue;
    BOOLEAN isTimeSensitiveThread;
    LONG ref;
};

struct x_async_provider
{
    XAsyncProviderCallback *callback;
    XAsyncProviderData *data;
    OPTIONAL PVOID identity;
    OPTIONAL LPCSTR identityName;
    XAsyncOp operation;
    UINT32 workDelay;
};

/**
 * The threadBlock in here is used as the root thread block, kind of like an iface.
 * The threadBlock in provider->data is used as the actual thread work.
 * IMPORTANT!: Only 1 continuous threadpool can run.
 */
struct x_async_work
{
    UINT32 magic;
    
    IWineAsyncWorkImpl IWineAsyncWorkImpl_iface;
    XAsyncBlock *threadBlock;
    HRESULT status;
    LONG ref;

    struct x_async_provider provider;
    HANDLE event;
    TP_WORK *async_run_work;
    CRITICAL_SECTION cs;
};

typedef struct XTask
{
    XTaskQueueCallback *callback;
    HANDLE taskHandle;
    UINT32 delayInMs;
    PVOID context;
    struct XTask *next;
} XTask;

struct XTaskQueuePortObject //<-- Our own XTaskQueuePortObject implementation
{    
    XTask *tasks_head, *tasks_tail;
    UINT32 tasksCount;
    XTaskQueueDispatchMode dispatchMode;

    BOOLEAN isRunning;
};

struct XTaskQueueObject //<-- Our own XTaskQueueObject implementation
{
    XTaskQueuePortHandle workPortHandle;
    XTaskQueuePortHandle completionPortHandle;

    HANDLE dispatchHandle;
    BOOLEAN isRunning;
    XTaskQueueTerminatedCallback *terminateCallback;
    CRITICAL_SECTION cs;

};

struct tp_work_arguments
{
    XTaskQueueHandle handle;
    XTaskQueuePort port;
    BOOLEAN cancelled;
    UINT32 current;
};

struct x_async_work *impl_from_XAsyncBlock( XAsyncBlock *block );

HRESULT WINAPI XInitializeBlock( XAsyncBlock* asyncBlock );
DWORD CALLBACK XTPTaskCallback( void *context );
VOID CALLBACK XTPAsyncCallback( TP_CALLBACK_INSTANCE *instance, void *iface, TP_WORK *work );
VOID CALLBACK XTPDispatchCallback( TP_CALLBACK_INSTANCE *instance, void *iface, TP_WORK *work );

#endif