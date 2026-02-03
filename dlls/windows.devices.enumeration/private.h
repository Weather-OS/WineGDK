/* WinRT Windows.Devices.Enumeration implementation
 *
 * Copyright 2021 Gijs Vermeulen
 * Copyright 2022 Julian Klemann for CodeWeavers
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

#ifndef __WINE_WINDOWS_DEVICES_ENUMERATION_PRIVATE_H
#define __WINE_WINDOWS_DEVICES_ENUMERATION_PRIVATE_H

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "objbase.h"
#include "devpropdef.h"
#include "devfiltertypes.h"
#include "devquerydef.h"
#include "propsys.h"

#include "activation.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Devices_Enumeration
#include "windows.devices.enumeration.h"

#include "wine/list.h"

#include "async_private.h"
#include "weakref.h"

struct vector_iids
{
    const GUID *vector;
    const GUID *view;
    const GUID *iterable;
    const GUID *iterator;
};

extern IActivationFactory *device_access_factory;

HRESULT typed_event_handlers_append( struct list *list, ITypedEventHandler_IInspectable_IInspectable *handler, EventRegistrationToken *token );
HRESULT typed_event_handlers_remove( struct list *list, EventRegistrationToken *token );
HRESULT typed_event_handlers_notify( struct list *list, IInspectable *sender, IInspectable *args );
HRESULT typed_event_handlers_clear( struct list *list );

extern HRESULT async_operation_inspectable_create( const GUID *iid, IUnknown *invoker, IUnknown *param, async_operation_callback callback,
                                                   IAsyncOperation_IInspectable **out );
extern HRESULT async_action_create( IUnknown *invoker, async_operation_callback callback, IAsyncAction **out );

extern HRESULT vector_create( const struct vector_iids *iids, void **out );
extern HRESULT device_information_create( const DEV_OBJECT *obj, IDeviceInformation **info );
extern HRESULT async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result, BOOL called_async );

#define DEFINE_IINSPECTABLE_( pfx, iface_type, impl_type, impl_from, iface_mem, expr )             \
    static inline impl_type *impl_from( iface_type *iface )                                        \
    {                                                                                              \
        return CONTAINING_RECORD( iface, impl_type, iface_mem );                                   \
    }                                                                                              \
    static HRESULT WINAPI pfx##_QueryInterface( iface_type *iface, REFIID iid, void **out )        \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_QueryInterface( (IInspectable *)(expr), iid, out );                    \
    }                                                                                              \
    static ULONG WINAPI pfx##_AddRef( iface_type *iface )                                          \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_AddRef( (IInspectable *)(expr) );                                      \
    }                                                                                              \
    static ULONG WINAPI pfx##_Release( iface_type *iface )                                         \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_Release( (IInspectable *)(expr) );                                     \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetIids( iface_type *iface, ULONG *iid_count, IID **iids )         \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetIids( (IInspectable *)(expr), iid_count, iids );                    \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetRuntimeClassName( iface_type *iface, HSTRING *class_name )      \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetRuntimeClassName( (IInspectable *)(expr), class_name );             \
    }                                                                                              \
    static HRESULT WINAPI pfx##_GetTrustLevel( iface_type *iface, TrustLevel *trust_level )        \
    {                                                                                              \
        impl_type *impl = impl_from( iface );                                                      \
        return IInspectable_GetTrustLevel( (IInspectable *)(expr), trust_level );                  \
    }
#define DEFINE_IINSPECTABLE( pfx, iface_type, impl_type, base_iface )                              \
    DEFINE_IINSPECTABLE_( pfx, iface_type, impl_type, impl_from_##iface_type, iface_type##_iface, &impl->base_iface )
#define DEFINE_IINSPECTABLE_OUTER( pfx, iface_type, impl_type, outer_iface )                       \
    DEFINE_IINSPECTABLE_( pfx, iface_type, impl_type, impl_from_##iface_type, iface_type##_iface, impl->outer_iface )

#define DEFINE_ASYNC_COMPLETED_HANDLER( name, iface_type, async_type )                              \
    struct name                                                                                     \
    {                                                                                               \
        iface_type iface_type##_iface;                                                              \
        LONG refcount;                                                                              \
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
        DWORD ret;                                                                                  \
                                                                                                    \
        event = CreateEventW( NULL, FALSE, FALSE, NULL );                                           \
        handler = name##_create( event );                                                           \
        async_type##_put_Completed( async, handler );                                               \
        ret = WaitForSingleObject( event, timeout );                                                \
        CloseHandle( event );                                                                       \
        iface_type##_Release( handler );                                                            \
                                                                                                    \
        return ret;                                                                                 \
    }

#endif
