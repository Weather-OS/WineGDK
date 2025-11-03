/*
 * Xbox Game runtime Library Tests
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

#include <stdlib.h>
#include <stdarg.h>
#define COBJMACROS
#include <initguid.h>
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <roapi.h>
#include <activation.h>
#include <unknwn.h>
#include <xgameerr.h>

#include "provider.h"
#include "wine/test.h"
#include "xthread.h"

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

static HMODULE xgameruntime = NULL;

typedef HRESULT (*InitializeApiImpl)( ULONG gdkVer, ULONG gsVer );
typedef HRESULT (*QueryApiImpl)( const GUID *runtimeClassId, REFIID interfaceId, void **out );

static InitializeApiImpl InitializeApiImpl_fun = NULL;
static QueryApiImpl QueryApiImpl_fun = NULL;

static const SIZE_T XSystemConsoleIdBytes = 39;
static const SIZE_T XSystemXboxLiveSandboxIdMaxBytes = 16;
static const SIZE_T XSystemXboxLiveSandboxIdBytes = 7;
static const SIZE_T XSystemAppSpecificDeviceIdBytes = 45;

static LPSTR testData = NULL;

#define check_interface(obj, iid, supported) _check_interface(__LINE__, obj, iid, supported)
static void _check_interface(unsigned int line, void *obj, const IID *iid, BOOL supported)
{
    IUnknown *iface = obj, *unknown;
    HRESULT hr;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unknown);
    ok_(__FILE__, line)(hr == S_OK || (!supported && hr == E_NOINTERFACE), "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unknown);
}

static inline HRESULT CALLBACK XAsyncProvider_testCallback( XAsyncOp op, const XAsyncProviderData* data )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;
    SIZE_T testDataSize = 7;

    hr = QueryApiImpl_fun( &CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void **)&xthreading );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    switch ( op )
    {
        case Begin:
            trace( "Begin invoked\n" );
            return S_OK;

        case DoWork:
            trace( "DoWork invoked\n" );
            IXThreadingImpl_XAsyncComplete( xthreading, data->async, E_PENDING, 0 );
            testData = (LPSTR)malloc( testDataSize * sizeof( CHAR ) );
            strcpy( testData, "foobar" );
            IXThreadingImpl_XAsyncComplete( xthreading, data->async, S_OK, testDataSize );
            return S_OK;

        case GetResult:
            trace( "GetResult invoked\n" );
            memcpy( data->buffer, (void *)testData, testDataSize);
            return S_OK;

        case Cancel:
            trace( "Cancel invoked\n" );
            IXThreadingImpl_XAsyncComplete( xthreading, data->async, E_ABORT, 0 );
            return S_OK;

        case Cleanup:
            trace( "Cleanup invoked\n" );
            free( testData );
            return S_OK;
    }

    return S_OK;
}

/**
 *  Testing xgameruntime.dll is a bit difficult to do because the core 
 * library (xgameruntime.lib) is responsible for most of the interfaces 
 * used by applications, and applications don't interact with this library
 * directly themselves.
 * 
 *  These test cases were curated to test whatever's in the library itself 
 * at this moment.
 */

static void test_GDKComponentInit(void)
{
    HRESULT hr;
    LPCSTR xgameruntime_libname = "xgameruntime.dll";

    xgameruntime = LoadLibraryA( xgameruntime_libname );
    ok( xgameruntime != NULL, "xgameruntime.dll failed to load! error code: %lu\n", GetLastError() );

    InitializeApiImpl_fun = (InitializeApiImpl)GetProcAddress( xgameruntime, "InitializeApiImpl" );
    ok( InitializeApiImpl_fun != NULL, "couldn't locate function InitializeApiImpl within %p! error code: %lu\n", xgameruntime, GetLastError() );

    hr = InitializeApiImpl_fun( GDKC_VERSION, GAMING_SERVICES_VERSION );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    QueryApiImpl_fun = (QueryApiImpl)GetProcAddress( xgameruntime, "QueryApiImpl" );
    ok( QueryApiImpl_fun != NULL, "couldn't locate function QueryApiImpl within %p! error code: %lu\n", xgameruntime, GetLastError() );
}

static void test_XSystem(void)
{
    IXSystemImpl *xsystem;
    BOOLEAN validHandle;
    HRESULT hr;
    SIZE_T consoleIdUsed;
    SIZE_T sandboxIdUsed;
    LPSTR consoleId;
    LPSTR sandboxId;

    hr = QueryApiImpl_fun( &CLSID_XSystemImpl, &IID_IXSystemImpl, (void **)&xsystem );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( xsystem, &IID_IUnknown, TRUE );
    check_interface( xsystem, &IID_IXSystemImpl, TRUE );

    /**
     * xgameruntime.lib::XSystemGetConsoleId
     */
    consoleId = (LPSTR)malloc( XSystemConsoleIdBytes * sizeof( CHAR ) );
    
    hr = IXSystemImpl_XSystemGetConsoleId( xsystem, XSystemConsoleIdBytes, consoleId, &consoleIdUsed );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( strcmp( consoleId, "00000000.00000000.00000000.00000000.00" ) == 0, "unexpected consoleId. got %s.\n", debugstr_a( consoleId ) );
    ok( consoleIdUsed == XSystemConsoleIdBytes, "unexpected consoleIdUsed. got %lld.\n", consoleIdUsed );

    /**
     * xgameruntime.lib::XSystemGetXboxLiveSandboxId
     */
    sandboxId = (LPSTR)malloc( XSystemXboxLiveSandboxIdMaxBytes * sizeof( CHAR ) );
    
    hr = IXSystemImpl_XSystemGetXboxLiveSandboxId( xsystem, XSystemXboxLiveSandboxIdMaxBytes, sandboxId, &sandboxIdUsed );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( strcmp( sandboxId, "RETAIL" ) == 0, "unexpected sandboxId. got %s.\n", debugstr_a( sandboxId ) );
    ok( sandboxIdUsed == XSystemXboxLiveSandboxIdBytes, "unexpected sandboxIdUsed. got %lld.\n", sandboxIdUsed );

    /**
     * xgameruntime.lib::XSystemGetAppSpecificDeviceId
     */
    hr = IXSystemImpl_XSystemGetAppSpecificDeviceId( xsystem, XSystemAppSpecificDeviceIdBytes, NULL, NULL );
    todo_wine ok( hr == S_OK, "got error %#lx.\n", hr );

    /**
     * xgameruntime.lib::XSystemHandleTrack
     */
    hr = IXSystemImpl_XSystemHandleTrack( xsystem );
    todo_wine ok( hr == S_OK, "got error %#lx.\n", hr );

    /**
     * xgameruntime.lib::XSystemIsHandleValid
     */
    validHandle = IXSystemImpl_XSystemIsHandleValid( xsystem );
    ok( validHandle, "got validHandle %d\n", validHandle );

    /**
     * xgameruntime.lib::XSystemAllowFullDownloadBandwidth
     */
    hr = IXSystemImpl_XSystemAllowFullDownloadBandwidth( xsystem, TRUE );
    todo_wine ok( hr == S_OK, "got error %#lx.\n", hr );

    IXSystemImpl_Release( xsystem );
    free( consoleId );
    free( sandboxId );
}

static void test_XSystemAnalytics(void)
{
    IXSystemAnalyticsImpl *xsystem_analytics;
    XSystemAnalyticsInfo analyticsInfo;
    RTL_OSVERSIONINFOEXW version_info = {0};
    HRESULT hr;
    DWORD ubr;
    DWORD ubr_size = sizeof(ubr);
    HKEY ubr_registry_key;

    hr = QueryApiImpl_fun( &CLSID_XSystemAnalyticsImpl, &IID_IXSystemAnalyticsImpl, (void **)&xsystem_analytics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( xsystem_analytics, &IID_IUnknown, TRUE );
    check_interface( xsystem_analytics, &IID_IXSystemAnalyticsImpl, TRUE );

    /**
     * xgameruntime.lib::XSystemGetAnalyticsInfo
     */
    version_info.dwOSVersionInfoSize = sizeof( version_info );
    ok( RtlGetVersion( &version_info ) == 0, "RtlGetVersion failed.\n" );
    ok( SUCCEEDED( RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ | KEY_WOW64_64KEY, &ubr_registry_key ) ),
        "RegOpenKeyExW failed with code %ld.\n", GetLastError() );
    ok( SUCCEEDED( RegQueryValueExW( ubr_registry_key, L"UBR", NULL, NULL, (LPBYTE)&ubr, &ubr_size ) ),
        "RegQueryValueExW failed with code %ld.\n", GetLastError() );
    RegCloseKey( ubr_registry_key );
    ubr = (ubr & 0xFFFF); // <-- lower bits of UBR is the actual update revision.


    analyticsInfo = IXSystemAnalyticsImpl_XSystemGetAnalyticsInfo( xsystem_analytics );
    ok( analyticsInfo.osVersion.major == (UINT16)version_info.dwMajorVersion, 
        "major version %d differs from %d.\n", analyticsInfo.osVersion.major, (UINT16)version_info.dwMajorVersion);
    ok( analyticsInfo.osVersion.minor == (UINT16)version_info.dwMinorVersion, 
        "minor version %d differs from %d.\n", analyticsInfo.osVersion.minor, (UINT16)version_info.dwMinorVersion);
    ok( analyticsInfo.osVersion.build == (UINT16)version_info.dwBuildNumber, 
        "build number %d differs from %d.\n", analyticsInfo.osVersion.build, (UINT16)version_info.dwBuildNumber);
    ok( analyticsInfo.osVersion.revision == (UINT16)ubr, 
        "update revision %d differs from %d.\n", analyticsInfo.hostingOsVersion.revision, (UINT16)ubr);
    ok( analyticsInfo.hostingOsVersion.major == (UINT16)version_info.dwMajorVersion, 
        "host major version %d differs from %d.\n", analyticsInfo.hostingOsVersion.major, (UINT16)version_info.dwMajorVersion);
    ok( analyticsInfo.hostingOsVersion.minor == (UINT16)version_info.dwMinorVersion, 
        "host minor version %d differs from %d.\n", analyticsInfo.hostingOsVersion.minor, (UINT16)version_info.dwMinorVersion);
    ok( analyticsInfo.hostingOsVersion.build == (UINT16)version_info.dwBuildNumber, 
        "host build number %d differs from %d.\n", analyticsInfo.hostingOsVersion.build, (UINT16)version_info.dwBuildNumber);
    ok( analyticsInfo.hostingOsVersion.revision == (UINT16)ubr, 
        "host update revision %d differs from %d.\n", analyticsInfo.osVersion.revision, (UINT16)ubr);
    ok( strcmp( analyticsInfo.family, "Windows" ) == 0, "unexpected family %s.\n", debugstr_a( analyticsInfo.family ) );
    ok( strcmp( analyticsInfo.form, "Desktop" ) == 0, "unexpected form %s.\n", debugstr_a( analyticsInfo.form ) );

    IXSystemAnalyticsImpl_Release( xsystem_analytics );
}

static void test_XGameRuntimeFeature(void)
{
    IXGameRuntimeFeatureImpl *xgame_runtime_feature;
    HRESULT hr;
    BOOLEAN isAvailable;

    hr = QueryApiImpl_fun( &CLSID_XGameRuntimeFeatureImpl, &IID_IXGameRuntimeFeatureImpl, (void **)&xgame_runtime_feature );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( xgame_runtime_feature, &IID_IUnknown, TRUE );
    check_interface( xgame_runtime_feature, &IID_IXGameRuntimeFeatureImpl, TRUE );

    /**
     * xgameruntime.lib::XGameRuntimeIsFeatureAvailable
     */
    isAvailable = IXGameRuntimeFeatureImpl_XGameRuntimeIsFeatureAvailable( xgame_runtime_feature, XGame );
    ok( isAvailable, "got unexpected isAvailable %d.\n", isAvailable );

    IXGameRuntimeFeatureImpl_Release( xgame_runtime_feature );
}

static void test_XThreading(void)
{
    IXThreadingImpl *xthreading;
    HRESULT hr;
    SIZE_T receivedBufferSize;
    SIZE_T bufferUsed;
    LPSTR receivedBuffer;

    hr = QueryApiImpl_fun( &CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void **)&xthreading );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( xthreading, &IID_IUnknown, TRUE );
    check_interface( xthreading, &IID_IXThreadingImpl, TRUE );

    /**
     * Microsoft is very vague about how XAsync is used in applications.
     * This is the best implementation I can get for now.
     */

    // --- XAsync --- //
    {
        XAsyncBlock currentBlock;

        currentBlock.callback = NULL;
        currentBlock.queue = NULL;

        /**
         * xgameruntime.lib::XAsyncBegin
         */
        hr = IXThreadingImpl_XAsyncBegin( xthreading, &currentBlock, NULL, NULL, NULL, XAsyncProvider_testCallback );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        /**
         * xgameruntime.lib::XAsyncGetStatus
         */
        hr = IXThreadingImpl_XAsyncGetStatus( xthreading, &currentBlock, TRUE );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        /**
         * xgameruntime.lib::XAsyncSchedule
         */
        hr = IXThreadingImpl_XAsyncSchedule( xthreading, &currentBlock, 1000 );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        hr = IXThreadingImpl_XAsyncGetStatus( xthreading, &currentBlock, TRUE );
        ok( hr == S_OK, "got hr %#lx.\n", hr );

        /**
         * xgameruntime.lib::XAsyncGetResultSize
         */
        hr = IXThreadingImpl_XAsyncGetResultSize( xthreading, &currentBlock, &receivedBufferSize );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        ok( receivedBufferSize == 7, "unexpected receivedBufferSize %lld.\b", receivedBufferSize );

        receivedBuffer = (LPSTR)malloc( receivedBufferSize );

        /**
         * xgameruntime.lib::XAsyncGetResult
         */
        hr = IXThreadingImpl_XAsyncGetResult( xthreading, &currentBlock, NULL, receivedBufferSize, (PVOID)receivedBuffer, &bufferUsed );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        ok( bufferUsed == 7, "unexpected bufferUsed %lld.\b", bufferUsed );
        ok( strcmp( receivedBuffer, "foobar" ) == 0, "unexpected receivedBuffer %s.\n", debugstr_a( receivedBuffer ) );
    }
}

START_TEST(xgameruntime)
{
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    test_GDKComponentInit();
    test_XSystem();
    test_XSystemAnalytics();
    test_XGameRuntimeFeature();
    test_XThreading();

    RoUninitialize();
}