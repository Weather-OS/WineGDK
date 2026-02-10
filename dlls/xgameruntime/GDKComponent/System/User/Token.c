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

#include "Token.h"

#define GetJsonValue(obj_type, ret_type)                                                            \
static inline HRESULT GetJson##obj_type##Value(IJsonObject *object, LPCWSTR key, ret_type value)    \
{                                                                                                   \
    HSTRING_HEADER key_hdr;                                                                         \
    HSTRING key_hstr;                                                                               \
    HRESULT hr;                                                                                     \
                                                                                                    \
    if (FAILED(hr = WindowsCreateStringReference(key, wcslen(key), &key_hdr, &key_hstr)))           \
        return hr;                                                                                  \
                                                                                                    \
    if (FAILED(hr = IJsonObject_GetNamed##obj_type(object, key_hstr, value)))                       \
        return hr;                                                                                  \
                                                                                                    \
    return S_OK;                                                                                    \
}

GetJsonValue(Array, IJsonArray**);
GetJsonValue(Number, DOUBLE*);
GetJsonValue(Object, IJsonObject**);
GetJsonValue(String, HSTRING*);

HRESULT HSTRINGToMultiByte(HSTRING hstr, LPSTR *str, UINT32 *str_len)
{
    UINT32 wstr_len;
    LPCWSTR wstr = WindowsGetStringRawBuffer(hstr, &wstr_len);

    if (!(*str_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wstr_len, NULL, 0, NULL, NULL)))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!(*str = calloc(1, *str_len))) return E_OUTOFMEMORY;

    if (!(*str_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wstr_len, *str, *str_len, NULL, NULL)))
    {
        free(*str);
        *str = NULL;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

static HRESULT HttpRequest(LPCWSTR method, LPCWSTR domain, LPCWSTR object, LPSTR data, LPCWSTR headers, LPCWSTR* accept, LPSTR* buffer, SIZE_T* bufferSize)
{
    HINTERNET connection = NULL;
    DWORD size = sizeof(DWORD);
    HINTERNET session = NULL;
    HINTERNET request = NULL;
    HRESULT hr = S_OK;
    DWORD status;

    if (!(session = WinHttpOpen(
        L"WineGDK/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    ))) return HRESULT_FROM_WIN32(GetLastError());

    if (!(connection = WinHttpConnect(
        session,
        domain,
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    ))) hr = HRESULT_FROM_WIN32(GetLastError());

    if (SUCCEEDED(hr) && !(request = WinHttpOpenRequest(
        connection,
        method,
        object,
        NULL,
        WINHTTP_NO_REFERER,
        accept,
        WINHTTP_FLAG_SECURE
    ))) hr = HRESULT_FROM_WIN32(GetLastError());

    if (SUCCEEDED(hr) && !WinHttpSendRequest(
        request,
        headers,
        -1,
        data,
        strlen(data),
        strlen(data),
        0
    )) hr = HRESULT_FROM_WIN32(GetLastError());

    if (SUCCEEDED(hr) && !WinHttpReceiveResponse(request, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());

    if (SUCCEEDED(hr) && !WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &status,
        &size,
        WINHTTP_NO_HEADER_INDEX
    )) hr = HRESULT_FROM_WIN32(GetLastError());

    if (SUCCEEDED(hr) && status / 100 != 2) hr = E_FAIL;

    /* buffer response data */
    *buffer = NULL;
    *bufferSize = 0;
    if (SUCCEEDED(hr))
    {
        do
        {
            if (!(WinHttpQueryDataAvailable(request, &size)))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

            if (!size) break;
            if (!(*buffer = realloc(*buffer, *bufferSize + size)))
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            if (!(WinHttpReadData(request, *buffer + *bufferSize, size, &size)))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }

            *bufferSize += size;
        }
        while (size);
    }

    if (connection) WinHttpCloseHandle(connection);
    if (request) WinHttpCloseHandle(request);
    if (session) WinHttpCloseHandle(session);
    if (FAILED(hr) && *buffer) free(*buffer);

    return hr;
}

static HRESULT ParseJsonObject(LPCSTR str, UINT32 str_size, IJsonObject **object)
{
    LPCWSTR class_str = RuntimeClass_Windows_Data_Json_JsonValue;
    IJsonValueStatics *statics;
    HSTRING_HEADER content_hdr;
    HSTRING_HEADER class_hdr;
    IJsonValue *value;
    UINT32 wstr_size;
    HSTRING content;
    HSTRING class;
    LPWSTR wstr;
    HRESULT hr;


    if (!(wstr_size = MultiByteToWideChar(CP_UTF8, 0, str, str_size, NULL, 0)))
        return HRESULT_FROM_WIN32(GetLastError());

    if (!(wstr = calloc(wstr_size, sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    if (!(wstr_size = MultiByteToWideChar(CP_UTF8, 0, str, str_size, wstr, wstr_size)))
    {
        free(wstr);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (FAILED(hr = WindowsCreateStringReference(wstr, wstr_size, &content_hdr, &content)))
    {
        free(wstr);
        return hr;
    }

    if (FAILED(hr = WindowsCreateStringReference(class_str, wcslen(class_str), &class_hdr, &class)))
    {
        free(wstr);
        return hr;
    }

    if (FAILED(hr = RoGetActivationFactory(class, &IID_IJsonValueStatics, (void**)&statics)))
    {
        free(wstr);
        return hr;
    }

    hr = IJsonValueStatics_Parse(statics, content, &value);
    IJsonValueStatics_Release(statics);
    free(wstr);
    if (FAILED(hr)) return hr;

    hr = IJsonValue_GetObject(value, object);
    IJsonValue_Release(value);
    if (FAILED(hr)) IJsonObject_Release(*object);

    return hr;
}

HRESULT RefreshOAuth(LPCSTR client_id, LPCSTR refresh_token, time_t *new_expiry, HSTRING *new_refresh_token, HSTRING *new_oauth_token)
{
    LPCSTR template = "grant_type=refresh_token&scope=service::user.auth.xboxlive.com::MBI_SSL&client_id=";
    LPCWSTR accept[] = {L"application/json", NULL};
    IJsonObject *object;
    time_t expiry;
    LPSTR buffer;
    DOUBLE delta;
    SIZE_T size;
    HRESULT hr;
    LPSTR data;

    if (!(data = calloc(strlen(template) + strlen(client_id) + strlen("&refresh_token=") + strlen(refresh_token) + 1, sizeof(CHAR))))
        return E_OUTOFMEMORY;

    strcpy(data, template);
    strcat(data, client_id);
    strcat(data, "&refresh_token=");
    strcat(data, refresh_token);

    hr = HttpRequest(
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
    if (FAILED(hr)) return hr;
    hr = ParseJsonObject(buffer, size, &object);
    free(buffer);
    if (FAILED(hr)) return hr;

    if (FAILED(hr = GetJsonStringValue(object, L"access_token", new_oauth_token)))
    {
        IJsonObject_Release(object);
        return hr;
    }

    if (FAILED(hr = GetJsonStringValue(object, L"refresh_token", new_refresh_token)))
    {
        IJsonObject_Release(object);
        return hr;
    }

    if (FAILED(hr = GetJsonNumberValue(object, L"expires_in", &delta)))
    IJsonObject_Release(object);
    if (FAILED(hr)) return hr;

    if ((expiry = time(NULL)) == -1) return E_FAIL;
    *new_expiry = expiry + delta;

    return S_OK;
}

HRESULT RequestXToken(LPCWSTR domain, LPCWSTR path, LPSTR data, HSTRING *token)
{
    LPCWSTR accept[] = {L"application/json", NULL};
    IJsonObject *object;
    LPSTR buffer;
    SIZE_T size;
    HRESULT hr;

    hr = HttpRequest(
        L"POST",
        domain,
        path,
        data,
        L"content-type: application/json",
        accept,
        &buffer,
        &size
    );

    if (FAILED(hr)) return hr;
    hr = ParseJsonObject(buffer, size, &object);
    free(buffer);
    if (FAILED(hr)) return hr;

    hr = GetJsonStringValue(object, L"Token", token);
    IJsonObject_Release(object);

    return hr;
}

HRESULT RequestUserToken(HSTRING oauth_token, HSTRING *token, XUserLocalId *local_id)
{
    LPCSTR template = "{\"RelyingParty\":\"http://auth.xboxlive.com\",\"TokenType\":\"JWT\",\"Properties\":{\"AuthMethod\":\"RPS\",\"SiteName\":\"user.auth.xboxlive.com\",\"RpsTicket\":\"";
    LPCWSTR accept[] = {L"application/json", NULL};
    IJsonObject *display_claims;
    UINT32 token_str_len;
    IJsonObject *object;
    UINT32 uhs_str_len;
    LPSTR token_str;
    IJsonArray *xui;
    LPSTR uhs_str;
    LPSTR buffer;
    SIZE_T size;
    HSTRING uhs;
    LPSTR data;
    HRESULT hr;

    if (FAILED(hr = HSTRINGToMultiByte(oauth_token, &token_str, &token_str_len)))
        return hr;

    if (!(data = calloc(strlen(template) + token_str_len + strlen("\"}}") + 1, sizeof(CHAR))))
    {
        free(token_str);
        return E_OUTOFMEMORY;
    }

    strcpy(data, template);
    strncat(data, token_str, token_str_len);
    free(token_str);
    strcat(data, "\"}}");

    hr = HttpRequest(
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
    if (FAILED(hr)) return hr;
    hr = ParseJsonObject(buffer, size, &object);
    free(buffer);
    if (FAILED(hr)) return hr;

    if (FAILED(hr = GetJsonStringValue(object, L"Token", token)))
    {
        IJsonObject_Release(object);
        return hr;
    }

    hr = GetJsonObjectValue(object, L"DisplayClaims", &display_claims);
    IJsonObject_Release(object);
    if (FAILED(hr))
    {
        WindowsDeleteString(*token);
        return hr;
    }

    hr = GetJsonArrayValue(display_claims, L"xui", &xui);
    IJsonObject_Release(display_claims);
    if (FAILED(hr))
    {
        WindowsDeleteString(*token);
        return hr;
    }

    hr = IJsonArray_GetObjectAt(xui, 0, &object);
    IJsonArray_Release(xui);
    if (FAILED(hr))
    {
        WindowsDeleteString(*token);
        return hr;
    }

    hr = GetJsonStringValue(object, L"uhs", &uhs);
    IJsonObject_Release(object);
    if (FAILED(hr))
    {
        WindowsDeleteString(*token);
        return hr;
    }

    hr = HSTRINGToMultiByte(uhs, &uhs_str, &uhs_str_len);
    WindowsDeleteString(uhs);
    if (FAILED(hr))
    {
        WindowsDeleteString(*token);
        return hr;
    }

    local_id->value = strtoull(uhs_str, NULL, 10);
    free(uhs_str);
    if (errno == ERANGE)
    {
        WindowsDeleteString(*token);
        errno = 0;
        return E_FAIL;
    }

    return hr;
}