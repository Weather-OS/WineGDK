/*
 * Xbox Game runtime Library
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

#ifndef __WINE_XGAMERUNTIME_PRIVATE_H
#define __WINE_XGAMERUNTIME_PRIVATE_H

#define COBJMACROS
#include <stdlib.h>
#include <windows.h>
#include <winstring.h>
#include <roapi.h>
#include <activation.h>

#include <xgameerr.h>

#include <unknwn.h>
#include "provider.h"
#include "wine/debug.h"
#include "xthread.h"
#include "xnetwork.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Globalization
#include "windows.globalization.h"
#define WIDL_using_Windows_System_Profile
#include "windows.system.profile.h"

// April 2025 Release of GDK
#define GDKC_VERSION 10001L
#define GAMING_SERVICES_VERSION 3181L

extern IXSystemImpl *x_system_impl;
extern IXSystemAnalyticsImpl *x_system_analytics_impl;
extern IXThreadingImpl *x_threading_impl;
extern IXGameRuntimeFeatureImpl *x_game_runtime_feature_impl;
extern IXNetworkingImpl *x_networking_impl;

typedef struct _INITIALIZE_OPTIONS
{
    int unused;
} INITIALIZE_OPTIONS;

// a85c3901-18ae-48c9-b066-d368f4523420
DEFINE_GUID(IID_IXTaskQueue, 0xa85c3901, 0x18ae, 0x48c9, 0xb0, 0x66, 0xd3, 0x68, 0xf4, 0x52, 0x34, 0x20);

// 7d2c63a1-77fe-46ab-83f0-dd1ffd46b380
DEFINE_GUID(IID_IXTaskQueuePort, 0x7d2c63a1, 0x77fe, 0x46ab, 0x83, 0xf0, 0xdd, 0x1f, 0xfd, 0x46, 0xb3, 0x80);

// 9c4a0e19-10f9-48d7-a547-72f4d5693dcb
DEFINE_GUID(IID_IXTaskQueueMonitorCallback, 0x9c4a0e19, 0x10f9, 0x48d7, 0xa5, 0x47, 0x72, 0xf4, 0xd5, 0x69, 0x3d, 0xcb);

// a44c19b4-7782-46e9-bbfb-f99608c9535a
DEFINE_GUID(IID_IXTaskQueuePortContext, 0xa44c19b4, 0x7782, 0x46e9, 0xbb, 0xfb, 0xf9, 0x96, 0x08, 0xc9, 0x53, 0x5a);

// 137c11ff-1e57-4983-8b7c-b86621430d40
DEFINE_GUID(IID_IXTaskQueueWaitCallback, 0x137c11ff, 0x1e57, 0x4983, 0x8b, 0x7c, 0xb8, 0x86, 0x21, 0x43, 0x0d, 0x40);

// c0858629-c135-4774-a8a8-ab3e6969aeeb
DEFINE_GUID(IID_IAtomicVector, 0xc0858629, 0xc135, 0x4774, 0xa8, 0xa8, 0xab, 0x3e, 0x69, 0x69, 0xae, 0xeb);

// 15b68a9c-386b-4364-9211-5dd598e6a019
DEFINE_GUID(IID_IXWaitTimer, 0x15b68a9c, 0x386b, 0x4364, 0x92, 0x11, 0x5d, 0xd5, 0x98, 0xe6, 0xa0, 0x19);

// 1c7e9426-2b0e-4c37-a57a-df7c165a18af
DEFINE_GUID(IID_IThreadPool, 0x1c7e9426, 0x2b0e, 0x4c37, 0xa5, 0x7a, 0xdf, 0x7c, 0x16, 0x5a, 0x18, 0xaf);

// b617596e-fcf5-42c8-bba4-5a91909f3411
DEFINE_GUID(IID_IAsyncState, 0xb617596e, 0xfcf5, 0x42c8, 0xbb, 0xa4, 0x5a, 0x91, 0x90, 0x9f, 0x34, 0x11);

// 93134919-80eb-472b-861c-e6c4871e2762
DEFINE_GUID(IID_IXAsyncBlockInternalGuard, 0x93134919, 0x80eb, 0x472b, 0x86, 0x1c, 0xe6, 0xc4, 0x87, 0x1e, 0x27, 0x62);

#endif