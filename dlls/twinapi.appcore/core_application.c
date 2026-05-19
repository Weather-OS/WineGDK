/* WinRT Windows.ApplicationModel.Core.CoreApplication implementation
 *
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(twinapi);

/* GDK/Xbox-specific GUID that Minecraft Bedrock queries for on the CoreApplication factory.
 * Not present in standard Windows SDK headers but the game expects it. We route it to
 * our ICoreApplication2 implementation since the game crashes (NULL deref) if QI fails. */
static const GUID IID_ICoreApplication2_GDK = {0x17b0e613, 0x942a, 0x422d, {0x90,0x4c, 0xf9,0x0d,0xc7,0x1a,0x7d,0xae}};

/* ========================================================================
 * Stub IPropertySet / IMap<HSTRING,IInspectable*> implementation
 *
 * This is a static singleton empty property bag. It implements:
 *   - IPropertySet (marker interface, no methods beyond IInspectable)
 *   - IMap<HSTRING, IInspectable*> (Lookup, get_Size, HasKey, GetView, Insert, Remove, Clear)
 *   - IObservableMap<HSTRING, IInspectable*> (add/remove_MapChanged)
 *   - IIterable<IKeyValuePair<HSTRING, IInspectable*>> (First)
 * All return empty/zero/false since the game just needs a valid object.
 * ======================================================================== */

struct property_set
{
    IPropertySet IPropertySet_iface;
    __FIMap_2_HSTRING_IInspectable IMap_iface;
    __FIObservableMap_2_HSTRING_IInspectable IObservableMap_iface;
    __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable IIterable_iface;
    LONG ref;
};

static struct property_set property_set_instance;

/* --- IPropertySet --- */

static HRESULT WINAPI prop_set_QueryInterface( IPropertySet *iface, REFIID iid, void **out )
{
    struct property_set *impl = &property_set_instance;

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IPropertySet ))
    {
        *out = &impl->IPropertySet_iface;
        IPropertySet_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID___FIMap_2_HSTRING_IInspectable ))
    {
        *out = &impl->IMap_iface;
        IPropertySet_AddRef( &impl->IPropertySet_iface );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID___FIObservableMap_2_HSTRING_IInspectable ))
    {
        *out = &impl->IObservableMap_iface;
        IPropertySet_AddRef( &impl->IPropertySet_iface );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID___FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable ))
    {
        *out = &impl->IIterable_iface;
        IPropertySet_AddRef( &impl->IPropertySet_iface );
        return S_OK;
    }

    FIXME( "IPropertySet %s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI prop_set_AddRef( IPropertySet *iface )
{
    struct property_set *impl = &property_set_instance;
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI prop_set_Release( IPropertySet *iface )
{
    struct property_set *impl = &property_set_instance;
    return InterlockedDecrement( &impl->ref );
}

static HRESULT WINAPI prop_set_GetIids( IPropertySet *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    *iid_count = 0;
    *iids = NULL;
    return S_OK;
}

static HRESULT WINAPI prop_set_GetRuntimeClassName( IPropertySet *iface, HSTRING *class_name )
{
    return WindowsCreateString( L"Windows.Foundation.Collections.PropertySet", 42, class_name );
}

static HRESULT WINAPI prop_set_GetTrustLevel( IPropertySet *iface, TrustLevel *trust_level )
{
    *trust_level = BaseTrust;
    return S_OK;
}

static const struct IPropertySetVtbl prop_set_vtbl =
{
    prop_set_QueryInterface,
    prop_set_AddRef,
    prop_set_Release,
    prop_set_GetIids,
    prop_set_GetRuntimeClassName,
    prop_set_GetTrustLevel,
};

/* --- IMap<HSTRING, IInspectable*> --- */

static HRESULT WINAPI prop_map_QueryInterface( __FIMap_2_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    return prop_set_QueryInterface( &property_set_instance.IPropertySet_iface, iid, out );
}

static ULONG WINAPI prop_map_AddRef( __FIMap_2_HSTRING_IInspectable *iface )
{
    return prop_set_AddRef( &property_set_instance.IPropertySet_iface );
}

static ULONG WINAPI prop_map_Release( __FIMap_2_HSTRING_IInspectable *iface )
{
    return prop_set_Release( &property_set_instance.IPropertySet_iface );
}

static HRESULT WINAPI prop_map_GetIids( __FIMap_2_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    return prop_set_GetIids( &property_set_instance.IPropertySet_iface, iid_count, iids );
}

static HRESULT WINAPI prop_map_GetRuntimeClassName( __FIMap_2_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    return prop_set_GetRuntimeClassName( &property_set_instance.IPropertySet_iface, class_name );
}

static HRESULT WINAPI prop_map_GetTrustLevel( __FIMap_2_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    return prop_set_GetTrustLevel( &property_set_instance.IPropertySet_iface, trust_level );
}

static HRESULT WINAPI prop_map_Lookup( __FIMap_2_HSTRING_IInspectable *iface, HSTRING key, IInspectable **value )
{
    FIXME( "key %s stub!\n", debugstr_hstring( key ) );
    *value = NULL;
    return E_BOUNDS;
}

static HRESULT WINAPI prop_map_get_Size( __FIMap_2_HSTRING_IInspectable *iface, unsigned int *size )
{
    *size = 0;
    return S_OK;
}

static HRESULT WINAPI prop_map_HasKey( __FIMap_2_HSTRING_IInspectable *iface, HSTRING key, boolean *found )
{
    *found = FALSE;
    return S_OK;
}

static HRESULT WINAPI prop_map_GetView( __FIMap_2_HSTRING_IInspectable *iface,
                                         __FIMapView_2_HSTRING_IInspectable **view )
{
    FIXME( "stub!\n" );
    *view = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI prop_map_Insert( __FIMap_2_HSTRING_IInspectable *iface,
                                        HSTRING key, IInspectable *value, boolean *replaced )
{
    FIXME( "key %s stub!\n", debugstr_hstring( key ) );
    if (replaced) *replaced = FALSE;
    return S_OK;
}

static HRESULT WINAPI prop_map_Remove( __FIMap_2_HSTRING_IInspectable *iface, HSTRING key )
{
    FIXME( "key %s stub!\n", debugstr_hstring( key ) );
    return S_OK;
}

static HRESULT WINAPI prop_map_Clear( __FIMap_2_HSTRING_IInspectable *iface )
{
    return S_OK;
}

static const struct __FIMap_2_HSTRING_IInspectableVtbl prop_map_vtbl =
{
    prop_map_QueryInterface,
    prop_map_AddRef,
    prop_map_Release,
    prop_map_GetIids,
    prop_map_GetRuntimeClassName,
    prop_map_GetTrustLevel,
    prop_map_Lookup,
    prop_map_get_Size,
    prop_map_HasKey,
    prop_map_GetView,
    prop_map_Insert,
    prop_map_Remove,
    prop_map_Clear,
};

/* --- IObservableMap<HSTRING, IInspectable*> --- */

static HRESULT WINAPI prop_obsmap_QueryInterface( __FIObservableMap_2_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    return prop_set_QueryInterface( &property_set_instance.IPropertySet_iface, iid, out );
}

static ULONG WINAPI prop_obsmap_AddRef( __FIObservableMap_2_HSTRING_IInspectable *iface )
{
    return prop_set_AddRef( &property_set_instance.IPropertySet_iface );
}

static ULONG WINAPI prop_obsmap_Release( __FIObservableMap_2_HSTRING_IInspectable *iface )
{
    return prop_set_Release( &property_set_instance.IPropertySet_iface );
}

static HRESULT WINAPI prop_obsmap_GetIids( __FIObservableMap_2_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    return prop_set_GetIids( &property_set_instance.IPropertySet_iface, iid_count, iids );
}

static HRESULT WINAPI prop_obsmap_GetRuntimeClassName( __FIObservableMap_2_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    return prop_set_GetRuntimeClassName( &property_set_instance.IPropertySet_iface, class_name );
}

static HRESULT WINAPI prop_obsmap_GetTrustLevel( __FIObservableMap_2_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    return prop_set_GetTrustLevel( &property_set_instance.IPropertySet_iface, trust_level );
}

static HRESULT WINAPI prop_obsmap_add_MapChanged( __FIObservableMap_2_HSTRING_IInspectable *iface,
        __FIMapChangedEventHandler_2_HSTRING_IInspectable *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    if (token) token->value = 100;
    return S_OK;
}

static HRESULT WINAPI prop_obsmap_remove_MapChanged( __FIObservableMap_2_HSTRING_IInspectable *iface,
        EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static const struct __FIObservableMap_2_HSTRING_IInspectableVtbl prop_obsmap_vtbl =
{
    prop_obsmap_QueryInterface,
    prop_obsmap_AddRef,
    prop_obsmap_Release,
    prop_obsmap_GetIids,
    prop_obsmap_GetRuntimeClassName,
    prop_obsmap_GetTrustLevel,
    prop_obsmap_add_MapChanged,
    prop_obsmap_remove_MapChanged,
};

/* --- IIterable<IKeyValuePair<HSTRING, IInspectable*>> --- */

static HRESULT WINAPI prop_iter_QueryInterface( __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable *iface, REFIID iid, void **out )
{
    return prop_set_QueryInterface( &property_set_instance.IPropertySet_iface, iid, out );
}

static ULONG WINAPI prop_iter_AddRef( __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable *iface )
{
    return prop_set_AddRef( &property_set_instance.IPropertySet_iface );
}

static ULONG WINAPI prop_iter_Release( __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable *iface )
{
    return prop_set_Release( &property_set_instance.IPropertySet_iface );
}

static HRESULT WINAPI prop_iter_GetIids( __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable *iface, ULONG *iid_count, IID **iids )
{
    return prop_set_GetIids( &property_set_instance.IPropertySet_iface, iid_count, iids );
}

static HRESULT WINAPI prop_iter_GetRuntimeClassName( __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable *iface, HSTRING *class_name )
{
    return prop_set_GetRuntimeClassName( &property_set_instance.IPropertySet_iface, class_name );
}

static HRESULT WINAPI prop_iter_GetTrustLevel( __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable *iface, TrustLevel *trust_level )
{
    return prop_set_GetTrustLevel( &property_set_instance.IPropertySet_iface, trust_level );
}

static HRESULT WINAPI prop_iter_First( __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable *iface,
                                        __FIIterator_1___FIKeyValuePair_2_HSTRING_IInspectable **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = NULL;
    return E_NOTIMPL;
}

static const struct __FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectableVtbl prop_iter_vtbl =
{
    prop_iter_QueryInterface,
    prop_iter_AddRef,
    prop_iter_Release,
    prop_iter_GetIids,
    prop_iter_GetRuntimeClassName,
    prop_iter_GetTrustLevel,
    prop_iter_First,
};

static struct property_set property_set_instance =
{
    {&prop_set_vtbl},
    {&prop_map_vtbl},
    {&prop_obsmap_vtbl},
    {&prop_iter_vtbl},
    1,
};

/* ========================================================================
 * CoreApplication statics factory
 * ======================================================================== */

struct core_application_statics
{
    IActivationFactory IActivationFactory_iface;
    ICoreApplication ICoreApplication_iface;
    ICoreApplication2 ICoreApplication2_iface;
    ICoreApplicationExit ICoreApplicationExit_iface;
    LONG ref;
};

static inline struct core_application_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct core_application_statics, IActivationFactory_iface );
}

static HRESULT WINAPI activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct core_application_statics *impl = impl_from_IActivationFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ICoreApplication ))
    {
        *out = &impl->ICoreApplication_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ICoreApplication2 ) ||
        IsEqualGUID( iid, &IID_ICoreApplication2_GDK ))
    {
        *out = &impl->ICoreApplication2_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_ICoreApplicationExit ))
    {
        *out = &impl->ICoreApplicationExit_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef( IActivationFactory *iface )
{
    struct core_application_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI activation_factory_Release( IActivationFactory *iface )
{
    struct core_application_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

/* --- ICoreApplication --- */

DEFINE_IINSPECTABLE_( core_app, ICoreApplication, struct core_application_statics,
                       impl_from_ICoreApplication, ICoreApplication_iface, &impl->IActivationFactory_iface )

static HRESULT WINAPI core_app_get_Id( ICoreApplication *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return WindowsCreateString( L"App", 3, value );
}

static HRESULT WINAPI core_app_add_Suspending( ICoreApplication *iface,
        __FIEventHandler_1_Windows__CApplicationModel__CSuspendingEventArgs *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    if (token) token->value = 1;
    return S_OK;
}

static HRESULT WINAPI core_app_remove_Suspending( ICoreApplication *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI core_app_add_Resuming( ICoreApplication *iface,
        __FIEventHandler_1_IInspectable *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    if (token) token->value = 2;
    return S_OK;
}

static HRESULT WINAPI core_app_remove_Resuming( ICoreApplication *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI core_app_get_Properties( ICoreApplication *iface, IPropertySet **value )
{
    TRACE( "iface %p, value %p.\n", iface, value );
    *value = &property_set_instance.IPropertySet_iface;
    IPropertySet_AddRef( *value );
    return S_OK;
}

static HRESULT WINAPI core_app_GetCurrentView( ICoreApplication *iface, ICoreApplicationView **value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI core_app_Run( ICoreApplication *iface, IFrameworkViewSource *view_source )
{
    FIXME( "iface %p, view_source %p stub!\n", iface, view_source );
    return E_NOTIMPL;
}

static HRESULT WINAPI core_app_RunWithActivationFactories( ICoreApplication *iface, IGetActivationFactory *factory )
{
    FIXME( "iface %p, factory %p stub!\n", iface, factory );
    return E_NOTIMPL;
}

static const struct ICoreApplicationVtbl core_app_vtbl =
{
    core_app_QueryInterface,
    core_app_AddRef,
    core_app_Release,
    /* IInspectable methods */
    core_app_GetIids,
    core_app_GetRuntimeClassName,
    core_app_GetTrustLevel,
    /* ICoreApplication methods */
    core_app_get_Id,
    core_app_add_Suspending,
    core_app_remove_Suspending,
    core_app_add_Resuming,
    core_app_remove_Resuming,
    core_app_get_Properties,
    core_app_GetCurrentView,
    core_app_Run,
    core_app_RunWithActivationFactories,
};

/* --- ICoreApplication2 --- */

DEFINE_IINSPECTABLE_( core_app2, ICoreApplication2, struct core_application_statics,
                       impl_from_ICoreApplication2, ICoreApplication2_iface, &impl->IActivationFactory_iface )

static HRESULT WINAPI core_app2_add_BackgroundActivated( ICoreApplication2 *iface,
        __FIEventHandler_1_Windows__CApplicationModel__CActivation__CBackgroundActivatedEventArgs *handler,
        EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    if (token) token->value = 10;
    return S_OK;
}

static HRESULT WINAPI core_app2_remove_BackgroundActivated( ICoreApplication2 *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI core_app2_add_LeavingBackground( ICoreApplication2 *iface,
        __FIEventHandler_1_Windows__CApplicationModel__CLeavingBackgroundEventArgs *handler,
        EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    if (token) token->value = 11;
    return S_OK;
}

static HRESULT WINAPI core_app2_remove_LeavingBackground( ICoreApplication2 *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI core_app2_add_EnteredBackground( ICoreApplication2 *iface,
        __FIEventHandler_1_Windows__CApplicationModel__CEnteredBackgroundEventArgs *handler,
        EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    if (token) token->value = 12;
    return S_OK;
}

static HRESULT WINAPI core_app2_remove_EnteredBackground( ICoreApplication2 *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static HRESULT WINAPI core_app2_EnablePrelaunch( ICoreApplication2 *iface, boolean value )
{
    FIXME( "iface %p, value %d stub!\n", iface, value );
    return S_OK;
}

static const struct ICoreApplication2Vtbl core_app2_vtbl =
{
    core_app2_QueryInterface,
    core_app2_AddRef,
    core_app2_Release,
    /* IInspectable methods */
    core_app2_GetIids,
    core_app2_GetRuntimeClassName,
    core_app2_GetTrustLevel,
    /* ICoreApplication2 methods */
    core_app2_add_BackgroundActivated,
    core_app2_remove_BackgroundActivated,
    core_app2_add_LeavingBackground,
    core_app2_remove_LeavingBackground,
    core_app2_add_EnteredBackground,
    core_app2_remove_EnteredBackground,
    core_app2_EnablePrelaunch,
};

/* --- ICoreApplicationExit --- */

DEFINE_IINSPECTABLE_( core_app_exit, ICoreApplicationExit, struct core_application_statics,
                       impl_from_ICoreApplicationExit, ICoreApplicationExit_iface, &impl->IActivationFactory_iface )

static HRESULT WINAPI core_app_exit_Exit( ICoreApplicationExit *iface )
{
    FIXME( "iface %p stub!\n", iface );
    ExitProcess( 0 );
    return S_OK;
}

static HRESULT WINAPI core_app_exit_add_Exiting( ICoreApplicationExit *iface,
        __FIEventHandler_1_IInspectable *handler, EventRegistrationToken *token )
{
    FIXME( "iface %p, handler %p, token %p stub!\n", iface, handler, token );
    if (token) token->value = 3;
    return S_OK;
}

static HRESULT WINAPI core_app_exit_remove_Exiting( ICoreApplicationExit *iface, EventRegistrationToken token )
{
    FIXME( "iface %p, token %#I64x stub!\n", iface, token.value );
    return S_OK;
}

static const struct ICoreApplicationExitVtbl core_app_exit_vtbl =
{
    core_app_exit_QueryInterface,
    core_app_exit_AddRef,
    core_app_exit_Release,
    /* IInspectable methods */
    core_app_exit_GetIids,
    core_app_exit_GetRuntimeClassName,
    core_app_exit_GetTrustLevel,
    /* ICoreApplicationExit methods */
    core_app_exit_Exit,
    core_app_exit_add_Exiting,
    core_app_exit_remove_Exiting,
};

/* --- Static singleton --- */

static struct core_application_statics core_application_statics =
{
    {&activation_factory_vtbl},
    {&core_app_vtbl},
    {&core_app2_vtbl},
    {&core_app_exit_vtbl},
    1,
};

IActivationFactory *core_application_factory = &core_application_statics.IActivationFactory_iface;
