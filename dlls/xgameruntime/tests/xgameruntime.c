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
#include <xasync.h>
#include <xasyncprovider.h>
#include <xnetworking.h>

#include "wine/test.h"

static HRESULT (WINAPI *pQueryApiImpl)( const GUID *class_id, REFIID iid, void **out );
#define QueryApiImpl pQueryApiImpl

static HANDLE completion_event;

static void __stdcall async_completed( XAsyncBlock *async )
{
    SetEvent( completion_event );
}

static IXNetworkingImpl *networking;
static IXThreadingImpl *threading;

static HRESULT query_async( BOOL utf16, const void *url, XAsyncBlock *async )
{
    if (utf16) return IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16Async( networking, url, async );
    return IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsync( networking, url, async );
}

static HRESULT query_size( BOOL utf16, XAsyncBlock *async, SIZE_T *size )
{
    if (utf16) return IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResultSize( networking, async, size );
    return IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsyncResultSize( networking, async, size );
}

static HRESULT query_result( BOOL utf16, XAsyncBlock *async, SIZE_T size, SIZE_T *used, UINT8 *buffer,
                             XNetworkingSecurityInformation **info )
{
    if (utf16) return IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlUtf16AsyncResult( networking, async, size, used, buffer, info );
    return IXNetworkingImpl_XNetworkingQuerySecurityInformationForUrlAsyncResult( networking, async, size, used, buffer, info );
}

/* A failed *AsyncResult call consumes the async operation, so every case starts a new one. */
static BOOL start_query( BOOL utf16, XAsyncBlock *async )
{
    static const WCHAR urlW[] = L"https://www.example.com/";
    static const char url[] = "https://www.example.com/";
    HRESULT hr;
    DWORD ret;

    memset( async, 0, sizeof(*async) );
    async->callback = async_completed;
    ResetEvent( completion_event );
    hr = query_async( utf16, utf16 ? (const void *)urlW : (const void *)url, async );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    if (hr != S_OK) return FALSE;

    hr = IXThreadingImpl_XAsyncGetStatus( threading, async, TRUE );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ret = WaitForSingleObject( completion_event, 5000 );
    ok( ret == WAIT_OBJECT_0, "completion callback not called, ret %#lx\n", ret );
    return TRUE;
}

static void test_QuerySecurityInformation( BOOL utf16 )
{
    XNetworkingSecurityInformation *info;
    XAsyncBlock async;
    SIZE_T size, used;
    UINT8 buffer[256];
    HRESULT hr;

    winetest_push_context( utf16 ? "Utf16" : "Utf8" );

    if (!start_query( utf16, &async )) goto done;

    size = 0xdeadbeef;
    hr = query_size( utf16, &async, &size );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( size == sizeof(XNetworkingSecurityInformation), "got size %Iu\n", size );

    memset( buffer, 0xcc, sizeof(buffer) );
    info = NULL;
    used = 0xdeadbeef;
    hr = query_result( utf16, &async, sizeof(buffer), &used, buffer, &info );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( used == sizeof(XNetworkingSecurityInformation), "got used %Iu\n", used );
    ok( info == (XNetworkingSecurityInformation *)buffer, "got info %p, buffer %p\n", info, buffer );
    if (hr == S_OK && info)
    {
        ok( info->enabledHttpSecurityProtocolFlags == 0x800, "got flags %#x\n", info->enabledHttpSecurityProtocolFlags );
        ok( info->thumbprintCount == 0, "got thumbprintCount %Iu\n", info->thumbprintCount );
        ok( !info->thumbprints, "got thumbprints %p\n", info->thumbprints );
        ok( buffer[sizeof(*info)] == 0xcc, "buffer overrun\n" );
    }

    /* the result can only be retrieved once */
    hr = query_result( utf16, &async, sizeof(buffer), &used, buffer, &info );
    ok( FAILED( hr ), "got hr %#lx\n", hr );

    /* exact size buffer, optional bufferUsed */
    if (start_query( utf16, &async ))
    {
        hr = query_result( utf16, &async, sizeof(*info), NULL, buffer, &info );
        ok( hr == S_OK, "got hr %#lx\n", hr );
    }

    /* undersized buffer fails and does not fill in the info pointer */
    if (start_query( utf16, &async ))
    {
        info = (void *)0xdeadbeef;
        hr = query_result( utf16, &async, sizeof(*info) - 1, &used, buffer, &info );
        ok( hr == E_NOT_SUFFICIENT_BUFFER, "got hr %#lx\n", hr );
        ok( info == (void *)0xdeadbeef, "got info %p\n", info );
    }

    /* NULL buffer */
    if (start_query( utf16, &async ))
    {
        hr = query_result( utf16, &async, sizeof(buffer), &used, NULL, &info );
        ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
    }

    /* NULL output pointer */
    if (start_query( utf16, &async ))
    {
        hr = query_result( utf16, &async, sizeof(buffer), &used, buffer, NULL );
        ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
        query_result( utf16, &async, sizeof(buffer), &used, buffer, &info );
    }

    /* NULL size pointer */
    if (start_query( utf16, &async ))
    {
        hr = query_size( utf16, &async, NULL );
        ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
        query_result( utf16, &async, sizeof(buffer), &used, buffer, &info );
    }

    /* NULL async block */
    size = 0xdeadbeef;
    hr = query_size( utf16, NULL, &size );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
    hr = query_result( utf16, NULL, sizeof(buffer), &used, buffer, &info );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    /* NULL url */
    memset( &async, 0, sizeof(async) );
    hr = query_async( utf16, NULL, &async );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    /* NULL async block for the Async call */
    hr = query_async( utf16, utf16 ? (const void *)L"https://www.example.com/" : (const void *)"https://www.example.com/", NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

done:
    winetest_pop_context();
}

START_TEST(xgameruntime)
{
    HKEY key;
    DWORD one = 1;
    HMODULE module = LoadLibraryA( "xgameruntime.dll" );
    HRESULT hr;

    pQueryApiImpl = (void *)GetProcAddress( module, "QueryApiImpl" );
    if (!pQueryApiImpl)
    {
        win_skip( "QueryApiImpl not found\n" );
        return;
    }

    /* Avoid the modal "native XThreading" prompt shown the first time XThreading is queried. */
    if (!RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Wine\\WineGDK", 0, NULL, 0, KEY_WRITE, NULL, &key, NULL ))
    {
        RegSetValueExA( key, "LoadOtherRuntimeAsked", 0, REG_DWORD, (BYTE *)&one, sizeof(one) );
        RegCloseKey( key );
    }

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    completion_event = CreateEventW( NULL, FALSE, FALSE, NULL );

    networking = NULL;
    threading = NULL;
    hr = QueryApiImpl( &CLSID_XNetworkingImpl, &IID_IXNetworkingImpl, (void **)&networking );
    ok( hr == S_OK, "QueryApiImpl XNetworking failed, hr %#lx\n", hr );
    hr = QueryApiImpl( &CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void **)&threading );
    ok( hr == S_OK, "QueryApiImpl XThreading failed, hr %#lx\n", hr );

    //test_GDKComponentInit();
    if (networking && threading)
    {
        test_QuerySecurityInformation( FALSE );
        test_QuerySecurityInformation( TRUE );
    }

    if (threading) IXThreadingImpl_Release( threading );
    if (networking) IXNetworkingImpl_Release( networking );

    CloseHandle( completion_event );
    RoUninitialize();
}
