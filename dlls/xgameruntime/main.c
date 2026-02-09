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

#include "initguid.h"
#include "private.h"

#include "GDKComponent/InitInternalGDKC.h"

WINE_DEFAULT_DEBUG_CHANNEL(xgameruntime);

static HMODULE xgameruntime;
static HMODULE xgameruntime_threading;

static VOID LoadOtherRuntime( DWORD *asked )
{
    HKEY hKey;
    LPCSTR subKey = "Software\\Wine\\WineGDK";
    LPCSTR valueName = "LoadOtherRuntimeAsked";
    DWORD value;
    DWORD dataSize = sizeof(DWORD);
    LONG result;

    *asked = 0;

    result = RegCreateKeyExA(
        HKEY_LOCAL_MACHINE,
        subKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        NULL,
        &hKey,
        NULL
    );

    if (result != ERROR_SUCCESS) {
        return;
    }

    // Try to read the value
    result = RegQueryValueExA(
        hKey,
        valueName,
        NULL,
        NULL,
        (LPBYTE)&value,
        &dataSize
    );

    if ( result == ERROR_FILE_NOT_FOUND ) 
    {
        value = 1;

        result = RegSetValueExA(
            hKey,
            valueName,
            0,
            REG_DWORD,
            (const BYTE*)&value,
            sizeof(DWORD)
        );
    } else if ( result == ERROR_SUCCESS ) 
    {
        *asked = value;

        value = 1;

        result = RegSetValueExA(
            hKey,
            valueName,
            0,
            REG_DWORD,
            (const BYTE*)&value,
            sizeof(DWORD)
        );
    }

    RegCloseKey( hKey );
    return;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return xgameruntime != NULL ? S_FALSE : S_OK;
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    TRACE("inst %p, reason %lu, reserved %p.\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinst);
            xgameruntime_threading = LoadLibraryA("xgameruntime.dll.threading");
            break;
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            if (xgameruntime) FreeLibrary(xgameruntime);
            if (xgameruntime_threading) FreeLibrary(xgameruntime_threading);
        break;
    }
    return TRUE;
}

typedef HRESULT (WINAPI *InitializeApiImplEx2_ext)( ULONG gdkVer, ULONG gsVer, CHAR mode, INITIALIZE_OPTIONS *options );

HRESULT WINAPI InitializeApiImplEx2( ULONG gdkVer, ULONG gsVer, CHAR mode, INITIALIZE_OPTIONS *options )
{
    //  Initialization can be done however we want on our side.
    // You can choose to return `S_OK` once the full SDK is implemented.
    //
    //  There's no documented information about what `INITIALIZE_OPTIONS` is,
    // and xgameruntime.lib never utilizes this argument anyway.
    TRACE("gdkVer %ld, gsVer %ld, mode %d, options %p stub!\n", gdkVer, gsVer, mode, options);
    return GDKC_InitAPI( gdkVer, gsVer, mode, options );
}

HRESULT WINAPI InitializeApiImplEx( ULONG gdkVer, ULONG gsVer, CHAR mode )
{
    TRACE("gdkVer %ld, gsVer %ld, mode %d\n", gdkVer, gsVer, mode);
    return InitializeApiImplEx2( gdkVer, gsVer, mode, NULL );
}

HRESULT WINAPI InitializeApiImpl( ULONG gdkVer, ULONG gsVer )
{
    TRACE("gdkVer %ld, gsVer %ld\n", gdkVer, gsVer);
    return InitializeApiImplEx2( gdkVer, gsVer, 0, NULL );
}

typedef HRESULT (WINAPI *QueryApiImpl_ext)( GUID *runtimeClassId, REFIID interfaceId, void **out );

HRESULT WINAPI QueryApiImpl( const GUID *runtimeClassId, REFIID interfaceId, void **out )
{
    // Interfaces returned are COM interfaces and inherit IUnknown*
    // 
    //  On MSDN, There's no official documentation on the order of these interfaces and functions.
    // However, we can hook a dummy `xgameruntime.dll` into test environments and individually query
    // each class and what signatures they posses. Once we've pass through an empty IUnknown* interface,
    // we can reconstruct the vtable of each class based on what function gets called.
    //
    //  Example: (e349bd1a-fc20-4e40-b99c-4178cc6b409f) corresponds to part of the `ISystem` class and implements
    // these functions in order:
    //
    //  /*** IUnknown methods ***/
    //  IXSystemImpl_QueryInterface,                    (offset 0)
    //  IXSystemImpl_AddRef,                            (offset 8)
    //  IXSystemImpl_Release,                           (offset 16)
    //  /*** IXSystemImpl methods ***/
    //  IXSystemImpl_XSystemGetConsoleId                (offset 24)
    //  IXSystemImpl_XSystemGetXboxLiveSandboxId        (offset 32)
    //  IXSystemImpl_XSystemGetAppSpecificDeviceId      (offset 40)
    //  IXSystemImpl_XSystemHandleTrack                 (offset 48)
    //  IXSystemImpl_XSystemIsHandleValid               (offset 56)
    //  IXSystemImpl_XSystemAllowFullDownloadBandwidth  (offset 64)
    //

    QueryApiImpl_ext func = (QueryApiImpl_ext)GetProcAddress( xgameruntime_threading, "QueryApiImpl" );
    DWORD asked;

    TRACE("runtimeClassId %s, interfaceId %s, out %p\n", debugstr_guid(runtimeClassId), debugstr_guid(interfaceId), out);

    if ( IsEqualGUID( runtimeClassId, &CLSID_XSystemImpl ) )
    {
        return IXSystemImpl_QueryInterface( x_system_impl, interfaceId, out );
    }
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XGameRuntimeFeatureImpl ) )
    {
        return IXGameRuntimeFeatureImpl_QueryInterface( x_game_runtime_feature_impl, interfaceId, out );
    }
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XSystemAnalyticsImpl ) )
    {
        return IXSystemAnalyticsImpl_QueryInterface( x_system_analytics_impl, interfaceId, out );
    }
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XThreadingImpl ) )
    {
        /**
         * For IXThreading, It's much better to use the native library instead.
         */
        if ( !func )
        {
            LoadOtherRuntime( &asked );
            if ( !asked )
            {
                MessageBoxA( NULL, "The game has requested XThreading\nIt's recommended that you use Microsoft's native binary for this instead.\nTo do so, copy xgameruntime.dll from a Windows machine and place it under the name \"xgameruntime.dll.threading\" within either the game's binaries or within your prefix's system32 folder.\nYou won't be asked this again.", "Attention Required!", MB_ICONEXCLAMATION );
            }
            return IXThreadingImpl_QueryInterface( x_threading_impl, interfaceId, out );
        }
        return func( runtimeClassId, interfaceId, out );
    }
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XNetworkingImpl ) )
    {
        return IXNetworkingImpl_QueryInterface( x_networking_impl, interfaceId, out );
    }
    
    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( runtimeClassId ) );
    return E_NOTIMPL;
}

HRESULT WINAPI UninitializeApiImpl( void )
{
    TRACE("stub!\n");
    return E_NOTIMPL;
}

HRESULT WINAPI XErrorReport( HRESULT status, LPCSTR message )
{
    TRACE("stub!\n");
    return E_NOTIMPL;
}

