/*
 * Written by Weather
 *
 * This is an implementation of Microsoft's OneCoreUAP binaries.
 *
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

#ifndef _WINDOWS_STORAGE_TESTS_
#define _WINDOWS_STORAGE_TESTS_

#define COBJMACROS
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
#include <cstdint>
#endif

#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "shlwapi.h"
#include "shlobj.h"
#include "roerrorapi.h"

#include "roapi.h"
#include "shobjidl.h"

#ifdef __cplusplus
// Bug: WinRT in C++ within Wine lacks proper C++ type handling
// Redefine boolean as bool, and DOUBLE as double to prevent compile issues.
// Casting is purely handled by libstdc++, so it's not an issue here.
#undef boolean
#define boolean bool
#undef DOUBLE
#define DOUBLE double

// Bug: __WINESRC__ is not defined in C++ contexts.
#define __WINESRC__ 1
#endif

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Storage
#define WIDL_using_Windows_Storage_Pickers
#define WIDL_using_Windows_Storage_Streams
#include "windows.storage.h"
#include "windows.storage.pickers.h"
#include "windows.storage.streams.h"

#include "wine/test.h"

#define DEFINE_ASYNC_COMPLETED_HANDLER( name, iface_type, async_type )                              \
    struct name                                                                                     \
    {                                                                                               \
        iface_type iface_type##_iface;                                                              \
        LONG refcount;                                                                              \
        BOOL invoked;                                                                               \
        HANDLE event;                                                                               \
    };                                                                                              \
                                                                                                    \
    struct name *impl_from_##name( iface_type *iface )                                              \
    {                                                                                               \
        return CONTAINING_RECORD( iface, struct name, iface_type##_iface );                         \
    }                                                                                               \
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
        *out = NULL;                                                                                \
        return E_NOINTERFACE;                                                                       \
    }                                                                                               \
                                                                                                    \
    static ULONG WINAPI name##_AddRef( iface_type *iface )                                          \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        return InterlockedIncrement( &impl->refcount );                                             \
    }                                                                                               \
                                                                                                    \
    static ULONG WINAPI name##_Release( iface_type *iface )                                         \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        ULONG ref = InterlockedDecrement( &impl->refcount );                                        \
        if (!ref) free( impl );                                                                     \
        return ref;                                                                                 \
    }                                                                                               \
                                                                                                    \
    static HRESULT WINAPI name##_Invoke( iface_type *iface, async_type *async, AsyncStatus status ) \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
                                                                                                    \
        trace( "iface %p, async %p, status %u\n", iface, async, status );                           \
                                                                                                    \
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
        impl->refcount = 1;                                                                         \
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

#define check_interface( obj, iid, broken ) check_interface_( __LINE__, obj, iid, broken )
void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL is_broken );

#define stubbed_interface( obj, iid ) stubbed_interface_( __LINE__, obj, iid )
void stubbed_interface_( unsigned int line, void *obj, const IID *iid );

#define CHECK_LAST_RESTRICTED_ERROR()                                                                   \
    {                                                                                                   \
        BSTR _error_desc;                                                                               \
        HRESULT _hr;                                                                                    \
        IRestrictedErrorInfo *_restricted_error = NULL;                                                 \
        _hr = GetRestrictedErrorInfo( &_restricted_error );                                             \
        if ( _hr != S_FALSE )                                                                           \
        {                                                                                               \
            IRestrictedErrorInfo_GetErrorDetails( _restricted_error, &_error_desc, &_hr, NULL, NULL );  \
            trace( "got hr %#lx, message: %s\n", _hr, debugstr_w(_error_desc) );                        \
            IRestrictedErrorInfo_Release( _restricted_error );                                          \
        }                                                                                               \
    }

#define CHECK_HR( hr )                      \
    ok( hr == S_OK, "got hr %#lx.\n", hr ); \

#define ACTIVATE_INSTANCE( instance_name, instance_object, instance_iid )                                                   \
    {                                                                                                                       \
        HSTRING _str;                                                                                                       \
        HRESULT _hr;                                                                                                        \
        _hr = WindowsCreateString( instance_name, wcslen( instance_name ), &_str );                                         \
        ok( _hr == S_OK, "got hr %#lx.\n", _hr );                                                                           \
                                                                                                                            \
        _hr = RoGetActivationFactory( _str, &instance_iid, (void **)&instance_object );                                     \
        WindowsDeleteString( _str );                                                                                        \
        ok( _hr == S_OK || broken( _hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", _hr );                                   \
        if (_hr == REGDB_E_CLASSNOTREG)                                                                                     \
        {                                                                                                                   \
            win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( instance_name ) );                \
        }                                                                                                                   \
        check_interface( instance_object, &IID_IUnknown, FALSE );                                                           \
        check_interface( instance_object, &IID_IInspectable, FALSE );                                                       \
                                                                                                                            \
        CHECK_HR( _hr );                                                                                                    \
    }

#endif