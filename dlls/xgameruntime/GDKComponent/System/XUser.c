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

static BOOLEAN HttpRequest(LPCWSTR method, LPCWSTR domain, LPCWSTR object, LPSTR data, LPCWSTR headers, LPCWSTR* accept, LPSTR* buffer, SIZE_T* bufferSize)
{
    HINTERNET connection = NULL;
    DWORD size = sizeof(DWORD);
    HINTERNET session = NULL;
    HINTERNET request = NULL;
    BOOLEAN response = FALSE;
    BOOLEAN result = TRUE;
    DWORD status;

    session = WinHttpOpen(
        L"WineGDK/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (session)
        connection = WinHttpConnect(
            session,
            domain,
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        );

    if (connection)
        request = WinHttpOpenRequest(
            connection,
            method,
            object,
            NULL,
            WINHTTP_NO_REFERER,
            accept,
            WINHTTP_FLAG_SECURE
        );

    if (request)
        response = WinHttpSendRequest(
            request,
            headers,
            -1,
            data,
            strlen(data),
            strlen(data),
            0
        );

    if (response)
        response = WinHttpReceiveResponse(request, NULL);

    if (response)
        response = WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status,
            &size,
            WINHTTP_NO_HEADER_INDEX
        );

    if (response && status / 100 != 2) response = FALSE;

    /* buffer response data */
    *buffer = NULL;
    *bufferSize = 0;
    if (response)
    {
        do
        {
            if (!(result = WinHttpQueryDataAvailable(request, &size))) break;
            if (!size) break;

            if (!(*buffer = realloc(*buffer, *bufferSize + size)))
            {
                result = FALSE;
                break;
            }

            if (!(result = WinHttpReadData(request, *buffer + *bufferSize, size, &size)))
                break;

            *bufferSize += size;
        }
        while (size);
    }
    else result = FALSE;

    if (connection) WinHttpCloseHandle(connection);
    if (request) WinHttpCloseHandle(request);
    if (session) WinHttpCloseHandle(session);
    if (!result && *buffer) free(*buffer);

    return result;
}

static BOOLEAN RefreshOAuth(LPCSTR client_id, LPCSTR refresh_token, time_t *new_expiry, HSTRING *new_refresh_token, HSTRING *new_oauth_token)
{
    LPCSTR template = "grant_type=refresh_token&scope=service::user.auth.xboxlive.com::MBI_SSL&client_id=";
    LPCWSTR class_str = RuntimeClass_Windows_Data_Json_JsonValue;
    LPCWSTR accept[] = {L"application/json", NULL};
    IJsonValueStatics *statics;
    HSTRING_HEADER content_hdr;
    HSTRING_HEADER expires_hdr;
    HSTRING_HEADER refresh_hdr;
    HSTRING_HEADER class_hdr;
    HSTRING_HEADER oauth_hdr;
    IJsonObject *object;
    IJsonValue *value;
    HSTRING content;
    HSTRING expires;
    HSTRING refresh;
    LPWSTR w_buffer;
    BOOLEAN result;
    HSTRING class;
    HSTRING oauth;
    LPSTR buffer;
    DOUBLE delta;
    SIZE_T size;
    HRESULT hr;
    LPSTR data;
    INT w_size;
    time_t tmp;

    if (!(data = calloc(1, strlen(template) + strlen(client_id) +
        strlen("&refresh_token=") + strlen(refresh_token) + 1))) return FALSE;

    strcpy(data, template);
    strcat(data, client_id);
    strcat(data, "&refresh_token=");
    strcat(data, refresh_token);

    result = HttpRequest(
        L"POST",
        L"login.live.com",
        L"/oauth20_token.srf",
        data,
        L"content-type: application/x-www-form-urlencoded",
        accept,
        &buffer,
        &size
    );

    free(data);
    if (!result) return FALSE;

    if (!(w_size = MultiByteToWideChar(CP_UTF8, 0, buffer, size, NULL, 0)))
    {
        free(buffer);
        return FALSE;
    }

    if (!(w_buffer = calloc(w_size, sizeof(WCHAR))))
    {
        free(buffer);
        return FALSE;
    }

    w_size = MultiByteToWideChar(CP_UTF8, 0, buffer, size, w_buffer, w_size);
    free(buffer);
    if (!w_size)
    {
        free(w_buffer);
        return FALSE;
    }

    hr = WindowsCreateStringReference(w_buffer, w_size, &content_hdr, &content);
    if (FAILED(hr))
    {
        free(w_buffer);
        return FALSE;
    }

    if (FAILED(WindowsCreateStringReference(
        L"access_token", wcslen(L"access_token"), &oauth_hdr, &oauth)))
    {
        free(w_buffer);
        return FALSE;
    }

    if (FAILED(WindowsCreateStringReference(
        L"refresh_token", wcslen(L"refresh_token"), &refresh_hdr, &refresh)))
    {
        free(w_buffer);
        return FALSE;
    }

    if (FAILED(WindowsCreateStringReference(
        L"expires_in", wcslen(L"expires_in"), &expires_hdr, &expires)))
    {
        free(w_buffer);
        return FALSE;
    }

    if (FAILED(WindowsCreateStringReference(class_str, wcslen(class_str), &class_hdr, &class)))
    {
        free(w_buffer);
        return FALSE;
    }

    if (FAILED(RoGetActivationFactory(class, &IID_IJsonValueStatics, (void**)&statics)))
    {
        free(w_buffer);
        return FALSE;
    }

    hr = IJsonValueStatics_Parse(statics, content, &value);
    free(w_buffer);
    IJsonValueStatics_Release(statics);
    if (FAILED(hr))
    {
        IJsonValue_Release(value);
        return FALSE;
    }

    hr = IJsonValue_GetObject(value, &object);
    IJsonValue_Release(value);
    if (FAILED(hr))
    {
        IJsonObject_Release(object);
        return FALSE;
    }

    if (FAILED(IJsonObject_GetNamedString(object, refresh, new_refresh_token)))
    {
        IJsonObject_Release(object);
        return FALSE;
    }

    if (FAILED(IJsonObject_GetNamedString(object, oauth, new_oauth_token)))
    {
        IJsonObject_Release(object);
        return FALSE;
    }

    hr = IJsonObject_GetNamedNumber(object, expires, &delta);
    IJsonObject_Release(object);
    if (FAILED(hr)) return FALSE;

    if ((tmp = time(NULL)) == -1) return FALSE;
    *new_expiry = tmp + delta;

    return TRUE;
}

static BOOLEAN RequestUserToken(HSTRING oauth_token, HSTRING *new_user_token)
{
    LPCSTR template = "{\"RelyingParty\":\"http://auth.xboxlive.com\",\"TokenType\":\"JWT\",\"Properties\":{\"AuthMethod\":\"RPS\",\"SiteName\":\"user.auth.xboxlive.com\",\"RpsTicket\":\"";
    LPCWSTR class_str = RuntimeClass_Windows_Data_Json_JsonValue;
    LPCWSTR accept[] = {L"application/json", NULL};
    IJsonValueStatics *statics;
    HSTRING_HEADER content_hdr;
    HSTRING_HEADER class_hdr;
    HSTRING_HEADER user_hdr;
    IJsonObject *object;
    IJsonValue *value;
    LPWSTR w_buffer;
    HSTRING content;
    BOOLEAN result;
    HSTRING class;
    HSTRING user;
    LPSTR buffer;
    SIZE_T size;
    LPSTR data;
    HRESULT hr;
    INT w_size;

    LPSTR oauth_token_str;
    UINT32 oauth_token_str_len;
    UINT32 oauth_token_wstr_len;
    LPCWSTR oauth_token_wstr = WindowsGetStringRawBuffer(oauth_token, &oauth_token_wstr_len);

    if (!(oauth_token_str_len = WideCharToMultiByte(
        CP_UTF8, 0, oauth_token_wstr, oauth_token_wstr_len, NULL, 0, NULL, NULL))) return FALSE;

    if (!(oauth_token_str = calloc(1, oauth_token_str_len))) return FALSE;

    if (!(oauth_token_str_len = WideCharToMultiByte(CP_UTF8, 0, oauth_token_wstr,
        oauth_token_wstr_len, oauth_token_str, oauth_token_str_len, NULL, NULL)))
    {
        free(oauth_token_str);
        return FALSE;
    }

    if (!(data = calloc(1, strlen(template) + strlen(oauth_token_str) + strlen("\"}}"))))
        return FALSE;

    strcpy(data, template);
    strncat(data, oauth_token_str, oauth_token_str_len);
    free(oauth_token_str);
    strcat(data, "\"}}");

    result = HttpRequest(
        L"POST",
        L"user.auth.xboxlive.com",
        L"/user/authenticate",
        data,
        L"content-type: application/json",
        accept,
        &buffer,
        &size
    );

    free(data);
    if (!result) return FALSE;

    if (!(w_size = MultiByteToWideChar(CP_UTF8, 0, buffer, size, NULL, 0)))
    {
        free(buffer);
        return FALSE;
    }

    if (!(w_buffer = calloc(w_size, sizeof(WCHAR))))
    {
        free(buffer);
        return FALSE;
    }

    w_size = MultiByteToWideChar(CP_UTF8, 0, buffer, size, w_buffer, w_size);
    free(buffer);
    if (!w_size)
    {
        free(w_buffer);
        return FALSE;
    }

    hr = WindowsCreateStringReference(w_buffer, w_size, &content_hdr, &content);
    if (FAILED(hr))
    {
        free(w_buffer);
        return FALSE;
    }

    if (FAILED(WindowsCreateStringReference(
        L"Token", wcslen(L"Token"), &user_hdr, &user)))
    {
        free(w_buffer);
        return FALSE;
    }

    if (FAILED(WindowsCreateStringReference(class_str, wcslen(class_str), &class_hdr, &class)))
    {
        free(w_buffer);
        return FALSE;
    }

    if (FAILED(RoGetActivationFactory(class, &IID_IJsonValueStatics, (void**)&statics)))
    {
        free(w_buffer);
        return FALSE;
    }

    hr = IJsonValueStatics_Parse(statics, content, &value);
    free(w_buffer);
    IJsonValueStatics_Release(statics);
    if (FAILED(hr))
    {
        IJsonValue_Release(value);
        return FALSE;
    }

    hr = IJsonValue_GetObject(value, &object);
    IJsonValue_Release(value);
    if (FAILED(hr))
    {
        IJsonObject_Release(object);
        return FALSE;
    }

    if (FAILED(hr = IJsonObject_GetNamedString(object, user, new_user_token)))
    {
        IJsonObject_Release(object);
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN LoadDefaultUser(XUserHandle *user, LPCSTR client_id)
{
    struct x_user *impl;
    BOOLEAN result;
    LSTATUS status;
    LPSTR buffer;
    DWORD size;

    if (ERROR_SUCCESS != (status = RegGetValueA(
        HKEY_LOCAL_MACHINE,
        "Software\\Wine\\WineGDK",
        "RefreshToken",
        RRF_RT_REG_SZ,
        NULL,
        NULL,
        &size
    ))) return FALSE;

    if (!(buffer = calloc(1, size))) return FALSE;

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
        return FALSE;
    }

    if (!(impl = calloc(1, sizeof(*impl)))) return FALSE;
    impl->IXUserImpl_iface.lpVtbl = &x_user_vtbl;
    impl->ref = 1;

    result = RefreshOAuth(
        client_id, buffer, &impl->oauth_token_expiry, &impl->refresh_token, &impl->oauth_token);

    free(buffer);
    if (result)
        result = RequestUserToken(impl->oauth_token, &impl->user_token);

    if (result) *user = (XUserHandle)impl;
    else IXUserImpl_Release(&impl->IXUserImpl_iface);

    return result;
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
        impl->IXUserImpl_iface.lpVtbl->AddRef(*out);
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
            {
                if (LoadDefaultUser(&context->user, context->client_id)) hr = S_OK;
                else hr = E_ABORT;
            }
            else if (context->options & XUserAddOptions_AddDefaultUserSilently)
            {
                if (LoadDefaultUser(&context->user, context->client_id)) hr = S_OK;
                else hr = E_GAMEUSER_NO_DEFAULT_USER;
            }
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
    FIXME("iface %p, user %p, localId %p stub!\n", iface, user, localId);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserFindUserByLocalId(IXUserImpl* iface, XUserLocalId localId, XUserHandle* user)
{
    FIXME("iface %p, localId %p, user %p stub!\n", iface, &localId, user);
    return E_NOTIMPL;
}

static HRESULT WINAPI x_user_XUserGetId(IXUserImpl* iface, XUserHandle user, UINT64* userId)
{
    FIXME("iface %p, user %p, userId %p stub!\n", iface, user, userId);
    return E_NOTIMPL;
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