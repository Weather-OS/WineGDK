/* WinRT Windows.Data.Json.JsonValue Implementation
 *
 * Copyright (C) 2024 Mohamad Al-Jaf
 * Copyright (C) 2026 Olivia Ryan
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(web);

struct json_value_statics
{
    IActivationFactory IActivationFactory_iface;
    IJsonValueStatics IJsonValueStatics_iface;
    LONG ref;
};

static inline struct json_value_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct json_value_statics, IActivationFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct json_value_statics *impl = impl_from_IActivationFactory( iface );

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

    if (IsEqualGUID( iid, &IID_IJsonValueStatics ))
    {
        *out = &impl->IJsonValueStatics_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct json_value_statics *impl = impl_from_IActivationFactory( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct json_value_statics *impl = impl_from_IActivationFactory( iface );
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
    FIXME( "iface %p, instance %p stub!\n", iface, instance );
    return E_NOTIMPL;
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

struct json_value
{
    IJsonValue IJsonValue_iface;
    LONG ref;

    JsonValueType json_value_type;
    HSTRING parsed_string;
    double parsed_number;
    boolean parsed_boolean;
    IJsonArray *parsed_array;
    IJsonObject *parsed_object;
};

static inline struct json_value *impl_from_IJsonValue( IJsonValue *iface )
{
    return CONTAINING_RECORD( iface, struct json_value, IJsonValue_iface );
}

static HRESULT WINAPI json_value_QueryInterface( IJsonValue *iface, REFIID iid, void **out )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IJsonValue ))
    {
        *out = &impl->IJsonValue_iface;
        IInspectable_AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI json_value_AddRef( IJsonValue *iface )
{
    struct json_value *impl = impl_from_IJsonValue( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI json_value_Release( IJsonValue *iface )
{
    struct json_value *impl = impl_from_IJsonValue( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        WindowsDeleteString( impl->parsed_string );
        if ( impl->parsed_array ) IJsonArray_Release( impl->parsed_array );
        if ( impl->parsed_object ) IJsonObject_Release( impl->parsed_object );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI json_value_GetIids( IJsonValue *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_GetRuntimeClassName( IJsonValue *iface, HSTRING *class_name )
{
    FIXME( "iface %p, class_name %p stub!\n", iface, class_name );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_GetTrustLevel( IJsonValue *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_get_ValueType( IJsonValue *iface, JsonValueType *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;

    *value = impl->json_value_type;
    return S_OK;
}

static HRESULT WINAPI json_value_Stringify( IJsonValue *iface, HSTRING *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_GetString( IJsonValue *iface, HSTRING *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->json_value_type != JsonValueType_String) return E_ILLEGAL_METHOD_CALL;
    if (!value) return E_POINTER;

    return WindowsDuplicateString( impl->parsed_string, value );
}

static HRESULT WINAPI json_value_GetNumber( IJsonValue *iface, DOUBLE *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->json_value_type != JsonValueType_Number) return E_ILLEGAL_METHOD_CALL;
    if (!value) return E_POINTER;

    *value = impl->parsed_number;
    return S_OK;
}

static HRESULT WINAPI json_value_GetBoolean( IJsonValue *iface, boolean *value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (impl->json_value_type != JsonValueType_Boolean) return E_ILLEGAL_METHOD_CALL;
    if (!value) return E_POINTER;

    *value = impl->parsed_boolean;
    return S_OK;
}

static HRESULT WINAPI json_value_GetArray( IJsonValue *iface, IJsonArray **value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;
    if (impl->json_value_type != JsonValueType_Array) return E_ILLEGAL_METHOD_CALL;

    IJsonArray_AddRef( impl->parsed_array );
    *value = impl->parsed_array;
    return S_OK;
}

static HRESULT WINAPI json_value_GetObject( IJsonValue *iface, IJsonObject **value )
{
    struct json_value *impl = impl_from_IJsonValue( iface );

    TRACE( "iface %p, value %p\n", iface, value );

    if (!value) return E_POINTER;
    if (impl->json_value_type != JsonValueType_Object) return E_ILLEGAL_METHOD_CALL;

    IJsonObject_AddRef( impl->parsed_object );
    *value = impl->parsed_object;
    return S_OK;
}

static const struct IJsonValueVtbl json_value_vtbl =
{
    json_value_QueryInterface,
    json_value_AddRef,
    json_value_Release,
    /* IInspectable methods */
    json_value_GetIids,
    json_value_GetRuntimeClassName,
    json_value_GetTrustLevel,
    /* IJsonValue methods */
    json_value_get_ValueType,
    json_value_Stringify,
    json_value_GetString,
    json_value_GetNumber,
    json_value_GetBoolean,
    json_value_GetArray,
    json_value_GetObject,
};

DEFINE_IINSPECTABLE( json_value_statics, IJsonValueStatics, struct json_value_statics, IActivationFactory_iface )

static void trim_string( const WCHAR **input, UINT32 *len )
{
    static const WCHAR valid_whitespace[] = L" \t\n\r";
    UINT32 end = *len, start = 0;

    while (start < end && wcschr( valid_whitespace, (*input)[start] )) start++;
    while (end > start && wcschr( valid_whitespace, (*input)[end - 1] )) end--;

    *len = end - start;
    *input += start;
}

static HRESULT parse_json_value( const WCHAR **json, UINT32 *len, struct json_value *impl );

static HRESULT parse_json_array( const WCHAR **json, UINT32 *len, struct json_value *impl )
{
    struct json_value *child;
    IJsonArray *array;
    HRESULT hr;

    if (FAILED(hr = IActivationFactory_ActivateInstance(
        json_array_factory, (IInspectable**)&array ))) return hr;

    (*json)++;
    (*len)--;

    trim_string( json, len );
    while (len && **json != ']')
    {
        if (!(child = calloc( 1, sizeof( *child ) )))
        {
            IJsonArray_Release( array );
            return E_OUTOFMEMORY;
        }

        child->IJsonValue_iface.lpVtbl = &json_value_vtbl;
        child->ref = 1;

        if (FAILED(hr = parse_json_value( json, len, child )))
        {
            IJsonValue_Release( &child->IJsonValue_iface );
            IJsonArray_Release( array );
            return hr;
        }

        trim_string( json, len );

        if (**json == ',')
        {
            (*json)++;
            (*len)--;
        }
        else if (**json != ']') return WEB_E_INVALID_JSON_STRING;

        trim_string( json, len );
    }

    (*json)++;
    (*len)--;

    impl->parsed_array = array;
    impl->json_value_type = JsonValueType_Array;
    return S_OK;
}

static HRESULT parse_json_string( const WCHAR **json, UINT32 *len, HSTRING *output )
{
    const WCHAR valid_hex_chars[] = L"abcdefABCDEF0123456789";
    const WCHAR valid_escape_chars[] = L"\"\\/bfnrtu";
    UINT32 string_len = 0;
    HSTRING_BUFFER buf;
    UINT32 offset = 0;
    HRESULT hr = S_OK;
    WCHAR *dst;

    (*json)++;
    (*len)--;

    /* validate string */

    while (offset < *len)
    {
        if ((*json)[offset] < 32) return WEB_E_INVALID_JSON_STRING;

        if ((*json)[offset] == '\\')
        {
            if (*len - offset < 3 ||
                !wcschr( valid_escape_chars, (*json)[offset + 1]) ||
                ( (*json)[offset + 1] == 'u' && (
                 !wcschr( valid_hex_chars, (*json)[offset + 2]) ||
                 !wcschr( valid_hex_chars, (*json)[offset + 3]) ||
                 !wcschr( valid_hex_chars, (*json)[offset + 4]) ||
                 !wcschr( valid_hex_chars, (*json)[offset + 5]) ) ))
                return WEB_E_INVALID_JSON_STRING;

            string_len++;
            if ((*json)[offset + 1] == 'u') offset += 6;
            else offset += 2;
        }
        else if ((*json)[offset] == '"') break;
        else
        {
            string_len++;
            offset++;
        }
    }

    if (offset == *len) return WEB_E_INVALID_JSON_STRING;

    /* create & escape string */

    if (FAILED(hr = WindowsPreallocateStringBuffer( string_len, &dst, &buf ))) return hr;

    for (UINT32 i = 0; i < string_len; i++)
    {
        if (**json == '\\')
        {
            (*json)++;
            *len -= 2;
            if (**json == '"') { *dst++ = '"'; (*json)++; }
            else if (**json == '\\') { *dst++ = '\\'; (*json)++; }
            else if (**json == '/') { *dst++ = '/'; (*json)++; }
            else if (**json == 'b') { *dst++ = '\b'; (*json)++; }
            else if (**json == 'f') { *dst++ = '\f'; (*json)++; }
            else if (**json == 'n') { *dst++ = '\n'; (*json)++; }
            else if (**json == 'r') { *dst++ = '\r'; (*json)++; }
            else if (**json == 't') { *dst++ = '\t'; (*json)++; }
            else
            {
                (*json)++;
                *len -= 4;
                if (**json >= 65) *dst = ((*(*json)++ & 0x7) + 10) << 12;
                else *dst = (*(*json)++ & 0xf) << 12;

                if (**json >= 65) *dst |= ((*(*json)++ & 0x7) + 10) << 8;
                else *dst |= (*(*json)++ & 0xf) << 8;

                if (**json >= 65) *dst |= ((*(*json++) & 0x7) + 10) << 4;
                else *dst |= (*(*json)++ & 0xf) << 4;

                if (**json >= 65) *dst++ |= (*(*json)++ & 0x7) + 10;
                else *dst++ |= *(*json)++ & 0xf;
            }
        }
        else
        {
            *dst++ = *(*json)++;
            (*len)--;
        }
    }

    (*json)++;
    (*len)--;

    return WindowsPromoteStringBuffer( buf, output );
}

static HRESULT parse_json_object( const WCHAR **json, UINT32 *len, struct json_value *impl )
{
    struct json_value *child;
    HSTRING name;
    HRESULT hr;

    TRACE( "json %s, impl %p", debugstr_wn( *json, *len ), impl );

    if (FAILED(hr = IActivationFactory_ActivateInstance(
        json_object_factory, (IInspectable**)&impl->parsed_object ))) return hr;

    (*json)++;
    (*len)--;

    trim_string( json, len );
    while (*len && **json != '}')
    {
        if (FAILED(hr = parse_json_string( json, len, &name )))
        {
            IJsonObject_Release( impl->parsed_object );
            return hr;
        }

        trim_string( json, len );
        if (!*len || **json != ':')
        {
            IJsonObject_Release( impl->parsed_object );
            WindowsDeleteString( name );
            return WEB_E_INVALID_JSON_STRING;
        }
        (*json)++;
        (*len)--;
        trim_string( json, len );

        if (!(child = calloc( 1, sizeof( *child ) )))
        {
            IJsonObject_Release( impl->parsed_object );
            WindowsDeleteString( name );
            return E_OUTOFMEMORY;
        }

        child->IJsonValue_iface.lpVtbl = &json_value_vtbl;
        child->ref = 1;

        if (FAILED(hr = parse_json_value( json, len, child )))
        {
            IJsonValue_Release( &child->IJsonValue_iface );
            IJsonObject_Release( impl->parsed_object );
            WindowsDeleteString( name );
            return hr;
        }

        if (FAILED(hr = IJsonObject_SetNamedValue(
            impl->parsed_object, name, &child->IJsonValue_iface )))
        {
            IJsonValue_Release( &child->IJsonValue_iface );
            IJsonObject_Release( impl->parsed_object );
            WindowsDeleteString( name );
            return hr;
        }

        trim_string( json, len );
        if (**json == ',') (*json)++;
        else if (**json != '}') return WEB_E_INVALID_JSON_STRING;
        trim_string( json, len );
    }

    if (!*len) return WEB_E_INVALID_JSON_STRING;
    (*json)++;
    (*len)--;

    impl->json_value_type = JsonValueType_Object;
    return S_OK;
}

static HRESULT parse_json_value( const WCHAR **json, UINT32 *len, struct json_value *impl )
{
    HRESULT hr = S_OK;

    if (!*len) return WEB_E_INVALID_JSON_STRING;

    if (*len >= 4 && !wcsncmp( L"null", *json, 4 ))
    {
        *json += 4;
        *len -= 4;
        impl->json_value_type = JsonValueType_Null;
    }
    else if (*len >= 4 && !wcsncmp( L"true", *json, 4))
    {
        *json += 4;
        *len -= 4;
        impl->parsed_boolean = true;
        impl->json_value_type = JsonValueType_Boolean;
    }
    else if (*len >= 5 && !wcsncmp( L"false", *json, 5 ))
    {
        *json += 5;
        *len -= 5;
        impl->parsed_boolean = false;
        impl->json_value_type = JsonValueType_Boolean;
    }
    else if (**json == '\"')
    {
        if (FAILED(hr = parse_json_string( json, len, &impl->parsed_string ))) return hr;
        impl->json_value_type = JsonValueType_String;
    }
    else if (**json == '[')
    {
        if (FAILED(hr = parse_json_array( json, len, impl ))) return hr;
    }
    else if (**json == '{')
    {
        if (FAILED(hr = parse_json_object( json, len, impl ))) return hr;
    }
    else
    {
        double result = 0;
        WCHAR *end;

        errno = 0;
        result = wcstold( *json, &end );

        *len -= end - *json;
        *json = end;

        if (errno || errno == ERANGE) return WEB_E_INVALID_JSON_NUMBER;

        impl->parsed_number = result;
        impl->json_value_type = JsonValueType_Number;
    }

    return hr;
}

static HRESULT parse_json( HSTRING string, struct json_value *impl )
{
    HRESULT hr;
    UINT32 len;
    const WCHAR *json = WindowsGetStringRawBuffer( string, &len );

    trim_string( &json, &len );
    if (!len) return WEB_E_INVALID_JSON_STRING;
    if (FAILED(hr = parse_json_value( &json, &len, impl ))) return hr;
    if (len) return WEB_E_INVALID_JSON_STRING;
    return S_OK;
}

static HRESULT WINAPI json_value_statics_Parse( IJsonValueStatics *iface, HSTRING input, IJsonValue **value )
{
    struct json_value *impl;
    HRESULT hr;

    TRACE( "iface %p, input %s, value %p\n", iface, debugstr_hstring( input ), value );

    if (!value) return E_POINTER;
    if (!input) return WEB_E_INVALID_JSON_STRING;
    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;

    if (FAILED(hr = parse_json( input, impl )))
    {
        free( impl );
        return hr;
    }
    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;

    *value = &impl->IJsonValue_iface;
    TRACE( "created IJsonValue %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI json_value_statics_TryParse( IJsonValueStatics *iface, HSTRING input, IJsonValue **result, boolean *succeeded )
{
    FIXME( "iface %p, input %s, result %p, succeeded %p stub!\n", iface, debugstr_hstring( input ), result, succeeded );
    return E_NOTIMPL;
}

static HRESULT WINAPI json_value_statics_CreateBooleanValue( IJsonValueStatics *iface, boolean input, IJsonValue **value )
{
    struct json_value *impl;

    TRACE( "iface %p, input %d, value %p\n", iface, input, value );

    if (!value) return E_POINTER;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;
    impl->json_value_type = JsonValueType_Boolean;
    impl->parsed_boolean = input != FALSE;

    *value = &impl->IJsonValue_iface;
    TRACE( "created IJsonValue %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI json_value_statics_CreateNumberValue( IJsonValueStatics *iface, DOUBLE input, IJsonValue **value )
{
    struct json_value *impl;

    TRACE( "iface %p, input %f, value %p\n", iface, input, value );

    if (!value) return E_POINTER;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;
    impl->json_value_type = JsonValueType_Number;
    impl->parsed_number = input;

    *value = &impl->IJsonValue_iface;
    TRACE( "created IJsonValue %p.\n", *value );
    return S_OK;
}

static HRESULT WINAPI json_value_statics_CreateStringValue( IJsonValueStatics *iface, HSTRING input, IJsonValue **value )
{
    struct json_value *impl;
    HRESULT hr;

    TRACE( "iface %p, input %s, value %p\n", iface, debugstr_hstring( input ), value );

    if (!value) return E_POINTER;
    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IJsonValue_iface.lpVtbl = &json_value_vtbl;
    impl->ref = 1;
    impl->json_value_type = JsonValueType_String;
    if (FAILED(hr = WindowsDuplicateString( input, &impl->parsed_string )))
    {
         free( impl );
         return hr;
    }

    *value = &impl->IJsonValue_iface;
    TRACE( "created IJsonValue %p.\n", *value );
    return S_OK;
}

static const struct IJsonValueStaticsVtbl json_value_statics_vtbl =
{
    json_value_statics_QueryInterface,
    json_value_statics_AddRef,
    json_value_statics_Release,
    /* IInspectable methods */
    json_value_statics_GetIids,
    json_value_statics_GetRuntimeClassName,
    json_value_statics_GetTrustLevel,
    /* IJsonValueStatics methods */
    json_value_statics_Parse,
    json_value_statics_TryParse,
    json_value_statics_CreateBooleanValue,
    json_value_statics_CreateNumberValue,
    json_value_statics_CreateStringValue,
};

static struct json_value_statics json_value_statics =
{
    {&factory_vtbl},
    {&json_value_statics_vtbl},
    1,
};

IActivationFactory *json_value_factory = &json_value_statics.IActivationFactory_iface;
