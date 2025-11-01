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


#include <stdlib.h>
#include <windows.h>
#include <winternl.h>

#include <xgameerr.h>

#define COBJMACROS
#include <unknwn.h>
#include "provider.h"
#include "wine/debug.h"

// April 2025 Release of GDK
#define GDKC_VERSION 10001L
#define GAMING_SERVICES_VERSION 3181L

extern IXSystemImpl *x_system_impl;
extern IXSystemAnalyticsImpl *x_system_analytics_impl;
extern IXGameRuntimeFeatureImpl *x_game_runtime_feature_impl;

typedef struct _INITIALIZE_OPTIONS
{
    int unused;
} INITIALIZE_OPTIONS;

#endif