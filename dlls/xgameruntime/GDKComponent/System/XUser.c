/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XUser
 *
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
#include "util.h"
#include <errno.h>
#include <ntdef.h>
#include <time.h>
#include <wincrypt.h>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static const WCHAR *ACCEPT_JSON[] = { L"application/json", NULL };
static const WCHAR CT_JSON[] = L"Content-Type: application/json";
static const WCHAR CT_FORM_URLENCODED[] = L"Content-Type: application/x-www-form-urlencoded";

static const char PROOF_KEY_TEMPLATE[] = "{\"alg\":\"ES256\",\"kty\":\"EC\",\"use\":\"sig\",\"crv\":\"P-256\",\"x\":\"";
static const char PROOF_KEY_TEMPLATE2[] = "\",\"y\":\"";
/* x & y are 43 chars */
#define PROOF_KEY_SIZE ARRAY_SIZE( PROOF_KEY_TEMPLATE ) + ARRAY_SIZE( PROOF_KEY_TEMPLATE2 ) + 86

static HRESULT parse_json( const char *json, SIZE_T jsonLen, IJsonObject **object )
{
    static const WCHAR *name = RuntimeClass_Windows_Data_Json_JsonValue;
    IJsonValueStatics *statics;
    HSTRING_HEADER header;
    IJsonValue *value;
    UINT32 wJsonLen;
    HSTRING string;
    WCHAR *wJson;
    HRESULT hr;

    TRACE( "json %s, object %p.\n", debugstr_an( json, jsonLen ), object );

    if (!(wJsonLen = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, json, jsonLen, NULL, 0 ))) return HRESULT_FROM_WIN32( GetLastError() );
    if (FAILED(hr = WindowsCreateStringReference( name, wcslen( name ), &header, &string ))) return hr;
    if (FAILED(hr = RoGetActivationFactory( string, &IID_IJsonValueStatics, (void **)&statics ))) return hr;
    if (!(wJson = calloc( wJsonLen + 1, sizeof(WCHAR) )))
    {
        IJsonValueStatics_Release( statics );
        return E_OUTOFMEMORY;
    }

    if (!MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, json, jsonLen, wJson, wJsonLen + 1 )) goto error;
    if (FAILED(hr = WindowsCreateStringReference( wJson, wJsonLen + 1, &header, &string ))) goto cleanup;
    if (FAILED(hr = IJsonValueStatics_Parse( statics, string, &value ))) goto cleanup;
    hr = IJsonValue_GetObject( value, object );
    IJsonValue_Release( value );
    goto cleanup;

error:
    hr = HRESULT_FROM_WIN32( GetLastError() );
cleanup:
    IJsonValueStatics_Release( statics );
    free( wJson );
    return hr;
}

struct XUser
{
    IUser IUser_iface;
    LONG ref;

    UINT64 xuid;
    HSTRING userHash;

    DOUBLE interval;
    time_t oauth_expiry;
    HSTRING deviceCode;
    HSTRING accessToken;
    HSTRING refreshToken;
    HSTRING userToken;
    HSTRING xstsToken;

    BCRYPT_KEY_HANDLE key;
    char proofKey[PROOF_KEY_SIZE];
};

static struct XUser *impl_from_IUser( IUser *iface )
{
    return CONTAINING_RECORD( iface, struct XUser, IUser_iface );
}

static ULONG WINAPI user_AddRef( IUser *iface )
{
    struct XUser *impl = impl_from_IUser( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI user_Release( IUser *iface )
{
    struct XUser *impl = impl_from_IUser( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    if (!ref)
    {
        if (impl->userHash) WindowsDeleteString( impl->userHash );
        if (impl->deviceCode) WindowsDeleteString( impl->deviceCode );
        if (impl->accessToken) WindowsDeleteString( impl->accessToken );
        if (impl->refreshToken) WindowsDeleteString( impl->refreshToken );
        if (impl->userToken) WindowsDeleteString( impl->userToken );
        if (impl->xstsToken) WindowsDeleteString( impl->xstsToken );
        if (impl->key) BCryptDestroyKey( impl->key );
        free( impl );
    }
    return ref;
}

static HRESULT WINAPI user_RequestOAuthCode( IUser *iface, HSTRING *user, HSTRING *uri )
{
    static const char template[] = "scope=service::user.auth.xboxlive.com::MBI_SSL&response_type=device_code&client_id=";
    struct XUser *impl = impl_from_IUser( iface );
    char *buffer = NULL, *data = NULL;
    IJsonObject *object = NULL;
    SIZE_T size = 0;
    HRESULT hr;

    TRACE( "iface %p, user %p, uri %p.\n", iface, user, uri );

    if (!(data = calloc( 1, ARRAY_SIZE( template ) + strlen( msaAppId ) )))
    {
        return E_OUTOFMEMORY;
    }
    strcpy( data, template );
    strcat( data, msaAppId );

    if (FAILED(hr = http_request( L"POST", L"login.live.com", L"/oauth20_connect.srf", data, CT_FORM_URLENCODED, ACCEPT_JSON, (UCHAR **)&buffer, &size )))
        goto cleanup;
    if (FAILED(hr = parse_json( buffer, size, &object ))) goto cleanup;
    if (FAILED(hr = get_json_string( object, L"device_code", &impl->deviceCode ))) goto cleanup;
    if (FAILED(hr = get_json_string( object, L"verification_uri", uri ))) goto cleanup;
    if (FAILED(hr = get_json_number( object, L"interval", &impl->interval ))) goto cleanup;
    hr = get_json_string( object, L"user_code", user );

cleanup:
    if (data) free( data );
    if (buffer) free( buffer );
    IJsonObject_Release( object );
    if (SUCCEEDED(hr)) return hr;
    if (*uri) WindowsDeleteString( *uri );
    if (*user) WindowsDeleteString( *user );
    if (impl->deviceCode) WindowsDeleteString( impl->deviceCode );
    return hr;
}

static HRESULT WINAPI user_RequestOAuthToken( IUser *iface )
{
    static const char template[] = "grant_type=device_code&device_code=";
    char *buffer = NULL, *data = NULL, *deviceCode = NULL;
    struct XUser *impl = impl_from_IUser( iface );
    UINT32 deviceCodeLen = 0, wDeviceCodeLen;
    HSTRING access = NULL, refresh = NULL;
    IJsonObject *object = NULL;
    const WCHAR *wDeviceCode;
    DOUBLE delta = 0;
    SIZE_T size = 0;
    time_t expiry;
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    wDeviceCode = WindowsGetStringRawBuffer( impl->deviceCode, &wDeviceCodeLen );
    if (!(deviceCodeLen = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wDeviceCode, wDeviceCodeLen, NULL, 0, NULL, NULL ))) goto error;
    if (!(data = calloc( 1, ARRAY_SIZE( template ) + deviceCodeLen + strlen( "&client_id=" ) + strlen( msaAppId ) )))
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    strcpy( data, template );
    if (!WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wDeviceCode, wDeviceCodeLen, data + ARRAY_SIZE( template ) - 1, deviceCodeLen, NULL, NULL )) goto error;
    strcat( data, "&client_id=" );
    strcat( data, msaAppId );

    while (TRUE)
    {
        if (SUCCEEDED(hr = http_request( L"POST", L"login.live.com", L"/oauth20_token.srf", data, CT_FORM_URLENCODED, ACCEPT_JSON, (UCHAR **)&buffer, &size ))) break;
        Sleep( impl->interval * 1000 );
    }

    if (FAILED(hr = parse_json( buffer, size, &object ))) goto cleanup;
    if (FAILED(hr = get_json_string( object, L"refresh_token", &refresh ))) goto cleanup;
    if (FAILED(hr = get_json_string( object, L"access_token", &access ))) goto cleanup;
    if (FAILED(hr = get_json_number( object, L"expires_in", &delta ))) goto cleanup;

    if ((expiry = time(NULL)) == -1) hr = E_FAIL;
    else impl->oauth_expiry = expiry + delta;
    goto cleanup;

error:
    hr = HRESULT_FROM_WIN32( GetLastError() );
cleanup:
    if (data) free( data );
    if (buffer) free( buffer );
    if (deviceCode) free( deviceCode );
    if (object) IJsonObject_Release( object );
    if (SUCCEEDED(hr))
    {
        if (impl->refreshToken) WindowsDeleteString( impl->refreshToken );
        if (impl->accessToken) WindowsDeleteString( impl->accessToken );
        impl->refreshToken = refresh;
        impl->accessToken = access;
        return hr;
    }
    if (refresh) WindowsDeleteString( refresh );
    if (access) WindowsDeleteString( access );
    return hr;
}

static HRESULT WINAPI user_RefreshOAuthToken( IUser *iface )
{
    static const char template[] = "grant_type=refresh_token&scope=service::user.auth.xboxlive.com::MBI_SSL&client_id=";
    struct XUser *impl = impl_from_IUser( iface );
    HSTRING newAccess = NULL, newRefresh = NULL;
    UINT32 refreshTokenLen, wRefreshTokenLen;
    char *buffer = NULL, *data = NULL;
    const WCHAR *wRefreshToken;
    IJsonObject *object = NULL;
    time_t expiry;
    DOUBLE delta;
    SIZE_T size;
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    wRefreshToken = WindowsGetStringRawBuffer( impl->refreshToken, &wRefreshTokenLen );
    if (!(refreshTokenLen = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wRefreshToken, wRefreshTokenLen, NULL, 0, NULL, NULL ))) goto error;
    if (!(data = calloc( 1, ARRAY_SIZE( template ) + strlen( msaAppId ) + strlen( "&refresh_token=" ) + refreshTokenLen )))
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    strcpy( data, template );
    strcat( data, msaAppId );
    strcat( data, "&refresh_token=" );
    if (!WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wRefreshToken, wRefreshTokenLen, data + strlen( data ), refreshTokenLen, NULL, NULL )) goto error;
    if (FAILED(hr = http_request( L"POST", L"login.live.com", L"/oauth20_token.srf", data, CT_FORM_URLENCODED, ACCEPT_JSON, (UCHAR **)&buffer, &size ))) goto cleanup;
    if (FAILED(hr = parse_json( buffer, size, &object ))) goto cleanup;
    if (FAILED(hr = get_json_string( object, L"refresh_token", &newRefresh ))) goto cleanup;
    if (FAILED(hr = get_json_string( object, L"access_token", &newAccess ))) goto cleanup;
    if (FAILED(hr = get_json_number( object, L"expires_in", &delta ))) goto cleanup;
    if ((expiry = time(NULL)) == -1) hr = E_FAIL;
    else impl->oauth_expiry = expiry + delta;
    goto cleanup;

error:
    hr = HRESULT_FROM_WIN32( GetLastError() );
cleanup:
    if (data) free( data );
    if (buffer) free( buffer );
    if (object) IJsonObject_Release( object );
    if (SUCCEEDED(hr))
    {
        impl->refreshToken = newRefresh;
        impl->accessToken = newAccess;
        return hr;
    }
    if (newRefresh) WindowsDeleteString( newRefresh );
    if (newAccess) WindowsDeleteString( newAccess );
    return hr;
}

static HRESULT WINAPI user_RequestUserToken( IUser *iface )
{
    const char *template = "{\"TokenType\":\"JWT\",\"RelyingParty\":\"http://auth.xboxlive.com\",\"Properties\":{\"AuthMethod\":\"RPS\",\"SiteName\":\"user.auth.xboxlive.com\",\"RpsTicket\":\"";
    struct XUser *impl = impl_from_IUser( iface );
    UINT32 tokenLen, wTokenLen;
    IJsonObject *object = NULL;
    const WCHAR *wToken;
    UCHAR *buf = NULL;
    char *body = NULL;
    SIZE_T bufSize;
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    wToken = WindowsGetStringRawBuffer( impl->accessToken, &wTokenLen );
    if (!(tokenLen = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wToken, wTokenLen, NULL, 0, NULL, NULL ))) goto error;
    if (!(body = calloc( strlen( template ) + tokenLen + strlen( "\"}}" ), sizeof(char) )))
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    /* construct request body */
    strcpy( body, template );
    if (!WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wToken, wTokenLen, body + strlen( template ), tokenLen, NULL, NULL )) goto error;
    strcat( body, "\"}}" );

    if (FAILED(hr = http_request( L"POST", L"user.auth.xboxlive.com", L"/user/authenticate", body, CT_JSON, ACCEPT_JSON, &buf, &bufSize ))) goto cleanup;
    if (FAILED(hr = parse_json( (char *)buf, bufSize, &object ))) goto cleanup;
    hr = get_json_string( object, L"Token", &impl->userToken );
    goto cleanup;

error:
    hr = HRESULT_FROM_WIN32( GetLastError() );
cleanup:
    if (buf) free( buf );
    if (body) free( body );
    if (object) IJsonObject_Release( object );
    return hr;
}

static HRESULT WINAPI user_RequestXstsToken( IUser *iface )
{
    static const char template[] = "{\"TokenType\":\"JWT\",\"RelyingParty\":\"http://xboxlive.com\",\"Properties\":{\"SandboxId\":\"RETAIL\",\"ProofKey\":";
    IJsonObject *child = NULL, *claims = NULL, *object = NULL;
    struct XUser *impl = impl_from_IUser( iface );
    UINT32 tokenLen, wTokenLen;
    char *body = NULL, *token;
    IJsonArray *array = NULL;
    UCHAR *buffer = NULL;
    HSTRING xuid = NULL;
    const WCHAR *wToken;
    SIZE_T bufferSize;
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    wToken = WindowsGetStringRawBuffer( impl->userToken, &wTokenLen );
    if (!(tokenLen = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wToken, wTokenLen, NULL, 0, NULL, NULL ))) goto error;
    if (!(body = calloc( 1, ARRAY_SIZE( template ) + PROOF_KEY_SIZE + strlen( ",\"UserTokens\":[\"\"]}}" ) + tokenLen )))
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    /* 32 bytes -> 43 base64url chars without padding */
    token = body + ARRAY_SIZE( template ) + PROOF_KEY_SIZE + strlen( ",\"UserTokens\":[\"" ) - 1;

    /* construct request body */
    strcpy( body, template );
    strncat( body, impl->proofKey, PROOF_KEY_SIZE );
    strcat( body, ",\"UserTokens\":[\"" );
    if (!WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, wToken, wTokenLen, token, tokenLen, NULL, NULL )) goto error;
    strcat( body, "\"]}}" );

    if (FAILED(hr = http_request( L"POST", L"xsts.auth.xboxlive.com", L"/xsts/authorize", body, CT_JSON, ACCEPT_JSON, &buffer, &bufferSize ))) goto cleanup;
    if (FAILED(hr = parse_json( (char *)buffer, bufferSize, &object ))) goto cleanup;
    if (FAILED(hr = get_json_string( object, L"Token", &impl->xstsToken ))) goto cleanup;
    if (FAILED(hr = get_json_object( object, L"DisplayClaims", &claims ))) goto cleanup;
    if (FAILED(hr = get_json_array( claims, L"xui", &array ))) goto cleanup;
    if (FAILED(hr = IJsonArray_GetObjectAt( array, 0, &child ))) goto cleanup;
    if (FAILED(hr = get_json_string( child, L"uhs", &impl->userHash ))) goto cleanup;
    if (FAILED(hr = get_json_string( child, L"xid", &xuid ))) goto cleanup;
    impl->xuid = wcstoull( WindowsGetStringRawBuffer( xuid, NULL ), NULL, 10 );
    if (errno == ERANGE) hr = E_UNEXPECTED;
    goto cleanup;

error:
    hr = HRESULT_FROM_WIN32( GetLastError() );
cleanup:
    if (body) free( body );
    if (buffer) free( buffer );
    if (xuid) WindowsDeleteString( xuid );
    if (array) IJsonArray_Release( array );
    if (child) IJsonObject_Release( child );
    if (claims) IJsonObject_Release( claims );
    if (object) IJsonObject_Release( object );
    return hr;
}

static HRESULT WINAPI user_GenerateKeyPair( IUser *iface )
{
    char proofKey[PROOF_KEY_SIZE + 1] = {}, *x, *y;
    struct XUser *impl = impl_from_IUser( iface );
    UCHAR blob[sizeof(BCRYPT_ECCKEY_BLOB) + 64];
    BCRYPT_ALG_HANDLE ecdsa = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    NTSTATUS status;
    ULONG dummy;
    HRESULT hr;

    TRACE( "iface %p.\n", iface );

    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider( &ecdsa, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0 ))) goto error;
    if (!NT_SUCCESS(status = BCryptGenerateKeyPair( ecdsa, &key, 256, 0 ))) goto error;
    if (!NT_SUCCESS(status = BCryptFinalizeKeyPair( key, 0 ))) goto error;
    if (!NT_SUCCESS(status = BCryptExportKey( key, NULL, BCRYPT_ECCPUBLIC_BLOB, blob, sizeof(blob), &dummy, 0 ))) goto error;

    /* convert proof key to jwk format */
    memcpy( proofKey, PROOF_KEY_TEMPLATE, ARRAY_SIZE( PROOF_KEY_TEMPLATE ) );
    x = proofKey + ARRAY_SIZE( PROOF_KEY_TEMPLATE ) - 1;
    y = x + 43 + ARRAY_SIZE( PROOF_KEY_TEMPLATE2 ) - 1;
    if (FAILED(hr = encode_base64_url( 32, blob + sizeof(BCRYPT_ECCKEY_BLOB), 43, x, FALSE ))) goto cleanup;
    strcat( proofKey, PROOF_KEY_TEMPLATE2 );
    if (FAILED(hr = encode_base64_url( 32, blob + sizeof(BCRYPT_ECCKEY_BLOB) + 32, 43, y, FALSE ))) goto cleanup;
    strcat( proofKey, "\"}" );
    goto cleanup;

error:
    hr = HRESULT_FROM_NT( status );
cleanup:
    if (ecdsa) BCryptCloseAlgorithmProvider( ecdsa, 0 );
    if (FAILED(hr)) BCryptDestroyKey( key );
    else
    {
        memcpy( impl->proofKey, proofKey, PROOF_KEY_SIZE ); /* ignore null terminator */
        if (impl->key) BCryptDestroyKey( impl->key );
        impl->key = key;
    }
    return hr;
}

static HRESULT WINAPI user_SignData( IUser *iface, ULONG dataSize, UCHAR *data, ULONG signatureSize, UCHAR *signature )
{
    struct XUser *impl = impl_from_IUser( iface );
    BCRYPT_ALG_HANDLE algorithm = NULL;
    BCRYPT_HASH_HANDLE object = NULL;
    HRESULT hr = S_OK;
    NTSTATUS status;
    UCHAR hash[32];
    ULONG dummy;

    TRACE( "iface %p, dataSize %lu, data %p, signatureSize %lu, signature %p.\n", iface, dataSize, data, signatureSize, signature );

    /* ES256 signs sha-256 hash of data */
    if (signatureSize < 64) return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider( &algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0 ))) goto error;
    if (!NT_SUCCESS(status = BCryptCreateHash( algorithm, &object, NULL, 0, NULL, 0, 0 ))) goto error;
    if (!NT_SUCCESS(status = BCryptHashData( object, data, dataSize, 0 ))) goto error;
    if (!NT_SUCCESS(status = BCryptFinishHash( object, hash, 32, 0 ))) goto error;
    if (!NT_SUCCESS(status = BCryptSignHash( impl->key, NULL, hash, 32, signature, signatureSize, &dummy, 0 ))) goto error;
    goto cleanup;

error:
    hr = HRESULT_FROM_NT( status );
cleanup:
    if (object) BCryptDestroyHash( object );
    if (algorithm) BCryptCloseAlgorithmProvider( algorithm, 0 );
    return hr;
}

static HRESULT WINAPI user_CacheEndpoints( IUser *iface )
{
    TRACE( "iface %p.\n", iface );
    return E_NOTIMPL;
}

static const struct IUserVtbl user_vtbl =
{
    NULL,
    user_AddRef,
    user_Release,
    /* IUser methods */
    user_RequestOAuthCode,
    user_RequestOAuthToken,
    user_RefreshOAuthToken,
    user_RequestUserToken,
    user_RequestXstsToken,
    user_GenerateKeyPair,
    user_SignData,
    user_CacheEndpoints,
};

static HRESULT LoadMsaUser( XUserHandle *user )
{
    XUserHandle impl;

    TRACE( "user %p.\n", user );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IUser_iface.lpVtbl = &user_vtbl;
    impl->ref = 1;

    *user = impl;
    return S_OK;
}

static HRESULT LoadDefaultUser( XUserHandle *user )
{
    XUserHandle impl;

    TRACE( "user %p.\n", user );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IUser_iface.lpVtbl = &user_vtbl;
    impl->ref = 1;

    *user = impl;
    return S_OK;
}

struct x_user
{
    IXUserImpl6 IXUserImpl_iface;
    IXUserGamertagImpl IXUserGamertagImpl_iface;
    LONG ref;
};

static inline struct x_user *impl_from_IXUserImpl( IXUserImpl6 *iface )
{
    return CONTAINING_RECORD( iface, struct x_user, IXUserImpl_iface );
}

static HRESULT WINAPI x_user_QueryInterface( IXUserImpl6 *iface, REFIID iid, void **out )
{
    struct x_user *impl = impl_from_IXUserImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown    ) ||
        IsEqualGUID( iid, &IID_IXUserImpl  ) ||
        IsEqualGUID( iid, &IID_IXUserImpl2 ) ||
        IsEqualGUID( iid, &IID_IXUserImpl3 ) ||
        IsEqualGUID( iid, &IID_IXUserImpl4 ) ||
        IsEqualGUID( iid, &IID_IXUserImpl5 ) ||
        IsEqualGUID( iid, &IID_IXUserImpl6 ))
    {
        IXUserImpl6_AddRef( *out = &impl->IXUserImpl_iface );
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_IXUserGamertagImpl ))
    {
        IXUserGamertagImpl_AddRef( *out = &impl->IXUserGamertagImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_user_AddRef( IXUserImpl6 *iface )
{
    struct x_user *impl = impl_from_IXUserImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_user_Release( IXUserImpl6 *iface )
{
    struct x_user *impl = impl_from_IXUserImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_user_XUserDuplicateHandle( IXUserImpl6 *iface, XUserHandle handle, XUserHandle *duplicatedHandle )
{
    TRACE( "iface %p, handle %p, duplicatedHandle %p.\n", iface, handle, duplicatedHandle );
    IUser_AddRef( &handle->IUser_iface );
    *duplicatedHandle = handle;
    return S_OK;
}

static void WINAPI x_user_XUserCloseHandle( IXUserImpl6 *iface, XUserHandle user )
{
    TRACE( "iface %p, user %p.\n", iface, user );
    if (!user) return;
    IUser_Release( &user->IUser_iface );
}

static INT32 WINAPI x_user_XUserCompare( IXUserImpl6 *iface, XUserHandle user1, XUserHandle user2 )
{
    FIXME( "iface %p, user1 %p, user2 %p stub!\n", iface, user1, user2 );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetMaxUsers( IXUserImpl6 *iface, UINT32 *maxUsers )
{
    TRACE( "iface %p, maxUsers %p.\n", iface, maxUsers );
    *maxUsers = 1;
    return S_OK;
}

struct XUserAddContext
{
    XUserAddOptions options;
    XUserHandle user;
};

#if XODUS_INTEROP
DEFINE_ASYNC_COMPLETED_HANDLER( async_operation_msa_token_response, IAsyncOperationCompletedHandler_IMsaTokenResponse, IAsyncOperation_IMsaTokenResponse )
#endif

static HRESULT WINAPI XUserAddProvider( XAsyncOp op, const XAsyncProviderData *data )
{
    struct XUserAddContext *context;
    IXThreadingImpl *xthreading;
    HRESULT hr;

    TRACE( "op %d, data %p.\n", op, data );

    if (FAILED(hr = QueryApiImpl( &CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void **)&xthreading ))) return hr;
    context = (struct XUserAddContext *)data->context;

    switch (op)
    {
        case XAsyncOp_Begin:
            hr = IXThreadingImpl_XAsyncSchedule( xthreading, data->async, 0 );
            break;

        case XAsyncOp_GetResult:
            memcpy( data->buffer, &context->user, sizeof(XUserHandle) );
            break;

        case XAsyncOp_DoWork:
#if XODUS_INTEROP
            {
                IAsyncOperation_IMsaTokenResponse *operation;
                DWORD async;

                if (context->options & XUserAddOptions_AddDefaultUserSilently ||
                    context->options & XUserAddOptions_AddDefaultUserAllowingUI)
                {
                    if (FAILED(hr = IXodusService_MsaTokenRequest(
                        xodus_service, msaAppId, context->options & XUserAddOptions_AddDefaultUserAllowingUI, fullTrust, &operation
                    ))) goto fallback;
                    if ((async = await_IAsyncOperation_IMsaTokenResponse( operation, IPC_REQUEST_TIMEOUT_MS )))
                    {
                        if (async == STATUS_TIMEOUT)
                            WARN( "Timeout while waiting for MSA_TOKEN_RESPONSE response.\n" );
                        else
                            WARN( "Async action await failed. Status was %ld.\n", async );
                        hr = E_ABORT;
                    }
                    else hr = LoadMsaUser( &context->user );
                }
                else hr = E_ABORT;
                goto complete;

            fallback:
                WARN( "failed to request msa token from xodus.\n" );
            }
#endif
            if (context->options & XUserAddOptions_AddDefaultUserSilently)
                hr = LoadDefaultUser( &context->user );
            else hr = E_ABORT;

        complete:
            IXThreadingImpl_XAsyncComplete( xthreading, data->async, hr, SUCCEEDED(hr) ? sizeof(XUserHandle) : 0 );
            hr = S_OK;
            break;

        case XAsyncOp_Cleanup:
            free( context );
            break;

        case XAsyncOp_Cancel:
            break;
    }

    IXThreadingImpl_Release( xthreading );
    return hr;
}

static HRESULT WINAPI x_user_XUserAddAsync( IXUserImpl6 *iface, XUserAddOptions options, XAsyncBlock *async )
{
    struct XUserAddContext *context;
    IXThreadingImpl *xthreading;
    HRESULT hr;

    TRACE( "iface %p, options %d, async %p.\n", iface, options, async );

    if (!async) return E_POINTER;
    if (FAILED(hr = QueryApiImpl( &CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void **)&xthreading ))) return hr;
    if (!(context = calloc( 1, sizeof(*context) )))
    {
        IXThreadingImpl_Release( xthreading );
        return E_OUTOFMEMORY;
    }

    context->options = options;
    hr = IXThreadingImpl_XAsyncBegin( xthreading, async, context, NULL, "XUserAddAsync", XUserAddProvider );
    IXThreadingImpl_Release( xthreading );
    if (FAILED(hr)) free( context );
    return hr;
}

static HRESULT WINAPI x_user_XUserAddResult( IXUserImpl6 *iface, XAsyncBlock *async, XUserHandle *newUser )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    TRACE( "iface %p, async %p, newUser %p.\n", iface, async, newUser );

    if (!async || !newUser) return E_POINTER;
    if (FAILED(hr = QueryApiImpl( &CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void **)&xthreading ))) return hr;
    hr = IXThreadingImpl_XAsyncGetResult( xthreading, async, NULL, sizeof(*newUser), newUser, NULL );
    IXThreadingImpl_Release( xthreading );
    return hr;
}

static HRESULT WINAPI x_user_XUserGetLocalId( IXUserImpl6 *iface, XUserHandle user, XUserLocalId *userLocalId )
{
    FIXME( "iface %p, user %p, userLocalId %p stub!\n", iface, user, userLocalId );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserFindUserByLocalId( IXUserImpl6 *iface, XUserLocalId userLocalId, XUserHandle *handle )
{
    FIXME( "iface %p, userLocalId %p, handle %p stub!\n", iface, &userLocalId, handle );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetId( IXUserImpl6 *iface, XUserHandle user, UINT64 *userId )
{
    TRACE( "iface %p, user %p, userId %p.\n", iface, user, userId );
    *userId = user->xuid;
    return S_OK;
}

static HRESULT WINAPI x_user_XUserFindUserById( IXUserImpl6 *iface, UINT64 userId, XUserHandle *handle )
{
    FIXME( "iface %p, userId %llu, handle %p stub!\n", iface, userId, handle );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetIsGuest( IXUserImpl6 *iface, XUserHandle user, BOOLEAN *isGuest )
{
    FIXME( "iface %p, user %p, isGuest %p stub!\n", iface, user, isGuest );
    *isGuest = TRUE;
    return S_OK;
}

static HRESULT WINAPI x_user_XUserGetState( IXUserImpl6 *iface, XUserHandle user, XUserState *state )
{
    FIXME( "iface %p, user %p, state %p stub!\n", iface, user, state );
    return E_NOTIMPL;
}

static HRESULT WINAPI __PADDING__( IXUserImpl6 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetGamerPictureAsync( IXUserImpl6 *iface, XUserHandle user, XUserGamerPictureSize pictureSize, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, pictureSize %d, async %p stub!\n", iface, user, pictureSize, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetGamerPictureResultSize( IXUserImpl6 *iface, XAsyncBlock *async, SIZE_T *bufferSize )
{
    FIXME( "iface %p, async %p, bufferSize %p stub!\n", iface, async, bufferSize );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetGamerPictureResult( IXUserImpl6 *iface, XAsyncBlock *async, SIZE_T bufferSize, void *buffer, SIZE_T *bufferUsed )
{
    FIXME( "iface %p, async %p, bufferSize %Iu, buffer %p, bufferUsed %p stub!\n", iface, async, bufferSize, buffer, bufferUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetAgeGroup( IXUserImpl6 *iface, XUserHandle user, XUserAgeGroup *ageGroup )
{
    FIXME( "iface %p, user %p, ageGroup %p stub!\n", iface, user, ageGroup );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserCheckPrivilege( IXUserImpl6 *iface, XUserHandle user, XUserPrivilegeOptions options, XUserPrivilege privilege, BOOLEAN *hasPrivilege, XUserPrivilegeDenyReason *reason )
{
    FIXME( "iface %p, user %p, options %d, privilege %d, hasPrivilege %p, reason %p stub!\n", iface, user, options, privilege, hasPrivilege, reason );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolvePrivilegeWithUiAsync( IXUserImpl6 *iface, XUserHandle user, XUserPrivilegeOptions options, XUserPrivilege privilege, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, options %d, privilege %d, async %p stub!\n", iface, user, options, privilege, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolvePrivilegeWithUiResult( IXUserImpl6 *iface, XAsyncBlock *async )
{
    FIXME( "iface %p, async %p stub!\n", iface, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureAsync( IXUserImpl6 *iface, XUserHandle user, XUserGetTokenAndSignatureOptions options, const char *method, const char *url, SIZE_T headerCount, const XUserGetTokenAndSignatureHttpHeader *headers, SIZE_T bodySize, const void *bodyBuffer, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, options %d, method %s, url %s, headerCount %Iu, headers %p, bodySize %Iu, bodyBuffer %p, async %p stub!\n", iface, user, options, debugstr_a( method ), debugstr_a( url ), headerCount, headers, bodySize, bodyBuffer, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureResultSize( IXUserImpl6 *iface, XAsyncBlock *async, SIZE_T *bufferSize )
{
    FIXME( "iface %p, async %p, bufferSize %p stub!\n", iface, async, bufferSize );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureResult( IXUserImpl6 *iface, XAsyncBlock *async, SIZE_T bufferSize, void *buffer, XUserGetTokenAndSignatureData **ptrToBuffer, SIZE_T *bufferUsed )
{
    FIXME( "iface %p, async %p, bufferSize %Iu, buffer %p, ptrToBuffer %p, bufferUsed %p stub!\n", iface, async, bufferSize, buffer, ptrToBuffer, bufferUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureUtf16Async( IXUserImpl6 *iface, XUserHandle user, XUserGetTokenAndSignatureOptions options, const WCHAR *method, const WCHAR *url, SIZE_T headerCount, const XUserGetTokenAndSignatureUtf16HttpHeader *headers, SIZE_T bodySize, const void *bodyBuffer, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, options %d, method %s, url %s, headerCount %Iu, headers %p, bodySize %Iu, bodyBuffer %p, async %p stub!\n", iface, user, options, debugstr_w( method ), debugstr_w( url ), headerCount, headers, bodySize, bodyBuffer, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureUtf16ResultSize( IXUserImpl6 *iface, XAsyncBlock *async, SIZE_T *bufferSize )
{
    FIXME( "iface %p, async %p, bufferSize %p stub!\n", iface, async, bufferSize );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureUtf16Result( IXUserImpl6 *iface, XAsyncBlock *async, SIZE_T bufferSize, void *buffer, XUserGetTokenAndSignatureUtf16Data **ptrToBuffer, SIZE_T *bufferUsed )
{
    FIXME( "iface %p, async %p, bufferSize %Iu, buffer %p, ptrToBuffer %p, bufferUsed %p stub!\n", iface, async, bufferSize, buffer, ptrToBuffer, bufferUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolveIssueWithUiAsync( IXUserImpl6 *iface, XUserHandle user, const char *url, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, url %s, async %p stub!\n", iface, user, debugstr_a( url ), async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolveIssueWithUiResult( IXUserImpl6 *iface, XAsyncBlock *async )
{
    FIXME( "iface %p, async %p stub!\n", iface, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolveIssueWithUiUtf16Async( IXUserImpl6 *iface, XUserHandle user, const WCHAR *url, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, url %s, async %p stub!\n", iface, user, debugstr_w( url ), async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolveIssueWithUiUtf16Result( IXUserImpl6 *iface, XAsyncBlock *async )
{
    FIXME( "iface %p, async %p stub!\n", iface, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserRegisterForChangeEvent( IXUserImpl6 *iface, XTaskQueueHandle queue, void *context, XUserChangeEventCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_user_XUserUnregisterForChangeEvent( IXUserImpl6 *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return FALSE;
}

static HRESULT WINAPI x_user_XUserGetSignOutDeferral( IXUserImpl6 *iface, XUserSignOutDeferralHandle *deferral )
{
    TRACE( "iface %p, deferral %p.\n", iface, deferral );
    *deferral = NULL;
    return E_GAMEUSER_DEFERRAL_NOT_AVAILABLE;
}

static void WINAPI x_user_XUserCloseSignOutDeferralHandle( IXUserImpl6 *iface, XUserSignOutDeferralHandle deferral )
{
    FIXME( "iface %p, deferral %p stub!\n", iface, deferral );
}

static HRESULT WINAPI x_user_XUserAddByIdWithUiAsync( IXUserImpl6 *iface, UINT64 userId, XAsyncBlock *async )
{
    FIXME( "iface %p, userId %llu, async %p stub!\n", iface, userId, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserAddByIdWithUiResult( IXUserImpl6 *iface, XAsyncBlock *async, XUserHandle *newUser )
{
    FIXME( "iface %p, async %p, newUser %p stub!\n", iface, async, newUser );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetMsaTokenSilentlyAsync( IXUserImpl6 *iface, XUserHandle user, XUserGetMsaTokenSilentlyOptions options, const char *scope, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, options %u, scope %s, async %p stub!\n", iface, user, options, debugstr_a( scope ), async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetMsaTokenSilentlyResult( IXUserImpl6 *iface, XAsyncBlock *async, SIZE_T resultTokenSize, char *resultToken, SIZE_T *resultTokenUsed )
{
    FIXME( "iface %p, async %p, resultTokenSize %Iu, resultToken %p, resultTokenUsed %p stub!\n", iface, async, resultTokenSize, resultToken, resultTokenUsed );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetMsaTokenSilentlyResultSize( IXUserImpl6 *iface, XAsyncBlock *async, SIZE_T *tokenSize )
{
    FIXME( "iface %p, async %p, tokenSize %p stub!\n", iface, async, tokenSize );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_user_XUserIsStoreUser( IXUserImpl6 *iface, XUserHandle user )
{
    FIXME( "iface %p, user %p stub!\n", iface, user );
    return TRUE;
}

static HRESULT WINAPI x_user_XUserPlatformRemoteConnectSetEventHandlers( IXUserImpl6 *iface, XTaskQueueHandle queue, XUserPlatformRemoteConnectEventHandlers *handlers )
{
    FIXME( "iface %p, queue %p, handlers %p stub!\n", iface, queue, handlers );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserPlatformRemoteConnectCancelPrompt( IXUserImpl6 *iface, XUserPlatformOperation operation )
{
    FIXME( "iface %p, operation %p stub!\n", iface, operation );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserPlatformSpopPromptSetEventHandlers( IXUserImpl6 *iface, XTaskQueueHandle queue, XUserPlatformSpopPromptEventHandler *handler, void *context )
{
    FIXME( "iface %p, queue %p, handler %p, context %p stub!\n", iface, queue, handler, context );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserPlatformSpopPromptComplete( IXUserImpl6 *iface, XUserPlatformOperation operation, XUserPlatformOperationResult result )
{
    FIXME( "iface %p, operation %p, result %d stub!\n", iface, operation, result );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_user_XUserIsSignOutPresent( IXUserImpl6 *iface )
{
    TRACE( "iface %p.\n", iface );
    return FALSE;
}

static HRESULT WINAPI x_user_XUserSignOutAsync( IXUserImpl6 *iface, XUserHandle user, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, async %p stub!\n", iface, user, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserSignOutResult( IXUserImpl6 *iface, XAsyncBlock *async )
{
    FIXME( "iface %p, async %p stub!\n", iface, async );
    return E_NOTIMPL;
}

static const struct IXUserImpl6Vtbl x_user_vtbl =
{
    x_user_QueryInterface,
    x_user_AddRef,
    x_user_Release,
    /* IXUserImpl methods */
    x_user_XUserDuplicateHandle,
    x_user_XUserCloseHandle,
    x_user_XUserCompare,
    x_user_XUserGetMaxUsers,
    x_user_XUserAddAsync,
    x_user_XUserAddResult,
    x_user_XUserGetLocalId,
    x_user_XUserFindUserByLocalId,
    x_user_XUserGetId,
    x_user_XUserFindUserById,
    x_user_XUserGetIsGuest,
    x_user_XUserGetState,
    __PADDING__,
    x_user_XUserGetGamerPictureAsync,
    x_user_XUserGetGamerPictureResultSize,
    x_user_XUserGetGamerPictureResult,
    x_user_XUserGetAgeGroup,
    x_user_XUserCheckPrivilege,
    x_user_XUserResolvePrivilegeWithUiAsync,
    x_user_XUserResolvePrivilegeWithUiResult,
    x_user_XUserGetTokenAndSignatureAsync,
    x_user_XUserGetTokenAndSignatureResultSize,
    x_user_XUserGetTokenAndSignatureResult,
    x_user_XUserGetTokenAndSignatureUtf16Async,
    x_user_XUserGetTokenAndSignatureUtf16ResultSize,
    x_user_XUserGetTokenAndSignatureUtf16Result,
    x_user_XUserResolveIssueWithUiAsync,
    x_user_XUserResolveIssueWithUiResult,
    x_user_XUserResolveIssueWithUiUtf16Async,
    x_user_XUserResolveIssueWithUiUtf16Result,
    x_user_XUserRegisterForChangeEvent,
    x_user_XUserUnregisterForChangeEvent,
    x_user_XUserGetSignOutDeferral,
    x_user_XUserCloseSignOutDeferralHandle,
    /* IXUserImpl2 methods */
    x_user_XUserAddByIdWithUiAsync,
    x_user_XUserAddByIdWithUiResult,
    /* IXUserImpl3 methods */
    x_user_XUserGetMsaTokenSilentlyAsync,
    x_user_XUserGetMsaTokenSilentlyResult,
    x_user_XUserGetMsaTokenSilentlyResultSize,
    /* IXUserImpl4 methods */
    x_user_XUserIsStoreUser,
    /* IXUserImpl5 methods */
    x_user_XUserPlatformRemoteConnectSetEventHandlers,
    x_user_XUserPlatformRemoteConnectCancelPrompt,
    x_user_XUserPlatformSpopPromptSetEventHandlers,
    x_user_XUserPlatformSpopPromptComplete,
    /* IXUserImpl6 methods */
    x_user_XUserIsSignOutPresent,
    x_user_XUserSignOutAsync,
    x_user_XUserSignOutResult,
};

static inline struct x_user *impl_from_IXUserGamertagImpl( IXUserGamertagImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_user, IXUserGamertagImpl_iface );
}

static HRESULT WINAPI x_user_gamertag_QueryInterface( IXUserGamertagImpl *iface, REFIID riid, void **out )
{
    struct x_user *impl = impl_from_IXUserGamertagImpl( iface );
    return IXUserImpl6_QueryInterface( &impl->IXUserImpl_iface, riid, out );
}

static ULONG WINAPI x_user_gamertag_AddRef( IXUserGamertagImpl *iface )
{
    struct x_user *impl = impl_from_IXUserGamertagImpl( iface );
    return IXUserImpl6_AddRef( &impl->IXUserImpl_iface );
}

static ULONG WINAPI x_user_gamertag_Release( IXUserGamertagImpl *iface )
{
    struct x_user *impl = impl_from_IXUserGamertagImpl( iface );
    return IXUserImpl6_Release( &impl->IXUserImpl_iface );
}

static HRESULT WINAPI x_user_gamertag_XUserGetGamertag( IXUserGamertagImpl *iface, XUserHandle user, XUserGamertagComponent gamertagComponent, SIZE_T gamertagSize, char *gamertag, SIZE_T *gamertagUsed )
{
    FIXME( "iface %p, user %p, gamertagComponent %d, gamertagSize %Iu, gamertag %p, gamertagUsed %p stub!\n", iface, user, gamertagComponent, gamertagSize, gamertag, gamertagUsed );
    return E_NOTIMPL;
}

static const struct IXUserGamertagImplVtbl x_user_gamertag_vtbl =
{
    x_user_gamertag_QueryInterface,
    x_user_gamertag_AddRef,
    x_user_gamertag_Release,
    /* IXUserGamertagImpl methods */
    x_user_gamertag_XUserGetGamertag,
};

struct x_user_device
{
    IXUserDeviceImpl IXUserDeviceImpl_iface;
    LONG ref;
};

static inline struct x_user_device *impl_from_IXUserDeviceImpl( IXUserDeviceImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_user_device, IXUserDeviceImpl_iface );
}

static HRESULT WINAPI x_user_device_QueryInterface( IXUserDeviceImpl *iface, REFIID iid, void **out )
{
    struct x_user_device *impl = impl_from_IXUserDeviceImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown         ) ||
        IsEqualGUID( iid, &IID_IXUserDeviceImpl ))
    {
        IXUserDeviceImpl_AddRef( *out = &impl->IXUserDeviceImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_user_device_AddRef( IXUserDeviceImpl *iface )
{
    struct x_user_device *impl = impl_from_IXUserDeviceImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_user_device_Release( IXUserDeviceImpl *iface )
{
    struct x_user_device *impl = impl_from_IXUserDeviceImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_user_device_XUserFindForDevice( IXUserDeviceImpl *iface, const APP_LOCAL_DEVICE_ID *deviceId, XUserHandle *handle )
{
    FIXME( "iface %p, deviceId %p, handle %p stub!\n", iface, deviceId, handle );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_device_XUserRegisterForDeviceAssociationChanged( IXUserDeviceImpl *iface, XTaskQueueHandle queue, void *context, XUserDeviceAssociationChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_user_device_XUserUnregisterForDeviceAssociationChanged( IXUserDeviceImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return FALSE;
}

static HRESULT WINAPI x_user_device_XUserGetDefaultAudioEndpointUtf16( IXUserDeviceImpl *iface, XUserLocalId user, XUserDefaultAudioEndpointKind defaultAudioEndpointKind, SIZE_T endpointIdUtf16Count, WCHAR *endpointIdUtf16, SIZE_T *endpointIdUtf16Used )
{
    FIXME( "iface %p, user %p, defaultAudioEndpointKind %d, endpointIdUtf16Count %Iu, endpointIdUtf16 %p, endpointIdUtf16used %p stub!\n", iface, &user, defaultAudioEndpointKind, endpointIdUtf16Count, endpointIdUtf16, endpointIdUtf16Used );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_device_XUserRegisterForDefaultAudioEndpointUtf16Changed( IXUserDeviceImpl *iface, XTaskQueueHandle queue, void *context, XUserDefaultAudioEndpointUtf16ChangedCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_user_device_XUserUnregisterForDefaultAudioEndpointUtf16Changed( IXUserDeviceImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return FALSE;
}

static HRESULT WINAPI x_user_device_XUserFindControllerForUserWithUiAsync( IXUserDeviceImpl *iface, XUserHandle user, XAsyncBlock *async )
{
    FIXME( "iface %p, user %p, async %p stub!\n", iface, user, async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_device_XUserFindControllerForUserWithUiResult( IXUserDeviceImpl *iface, XAsyncBlock *async, APP_LOCAL_DEVICE_ID *deviceId )
{
    FIXME( "iface %p, async %p, deviceId %p stub!\n", iface, async, deviceId );
    return E_NOTIMPL;
}

static const struct IXUserDeviceImplVtbl x_user_device_vtbl =
{
    x_user_device_QueryInterface,
    x_user_device_AddRef,
    x_user_device_Release,
    /* IXUserDeviceImpl methods */
    x_user_device_XUserFindForDevice,
    x_user_device_XUserRegisterForDeviceAssociationChanged,
    x_user_device_XUserUnregisterForDeviceAssociationChanged,
    x_user_device_XUserGetDefaultAudioEndpointUtf16,
    x_user_device_XUserRegisterForDefaultAudioEndpointUtf16Changed,
    x_user_device_XUserUnregisterForDefaultAudioEndpointUtf16Changed,
    x_user_device_XUserFindControllerForUserWithUiAsync,
    x_user_device_XUserFindControllerForUserWithUiResult,
};

static struct x_user x_user_impl =
{
    {&x_user_vtbl},
    {&x_user_gamertag_vtbl},
    0,
};

static struct x_user_device x_user_device_impl =
{
    {&x_user_device_vtbl},
    0,
};

IXUserImpl6 *x_user = &x_user_impl.IXUserImpl_iface;
IXUserDeviceImpl *x_user_device = &x_user_device_impl.IXUserDeviceImpl_iface;
