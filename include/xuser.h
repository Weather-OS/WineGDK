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

#ifndef __WINE_XUSER_H
#define __WINE_XUSER_H

#include <windef.h>
#include <winnt.h>

typedef enum XUserAddOptions
{
    AddOptionsNone                      = 0,
    AddOptionsAddDefaultUserSilently    = 1,
    AddOptionsAllowGuests               = 2,
    AddDefaultUserAllowingUI            = 4
} XUserAddOptions;

typedef enum XUserAgeGroup
{
    AgeGroupUnknown = 0,
    AgeGroupChild   = 1,
    AgeGroupTeen    = 2,
    AgeGroupAdult   = 3
} XUserAgeGroup;

typedef enum XUserChangeEvent
{
    ChangeEventSignedInAgain    = 0,
    ChangeEventSigningOut       = 1,
    ChangeEventSignedOut        = 2,
    ChangeEventGamertag         = 3,
    ChangeEventGamerPicture     = 4,
    ChangeEventPrivileges       = 5
} XUserChangeEvent;

typedef enum XUserDefaultAudioEndpointKind
{
    DefaultAudioEndpointKindCommunicationRender     = 0,
    DefaultAudioEndpointKindCommunicationCapture    = 1
} XUserDefaultAudioEndpointKind;

typedef enum XUserGamerPictureSize
{
    GamerPictureSizeSmall       = 0,
    GamerPictureSizeMedium      = 1,
    GamerPictureSizeLarge       = 2,
    GamerPictureSizeExtraLarge  = 3
} XUserGamerPictureSize;

typedef enum XUserGamertagComponent
{
    GamertagComponentClassic        = 0,
    GamertagComponentModern         = 1,
    GamertagComponentModernSuffix   = 2,
    GamertagComponentUniqueModern   = 3,
} XUserGamertagComponent;

typedef enum XUserGetMsaTokenSilentlyOptions
{
    GetMsaTokenSilentlyOptionsNone  = 0
} XUserGetMsaTokenSilentlyOptions;

typedef enum XUserGetTokenAndSignatureOptions
{
    GetTokenAndSignatureOptionsNone         = 0,
    GetTokenAndSignatureOptionsForceRefresh = 1,
    GetTokenAndSignatureOptionsAllUsers     = 2
} XUserGetTokenAndSignatureOptions;

typedef enum XUserPrivilege
{
    PrivilegeCrossPlay              = 185,
    PrivilegeClubs                  = 188,
    PrivilegeSessions               = 189,
    PrivilegeBroadcast              = 190,
    PrivilegeManageProfilePrivacy   = 196,
    PrivilegeGameDvr                = 198,
    PrivilegeMultiplayerParties     = 203,
    PrivilegeCloudManageSession     = 207,
    PrivilegeCloudJoinSession       = 208,
    PrivilegeCloudSavedGames        = 209,
    PrivilegeSocialNetworkSharing   = 220,
    PrivilegeUserGeneratedContent   = 247,
    PrivilegeCommunications         = 252,
    PrivilegeMultiplayer            = 254,
    PrivilegeAddFriends             = 255
} XUserPrivilege;

typedef enum XUserPrivilegeDenyReason
{
    PrivilegeDenyReasonNone             = 0,
    PrivilegeDenyReasonPurchaseRequired = 1,
    PrivilegeRestricted                 = 2,
    PrivilegeBanned                     = 3,
    PrivilegeUnknown                    = -1
} XUserPrivilegeDenyReason;

typedef enum XUserPrivilegeOptions
{
    PrivilegeOptionsNone        = 0,
    PrivilegeOptionsAllUsers    = 1
} XUserPrivilegeOptions;

typedef enum XUserState
{
    StateSignedIn   = 0,
    StateSigningOut = 1,
    StateSignedOut  = 2
} XUserState;

typedef struct XUserDeviceAssociationChange
{
    APP_LOCAL_DEVICE_ID deviceId;
    XUserLocalId oldUser;
    XUserLocalId newUser;
} XUserDeviceAssociationChange;

typedef struct XUserGetTokenAndSignatureData
{
    SIZE_T tokenSize;
    SIZE_T signatureSize;
    LPCSTR token;
    LPCSTR signature;
} XUserGetTokenAndSignatureData;

typedef struct XUserGetTokenAndSignatureUtf16Data
{
    SIZE_T tokenSize;
    SIZE_T signatureCount;
    LPCWSTR token;
    LPCWSTR signature;
} XUserGetTokenAndSignatureUtf16Data;

typedef struct XUserGetTokenAndSignatureHttpHeader
{
    LPCSTR name;
    LPCSTR value;
} XUserGetTokenAndSignatureHttpHeader;

typedef struct XUserGetTokenAndSignatureUtf16HttpHeader
{
    LPCWSTR name;
    LPCWSTR value;
} XUserGetTokenAndSignatureUtf16HttpHeader;

typedef struct XUserLocalId
{
    UINT64 value;
} XUserLocalId;

typedef void CALLBACK XUserChangeEventCallback(_In_opt_ PVOID context, _In_ XUserLocalId userLocalId, _In_ XUserChangeEvent event);
typedef void CALLBACK XUserDefaultAudioEndpointUtf16ChangedCallback(_In_opt_ PVOID context, _In_ XUserLocalId user, _In_ XUserDefaultAudioEndpointKind defaultAudioEndpointKind, _In_opt_z_ LPCWSTR endpointIdUtf16);
typedef void CALLBACK XUserDeviceAssociationChangedCallback(_In_opt_ PVOID context, _In_ const XUserDeviceAssociationChange *change);

#endif /* __WINE_XUSER_H */