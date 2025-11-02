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

#ifndef __WINE_XASYNCPROVIDER_H
#define __WINE_XASYNCPROVIDER_H

#include <stdint.h>
#include "xasync.h"

typedef enum XAsyncOp 
{
    Begin,
    DoWork,
    GetResult,
    Cancel,
    Cleanup
} XAsyncOp;

typedef struct XAsyncProviderData
{
    XAsyncBlock* async;
    size_t bufferSize;
    void* buffer;
    void* context;
} XAsyncProviderData;

typedef HRESULT CALLBACK XAsyncProviderCallback(_In_ XAsyncOp op, _Inout_ const XAsyncProviderData* data);

#define XASYNC_IDENTITY(method) #method

#endif