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

#include <cstring>
#include <atomic>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

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
        FIXME( "handle %p, duplicatedHandle %p stub!\n", handle, duplicatedHandle );
        return E_NOTIMPL;
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
        FIXME( "options %d, async %p stub!\n", (int)options, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserAddResult( XAsyncBlock *async, XUserHandle *newUser ) override
    {
        FIXME( "async %p, newUser %p stub!\n", async, newUser );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetLocalId( XUserHandle user, XUserLocalId *userLocalId ) override
    {
        FIXME( "user %p, userLocalId %p stub!\n", user, userLocalId );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserFindUserByLocalId( XUserLocalId userLocalId, XUserHandle *handle ) override
    {
        FIXME( "userLocalId %p, handle %p stub!\n", &userLocalId, handle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetId( XUserHandle user, UINT64 *userId ) override
    {
        FIXME( "user %p, userId %p stub!\n", user, userId );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserFindUserById( UINT64 userId, XUserHandle *handle ) override
    {
        FIXME( "userId %llu, handle %p stub!\n", userId, handle );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetIsGuest( XUserHandle user, BOOLEAN *isGuest ) override
    {
        FIXME( "user %p, isGuest %p stub!\n", user, isGuest );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetState( XUserHandle user, XUserState *state ) override
    {
        FIXME( "user %p, state %p stub!\n", user, state );
        return E_NOTIMPL;
    }

    HRESULT WINAPI __PADDING__() override
    {
        WARN( "padding function called! It's unknown what this function does.\n" );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetGamerPictureAsync( XUserHandle user, XUserGamerPictureSize pictureSize, XAsyncBlock *async ) override
    {
        FIXME( "user %p, pictureSize %d, async %p stub!\n", user, (int)pictureSize, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetGamerPictureResultSize( XAsyncBlock *async, SIZE_T *bufferSize ) override
    {
        FIXME( "async %p, bufferSize %p stub!\n", async, bufferSize );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetGamerPictureResult( XAsyncBlock *async, SIZE_T bufferSize, void *buffer, SIZE_T *bufferUsed ) override
    {
        FIXME( "async %p, bufferSize %Iu, buffer %p, bufferUsed %p stub!\n", async, bufferSize, buffer, bufferUsed );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetAgeGroup( XUserHandle user, XUserAgeGroup *ageGroup ) override
    {
        FIXME( "user %p, ageGroup %p stub!\n", user, ageGroup );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserCheckPrivilege( XUserHandle user, XUserPrivilegeOptions options, XUserPrivilege privilege, BOOLEAN *hasPrivilege, XUserPrivilegeDenyReason *reason ) override
    {
        FIXME( "user %p, options %d, privilege %d, hasPrivilege %p, reason %p stub!\n", user, (int)options, (int)privilege, hasPrivilege, reason );
        return E_NOTIMPL;
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
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetTokenAndSignatureResultSize( XAsyncBlock *async, SIZE_T *bufferSize ) override
    {
        FIXME( "async %p, bufferSize %p stub!\n", async, bufferSize );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetTokenAndSignatureResult( XAsyncBlock *async, SIZE_T bufferSize, void *buffer, XUserGetTokenAndSignatureData **ptrToBuffer, SIZE_T *bufferUsed ) override
    {
        FIXME( "async %p, bufferSize %Iu, buffer %p, ptrToBuffer %p, bufferUsed %p stub!\n", async, bufferSize, buffer, ptrToBuffer, bufferUsed );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetTokenAndSignatureUtf16Async( XUserHandle user, XUserGetTokenAndSignatureOptions options, const WCHAR *method, const WCHAR *url, SIZE_T headerCount, const XUserGetTokenAndSignatureUtf16HttpHeader *headers, SIZE_T bodySize, const void *bodyBuffer, XAsyncBlock *async ) override
    {
        FIXME( "user %p, options %d, method %s, url %s, headerCount %Iu, headers %p, bodySize %Iu, bodyBuffer %p, async %p stub!\n", user, (int)options, debugstr_w( method ), debugstr_w( url ), headerCount, headers, bodySize, bodyBuffer, async );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetTokenAndSignatureUtf16ResultSize( XAsyncBlock *async, SIZE_T *bufferSize ) override
    {
        FIXME( "async %p, bufferSize %p stub!\n", async, bufferSize );
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
        FIXME( "queue %p, context %p, callback %p, token %p stub!\n", queue, context, callback, token );
        return E_NOTIMPL;
    }

    BOOLEAN WINAPI XUserUnregisterForChangeEvent( XTaskQueueRegistrationToken token, BOOLEAN wait ) override
    {
        FIXME( "token %p, wait %d stub!\n", &token, wait );
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
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetMsaTokenSilentlyResult( XAsyncBlock *async, SIZE_T resultTokenSize, char *resultToken, SIZE_T *resultTokenUsed ) override
    {
        FIXME( "async %p, resultTokenSize %Iu, resultToken %p, resultTokenUsed %p stub!\n", async, resultTokenSize, resultToken, resultTokenUsed );
        return E_NOTIMPL;
    }

    HRESULT WINAPI XUserGetMsaTokenSilentlyResultSize( XAsyncBlock *async, SIZE_T *tokenSize ) override
    {
        FIXME( "async %p, tokenSize %p stub!\n", async, tokenSize );
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

private:
    std::atomic_long ref{ 1 };
};

static XUserImpl g_x_user;

IXUserImpl *x_user = static_cast<IXUserImpl*>(&g_x_user);