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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

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

static HRESULT GetJsonStringValue(IJsonObject *object, LPCWSTR key, HSTRING *content)
{
    HSTRING_HEADER key_hdr;
    HSTRING key_hstr;
    HRESULT hr;

    if (FAILED(hr = WindowsCreateStringReference(key, wcslen(key), &key_hdr, &key_hstr)))
        return hr;

    if (FAILED(hr = IJsonObject_GetNamedString(object, key_hstr, content)))
        return hr;

    return S_OK;
}

HRESULT RefreshOAuth(LPCSTR client_id, LPCSTR refresh_token, time_t *new_expiry, HSTRING *new_refresh_token, HSTRING *new_oauth_token)
{
    LPCSTR template = "grant_type=refresh_token&scope=service::user.auth.xboxlive.com::MBI_SSL&client_id=";
    LPCWSTR class_str = RuntimeClass_Windows_Data_Json_JsonValue;
    LPCWSTR accept[] = {L"application/json", NULL};
    IJsonValueStatics *statics;
    HSTRING_HEADER content_hdr;
    HSTRING_HEADER expires_hdr;
    HSTRING_HEADER class_hdr;
    IJsonObject *object;
    IJsonValue *value;
    HSTRING content;
    HSTRING expires;
    LPWSTR w_buffer;
    UINT32 w_size;
    time_t expiry;
    HSTRING class;
    LPSTR buffer;
    DOUBLE delta;
    SIZE_T size;
    HRESULT hr;
    LPSTR data;

    if (!(data = calloc(1, strlen(template) + strlen(client_id) +
        strlen("&refresh_token=") + strlen(refresh_token) + 1))) return E_OUTOFMEMORY;

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
    if (FAILED(hr)) {
        TRACE("web request failed\n");
        return hr;
    }

    if (!(w_size = MultiByteToWideChar(CP_UTF8, 0, buffer, size, NULL, 0)))
    {
        free(buffer);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!(w_buffer = calloc(w_size, sizeof(WCHAR))))
    {
        free(buffer);
        return E_OUTOFMEMORY;
    }

    w_size = MultiByteToWideChar(CP_UTF8, 0, buffer, size, w_buffer, w_size);
    free(buffer);
    if (!w_size)
    {
        free(w_buffer);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (FAILED(hr = WindowsCreateStringReference(w_buffer, w_size, &content_hdr, &content)))
    {
        free(w_buffer);
        return hr;
    }

    if (FAILED(hr = WindowsCreateStringReference(class_str, wcslen(class_str), &class_hdr, &class)))
    {
        free(w_buffer);
        return hr;
    }

    if (FAILED(hr = RoGetActivationFactory(class, &IID_IJsonValueStatics, (void**)&statics)))
    {
        free(w_buffer);
        return hr;
    }

    hr = IJsonValueStatics_Parse(statics, content, &value);
    free(w_buffer);
    IJsonValueStatics_Release(statics);
    if (FAILED(hr))
    {
        IJsonValue_Release(value);
        return hr;
    }

    hr = IJsonValue_GetObject(value, &object);
    IJsonValue_Release(value);
    if (FAILED(hr))
    {
        IJsonObject_Release(object);
        return hr;
    }

    if (FAILED(hr = GetJsonStringValue(object, L"access_token", new_oauth_token)))
    {
        TRACE("failed to get access_token\n");
        IJsonObject_Release(object);
        return hr;
    }

    if (FAILED(hr = GetJsonStringValue(object, L"refresh_token", new_refresh_token)))
    {
        TRACE("failed to get refresh_token\n");
        IJsonObject_Release(object);
        return hr;
    }    

    if (FAILED(hr = WindowsCreateStringReference(
        L"expires_in", wcslen(L"expires_in"), &expires_hdr, &expires)))
    {
        IJsonObject_Release(object);
        return hr;
    }

    hr = IJsonObject_GetNamedNumber(object, expires, &delta);
    IJsonObject_Release(object);
    if (FAILED(hr))
    {
        TRACE("failed to get expires_in\n");
        return hr;
    }

    if ((expiry = time(NULL)) == -1) return E_FAIL;
    *new_expiry = expiry + delta;

    return S_OK;
}

HRESULT RequestXToken(LPCWSTR domain, LPCWSTR path, LPSTR data, HSTRING *token)
{
    LPCWSTR class_str = RuntimeClass_Windows_Data_Json_JsonValue;
    LPCWSTR accept[] = {L"application/json", NULL};
    HSTRING_HEADER token_key_hdr;
    IJsonValueStatics *statics;
    HSTRING_HEADER content_hdr;
    HSTRING_HEADER class_hdr;
    IJsonObject *object;
    IJsonValue *value;
    HSTRING token_key;
    LPWSTR w_buffer;
    HSTRING content;
    HSTRING class;
    LPSTR buffer;
    SIZE_T size;
    HRESULT hr;
    INT w_size;

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

    if (FAILED(hr)) return FALSE;

    if (!(w_size = MultiByteToWideChar(CP_UTF8, 0, buffer, size, NULL, 0)))
    {
        free(buffer);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!(w_buffer = calloc(w_size, sizeof(WCHAR))))
    {
        free(buffer);
        return E_OUTOFMEMORY;
    }

    w_size = MultiByteToWideChar(CP_UTF8, 0, buffer, size, w_buffer, w_size);
    free(buffer);
    if (!w_size)
    {
        free(w_buffer);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (FAILED(hr = WindowsCreateStringReference(w_buffer, w_size, &content_hdr, &content)))
    {
        free(w_buffer);
        return hr;
    }

    if (FAILED(hr = WindowsCreateStringReference(
        L"Token", wcslen(L"Token"), &token_key_hdr, &token_key)))
    {
        free(w_buffer);
        return hr;
    }

    if (FAILED(hr = WindowsCreateStringReference(class_str, wcslen(class_str), &class_hdr, &class)))
    {
        free(w_buffer);
        return hr;
    }

    if (FAILED(hr = RoGetActivationFactory(class, &IID_IJsonValueStatics, (void**)&statics)))
    {
        free(w_buffer);
        return hr;
    }

    hr = IJsonValueStatics_Parse(statics, content, &value);
    IJsonValueStatics_Release(statics);
    free(w_buffer);
    if (FAILED(hr))
    {
        IJsonValue_Release(value);
        return hr;
    }

    hr = IJsonValue_GetObject(value, &object);
    IJsonValue_Release(value);
    if (FAILED(hr))
    {
        IJsonObject_Release(object);
        return hr;
    }

    if (FAILED(hr = IJsonObject_GetNamedString(object, token_key, token)))
        IJsonObject_Release(object);

    return hr;
}