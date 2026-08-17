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

/* Which service the caller is authenticating to decides which stored token fits;
 * both travel to GetResult as the provider context. */
struct TokenAndSignatureContext
{
    XUserHandle user;
    bool playfab;
};

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
            xthreading->XAsyncComplete( data->async, S_OK,
                                        sizeof(XUserGetTokenAndSignatureData)
                                        + XSTS_TOKEN_MAX + 1 );
            /* Completed here, so the framework must not complete it again. */
            return E_PENDING;

        case XAsyncOp::GetResult:
        {
            auto *result = (XUserGetTokenAndSignatureData *)data->buffer;
            char *token = (char *)data->buffer + sizeof(*result);
            auto *ctx = (TokenAndSignatureContext *)data->context;
            XUserHandle user = ctx ? ctx->user : nullptr;
            HSTRING stored = nullptr;
            UINT32 length = 0;
            LPCWSTR raw;
            INT written = 0;

            token[0] = '\0';

            if ( user && user->m_signature == X_USER_SIGNATURE && user->m_user &&
                 SUCCEEDED( ctx->playfab ? user->m_user->GetPlayfabToken( &stored )
                                         : user->m_user->GetXstsToken( &stored ) ) )
            {
                raw = WindowsGetStringRawBuffer( stored, &length );
                if ( raw && length )
                    written = WideCharToMultiByte( CP_UTF8, 0, raw, length,
                                                   token, XSTS_TOKEN_MAX, nullptr, nullptr );
                WindowsDeleteString( stored );
            }

            token[written] = '\0';

            /* No request signature: Xbox signs requests with the device key, which
             * xodus does not hold. The token alone is what most services check. */
            result->tokenSize = written;
            result->signatureSize = 0;
            result->token = token;
            result->signature = token + written;
            return S_OK;
        }

        case XAsyncOp::Cleanup:
            delete (TokenAndSignatureContext *)data->context;
            break;

        default:
            break;
    }

    return S_OK;
}

static HRESULT begin_token_and_signature( XUserHandle user, const char *url, XAsyncBlock *async )
{
    IXThreadingImpl *xthreading;
    HRESULT hr;

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) )
        return hr;

    auto *ctx = new (std::nothrow) TokenAndSignatureContext;
    if ( !ctx ) return E_OUTOFMEMORY;

    ctx->user = user;
    ctx->playfab = url && strstr( url, "playfab" ) != nullptr;

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
    public IXUserImpl6
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
        FIXME( "user1 %p, user2 %p stub!\n", user1, user2 );
        return E_NOTIMPL;
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
        return ::XUserAddResult( async, newUser );
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
        FIXME( "userId %llu, handle %p stub!\n", userId, handle );
        return E_NOTIMPL;
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
        FIXME( "user %p, ageGroup %p stub!\n", user, ageGroup );
        return E_NOTIMPL;
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
        return begin_token_and_signature( user, url, async );
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

        return begin_token_and_signature( user, urlA, async );
    }

    HRESULT WINAPI XUserGetTokenAndSignatureUtf16ResultSize( XAsyncBlock *async, SIZE_T *bufferSize ) override
    {
        FIXME( "async %p, bufferSize %p stub!\n", async, bufferSize );
        if ( bufferSize ) *bufferSize = 0;
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetTokenAndSignatureUtf16Result( XAsyncBlock *async, SIZE_T bufferSize, void *buffer, XUserGetTokenAndSignatureUtf16Data **ptrToBuffer, SIZE_T *bufferUsed ) override
    {
        FIXME( "async %p, bufferSize %Iu, buffer %p, ptrToBuffer %p, bufferUsed %p stub!\n", async, bufferSize, buffer, ptrToBuffer, bufferUsed );
        return E_NOTIMPL;
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
        FIXME( "queue %p, context %p, callback %p semi-stub: no change events are raised.\n", queue, context, callback );

        /* The single xodus user never signs out mid-session, so nothing will fire.
         * The caller still keeps the token and unregisters with it later, so it has
         * to be a real value rather than left uninitialized. */
        if ( token ) token->token = ++m_NextChangeEventToken;
        return S_OK;
    }

    BOOLEAN WINAPI XUserUnregisterForChangeEvent( XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        TRACE( "token %llu, wait %d.\n", (UINT64)token.token, wait );
        return TRUE;
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