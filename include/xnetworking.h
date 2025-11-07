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

#ifndef __WINE_XNETWORKING_H
#define __WINE_XNETWORKING_H

#include "xasync.h"

typedef enum XNetworkingThumbprintType
{  
    Leaf = 0,  
    Issuer = 1,  
    Root = 2,  
} XNetworkingThumbprintType;

typedef enum XNetworkingConnectivityLevelHint
{
    ConnectivityLevelHintUnknown = 0,
    ConnectivityLevelHintNone = 1,
    ConnectivityLevelHintLocalAccess = 2,
    ConnectivityLevelHintInternetAccess = 3,
    ConnectivityLevelHintConstrainedInternetAccess = 4,
} XNetworkingConnectivityLevelHint;

typedef enum XNetworkingConnectivityCostHint
{
    ConnectivityCostHintUnknown = 0,
    ConnectivityCostHintUnrestricted = 1,
    ConnectivityCostHintFixed = 2,
    ConnectivityCostHintVariable = 3,
} XNetworkingConnectivityCostHint;

typedef struct XNetworkingThumbprint 
{  
    XNetworkingThumbprintType thumbprintType;  
    SIZE_T thumbprintBufferByteCount;  
    UINT8* thumbprintBuffer;  
} XNetworkingThumbprint;

typedef struct XNetworkingSecurityInformation 
{
    UINT32 enabledHttpSecurityProtocolFlags;
    SIZE_T thumbprintCount;
    XNetworkingThumbprint* thumbprints;
} XNetworkingSecurityInformation;

typedef struct XNetworkingConnectivityHint 
{
    XNetworkingConnectivityLevelHint connectivityLevel;
    XNetworkingConnectivityCostHint connectivityCost;
    UINT32 ianaInterfaceType;
    BOOLEAN networkInitialized;
    BOOLEAN approachingDataLimit;
    BOOLEAN overDataLimit;
    BOOLEAN roaming;
} XNetworkingConnectivityHint;

typedef enum XNetworkingConfigurationSetting
{  
    MaxTitleTcpQueuedReceiveBufferSize = 0,  
    MaxSystemTcpQueuedReceiveBufferSize = 1,  
    MaxToolsTcpQueuedReceiveBufferSize = 2,  
} XNetworkingConfigurationSetting;

typedef enum XNetworkingStatisticsType  
{  
    TitleTcpQueuedReceivedBufferUsage = 0,  
    SystemTcpQueuedReceivedBufferUsage = 1,  
    ToolsTcpQueuedReceivedBufferUsage = 2,  
} XNetworkingStatisticsType;

typedef struct XNetworkingTcpQueuedReceivedBufferUsageStatistics 
{  
    UINT64 numBytesCurrentlyQueued;  
    UINT64 peakNumBytesEverQueued;  
    UINT64 totalNumBytesQueued;  
    UINT64 numBytesDroppedForExceedingConfiguredMax;  
    UINT64 numBytesDroppedDueToAnyFailure;  
} XNetworkingTcpQueuedReceivedBufferUsageStatistics;

typedef union XNetworkingStatisticsBuffer 
{  
    XNetworkingTcpQueuedReceivedBufferUsageStatistics tcpQueuedReceiveBufferUsage;  
} XNetworkingStatisticsBuffer;

typedef void CALLBACK XNetworkingPreferredLocalUdpMultiplayerPortChangedCallback(_In_opt_ PVOID context, _In_ UINT16 preferredLocalUdpMultiplayerPort);
typedef void CALLBACK XNetworkingConnectivityHintChangedCallback(_In_opt_ PVOID context, _In_ const XNetworkingConnectivityHint* connectivityHint);
#endif