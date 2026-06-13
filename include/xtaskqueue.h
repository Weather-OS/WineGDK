/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_XTASKQUEUE_H
#define __WINE_XTASKQUEUE_H

#ifdef __cplusplus
extern "C" {

enum class XTaskQueueDispatchMode : UINT32
{
    Manual,
    ThreadPool,
    SerializedThreadPool,
    Immediate,
};

enum class XTaskQueuePort : UINT32
{
    Work,
    Completion,
};

#elif defined(__WINESRC__)

typedef enum XTaskQueueDispatchMode
{
    XTaskQueueDispatchMode_Manual,
    XTaskQueueDispatchMode_ThreadPool,
    XTaskQueueDispatchMode_SerializedThreadPool,
    XTaskQueueDispatchMode_Immediate,
} XTaskQueueDispatchMode;

typedef enum XTaskQueuePort
{
    XTaskQueuePort_Work,
    XTaskQueuePort_Completion,
} XTaskQueuePort;

#endif

typedef struct XTaskQueueObject *XTaskQueueHandle;
typedef struct XTaskQueuePortObject *XTaskQueuePortHandle;

typedef struct XTaskQueueRegistrationToken XTaskQueueRegistrationToken;

typedef void __stdcall XTaskQueueCallback( void *context, BOOLEAN canceled );
typedef void __stdcall XTaskQueueMonitorCallback( void *context, XTaskQueueHandle queue, XTaskQueuePort port );
typedef void __stdcall XTaskQueueTerminatedCallback( void *context );

struct XTaskQueueRegistrationToken
{
    UINT64 token;
};

void XTaskQueueCloseHandle( XTaskQueueHandle queue );
HRESULT XTaskQueueCreate( XTaskQueueDispatchMode workDispatchMode, XTaskQueueDispatchMode completionDispatchMode, XTaskQueueHandle *queue );
HRESULT XTaskQueueCreateComposite( XTaskQueuePortHandle workPort, XTaskQueuePortHandle completionPort, XTaskQueueHandle *queue );
HRESULT XTaskQueueGetPort( XTaskQueueHandle queue, XTaskQueuePort port, XTaskQueuePortHandle *portHandle );
HRESULT XTaskQueueDuplicateHandle( XTaskQueueHandle queueHandle, XTaskQueueHandle *duplicatedHandle );
BOOLEAN XTaskQueueDispatch( XTaskQueueHandle queue, XTaskQueuePort port, UINT32 timeoutInMs );
void XTaskQueueCloseHandle( XTaskQueueHandle queue );
HRESULT XTaskQueueTerminate( XTaskQueueHandle queue, BOOLEAN wait, void *callbackContext, XTaskQueueTerminatedCallback *callback );
HRESULT XTaskQueueSubmitCallback( XTaskQueueHandle queue, XTaskQueuePort port, void *callbackContext, XTaskQueueCallback *callback );
HRESULT XTaskQueueSubmitDelayedCallback( XTaskQueueHandle queue, XTaskQueuePort port, UINT32 delayMs, void *callbackContext, XTaskQueueCallback *callback );
HRESULT XTaskQueueRegisterWaiter( XTaskQueueHandle queue, XTaskQueuePort port, HANDLE waitHandle, void *callbackContext, XTaskQueueCallback *callback, XTaskQueueRegistrationToken *token );
void XTaskQueueUnregisterWaiter( XTaskQueueHandle queue, XTaskQueueRegistrationToken token );
HRESULT XTaskQueueRegisterMonitor( XTaskQueueHandle queue, void *callbackContext, XTaskQueueMonitorCallback *callback, XTaskQueueRegistrationToken *token );
void XTaskQueueUnregisterMonitor( XTaskQueueHandle queue, XTaskQueueRegistrationToken token );
BOOLEAN XTaskQueueGetCurrentProcessTaskQueue( XTaskQueueHandle *queue );
void XTaskQueueSetCurrentProcessTaskQueue( XTaskQueueHandle queue );
HRESULT XThreadSetTimeSensitive( BOOLEAN isTimeSensitiveThread );
void XThreadAssertNotTimeSensitive();
BOOLEAN XThreadIsTimeSensitive();

#ifdef __cplusplus
}
#endif

#endif