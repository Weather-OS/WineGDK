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

#include "ntstatus.h"

WINE_DEFAULT_DEBUG_CHANNEL(xgameruntime);

static HMODULE xgameruntime;
static HMODULE xgameruntime_threading;

unixlib_module_t unixlib;
unixlib_handle_t unixhandle;

BOOLEAN initializeCalled = FALSE;

DEFINE_ASYNC_COMPLETED_HANDLER( async_action, IAsyncActionCompletedHandler, IAsyncAction );

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
        {
            DisableThreadLibraryCalls(hinst);
            xgameruntime_threading = LoadLibraryA("xgameruntime.dll.threading");
            break;
        }
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            if (xgameruntime) FreeLibrary(xgameruntime);
            if (xgameruntime_threading) FreeLibrary(xgameruntime_threading);
            if (unixlib) __wine_unload_unix_lib( unixlib );
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
    //   Documentation for INITIALIZE_OPTIONS is at 
    //  https://learn.microsoft.com/en-us/xbox/gdk/docs/reference/system/xgameruntimeinit/functions/xgameruntimeinitializewithoptions
    // 
    // NOTE: Never rely on INITIALIZE_OPTIONS to provide anything, as it can be nullptr.
    //

#if XODUS_INTEROP
    HRESULT hr;
    NTSTATUS nts;
    UNICODE_STRING modname;
    DWORD async;
    LPCSTR xodus_prefix = XODUS_SOCKET_SUFFIX;

    IAsyncAction *pingAction = NULL;

    if (initializeCalled) goto _INIT;

    // load the unix lib as well.
    // The library is called xgameruntime.so on both macOS and Linux
    RtlInitUnicodeString( &modname, L"xgameruntime.so" );
    nts = __wine_load_unix_lib( &modname, &unixlib, &unixhandle );
    if ( FAILED( nts ) )
    {
        WARN("Failed to load unix lib %s\n", "xgameruntime.so");
        return FALSE;
    }
    nts = __wine_unix_call( unixhandle, conn_socket, (void *)xodus_prefix );
    if ( nts == STATUS_CONNECTION_REFUSED )
    {
        WARN("Failed to do unix call %s\n", "conn_socket");
        MessageBoxA( NULL, "Could not load Xodus's service socket.\nXbox account functionality will be missing.\n", "Attention Required!", MB_ICONEXCLAMATION );
        goto _INIT;
    }
    else if ( FAILED( nts ) )
    {
        WARN("Failed to do unix call %s\n", "conn_socket");
        goto _INIT;
    }

    hr = IIPCLayer_InitializeSocket( xodus_ipclayer );
    if ( FAILED( hr ) ) 
    {
        WARN("Socket initialization failed with %#lx\n", hr);
        goto _INIT;
    }
    hr = IXodusService_Ping( xodus_service, &pingAction );
    if ( FAILED( hr ) ) 
    {
        WARN("Xodus Ping Dispatch failed with %#lx\n", hr);
        goto _INIT;
    }

    async = await_IAsyncAction( pingAction, IPC_REQUEST_TIMEOUT_MS );
    if ( async )
    {
        if ( async == STATUS_TIMEOUT )
            WARN("Timeout while waiting for PING response.\n");
        else 
            WARN("Async action await failed. Status was %ld\n", async);
        goto _INIT;
    }

    hr = IAsyncAction_GetResults( pingAction );
    if ( FAILED( hr ) )
    {
        WARN("PING response error. HR was %#lx\n", hr);
        goto _INIT;
    }
_INIT:
#endif

    TRACE("gdkVer %ld, gsVer %ld, mode %d, options %p stub!\n", gdkVer, gsVer, mode, options);

    if (initializeCalled) return S_OK;
    initializeCalled = TRUE;
    return S_OK;
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

typedef HRESULT (WINAPI *QueryApiImpl_ext)( const GUID *runtimeClassId, REFIID interfaceId, void **out );

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
        return IXSystemImpl_QueryInterface( x_system, interfaceId, out );
    }
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XGameRuntimeFeatureImpl ) )
    {
        return IXGameRuntimeFeatureImpl_QueryInterface( x_game_runtime_feature, interfaceId, out );
    }
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XSystemAnalyticsImpl ) )
    {
        return IXSystemAnalyticsImpl_QueryInterface( x_system_analytics, interfaceId, out );
    }
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XNetworkingImpl ) )
    {
        return IXNetworkingImpl_QueryInterface( x_networking, interfaceId, out );
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
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XUserImpl ) )
    {
        return IXUserImpl6_QueryInterface( x_user, interfaceId, out );
    }
    else if ( IsEqualGUID( runtimeClassId, &CLSID_XUserDeviceImpl ) )
    {
        return IXUserDeviceImpl_QueryInterface( x_user_device, interfaceId, out );
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