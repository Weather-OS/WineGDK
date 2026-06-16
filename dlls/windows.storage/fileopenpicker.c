/*
 * Copyright 2026 Olivia Ryan
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

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

struct file_open_picker
{
    IFileOpenPicker IFileOpenPicker_iface;
    IInitializeWithWindow IInitializeWithWindow_iface;
    LONG ref;

    IVector_HSTRING *filter;
};

static inline struct file_open_picker *impl_from_IFileOpenPicker( IFileOpenPicker *iface )
{
    return CONTAINING_RECORD( iface, struct file_open_picker, IFileOpenPicker_iface );
}

static HRESULT WINAPI file_open_picker_QueryInterface( IFileOpenPicker *iface, REFIID iid, void **out )
{
    struct file_open_picker *impl = impl_from_IFileOpenPicker( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown        ) ||
        IsEqualGUID( iid, &IID_IInspectable    ) ||
        IsEqualGUID( iid, &IID_IFileOpenPicker ))
    {
        IInspectable_AddRef( *out = &impl->IFileOpenPicker_iface );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IInitializeWithWindow ))
    {
        IInitializeWithWindow_AddRef( *out = &impl->IInitializeWithWindow_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI file_open_picker_AddRef( IFileOpenPicker *iface )
{
    struct file_open_picker *impl = impl_from_IFileOpenPicker( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI file_open_picker_Release( IFileOpenPicker *iface )
{
    struct file_open_picker *impl = impl_from_IFileOpenPicker( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    if (!ref)
    {
        IVector_HSTRING_Release( impl->filter );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI file_open_picker_GetIids( IFileOpenPicker *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_GetRuntimeClassName( IFileOpenPicker *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_GetTrustLevel( IFileOpenPicker *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_get_ViewMode( IFileOpenPicker *iface, PickerViewMode *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_put_ViewMode( IFileOpenPicker *iface, PickerViewMode value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_get_SettingsIdentifier( IFileOpenPicker *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_put_SettingsIdentifier( IFileOpenPicker *iface, HSTRING value )
{
    FIXME( "iface %p, value %s stub!\n", iface, debugstr_hstring( value ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_get_SuggestedStartLocation( IFileOpenPicker *iface, PickerLocationId *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_put_SuggestedStartLocation( IFileOpenPicker *iface, PickerLocationId value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_get_CommitButtonText( IFileOpenPicker *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_put_CommitButtonText( IFileOpenPicker *iface, HSTRING value )
{
    FIXME( "iface %p, value %s stub!\n", iface, debugstr_hstring( value ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_get_FileTypeFilter( IFileOpenPicker *iface, IVector_HSTRING **value )
{
    struct file_open_picker *impl = impl_from_IFileOpenPicker( iface );
    TRACE( "iface %p, value %p.\n", iface, value );
    IVector_HSTRING_AddRef( *value = impl->filter );
    return S_OK;
}

static HRESULT WINAPI file_open_picker_PickSingleFileAsync( IFileOpenPicker *iface, IAsyncOperation_StorageFile **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_open_picker_PickMultipleFilesAsync( IFileOpenPicker *iface, IAsyncOperation_IVectorView_StorageFile **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static const struct IFileOpenPickerVtbl file_open_picker_vtbl =
{
    file_open_picker_QueryInterface,
    file_open_picker_AddRef,
    file_open_picker_Release,
    /* IInspectable methods */
    file_open_picker_GetIids,
    file_open_picker_GetRuntimeClassName,
    file_open_picker_GetTrustLevel,
    /* IFileOpenPicker methods */
    file_open_picker_get_ViewMode,
    file_open_picker_put_ViewMode,
    file_open_picker_get_SettingsIdentifier,
    file_open_picker_put_SettingsIdentifier,
    file_open_picker_get_SuggestedStartLocation,
    file_open_picker_put_SuggestedStartLocation,
    file_open_picker_get_CommitButtonText,
    file_open_picker_put_CommitButtonText,
    file_open_picker_get_FileTypeFilter,
    file_open_picker_PickSingleFileAsync,
    file_open_picker_PickMultipleFilesAsync,
};

static inline struct file_open_picker *impl_from_IInitializeWithWindow( IInitializeWithWindow *iface )
{
    return CONTAINING_RECORD( iface, struct file_open_picker, IInitializeWithWindow_iface );
}

static HRESULT WINAPI initialize_with_window_QueryInterface( IInitializeWithWindow *iface, REFIID iid, void **out )
{
    struct file_open_picker *impl = impl_from_IInitializeWithWindow( iface );
    return IFileOpenPicker_QueryInterface( &impl->IFileOpenPicker_iface, iid, out );
}

static ULONG WINAPI initialize_with_window_AddRef( IInitializeWithWindow *iface )
{
    struct file_open_picker *impl = impl_from_IInitializeWithWindow( iface );
    return IFileOpenPicker_AddRef( &impl->IFileOpenPicker_iface );
}

static ULONG WINAPI initialize_with_window_Release( IInitializeWithWindow *iface )
{
    struct file_open_picker *impl = impl_from_IInitializeWithWindow( iface );
    return IFileOpenPicker_Release( &impl->IFileOpenPicker_iface );
}

static HRESULT WINAPI initialize_with_window_Initialize( IInitializeWithWindow *iface, HWND hwnd )
{
    FIXME( "iface %p, hwnd %p stub!\n", iface, hwnd );
    return E_NOTIMPL;
}

static const struct IInitializeWithWindowVtbl initialize_with_window_vtbl =
{
    initialize_with_window_QueryInterface,
    initialize_with_window_AddRef,
    initialize_with_window_Release,
    /* IInitializeWithWindow methods */
    initialize_with_window_Initialize,
};

struct file_open_picker_statics
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct file_open_picker_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct file_open_picker_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct file_open_picker_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown           ) ||
        IsEqualGUID( iid, &IID_IInspectable       ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IInspectable_AddRef( *out = &impl->IActivationFactory_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct file_open_picker_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct file_open_picker_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    struct file_open_picker *impl;
    HRESULT hr;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IFileOpenPicker_iface.lpVtbl = &file_open_picker_vtbl;
    impl->IInitializeWithWindow_iface.lpVtbl = &initialize_with_window_vtbl;
    impl->ref = 1;

    if (FAILED(hr = create_vector( &impl->filter )))
    {
        free( impl );
        return hr;
    }

    *instance = (IInspectable *)&impl->IFileOpenPicker_iface;
    return S_OK;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

static struct file_open_picker_statics file_open_picker_statics =
{
    {&factory_vtbl},
    0,
};

IActivationFactory *file_open_picker_factory = &file_open_picker_statics.IActivationFactory_iface;
