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

struct file_save_picker
{
    IFileSavePicker IFileSavePicker_iface;
    IInitializeWithWindow IInitializeWithWindow_iface;
    LONG ref;

    HWND hwnd;
    IFileSaveDialog *dialog;
    HSTRING suggestedFileName;
    IMap_HSTRING_IInspectable *fileTypeChoices;
};

static inline struct file_save_picker *impl_from_IFileSavePicker( IFileSavePicker *iface )
{
    return CONTAINING_RECORD( iface, struct file_save_picker, IFileSavePicker_iface );
}

static HRESULT WINAPI file_save_picker_QueryInterface( IFileSavePicker *iface, REFIID iid, void **out )
{
    struct file_save_picker *impl = impl_from_IFileSavePicker( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown        ) ||
        IsEqualGUID( iid, &IID_IInspectable    ) ||
        IsEqualGUID( iid, &IID_IFileSavePicker ))
    {
        IInspectable_AddRef( *out = &impl->IFileSavePicker_iface );
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

static ULONG WINAPI file_save_picker_AddRef( IFileSavePicker *iface )
{
    struct file_save_picker *impl = impl_from_IFileSavePicker( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI file_save_picker_Release( IFileSavePicker *iface )
{
    struct file_save_picker *impl = impl_from_IFileSavePicker( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    if (!ref)
    {
        if (impl->suggestedFileName) WindowsDeleteString( impl->suggestedFileName );
        IMap_HSTRING_IInspectable_Release( impl->fileTypeChoices );
        IFileSaveDialog_Release( impl->dialog );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI file_save_picker_GetIids( IFileSavePicker *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_GetRuntimeClassName( IFileSavePicker *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_GetTrustLevel( IFileSavePicker *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_get_SettingsIdentifier( IFileSavePicker *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_put_SettingsIdentifier( IFileSavePicker *iface, HSTRING value )
{
    FIXME( "iface %p, value %s stub!\n", iface, debugstr_hstring( value ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_get_SuggestedStartLocation( IFileSavePicker *iface, PickerLocationId *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_put_SuggestedStartLocation( IFileSavePicker *iface, PickerLocationId value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_get_CommitButtonText( IFileSavePicker *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_put_CommitButtonText( IFileSavePicker *iface, HSTRING value )
{
    FIXME( "iface %p, value %s stub!\n", iface, debugstr_hstring( value ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_get_FileTypeChoices( IFileSavePicker *iface, IMap_HSTRING_IVector_HSTRING **value )
{
    struct file_save_picker *impl = impl_from_IFileSavePicker( iface );
    TRACE( "iface %p, value %p.\n", iface, value );
    IMap_HSTRING_IInspectable_AddRef( impl->fileTypeChoices );
    *value = (IMap_HSTRING_IVector_HSTRING *)impl->fileTypeChoices;
    return S_OK;
}

static HRESULT WINAPI file_save_picker_get_DefaultFileExtension( IFileSavePicker *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_put_DefaultFileExtension( IFileSavePicker *iface, HSTRING value )
{
    FIXME( "iface %p, value %s stub!\n", iface, debugstr_hstring( value ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_get_SuggestedSaveFile( IFileSavePicker *iface, IStorageFile **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_put_SuggestedSaveFile( IFileSavePicker *iface, IStorageFile *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI file_save_picker_get_SuggestedFileName( IFileSavePicker *iface, HSTRING *value )
{
    struct file_save_picker *impl = impl_from_IFileSavePicker( iface );
    TRACE( "iface %p, value %p.\n", iface, value );
    return WindowsDuplicateString( impl->suggestedFileName, value );
}

static HRESULT WINAPI file_save_picker_put_SuggestedFileName( IFileSavePicker *iface, HSTRING value )
{
    struct file_save_picker *impl = impl_from_IFileSavePicker( iface );

    TRACE( "iface %p, value %s.\n", iface, debugstr_hstring( value ) );

    if (impl->suggestedFileName)
    {
        WindowsDeleteString( impl->suggestedFileName );
        impl->suggestedFileName = NULL;
    }

    return WindowsDuplicateString( value, &impl->suggestedFileName );
}

static HRESULT pick_save_file_async( IUnknown *invoker, IUnknown *param, PROPVARIANT *result, BOOL called_async )
{
    struct file_save_picker *impl = impl_from_IFileSavePicker( (IFileSavePicker *)invoker );
    IModalWindow *modal;
    IFileDialog *dialog;
    HRESULT hr;

    if (!called_async) return STATUS_PENDING;

    if (FAILED(hr = IFileSaveDialog_QueryInterface( impl->dialog, &IID_IFileDialog, (void **)&dialog ))) return hr;
    if (FAILED(hr = IFileDialog_SetFileName( dialog, WindowsGetStringRawBuffer( impl->suggestedFileName, NULL ) ))) goto cleanup;
    if (FAILED(hr = IFileSaveDialog_QueryInterface( impl->dialog, &IID_IModalWindow, (void **)&modal ))) goto cleanup;
    if (FAILED(hr = IModalWindow_Show( modal, impl->hwnd ))) goto cleanup;

cleanup:
    if (modal) IModalWindow_Release( modal );
    IFileDialog_Release( dialog );
    return hr;
}

static HRESULT WINAPI file_save_picker_PickSaveFileAsync( IFileSavePicker *iface, IAsyncOperation_StorageFile **value )
{
    TRACE( "iface %p, value %p.\n", iface, value );
    return async_operation_file_create( (IUnknown *)iface, NULL, pick_save_file_async, value );
}

static const struct IFileSavePickerVtbl file_save_picker_vtbl =
{
    file_save_picker_QueryInterface,
    file_save_picker_AddRef,
    file_save_picker_Release,
    /* IInspectable methods */
    file_save_picker_GetIids,
    file_save_picker_GetRuntimeClassName,
    file_save_picker_GetTrustLevel,
    /* IFileSavePicker methods */
    file_save_picker_get_SettingsIdentifier,
    file_save_picker_put_SettingsIdentifier,
    file_save_picker_get_SuggestedStartLocation,
    file_save_picker_put_SuggestedStartLocation,
    file_save_picker_get_CommitButtonText,
    file_save_picker_put_CommitButtonText,
    file_save_picker_get_FileTypeChoices,
    file_save_picker_get_DefaultFileExtension,
    file_save_picker_put_DefaultFileExtension,
    file_save_picker_get_SuggestedSaveFile,
    file_save_picker_put_SuggestedSaveFile,
    file_save_picker_get_SuggestedFileName,
    file_save_picker_put_SuggestedFileName,
    file_save_picker_PickSaveFileAsync,
};

static inline struct file_save_picker *impl_from_IInitializeWithWindow( IInitializeWithWindow *iface )
{
    return CONTAINING_RECORD( iface, struct file_save_picker, IInitializeWithWindow_iface );
}

static HRESULT WINAPI initialize_with_window_QueryInterface( IInitializeWithWindow *iface, REFIID iid, void **out )
{
    struct file_save_picker *impl = impl_from_IInitializeWithWindow( iface );
    return IFileSavePicker_QueryInterface( &impl->IFileSavePicker_iface, iid, out );
}

static ULONG WINAPI initialize_with_window_AddRef( IInitializeWithWindow *iface )
{
    struct file_save_picker *impl = impl_from_IInitializeWithWindow( iface );
    return IFileSavePicker_AddRef( &impl->IFileSavePicker_iface );
}

static ULONG WINAPI initialize_with_window_Release( IInitializeWithWindow *iface )
{
    struct file_save_picker *impl = impl_from_IInitializeWithWindow( iface );
    return IFileSavePicker_Release( &impl->IFileSavePicker_iface );
}

static HRESULT WINAPI initialize_with_window_Initialize( IInitializeWithWindow *iface, HWND hwnd )
{
    struct file_save_picker *impl = impl_from_IInitializeWithWindow( iface );
    TRACE( "iface %p, hwnd %p.\n", iface, hwnd );
    impl->hwnd = hwnd;
    return S_OK;
}

static const struct IInitializeWithWindowVtbl initialize_with_window_vtbl =
{
    initialize_with_window_QueryInterface,
    initialize_with_window_AddRef,
    initialize_with_window_Release,
    /* IInitializeWithWindow methods */
    initialize_with_window_Initialize,
};

struct file_save_picker_statics
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct file_save_picker_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct file_save_picker_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct file_save_picker_statics *impl = impl_from_IActivationFactory( iface );

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
    struct file_save_picker_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct file_save_picker_statics *impl = impl_from_IActivationFactory( iface );
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
    static const WCHAR *propertyset_name = RuntimeClass_Windows_Foundation_Collections_PropertySet;
    struct file_save_picker *impl;
    IPropertySet *propertyset;
    HSTRING_HEADER hdr;
    HSTRING str;
    HRESULT hr;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IFileSavePicker_iface.lpVtbl = &file_save_picker_vtbl;
    impl->IInitializeWithWindow_iface.lpVtbl = &initialize_with_window_vtbl;
    impl->ref = 1;

    if (FAILED(hr = WindowsCreateStringReference( propertyset_name, wcslen( propertyset_name ), &hdr, &str ))) goto error;
    if (FAILED(hr = RoActivateInstance( str, (IInspectable **)&propertyset ))) goto error;

    hr = IPropertySet_QueryInterface( propertyset, &IID_IMap_HSTRING_IInspectable, (void **)&impl->fileTypeChoices );
    IPropertySet_Release( propertyset );
    if (FAILED(hr)) goto error;

    if (FAILED(hr = CoCreateInstance( &CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileSaveDialog, (void **)&impl->dialog )))
    {
        IMap_HSTRING_IInspectable_Release( impl->fileTypeChoices );
        goto error;
    }


    *instance = (IInspectable *)&impl->IFileSavePicker_iface;
    return S_OK;

error:
    free( impl );
    return hr;
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

static struct file_save_picker_statics file_save_picker_statics =
{
    {&factory_vtbl},
    0,
};

IActivationFactory *file_save_picker_factory = &file_save_picker_statics.IActivationFactory_iface;
