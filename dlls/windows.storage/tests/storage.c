/*
 * Copyright (C) 2025 Mohamad Al-Jaf
 * Copyright 2026 Conor McCarthy for CodeWeavers
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
#include "test.h"

DEFINE_ASYNC_COMPLETED_HANDLER( async_storage_folder_handler, IAsyncOperationCompletedHandler_StorageFolder, IAsyncOperation_StorageFolder )
DEFINE_ASYNC_COMPLETED_HANDLER( async_action_handler, IAsyncActionCompletedHandler, IAsyncAction )

void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL is_broken )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK || broken( is_broken && hr == E_NOINTERFACE ), "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

#define DEFINE_ASYNC_COMPLETED_HANDLER( name, iface_type, async_type )                              \
    struct name                                                                                     \
    {                                                                                               \
        iface_type iface_type##_iface;                                                              \
        LONG ref;                                                                                   \
        BOOL invoked;                                                                               \
        HANDLE event;                                                                               \
    };                                                                                              \
                                                                                                    \
    static HRESULT WINAPI name##_QueryInterface( iface_type *iface, REFIID iid, void **out )        \
    {                                                                                               \
        if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IAgileObject ) ||           \
            IsEqualGUID( iid, &IID_##iface_type ))                                                  \
        {                                                                                           \
            IUnknown_AddRef( iface );                                                               \
            *out = iface;                                                                           \
            return S_OK;                                                                            \
        }                                                                                           \
                                                                                                    \
        trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );            \
        *out = NULL;                                                                                \
        return E_NOINTERFACE;                                                                       \
    }                                                                                               \
                                                                                                    \
    static ULONG WINAPI name##_AddRef( iface_type *iface )                                          \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        return InterlockedIncrement( &impl->ref );                                                  \
    }                                                                                               \
                                                                                                    \
    static ULONG WINAPI name##_Release( iface_type *iface )                                         \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        ULONG ref = InterlockedDecrement( &impl->ref );                                             \
        if (!ref) free( impl );                                                                     \
        return ref;                                                                                 \
    }                                                                                               \
                                                                                                    \
    static HRESULT WINAPI name##_Invoke( iface_type *iface, async_type *async, AsyncStatus status ) \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        ok( !impl->invoked, "invoked twice\n" );                                                    \
        impl->invoked = TRUE;                                                                       \
        if (impl->event) SetEvent( impl->event );                                                   \
        return S_OK;                                                                                \
    }                                                                                               \
                                                                                                    \
    static iface_type##Vtbl name##_vtbl =                                                           \
    {                                                                                               \
        name##_QueryInterface,                                                                      \
        name##_AddRef,                                                                              \
        name##_Release,                                                                             \
        name##_Invoke,                                                                              \
    };                                                                                              \
                                                                                                    \
    static iface_type *name##_create( HANDLE event )                                                \
    {                                                                                               \
        struct name *impl;                                                                          \
                                                                                                    \
        if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;                                      \
        impl->iface_type##_iface.lpVtbl = &name##_vtbl;                                             \
        impl->event = event;                                                                        \
        impl->ref = 1;                                                                              \
                                                                                                    \
        return &impl->iface_type##_iface;                                                           \
    }                                                                                               \
                                                                                                    \
    static DWORD await_##async_type( async_type *async, DWORD timeout )                             \
    {                                                                                               \
        iface_type *handler;                                                                        \
        HANDLE event;                                                                               \
        HRESULT hr;                                                                                 \
        DWORD ret;                                                                                  \
                                                                                                    \
        event = CreateEventW( NULL, FALSE, FALSE, NULL );                                           \
        ok( !!event, "CreateEventW failed, error %lu\n", GetLastError() );                          \
        handler = name##_create( event );                                                           \
        ok( !!handler, "Failed to create completion handler\n" );                                   \
        hr = async_type##_put_Completed( async, handler );                                          \
        ok( hr == S_OK, "put_Completed returned %#lx\n", hr );                                      \
        ret = WaitForSingleObject( event, timeout );                                                \
        ok( !ret, "WaitForSingleObject returned %#lx\n", ret );                                     \
        CloseHandle( event );                                                                       \
        iface_type##_Release( handler );                                                            \
                                                                                                    \
        return ret;                                                                                 \
    }

static HRESULT get_activation_factory( const WCHAR *name, IActivationFactory **factory )
{
    HSTRING str = NULL;
    HRESULT hr;

    hr = WindowsCreateString( name, wcslen( name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );

    if (hr == REGDB_E_CLASSNOTREG)
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( name ) );
    else if (hr == CLASS_E_CLASSNOTAVAILABLE)
        skip( "%s runtimeclass not available, skipping tests.\n", wine_dbgstr_w( name ) );

    return hr;
}

static void test_RandomAccessStreamReference(void)
{
    static const WCHAR *random_access_stream_reference_statics_name = L"Windows.Storage.Streams.RandomAccessStreamReference";
    IRandomAccessStreamReferenceStatics *random_access_stream_reference_statics = (void *)0xdeadbeef;
    IRandomAccessStreamReference *random_access_stream_reference = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    HSTRING str = NULL;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( random_access_stream_reference_statics_name, wcslen( random_access_stream_reference_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( random_access_stream_reference_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, FALSE );
    check_interface( factory, &IID_IInspectable, FALSE );
    check_interface( factory, &IID_IAgileObject, TRUE /* Supported after Windows 10 1607 */ );

    hr = IActivationFactory_QueryInterface( factory, &IID_IRandomAccessStreamReferenceStatics, (void **)&random_access_stream_reference_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IRandomAccessStreamReferenceStatics_CreateFromStream( random_access_stream_reference_statics, NULL, &random_access_stream_reference );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    ok( random_access_stream_reference == NULL, "IRandomAccessStreamReferenceStatics_CreateFromStream returned %p.\n", random_access_stream_reference );

    ref = IRandomAccessStreamReferenceStatics_Release( random_access_stream_reference_statics );
    ok( ref == 1, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 0, "got ref %ld.\n", ref );
}

/**
 * ABI::Windows::Storage::StorageFolder
 */
void test_StorageFolder( const wchar_t* path )
{
    static const WCHAR *storage_folder_statics_name = L"Windows.Storage.StorageFolder";

    IStorageItem *folderItem = NULL;
    IStorageFolder *folder = NULL;
    IStorageFolderStatics *storage_folder_statics = NULL;

    IAsyncOperation_StorageFolder *storage_folder_operation = NULL;
    IAsyncAction *storage_item_action = NULL;

    HSTRING pathString;
    HSTRING tmpStr;

    HRESULT hr;
    DWORD asyncRes;

    ACTIVATE_INSTANCE( storage_folder_statics_name, storage_folder_statics, IID_IStorageFolderStatics );

    /**
     * ABI::Windows::Storage::IStorageFolderStatics::GetFolderFromPathAsync
     */
    WindowsCreateString( path, wcslen( path ), &pathString );
    hr = IStorageFolderStatics_GetFolderFromPathAsync( storage_folder_statics, pathString, &storage_folder_operation );
    CHECK_HR( hr );

    asyncRes = await_IAsyncOperation_StorageFolder( storage_folder_operation, INFINITE );
    ok( !asyncRes, "got asyncRes %#lx\n", asyncRes );

    hr = IAsyncOperation_StorageFolder_GetResults( storage_folder_operation, &folder );
    CHECK_HR( hr );

    hr = IStorageFolder_QueryInterface( folder, &IID_IStorageItem, (void **)&folderItem );
    CHECK_HR( hr );

    hr = IStorageItem_get_Path( folderItem, &tmpStr );
    ok( !lstrcmpW( path, WindowsGetStringRawBuffer( tmpStr, NULL ) ), "path %s and tmpStr %s do not match!\n", wine_dbgstr_w( path ), wine_dbgstr_hstring( tmpStr ) );

    hr = IStorageItem_DeleteAsync( folderItem, StorageDeleteOption_PermanentDelete, &storage_item_action );
    CHECK_HR( hr );

    asyncRes = await_IAsyncAction( storage_item_action, INFINITE );
    ok( !asyncRes, "got asyncRes %#lx\n", asyncRes );

    IStorageFolderStatics_Release( storage_folder_statics );
}

START_TEST(storage)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_RandomAccessStreamReference();
    test_StorageFolder( L"C:\\users\\office\\AppData\\Local\\Temp\\a" );

    RoUninitialize();
}
