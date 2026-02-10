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

/*
 * Xbox Game runtime Library
 * GDK Component: System API -> XUser
 */

#include "XUser.h"
#include "winhttp.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static const struct IXUserImplVtbl x_user_vtbl;

static HRESULT LoadDefaultUser(XUserHandle *user, LPCSTR client_id)
{
    struct x_user *impl;
    LSTATUS status;
    LPSTR buffer;
    HRESULT hr;
    DWORD size;

    if (ERROR_SUCCESS != (status = RegGetValueA(
        HKEY_LOCAL_MACHINE,
        "Software\\Wine\\WineGDK",
        "RefreshToken",
        RRF_RT_REG_SZ,
        NULL,
        NULL,
        &size
    ))) return HRESULT_FROM_WIN32(status);

    if (!(buffer = calloc(1, size))) return E_OUTOFMEMORY;

    if (ERROR_SUCCESS != (status = RegGetValueA(
        HKEY_LOCAL_MACHINE,
        "Software\\Wine\\WineGDK",
        "RefreshToken",
        RRF_RT_REG_SZ,
        NULL,
        buffer,
        &size
    )))
    {
        free(buffer);
        return HRESULT_FROM_WIN32(status);
    }

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        free(buffer);
        return E_OUTOFMEMORY;
    }

    impl->IXUserImpl_iface.lpVtbl = &x_user_vtbl;
    impl->ref = 1;

    hr = RefreshOAuth(
        client_id, buffer, &impl->oauth_token_expiry, &impl->refresh_token, &impl->oauth_token);

    free(buffer);
    if (FAILED(hr))
    {
        TRACE("failed to get oauth token\n");
        IXUserImpl_Release(&impl->IXUserImpl_iface);
        return hr;
    }

    if (FAILED(hr = RequestUserToken(impl->oauth_token, &impl->user_token, &impl->local_id)))
    {
        TRACE("failed to get user token\n");
        IXUserImpl_Release(&impl->IXUserImpl_iface);
        return hr;
    }

    if (FAILED(hr = RequestXstsToken(impl->user_token, &impl->xsts_token, &impl->xuid)))
    {
        TRACE("failed to get xsts token\n");
        IXUserImpl_Release(&impl->IXUserImpl_iface);
        return hr;
    }

    *user = (XUserHandle)impl;

    return hr;
}

static inline struct x_user *impl_from_IXUserImpl(IXUserImpl *iface)
{
    return CONTAINING_RECORD(iface, struct x_user, IXUserImpl_iface);
}

static HRESULT WINAPI x_user_QueryInterface(IXUserImpl *iface, REFIID iid, void **out)
{
    struct x_user *impl = impl_from_IXUserImpl(iface);

    TRACE("iface %p, iid %s, out %p\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
    || IsEqualGUID(iid, &CLSID_XUserImpl)
    || IsEqualGUID(iid, &IID_IXUserImpl))
    {
        *out = &impl->IXUserImpl_iface;
        IXUserImpl_AddRef(*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_user_AddRef(IXUserImpl *iface)
{
    struct x_user *impl = impl_from_IXUserImpl(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p increasing refcount to %lu\n", iface, ref);
    return ref;
}

static ULONG WINAPI x_user_Release(IXUserImpl *iface)
{
    struct x_user *impl = impl_from_IXUserImpl(iface);
    ULONG ref = InterlockedDecrement(&impl-> ref);
    TRACE("iface %p decreasing refcount to %lu\n", iface, ref);
    if (!ref)
    {
        WindowsDeleteString(impl->refresh_token);
        WindowsDeleteString(impl->oauth_token);
        WindowsDeleteString(impl->user_token);
        WindowsDeleteString(impl->xsts_token);
        free(impl);
    }
    return ref;
}

static HRESULT WINAPI x_user_XUserDuplicateHandle(IXUserImpl* iface, XUserHandle user, XUserHandle* duplicated)
{
    TRACE("iface %p, user %p, duplicated %p\n", iface, user, duplicated);
    IXUserImpl_AddRef(&((struct x_user*)user)->IXUserImpl_iface);
    *duplicated = user;
    return S_OK;
}

static void WINAPI x_user_XUserCloseHandle(IXUserImpl* iface, XUserHandle user)
{
    TRACE("iface %p, user %p\n", iface, user);
    IXUserImpl_Release(&((struct x_user*)user)->IXUserImpl_iface);
}

static INT32 WINAPI x_user_XUserCompare(IXUserImpl* iface, XUserHandle user1, XUserHandle user2)
{
    FIXME("iface %p, user1 %p, user2 %p stub!\n", iface, user1, user2);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetMaxUsers(IXUserImpl* iface, UINT32* maxUsers)
{
    FIXME("iface %p, maxUsers %p stub!\n", iface, maxUsers);
    return E_NOTIMPL;
}

struct XUserAddContext {
    XUserAddOptions options;
    XUserHandle user;
    LPCSTR client_id;
};

HRESULT XUserAddProvider(XAsyncOp operation, const XAsyncProviderData* providerData)
{
    struct XUserAddContext* context;
    IXThreadingImpl* impl;
    HRESULT hr;

    TRACE("operation %d, providerData %p\n", operation, providerData);

    if (FAILED(QueryApiImpl(&CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void**)&impl))) return E_FAIL;
    context = providerData->context;

    switch (operation)
    {
        case Begin:
            return impl->lpVtbl->XAsyncSchedule(impl, providerData->async, 0);

        case GetResult:
            memcpy(providerData->buffer, &context->user, sizeof(XUserHandle));
            break;

        case DoWork:
            if (context->options & XUserAddOptions_AddDefaultUserAllowingUI)
                hr = LoadDefaultUser(&context->user, context->client_id);
            else if (context->options & XUserAddOptions_AddDefaultUserSilently)
                hr = LoadDefaultUser(&context->user, context->client_id);
            else hr = E_ABORT;

            impl->lpVtbl->XAsyncComplete(impl, providerData->async, hr, sizeof(XUserHandle));
            break;

        case Cleanup:
            free(context);
            break;

        case Cancel:
            break;
    }

    return S_OK;
}

static HRESULT WINAPI x_user_XUserAddAsync(IXUserImpl* iface, XUserAddOptions options, XAsyncBlock* asyncBlock)
{
    struct XUserAddContext* context;
    IXThreadingImpl* impl;

    TRACE("iface %p, options %d, asyncBlock %p\n", iface, options, asyncBlock);

    if (FAILED(QueryApiImpl(&CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void**)&impl))) return E_NOTIMPL;
    if (!(context = calloc(1, sizeof(struct XUserAddContext)))) return E_OUTOFMEMORY;
    context->options = options;

    return impl->lpVtbl->XAsyncBegin(impl, asyncBlock, context, x_user_XUserAddAsync, "XUserAddAsync", XUserAddProvider);
}

static HRESULT WINAPI x_user_XUserAddResult(IXUserImpl* iface, XAsyncBlock* asyncBlock, XUserHandle* user)
{
    IXThreadingImpl* impl;

    TRACE("iface %p, asyncBlock %p, user %p\n", iface, asyncBlock, *user);

    if (FAILED(QueryApiImpl(&CLSID_XThreadingImpl, &IID_IXThreadingImpl, (void**)&impl))) return E_NOTIMPL;
    return impl->lpVtbl->XAsyncGetResult(impl, asyncBlock, x_user_XUserAddAsync, sizeof(XUserHandle), user, NULL);
}

static HRESULT WINAPI x_user_XUserGetLocalId(IXUserImpl* iface, XUserHandle user, XUserLocalId* localId)
{
    TRACE("iface %p, user %p, localId %p\n", iface, user, localId);
    if (!localId) return E_POINTER;
    *localId = ((struct x_user*)user)->local_id;
    return S_OK;
}

static HRESULT WINAPI x_user_XUserFindUserByLocalId(IXUserImpl* iface, XUserLocalId localId, XUserHandle* user)
{
    FIXME("iface %p, localId %p, user %p stub!\n", iface, &localId, user);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetId(IXUserImpl* iface, XUserHandle user, UINT64* userId)
{
    TRACE("iface %p, user %p, userId %p\n", iface, user, userId);
    if (!userId) return E_POINTER;
    *userId = ((struct x_user*)user)->xuid;
    return S_OK;
}

static HRESULT WINAPI x_user_XUserFindUserById(IXUserImpl* iface, UINT64 userId, XUserHandle* user)
{
    FIXME("iface %p, userId %llu, user %p stub!\n", iface, userId, user);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetIsGuest(IXUserImpl* iface, XUserHandle user, BOOLEAN* isGuest)
{
    FIXME("iface %p, user %p, isGuest %p stub!\n", iface, user, isGuest);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetState(IXUserImpl* iface, XUserHandle user, XUserState* state)
{
    FIXME("iface %p, user %p, state %p stub!\n", iface, user, state);
    return E_NOTIMPL;
}

static HRESULT WINAPI __PADDING__(IXUserImpl* iface)
{
    WARN("iface %p padding function called! It's unknown what this function does\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetGamerPictureAsync(IXUserImpl* iface, XUserHandle user, XUserGamerPictureSize size, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, user %p, size %p, asyncBlock %p stub!\n", iface, user, &size, asyncBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetGamerPictureResultSize(IXUserImpl* iface, XAsyncBlock* asyncBlock, SIZE_T* size)
{
    FIXME("iface %p, asyncBlock %p, size %p stub!\n", iface, asyncBlock, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetGamerPictureResult(IXUserImpl* iface, XAsyncBlock* asyncBlock, SIZE_T size, void* buffer, SIZE_T* used)
{
    FIXME("iface %p, asyncBlock %p, size %llu, buffer %p, used %p stub!\n", iface, asyncBlock, size, buffer, used);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetAgeGroup(IXUserImpl* iface, XUserHandle user, XUserAgeGroup* group)
{
    FIXME("user %p, group %p stub!\n", user, group);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserCheckPrivilege(IXUserImpl* iface, XUserHandle user, XUserPrivilegeOptions options, XUserPrivilege privilege, BOOLEAN* hasPrivilege, XUserPrivilegeDenyReason* reason)
{
    FIXME("iface %p, user %p, options %d, privilege %d, hasPrivilege %p, reason %p stub!\n", iface, user, options, privilege, hasPrivilege, reason);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolvePrivilegeWithUiAsync(IXUserImpl* iface, XUserHandle user, XUserPrivilegeOptions options, XUserPrivilege privilege, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, user %p, options %d, privilege %d, asyncBlock %p stub!\n", iface, user, options, privilege, asyncBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolvePrivilegeWithUiResult(IXUserImpl* iface, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, asyncBlock %p stub!\n", iface, asyncBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureAsync(IXUserImpl* iface, XUserHandle user, XUserGetTokenAndSignatureOptions options, LPCSTR method, LPCSTR url, SIZE_T count, const XUserGetTokenAndSignatureHttpHeader* headers, SIZE_T size, const void* buffer, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, user %p, options %d, method %s, url %s, count %llu, headers %p, size %llu, buffer %p, asyncBlock %p stub!\n", iface, user, options, method, url, count, headers, size, buffer, asyncBlock);
    return E_NOTIMPL;
}
static HRESULT WINAPI x_user_XUserGetTokenAndSignatureResultSize(IXUserImpl* iface, XAsyncBlock* asyncBlock, SIZE_T* size)
{
    FIXME("iface %p, asyncBlock %p, size %p stub!\n", iface, asyncBlock, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureResult(IXUserImpl* iface, XAsyncBlock* asyncBlock, SIZE_T size, void* buffer, XUserGetTokenAndSignatureData** ptr, SIZE_T* used)
{
    FIXME("iface %p, asyncBlock %p, size %llu, buffer %p, ptr %p, used %p stub!\n", iface, asyncBlock, size, buffer, ptr, used);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureUtf16Async(IXUserImpl* iface, XUserHandle user, XUserGetTokenAndSignatureOptions options, LPCWSTR method, LPCWSTR url, SIZE_T count, const XUserGetTokenAndSignatureUtf16HttpHeader* headers, SIZE_T size, const void* buffer, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, user %p, options %d, method %hs, url %hs, count %llu, headers %p, size %llu, buffer %p, asyncBlock %p stub!\n", iface, user, options, method, url, count, headers, size, buffer, asyncBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureUtf16ResultSize(IXUserImpl* iface, XAsyncBlock* asyncBlock, SIZE_T* size)
{
    FIXME("iface %p, asyncBlock %p, size %p stub!\n", iface, asyncBlock, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetTokenAndSignatureUtf16Result(IXUserImpl* iface, XAsyncBlock* asyncBlock, SIZE_T size, void* buffer, XUserGetTokenAndSignatureUtf16Data** ptr, SIZE_T* used)
{
    FIXME("iface %p, asyncBlock %p, size %llu, buffer %p, ptr %p, used %p stub!\n", iface, asyncBlock, size, buffer, ptr, used);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolveIssueWithUiAsync(IXUserImpl* iface, XUserHandle user, LPCSTR url, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, user %p, url %s, asyncBlock %p stub!\n", iface, user, url, asyncBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolveIssueWithUiResult(IXUserImpl* iface, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, asyncBlock %p stub!\n", iface, asyncBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolveIssueWithUiUtf16Async(IXUserImpl* iface, XUserHandle user, LPCWSTR url, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, user %p, url %hs, asyncBlock %p stub!\n", iface, user, url, asyncBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserResolveIssueWithUiUtf16Result(IXUserImpl* iface, XAsyncBlock* asyncBlock)
{
    FIXME("iface %p, asyncBlock %p stub!\n", iface, asyncBlock);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserRegisterForChangeEvent(IXUserImpl* iface, XTaskQueueHandle queue, void* context, XUserChangeEventCallback* callback, XTaskQueueRegistrationToken* token)
{
    FIXME("iface %p, context %p, callback %p, token %p stub!\n", iface, context, callback, token);
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_user_XUserUnregisterForChangeEvent(IXUserImpl* iface, XTaskQueueRegistrationToken token, BOOLEAN wait)
{
    FIXME("iface %p, token %p, wait %d stub!\n", iface, &token, wait);
    return FALSE;
}

static HRESULT WINAPI x_user_XUserGetSignOutDeferral(IXUserImpl* iface, XUserSignOutDeferralHandle* deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
    return E_GAMEUSER_DEFERRAL_NOT_AVAILABLE;
}

static void WINAPI x_user_XUserCloseSignOutDeferralHandle(IXUserImpl* iface, XUserSignOutDeferralHandle deferral)
{
    FIXME("iface %p, deferral %p stub!\n", iface, deferral);
}

static const struct IXUserImplVtbl x_user_vtbl =
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
    x_user_XUserCloseSignOutDeferralHandle
};

static struct x_user x_user = {
    {&x_user_vtbl},
    0,
};

IXUserImpl *x_user_impl = &x_user.IXUserImpl_iface;