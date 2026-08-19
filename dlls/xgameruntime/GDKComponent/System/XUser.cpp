/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XUser
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

#include "../../private.h"
#include "User/UserImpl.h"

#include <cstring>
#include <atomic>
#include <new>
#include <mutex>
#include <vector>

#include <bcrypt.h>
#include <wincrypt.h>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static HRESULT __stdcall not_implemented_async_work( XAsyncBlock * )
{
    return E_NOTIMPL;
}

/* No XSTS token or request signature is available yet - the xodus IPC protocol only
 * carries MSA tokens. Hand back an empty but well-formed result so callers get a
 * definite answer instead of a failure they turn into an exception. */
/* An XSTS header is a few kilobytes of base64. */
#define XSTS_TOKEN_MAX 8192
/* XUserGamertagComponentUniqueModernMaxBytes, the largest of the four components. */
#define XUSER_GAMERTAG_MAX 64
/* base64 of the 76 byte signature blob, plus room for the terminator. */
#define SIGNATURE_MAX 128

/* Xbox binds a token to the ECDSA P-256 key whose JWK went out as ProofKey, and
 * verifies request signatures against it. One key per process is enough here: xodus
 * signs in a single user. See
 * https://learn.microsoft.com/gaming/gdk/docs/services/fundamentals/s2s-auth-calls/s2s-calls/live-title-service-calls-xbox-live#proof-keys */
static std::once_flag g_proof_key_once;
static BCRYPT_KEY_HANDLE g_proof_key = nullptr;
static char g_proof_key_jwk[512];

static HRESULT base64_url_no_pad( const BYTE *data, DWORD size, char *out, DWORD outSize )
{
    char *p;

    if ( !CryptBinaryToStringA( data, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out, &outSize ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    for ( p = out; *p; p++ )
    {
        if ( *p == '+' ) *p = '-';
        else if ( *p == '/' ) *p = '_';
        else if ( *p == '=' ) { *p = '\0'; break; }
    }
    return S_OK;
}

static void create_proof_key()
{
    UCHAR blob[sizeof(BCRYPT_ECCKEY_BLOB) + 64];
    char x[64] = {}, y[64] = {};
    BCRYPT_ALG_HANDLE ecdsa = nullptr;
    BCRYPT_KEY_HANDLE key = nullptr;
    ULONG written;

    if ( !BCRYPT_SUCCESS( BCryptOpenAlgorithmProvider( &ecdsa, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0 ) ) )
        return;

    if ( BCRYPT_SUCCESS( BCryptGenerateKeyPair( ecdsa, &key, 256, 0 ) ) &&
         BCRYPT_SUCCESS( BCryptFinalizeKeyPair( key, 0 ) ) &&
         BCRYPT_SUCCESS( BCryptExportKey( key, nullptr, BCRYPT_ECCPUBLIC_BLOB, blob, sizeof(blob), &written, 0 ) ) &&
         SUCCEEDED( base64_url_no_pad( blob + sizeof(BCRYPT_ECCKEY_BLOB), 32, x, sizeof(x) ) ) &&
         SUCCEEDED( base64_url_no_pad( blob + sizeof(BCRYPT_ECCKEY_BLOB) + 32, 32, y, sizeof(y) ) ) )
    {
        snprintf( g_proof_key_jwk, sizeof(g_proof_key_jwk),
                  "{\"alg\":\"ES256\",\"kty\":\"EC\",\"use\":\"sig\",\"crv\":\"P-256\",\"x\":\"%s\",\"y\":\"%s\"}",
                  x, y );
        g_proof_key = key;
        key = nullptr;
    }

    if ( key ) BCryptDestroyKey( key );
    BCryptCloseAlgorithmProvider( ecdsa, 0 );
}

EXTERN_C const char *XodusProofKeyJwk( void )
{
    std::call_once( g_proof_key_once, create_proof_key );
    return g_proof_key_jwk[0] ? g_proof_key_jwk : nullptr;
}

/* The signed blob is each field NUL terminated in order: version, timestamp, method,
 * path and query, the Authorization header, then the body. */
static HRESULT sign_request( const char *method, const char *url, const char *authorization,
                             const void *body, SIZE_T bodySize, char *out, DWORD outSize )
{
    BYTE raw[76] = {}, hash[32];
    BCRYPT_ALG_HANDLE sha = nullptr;
    BCRYPT_HASH_HANDLE hashObj = nullptr;
    const char *pathAndQuery;
    FILETIME ft;
    ULONGLONG stamp;
    ULONG written;
    NTSTATUS status;
    HRESULT hr = E_FAIL;
    UINT32 version = 1;

    std::call_once( g_proof_key_once, create_proof_key );
    if ( !g_proof_key ) return E_FAIL;

    /* Skip the scheme and host; the policy signs only the path with its query. */
    pathAndQuery = url ? strstr( url, "://" ) : nullptr;
    pathAndQuery = pathAndQuery ? strchr( pathAndQuery + 3, '/' ) : nullptr;
    if ( !pathAndQuery ) pathAndQuery = "/";

    GetSystemTimeAsFileTime( &ft );
    stamp = ( (ULONGLONG)ft.dwHighDateTime << 32 ) | ft.dwLowDateTime;

    raw[0] = (BYTE)(version >> 24); raw[1] = (BYTE)(version >> 16);
    raw[2] = (BYTE)(version >> 8);  raw[3] = (BYTE)version;
    for ( int i = 0; i < 8; i++ ) raw[4 + i] = (BYTE)( stamp >> ( 56 - i * 8 ) );

    if ( !BCRYPT_SUCCESS( status = BCryptOpenAlgorithmProvider( &sha, BCRYPT_SHA256_ALGORITHM, nullptr, 0 ) ) )
        return HRESULT_FROM_NT( status );

    if ( BCRYPT_SUCCESS( BCryptCreateHash( sha, &hashObj, nullptr, 0, nullptr, 0, 0 ) ) )
    {
        static const BYTE nul = 0;
        BCryptHashData( hashObj, raw, 12, 0 );
        BCryptHashData( hashObj, (PUCHAR)&nul, 1, 0 );
        BCryptHashData( hashObj, (PUCHAR)( method ? method : "GET" ), method ? strlen( method ) : 3, 0 );
        BCryptHashData( hashObj, (PUCHAR)&nul, 1, 0 );
        BCryptHashData( hashObj, (PUCHAR)pathAndQuery, strlen( pathAndQuery ), 0 );
        BCryptHashData( hashObj, (PUCHAR)&nul, 1, 0 );
        BCryptHashData( hashObj, (PUCHAR)( authorization ? authorization : "" ), authorization ? strlen( authorization ) : 0, 0 );
        BCryptHashData( hashObj, (PUCHAR)&nul, 1, 0 );
        if ( body && bodySize ) BCryptHashData( hashObj, (PUCHAR)body, (ULONG)bodySize, 0 );
        BCryptHashData( hashObj, (PUCHAR)&nul, 1, 0 );

        if ( BCRYPT_SUCCESS( BCryptFinishHash( hashObj, hash, sizeof(hash), 0 ) ) &&
             BCRYPT_SUCCESS( BCryptSignHash( g_proof_key, nullptr, hash, sizeof(hash),
                                             raw + 12, 64, &written, 0 ) ) )
        {
            hr = CryptBinaryToStringA( raw, sizeof(raw), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                                       out, &outSize ) ? S_OK : HRESULT_FROM_WIN32( GetLastError() );
        }
        BCryptDestroyHash( hashObj );
    }

    BCryptCloseAlgorithmProvider( sha, 0 );
    return hr;
}

/* Titles subscribe to user change events and only refresh their account UI when one
 * arrives. Nothing here ever signs out, but a subscriber that registers after the user
 * is already present would otherwise never hear about it and would keep showing the
 * signed-out state, so registration replays SignedInAgain for the current user. */
struct ChangeEventRegistration
{
    UINT64 token;
    XTaskQueueHandle queue;
    void *context;
    XUserChangeEventCallback *callback;
};

static std::mutex g_change_events_lock;
static std::vector<ChangeEventRegistration> g_change_events;
static XUserHandle g_current_user = nullptr;

struct ChangeEventDispatch
{
    void *context;
    XUserChangeEventCallback *callback;
    XUserLocalId localId;
    XUserChangeEvent event;
};

static void __stdcall change_event_thunk( void *context, BOOLEAN canceled )
{
    auto *dispatch = (ChangeEventDispatch *)context;

    TRACE( "delivering change event %d for user %#llx, canceled %d.\n",
           (int)dispatch->event, (unsigned long long)dispatch->localId.value, canceled );

    if ( !canceled && dispatch->callback )
        dispatch->callback( dispatch->context, dispatch->localId, dispatch->event );

    delete dispatch;
}

static void raise_change_event( const ChangeEventRegistration &reg, XUserHandle user,
                                XUserChangeEvent event )
{
    if ( !reg.callback || !user || !user->m_user ) return;

    auto *dispatch = new (std::nothrow) ChangeEventDispatch;
    if ( !dispatch ) return;

    dispatch->context = reg.context;
    dispatch->callback = reg.callback;
    dispatch->localId.value = (UINT64)(ULONG_PTR)user->m_user;
    dispatch->event = event;

    if ( FAILED( XTaskQueueSubmitCallback( reg.queue, XTaskQueuePort::Completion,
                                           dispatch, change_event_thunk ) ) )
    {
        WARN( "could not queue a user change event; delivering it inline.\n" );
        change_event_thunk( dispatch, FALSE );
    }
}

/* Which service the caller is authenticating to decides which stored token fits;
 * both travel to GetResult as the provider context. */
struct TokenAndSignatureContext
{
    XUserHandle user;
    bool playfab;
    char *method;
    char *url;
    void *body;
    SIZE_T bodySize;
    /* Resolved on the worker thread, because a title's own back end needs a token
     * fetched for its relying party rather than one of the two settled at sign-in. */
    char *token;
    /* The UTF-16 entry points want the same answer in wide characters. */
    bool utf16;
};

/* Xbox Live's own hosts all sit behind the relying party settled at sign-in; a
 * title's back end does not, and needs its own exchange. */
static bool is_xboxlive_host( const char *url )
{
    const char *host = url ? strstr( url, "://" ) : nullptr;
    const char *end;
    size_t len;

    if ( !host ) return false;
    host += 3;
    end = strchr( host, '/' );
    len = end ? (size_t)(end - host) : strlen( host );

    return len >= 12 && !strncmp( host + len - 12, "xboxlive.com", 12 );
}

/* Fills ctx->token with the UTF-8 token the caller should present. Runs on the
 * worker thread because an unseen back end costs a round trip to the service. */
static void resolve_token( TokenAndSignatureContext *ctx )
{
    XUserHandle user = ctx ? ctx->user : nullptr;
    HSTRING stored = nullptr;
    HSTRING url = nullptr;
    UINT32 length = 0;
    LPCWSTR raw;

    if ( !ctx || ctx->token ) return;
    if ( !user || user->m_signature != X_USER_SIGNATURE || !user->m_user ) return;

    if ( ctx->playfab )
    {
        if ( FAILED( user->m_user->GetPlayfabToken( &stored ) ) ) return;
    }
    else if ( !ctx->url || is_xboxlive_host( ctx->url ) )
    {
        if ( FAILED( user->m_user->GetXstsToken( &stored ) ) ) return;
    }
    else
    {
        INT32 wide = MultiByteToWideChar( CP_UTF8, 0, ctx->url, -1, nullptr, 0 );
        LPWSTR urlW = wide ? (LPWSTR)CoTaskMemAlloc( wide * sizeof(WCHAR) ) : nullptr;

        if ( !urlW ) return;
        MultiByteToWideChar( CP_UTF8, 0, ctx->url, -1, urlW, wide );
        HRESULT hr = WindowsCreateString( urlW, lstrlenW( urlW ), &url );
        CoTaskMemFree( urlW );
        if ( FAILED( hr ) ) return;

        hr = user->m_user->GetServiceToken( url, &stored );
        WindowsDeleteString( url );
        if ( FAILED( hr ) )
        {
            /* Better a generic token than none - some back ends accept it. */
            WARN( "falling back to the Xbox Live token for %s.\n", debugstr_a( ctx->url ) );
            if ( FAILED( user->m_user->GetXstsToken( &stored ) ) ) return;
        }
    }

    raw = WindowsGetStringRawBuffer( stored, &length );
    if ( raw && length )
    {
        INT bytes = WideCharToMultiByte( CP_UTF8, 0, raw, length, nullptr, 0, nullptr, nullptr );
        if ( bytes > 0 && (ctx->token = (char *)malloc( bytes + 1 )) )
        {
            WideCharToMultiByte( CP_UTF8, 0, raw, length, ctx->token, bytes, nullptr, nullptr );
            ctx->token[bytes] = '\0';
        }
    }
    WindowsDeleteString( stored );
}

static HRESULT __stdcall token_and_signature_provider( XAsyncOp op, const XAsyncProviderData *data )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    switch (op)
    {
        case XAsyncOp::Begin:
            return xthreading->XAsyncSchedule( data->async, 0 );

        case XAsyncOp::DoWork:
            resolve_token( (TokenAndSignatureContext *)data->context );
            /* Sized for the wide form, which is the larger of the two shapes. */
            xthreading->XAsyncComplete( data->async, S_OK,
                                        sizeof(XUserGetTokenAndSignatureUtf16Data)
                                        + ( XSTS_TOKEN_MAX + 1 + SIGNATURE_MAX + 1 ) * sizeof(WCHAR) );
            /* Completed here, so the framework must not complete it again. */
            return E_PENDING;

        case XAsyncOp::GetResult:
        {
            auto *ctx = (TokenAndSignatureContext *)data->context;

            if ( ctx && ctx->utf16 )
            {
                auto *wide = (XUserGetTokenAndSignatureUtf16Data *)data->buffer;
                WCHAR *wtoken = (WCHAR *)((char *)data->buffer + sizeof(*wide));
                char narrow[SIGNATURE_MAX];
                WCHAR *wsignature;
                INT count = 0;

                wtoken[0] = 0;
                if ( ctx->token )
                {
                    count = MultiByteToWideChar( CP_UTF8, 0, ctx->token, -1, wtoken, XSTS_TOKEN_MAX );
                    /* The count includes the terminator, which the caller does not want. */
                    if ( count > 0 ) count--;
                }

                wsignature = wtoken + count + 1;
                wsignature[0] = 0;
                narrow[0] = '\0';
                if ( count && SUCCEEDED( sign_request( ctx->method, ctx->url, ctx->token,
                                                       ctx->body, ctx->bodySize,
                                                       narrow, SIGNATURE_MAX ) ) )
                    MultiByteToWideChar( CP_UTF8, 0, narrow, -1, wsignature, SIGNATURE_MAX );
                else if ( count )
                    WARN( "request signature unavailable; sending the token unsigned.\n" );

                wide->tokenCount = count;
                wide->signatureCount = lstrlenW( wsignature );
                wide->token = wtoken;
                wide->signature = wsignature;
                return S_OK;
            }

            auto *result = (XUserGetTokenAndSignatureData *)data->buffer;
            char *token = (char *)data->buffer + sizeof(*result);
            XUserHandle user = ctx ? ctx->user : nullptr;
            HSTRING stored = nullptr;
            UINT32 length = 0;
            LPCWSTR raw;
            INT written = 0;

            token[0] = '\0';

            if ( ctx && ctx->token )
            {
                written = (INT)strlen( ctx->token );
                if ( written > XSTS_TOKEN_MAX ) written = XSTS_TOKEN_MAX;
                memcpy( token, ctx->token, written );
            }

            token[written] = '\0';
            (void)user; (void)stored; (void)raw; (void)length;

            char *signature = token + written + 1;
            signature[0] = '\0';
            if ( written && FAILED( sign_request( ctx ? ctx->method : nullptr,
                                                  ctx ? ctx->url : nullptr, token,
                                                  ctx ? ctx->body : nullptr,
                                                  ctx ? ctx->bodySize : 0,
                                                  signature, SIGNATURE_MAX ) ) )
            {
                WARN( "request signature unavailable; sending the token unsigned.\n" );
                signature[0] = '\0';
            }

            result->tokenSize = written;
            result->signatureSize = strlen( signature );
            result->token = token;
            result->signature = signature;
            return S_OK;
        }

        case XAsyncOp::Cleanup:
        {
            auto *ctx = (TokenAndSignatureContext *)data->context;
            if ( ctx )
            {
                free( ctx->method );
                free( ctx->url );
                free( ctx->body );
                free( ctx->token );
            }
            delete ctx;
            break;
        }

        default:
            break;
    }

    return S_OK;
}

static char *dup_string( const char *value )
{
    return value ? _strdup( value ) : nullptr;
}

static HRESULT begin_token_and_signature( XUserHandle user, const char *method, const char *url,
                                          const void *body, SIZE_T bodySize, XAsyncBlock *async,
                                          bool utf16 = false )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    auto *ctx = new (std::nothrow) TokenAndSignatureContext;
    if ( !ctx ) return E_OUTOFMEMORY;

    ctx->user = user;
    ctx->playfab = url && strstr( url, "playfab" ) != nullptr;
    /* The provider runs later on a worker thread, so the request has to be copied. */
    ctx->method = dup_string( method );
    ctx->url = dup_string( url );
    ctx->bodySize = body ? bodySize : 0;
    ctx->body = nullptr;
    ctx->token = nullptr;
    ctx->utf16 = utf16;
    if ( ctx->bodySize && (ctx->body = malloc( ctx->bodySize )) )
        memcpy( ctx->body, body, ctx->bodySize );

    hr = xthreading->XAsyncBegin( async, ctx, (void *)token_and_signature_provider,
                                  "XUserGetTokenAndSignature", token_and_signature_provider );
    if ( FAILED( hr ) ) delete ctx;

    return hr;
}

static HRESULT token_and_signature_result( XAsyncBlock *async, SIZE_T bufferSize, void *buffer,
                                           XUserGetTokenAndSignatureData **ptrToBuffer, SIZE_T *bufferUsed )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    hr = xthreading->XAsyncGetResult( async, (void *)token_and_signature_provider,
                                      bufferSize, buffer, bufferUsed );
    if ( SUCCEEDED( hr ) && ptrToBuffer )
        *ptrToBuffer = (XUserGetTokenAndSignatureData *)buffer;

    return hr;
}

static HRESULT token_and_signature_result_size( XAsyncBlock *async, SIZE_T *bufferSize )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    return xthreading->XAsyncGetResultSize( async, bufferSize );
}

class XUserImpl :
    public IXUserImpl6,
    public IXUserGamertagImpl
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out )
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXUserImpl ) )
        {
            AddRef();
            *out = static_cast<IXUserImpl *>(this);
            return S_OK;
        }

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXUserImpl2 ) )
        {
            AddRef();
            *out = static_cast<IXUserImpl2 *>(this);
            return S_OK;
        }

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXUserImpl3 ) )
        {
            AddRef();
            *out = static_cast<IXUserImpl3 *>(this);
            return S_OK;
        }

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXUserImpl4 ) )
        {
            AddRef();
            *out = static_cast<IXUserImpl4 *>(this);
            return S_OK;
        }

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXUserImpl5 ) )
        {
            AddRef();
            *out = static_cast<IXUserImpl5 *>(this);
            return S_OK;
        }

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXUserImpl6 ) )
        {
            AddRef();
            *out = static_cast<IXUserImpl6 *>(this);
            return S_OK;
        }

        /* Titles ask for this separately from the main interface; without it they
         * have no way to read the gamertag and fall back to a placeholder name. */
        if ( iid == __uuidof( IXUserGamertagImpl ) )
        {
            AddRef();
            *out = static_cast<IXUserGamertagImpl *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI 
    AddRef() noexcept override
    {
        ULONG curr = static_cast<ULONG>(++ref);
        TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
        return curr;
    }

    ULONG WINAPI 
    Release() noexcept override
    {
        ULONG curr = static_cast<ULONG>(--ref);
        TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

        // Polymorphic classes should not be deleted.
        /*
        if ( !curr )
            delete this;
        */

        return curr;
    }


    HRESULT WINAPI XUserDuplicateHandle( XUserHandle handle, XUserHandle *duplicatedHandle ) override
    {
        TRACE( "handle %p, duplicatedHandle %p.\n", handle, duplicatedHandle );

        if ( !duplicatedHandle ) return E_POINTER;
        if ( !handle || handle->m_signature != X_USER_SIGNATURE ) return E_GAMERUNTIME_INVALID_HANDLE;

        /* The duplicate has to survive XUserCloseHandle on either copy, so it gets
         * its own struct and a reference on the shared user object. */
        XUser *copy = new (std::nothrow) XUser;
        if ( !copy ) return E_OUTOFMEMORY;

        copy->m_signature = X_USER_SIGNATURE;
        copy->m_user = handle->m_user;
        if ( copy->m_user ) copy->m_user->AddRef();

        *duplicatedHandle = copy;
        return S_OK;
    }

    void WINAPI XUserCloseHandle( XUserHandle user ) override
    {
        FIXME( "user %p stub!\n", user );
    }

    INT32 WINAPI XUserCompare( XUserHandle user1, XUserHandle user2 ) override
    {
        UINT64 xuid1, xuid2;

        TRACE( "user1 %p, user2 %p.\n", user1, user2 );

        /* This is an ordering, not an HRESULT. Callers read zero as "the same user",
         * so returning a failure code here means no two handles ever match and a
         * caller looking for one spins forever. */
        if ( user1 == user2 ) return 0;

        xuid1 = ( user1 && user1->m_signature == X_USER_SIGNATURE && user1->m_user )
                ? user1->m_user->GetXuid() : 0;
        xuid2 = ( user2 && user2->m_signature == X_USER_SIGNATURE && user2->m_user )
                ? user2->m_user->GetXuid() : 0;

        if ( xuid1 < xuid2 ) return -1;
        if ( xuid1 > xuid2 ) return 1;
        return 0;
    }

    HRESULT WINAPI XUserGetMaxUsers( UINT32 *maxUsers ) override
    {
        TRACE( "maxUsers %p.\n", maxUsers );
        *maxUsers = 1;
        return S_OK;
    }

    HRESULT WINAPI XUserAddAsync( XUserAddOptions options, XAsyncBlock *async ) override
    {
        TRACE( "options %d, async %p.\n", (int)options, async );
        return ::XUserAddAsync( options, async );
    }

    HRESULT WINAPI XUserAddResult( XAsyncBlock *async, XUserHandle *newUser ) override
    {
        TRACE( "async %p, newUser %p.\n", async, newUser );

        HRESULT hr = ::XUserAddResult( async, newUser );
        if ( SUCCEEDED( hr ) && newUser && *newUser && (*newUser)->m_signature == X_USER_SIGNATURE )
        {
            std::lock_guard<std::mutex> lock( g_change_events_lock );
            g_current_user = *newUser;
            for ( const auto &reg : g_change_events )
                raise_change_event( reg, g_current_user, XUserChangeEvent::SignedInAgain );
        }
        return hr;
    }

    HRESULT WINAPI XUserGetLocalId( XUserHandle user, XUserLocalId *userLocalId ) override
    {
        TRACE( "user %p, userLocalId %p.\n", user, userLocalId );

        if ( !userLocalId ) return E_POINTER;
        if ( !user || user->m_signature != X_USER_SIGNATURE ) return E_GAMERUNTIME_INVALID_HANDLE;

        /* Keyed on the shared user object, not the handle: XUserDuplicateHandle hands
         * out a second handle for the same user, and both must report one id. */
        userLocalId->value = (UINT64)(ULONG_PTR)user->m_user;
        return S_OK;
    }

    HRESULT WINAPI XUserFindUserByLocalId( XUserLocalId userLocalId, XUserHandle *handle ) override
    {
        FIXME( "userLocalId %p, handle %p stub!\n", &userLocalId, handle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetId( XUserHandle user, UINT64 *userId ) override
    {
        TRACE( "user %p, userId %p.\n", user, userId );

        if ( !userId ) return E_POINTER;
        if ( !user || user->m_signature != X_USER_SIGNATURE || !user->m_user )
            return E_GAMERUNTIME_INVALID_HANDLE;

        *userId = user->m_user->GetXuid();
        if ( !*userId )
        {
            WARN( "no XUID for user %p; the XSTS exchange did not complete.\n", user );
            return E_GAMEUSER_NO_AUTH_USER;
        }

        return S_OK;
    }

    HRESULT WINAPI XUserFindUserById( UINT64 userId, XUserHandle *handle ) override
    {
        TRACE( "userId %llu, handle %p.\n", userId, handle );

        if ( !handle ) return E_POINTER;
        /* One user is signed in, so the only id that can resolve is theirs. */
        if ( !g_current_user || !g_current_user->m_user ||
             g_current_user->m_user->GetXuid() != userId )
            return E_GAMEUSER_NO_DEFAULT_USER;

        *handle = g_current_user;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE XUserGetGamertag( XUserHandle user, XUserGamertagComponent gamertagComponent,
                                                SIZE_T gamertagSize, char *gamertag, SIZE_T *gamertagUsed ) override
    {
        char utf8[XUSER_GAMERTAG_MAX];
        HSTRING stored = nullptr;
        UINT32 length = 0;
        LPCWSTR raw;
        INT written = 0;

        TRACE( "user %p, component %d, gamertagSize %Iu, gamertag %p, gamertagUsed %p.\n",
               user, (int)gamertagComponent, gamertagSize, gamertag, gamertagUsed );

        if ( !gamertag ) return E_POINTER;
        if ( !user || user->m_signature != X_USER_SIGNATURE || !user->m_user )
            return E_GAMERUNTIME_INVALID_HANDLE;

        /* xodus reports one gamertag. The modern suffix is the part after the '#'
         * of a unique modern gamertag, which accounts without one do not have. */
        if ( gamertagComponent != XUserGamertagComponent::ModernSuffix &&
             SUCCEEDED( user->m_user->GetGamertag( &stored ) ) )
        {
            raw = WindowsGetStringRawBuffer( stored, &length );
            if ( raw && length )
                written = WideCharToMultiByte( CP_UTF8, 0, raw, length,
                                               utf8, sizeof(utf8) - 1, nullptr, nullptr );
            WindowsDeleteString( stored );
        }

        utf8[written] = '\0';

        if ( !written && gamertagComponent != XUserGamertagComponent::ModernSuffix )
        {
            WARN( "no gamertag for user %p; the XSTS exchange did not complete.\n", user );
            return E_GAMEUSER_NO_AUTH_USER;
        }

        if ( gamertagSize < (SIZE_T)written + 1 )
        {
            if ( gamertagUsed ) *gamertagUsed = written + 1;
            return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }

        memcpy( gamertag, utf8, written + 1 );
        if ( gamertagUsed ) *gamertagUsed = written + 1;

        return S_OK;
    }

    HRESULT WINAPI XUserGetIsGuest( XUserHandle user, BOOLEAN *isGuest ) override
    {
        TRACE( "user %p, isGuest %p.\n", user, isGuest );

        if ( !isGuest ) return E_POINTER;
        if ( !user || user->m_signature != X_USER_SIGNATURE ) return E_GAMERUNTIME_INVALID_HANDLE;

        /* xodus signs in the account that owns the tokens, never a guest. */
        *isGuest = FALSE;
        return S_OK;
    }

    HRESULT WINAPI XUserGetState( XUserHandle user, XUserState *state ) override
    {
        TRACE( "user %p, state %p.\n", user, state );

        if ( !state ) return E_POINTER;
        if ( !user || user->m_signature != X_USER_SIGNATURE ) return E_GAMERUNTIME_INVALID_HANDLE;

        *state = XUserState::SignedIn;
        return S_OK;
    }

    HRESULT WINAPI __PADDING__() override
    {
        WARN( "padding function called! It's unknown what this function does.\n" );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetGamerPictureAsync( XUserHandle user, XUserGamerPictureSize pictureSize, XAsyncBlock *async ) override
    {
        FIXME( "user %p, pictureSize %d semi-stub: no gamer picture.\n", user, (int)pictureSize );

        /* Callers wait on the block, so the failure has to be delivered through the
         * async completion rather than returned synchronously. */
        return XAsyncRun( async, not_implemented_async_work );
    }

    HRESULT WINAPI XUserGetGamerPictureResultSize( XAsyncBlock *async, SIZE_T *bufferSize ) override
    {
        TRACE( "async %p, bufferSize %p: no picture.\n", async, bufferSize );

        /* The size has to be written even when reporting failure - callers size an
         * allocation from it and would otherwise use whatever was on the stack. */
        if ( !bufferSize ) return E_POINTER;
        *bufferSize = 0;
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetGamerPictureResult( XAsyncBlock *async, SIZE_T bufferSize, void *buffer, SIZE_T *bufferUsed ) override
    {
        TRACE( "async %p, bufferSize %Iu, buffer %p, bufferUsed %p: no picture.\n", async, bufferSize, buffer, bufferUsed );

        if ( bufferUsed ) *bufferUsed = 0;
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetAgeGroup( XUserHandle user, XUserAgeGroup *ageGroup ) override
    {
        TRACE( "user %p, ageGroup %p.\n", user, ageGroup );

        if ( !ageGroup ) return E_POINTER;
        if ( !user || user->m_signature != X_USER_SIGNATURE ) return E_GAMERUNTIME_INVALID_HANDLE;

        /* xodus does not carry the account's age group, and titles gate features on
         * it - Minecraft treats an account whose age group it cannot determine as
         * unusable and keeps offering the sign-in prompt. Reporting an adult account
         * applies no restrictions, which matches how the signed-in account is used
         * here; a real answer would need the profile service. */
        *ageGroup = XUserAgeGroup::Adult;
        return S_OK;
    }

    HRESULT WINAPI XUserCheckPrivilege( XUserHandle user, XUserPrivilegeOptions options, XUserPrivilege privilege, BOOLEAN *hasPrivilege, XUserPrivilegeDenyReason *reason ) override
    {
        FIXME( "user %p, options %d, privilege %d semi-stub: granting.\n", user, (int)options, (int)privilege );

        if ( !hasPrivilege ) return E_POINTER;
        if ( !user || user->m_signature != X_USER_SIGNATURE ) return E_GAMERUNTIME_INVALID_HANDLE;

        /* Privileges are not queried from Xbox Live yet; grant them so the caller
         * gets a definite answer instead of an uninitialized one. */
        *hasPrivilege = TRUE;
        if ( reason ) *reason = XUserPrivilegeDenyReason::None;
        return S_OK;
    }

    HRESULT WINAPI XUserResolvePrivilegeWithUiAsync( XUserHandle user, XUserPrivilegeOptions options, XUserPrivilege privilege, XAsyncBlock *async ) override
    {
        FIXME( "user %p, options %d, privilege %d, async %p stub!\n", user, (int)options, (int)privilege, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserResolvePrivilegeWithUiResult( XAsyncBlock *async ) override
    {
        FIXME( "async %p stub!\n", async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetTokenAndSignatureAsync( XUserHandle user, XUserGetTokenAndSignatureOptions options, const char *method, const char *url, SIZE_T headerCount, const XUserGetTokenAndSignatureHttpHeader *headers, SIZE_T bodySize, const void *bodyBuffer, XAsyncBlock *async ) override
    {
        FIXME( "user %p, options %d, method %s, url %s, headerCount %Iu, headers %p, bodySize %Iu, bodyBuffer %p, async %p stub!\n", user, (int)options, debugstr_a( method ), debugstr_a( url ), headerCount, headers, bodySize, bodyBuffer, async );
        /* The caller waits on the block; a synchronous failure leaves it pending
         * and the request is torn down half-initialized. */
        return begin_token_and_signature( user, method, url, bodyBuffer, bodySize, async );
    }

    HRESULT WINAPI XUserGetTokenAndSignatureResultSize( XAsyncBlock *async, SIZE_T *bufferSize ) override
    {
        TRACE( "async %p, bufferSize %p.\n", async, bufferSize );
        return token_and_signature_result_size( async, bufferSize );
    }

    HRESULT WINAPI XUserGetTokenAndSignatureResult( XAsyncBlock *async, SIZE_T bufferSize, void *buffer, XUserGetTokenAndSignatureData **ptrToBuffer, SIZE_T *bufferUsed ) override
    {
        TRACE( "async %p, bufferSize %Iu, buffer %p, ptrToBuffer %p, bufferUsed %p.\n", async, bufferSize, buffer, ptrToBuffer, bufferUsed );
        return token_and_signature_result( async, bufferSize, buffer, ptrToBuffer, bufferUsed );
    }

    HRESULT WINAPI XUserGetTokenAndSignatureUtf16Async( XUserHandle user, XUserGetTokenAndSignatureOptions options, const WCHAR *method, const WCHAR *url, SIZE_T headerCount, const XUserGetTokenAndSignatureUtf16HttpHeader *headers, SIZE_T bodySize, const void *bodyBuffer, XAsyncBlock *async ) override
    {
        CHAR urlA[512];

        TRACE( "user %p, options %d, method %s, url %s.\n",
               user, (int)options, debugstr_w( method ), debugstr_w( url ) );

        urlA[0] = '\0';
        if ( url )
            WideCharToMultiByte( CP_UTF8, 0, url, -1, urlA, sizeof(urlA), nullptr, nullptr );

        char methodA[32];

        methodA[0] = '\0';
        if ( method )
            WideCharToMultiByte( CP_UTF8, 0, method, -1, methodA, sizeof(methodA), nullptr, nullptr );

        return begin_token_and_signature( user, methodA, urlA, bodyBuffer, bodySize, async, true );
    }

    HRESULT WINAPI XUserGetTokenAndSignatureUtf16ResultSize( XAsyncBlock *async, SIZE_T *bufferSize ) override
    {
        TRACE( "async %p, bufferSize %p.\n", async, bufferSize );
        return token_and_signature_result_size( async, bufferSize );
    }

    HRESULT WINAPI XUserGetTokenAndSignatureUtf16Result( XAsyncBlock *async, SIZE_T bufferSize, void *buffer, XUserGetTokenAndSignatureUtf16Data **ptrToBuffer, SIZE_T *bufferUsed ) override
    {
        IXThreadingImpl *xthreading;
        HRESULT hr;

        TRACE( "async %p, bufferSize %Iu, buffer %p, ptrToBuffer %p, bufferUsed %p.\n",
               async, bufferSize, buffer, ptrToBuffer, bufferUsed );

        if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
            return hr;

        hr = xthreading->XAsyncGetResult( async, (void *)token_and_signature_provider,
                                          bufferSize, buffer, bufferUsed );
        if ( SUCCEEDED( hr ) && ptrToBuffer )
            *ptrToBuffer = (XUserGetTokenAndSignatureUtf16Data *)buffer;

        return hr;
    }

    HRESULT WINAPI XUserResolveIssueWithUiAsync( XUserHandle user, const char *url, XAsyncBlock *async ) override
    {
        FIXME( "user %p, url %s, async %p stub!\n", user, debugstr_a( url ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserResolveIssueWithUiResult( XAsyncBlock *async ) override
    {
        FIXME( "async %p stub!\n", async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserResolveIssueWithUiUtf16Async( XUserHandle user, const WCHAR *url, XAsyncBlock *async ) override
    {
        FIXME( "user %p, url %s, async %p stub!\n", user, debugstr_w( url ), async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserResolveIssueWithUiUtf16Result( XAsyncBlock *async ) override
    {
        FIXME( "async %p stub!\n", async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserRegisterForChangeEvent( XTaskQueueHandle queue, void *context, XUserChangeEventCallback *callback, XTaskQueueRegistrationToken *token ) override
    {
        TRACE( "queue %p, context %p, callback %p.\n", queue, context, callback );

        if ( !callback ) return E_INVALIDARG;

        ChangeEventRegistration reg{ ++m_NextChangeEventToken, queue, context, callback };
        if ( token ) token->token = reg.token;

        std::lock_guard<std::mutex> lock( g_change_events_lock );
        g_change_events.push_back( reg );

        /* Subscribers that arrive after sign-in still need to be told there is a user. */
        if ( g_current_user )
            raise_change_event( reg, g_current_user, XUserChangeEvent::SignedInAgain );

        return S_OK;
    }

    BOOLEAN WINAPI XUserUnregisterForChangeEvent( XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        TRACE( "token %llu, wait %d.\n", (UINT64)token.token, wait );

        std::lock_guard<std::mutex> lock( g_change_events_lock );
        for ( auto it = g_change_events.begin(); it != g_change_events.end(); ++it )
        {
            if ( it->token == token.token )
            {
                g_change_events.erase( it );
                return TRUE;
            }
        }
        return FALSE;
    }

    HRESULT WINAPI XUserGetSignOutDeferral( XUserSignOutDeferralHandle *deferral ) override
    {
        TRACE( "deferral %p.\n", deferral );
        *deferral = NULL;
        return E_GAMEUSER_DEFERRAL_NOT_AVAILABLE;
    }

    void WINAPI XUserCloseSignOutDeferralHandle( XUserSignOutDeferralHandle deferral ) override
    {
        TRACE( "deferral %p.\n", deferral );
    }

    HRESULT WINAPI XUserAddByIdWithUiAsync( UINT64 userId, XAsyncBlock *async ) override
    {
        FIXME( "userId %llu, async %p stub!\n", userId, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserAddByIdWithUiResult( XAsyncBlock *async, XUserHandle *newUser ) override
    {
        FIXME( "async %p, newUser %p stub!\n", async, newUser );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetMsaTokenSilentlyAsync( XUserHandle user, XUserGetMsaTokenSilentlyOptions options, const char *scope, XAsyncBlock *async ) override
    {
        FIXME( "user %p, options %u, scope %s, async %p stub!\n", user, (int)options, debugstr_a( scope ), async );
        /* The caller waits on the block; a synchronous failure leaves it pending
         * and the request is torn down half-initialized. */
        return XAsyncRun( async, not_implemented_async_work );
    }

    HRESULT WINAPI XUserGetMsaTokenSilentlyResult( XAsyncBlock *async, SIZE_T resultTokenSize, char *resultToken, SIZE_T *resultTokenUsed ) override
    {
        FIXME( "async %p, resultTokenSize %Iu, resultToken %p, resultTokenUsed %p stub!\n", async, resultTokenSize, resultToken, resultTokenUsed );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetMsaTokenSilentlyResultSize( XAsyncBlock *async, SIZE_T *tokenSize ) override
    {
        FIXME( "async %p, tokenSize %p stub!\n", async, tokenSize );
        if ( tokenSize ) *tokenSize = 0;
        return E_NOTIMPL;
    }

    BOOLEAN WINAPI XUserIsStoreUser( XUserHandle user ) override
    {
        FIXME( "user %p stub!\n", user );
        return TRUE;
    }

    HRESULT WINAPI XUserPlatformRemoteConnectSetEventHandlers( XTaskQueueHandle queue, XUserPlatformRemoteConnectEventHandlers *handlers ) override
    {
        FIXME( "queue %p, handlers %p stub!\n", queue, handlers );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserPlatformRemoteConnectCancelPrompt( XUserPlatformOperation operation ) override
    {
        FIXME( "operation %p stub!\n", operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserPlatformSpopPromptSetEventHandlers( XTaskQueueHandle queue, XUserPlatformSpopPromptEventHandler *handler, void *context ) override
    {
        FIXME( "queue %p, handler %p, context %p stub!\n", queue, handler, context );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserPlatformSpopPromptComplete( XUserPlatformOperation operation, XUserPlatformOperationResult result ) override
    {
        FIXME( "operation %p, result %d stub!\n", operation, (int)result );
        return E_NOTIMPL;
    }

    BOOLEAN WINAPI XUserIsSignOutPresent() override
    {
        TRACE( "\n" );
        return FALSE;
    }

    HRESULT WINAPI XUserSignOutAsync( XUserHandle user, XAsyncBlock *async ) override
    {
        FIXME( "user %p, async %p stub!\n", user, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserSignOutResult( XAsyncBlock *async ) override
    {
        FIXME( "async %p stub!\n", async );
        return E_NOTIMPL;
    }

    std::atomic_long ref{ 1 };
    std::atomic<UINT64> m_NextChangeEventToken{ 0 };
};

static XUserImpl g_x_user;

IXUserImpl *x_user = static_cast<IXUserImpl*>(&g_x_user);

/* Titles that start on a "press to begin" screen use this to work out which user a
 * given input device belongs to. xodus signs in exactly one user, so every device
 * maps to that one. */
class XUserDeviceImpl :
    public IXUserDeviceImpl2
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) override
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IXUserDeviceImpl ) ||
             iid == __uuidof( IXUserDeviceImpl2 ) )
        {
            AddRef();
            *out = static_cast<IXUserDeviceImpl2 *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        return E_NOINTERFACE;
    }

    ULONG WINAPI AddRef() noexcept override { return static_cast<ULONG>(++ref); }
    ULONG WINAPI Release() noexcept override { return static_cast<ULONG>(--ref); }

    HRESULT WINAPI XUserFindForDevice( const APP_LOCAL_DEVICE_ID *deviceId, XUserHandle *handle ) override
    {
        TRACE( "deviceId %p, handle %p.\n", deviceId, handle );

        if ( !handle ) return E_POINTER;
        if ( !g_current_user ) return E_GAMEUSER_NO_DEFAULT_USER;

        *handle = g_current_user;
        return S_OK;
    }

    HRESULT WINAPI XUserRegisterForDeviceAssociationChanged( XTaskQueueHandle queue, void *context,
                                                             XUserDeviceAssociationChangedCallback *callback,
                                                             XTaskQueueRegistrationToken *token ) override
    {
        FIXME( "queue %p, context %p, callback %p semi-stub: the association never changes.\n",
               queue, context, callback );

        /* The caller unregisters with this later, so it has to be written even though
         * no change will ever be reported. */
        if ( token ) token->token = ++m_nextToken;
        return S_OK;
    }

    BOOLEAN WINAPI XUserUnregisterForDeviceAssociationChanged( XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        TRACE( "token %llu, wait %d.\n", (UINT64)token.token, wait );
        return TRUE;
    }

    HRESULT WINAPI XUserGetDefaultAudioEndpointUtf16( XUserLocalId user, XUserDefaultAudioEndpointKind kind,
                                                      SIZE_T count, WCHAR *endpointId, SIZE_T *used ) override
    {
        FIXME( "user %llu, kind %d, count %Iu stub!\n", (UINT64)user.value, (int)kind, count );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserRegisterForDefaultAudioEndpointUtf16Changed( XTaskQueueHandle queue, void *context,
                                                                     XUserDefaultAudioEndpointUtf16ChangedCallback *callback,
                                                                     XTaskQueueRegistrationToken *token ) override
    {
        FIXME( "queue %p, context %p, callback %p semi-stub: the endpoint never changes.\n",
               queue, context, callback );
        if ( token ) token->token = ++m_nextToken;
        return S_OK;
    }

    BOOLEAN WINAPI XUserUnregisterForDefaultAudioEndpointUtf16Changed( XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        TRACE( "token %llu, wait %d.\n", (UINT64)token.token, wait );
        return TRUE;
    }

    HRESULT WINAPI XUserFindControllerForUserWithUiAsync( XUserHandle user, XAsyncBlock *async ) override
    {
        FIXME( "user %p, async %p semi-stub: answering with the device the title already has.\n",
               user, async );
        return XAsyncRun( async, controller_work );
    }

    HRESULT WINAPI XUserFindControllerForUserWithUiResult( XAsyncBlock *async, APP_LOCAL_DEVICE_ID *deviceId ) override
    {
        TRACE( "async %p, deviceId %p.\n", async, deviceId );

        if ( !deviceId ) return E_POINTER;
        /* A zeroed id means "whichever device the title is already reading", which is
         * the only answer that fits a single-user desktop. */
        memset( deviceId, 0, sizeof(*deviceId) );
        return S_OK;
    }

private:
    static HRESULT __stdcall controller_work( XAsyncBlock * ) { return S_OK; }

    std::atomic_long ref{ 1 };
    std::atomic<UINT64> m_nextToken{ 0 };
};

static XUserDeviceImpl g_x_user_device;

IXUserDeviceImpl2 *x_user_device = static_cast<IXUserDeviceImpl2 *>(&g_x_user_device);
