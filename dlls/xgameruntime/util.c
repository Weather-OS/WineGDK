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

#include "private.h"
#include <winhttp.h>

WINE_DEFAULT_DEBUG_CHANNEL(xgameruntime);

static const WCHAR USER_AGENT[] = L"curl/1.0";

HRESULT http_request( const WCHAR *method, const WCHAR *hostName, const WCHAR *pathAndQuery, char *data,
                     const WCHAR *headers, const WCHAR **accept, UCHAR **buffer, SIZE_T *bufferSize )
{
    HINTERNET connection = NULL, request = NULL, session = NULL;
    DWORD size = sizeof( DWORD ), status;
    HRESULT hr = S_OK;

    TRACE( "method %s, hostName %s, pathAndQuery %s, data %s, headers %s, accept %p, buffer %p, bufferSize %p.\n",
           debugstr_w( method ), debugstr_w( hostName ), debugstr_w( pathAndQuery ), debugstr_a( data ), debugstr_w( headers ), accept, buffer, bufferSize );

    if (!(session = WinHttpOpen( USER_AGENT, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 ))) goto error;
    if (!(connection = WinHttpConnect( session, hostName, INTERNET_DEFAULT_HTTPS_PORT, 0 ))) goto error;
    if (!(request = WinHttpOpenRequest( connection, method, pathAndQuery, NULL, WINHTTP_NO_REFERER, accept, WINHTTP_FLAG_SECURE ))) goto error;
    if (!WinHttpSendRequest( request, headers, -1, data, (data ? strlen( data ) : 0), (data ? strlen( data ) : 0), 0 )) goto error;
    if (!WinHttpReceiveResponse( request, NULL )) goto error;
    if (!WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                              WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX )) goto error;
    if (status != 200)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    /* buffer response data */
    *bufferSize = 0;
    *buffer = NULL;
    do
    {
        if (!WinHttpQueryDataAvailable( request, &size )) goto error;
        if (!size) break;
        if (!(*buffer = realloc( *buffer, *bufferSize + size )))
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        if (!WinHttpReadData( request, *buffer + *bufferSize, size, &size )) goto error;
        *bufferSize += size;
    }
    while (size);
    goto cleanup;

error:
    hr = HRESULT_FROM_WIN32( GetLastError() );
cleanup:
    if (connection) WinHttpCloseHandle( connection );
    if (request) WinHttpCloseHandle( request );
    if (session) WinHttpCloseHandle( session );
    if (SUCCEEDED(hr)) return hr;
    if (*buffer) free( *buffer );
    *bufferSize = 0;
    *buffer = NULL;
    return hr;
}

#define encode_base64_(sfx,type,alph)                                                                                       \
HRESULT encode_base64##sfx( const UINT32 dataSize, const BYTE *data, const UINT32 base64Size, type *base64, BOOLEAN pad )   \
{                                                                                                                           \
    UINT32 size = (dataSize * 8 + 5) / 6, rem = (size % 4) ? 4 - (size % 4) : 0;                                            \
    UINT32 div = dataSize / 3; /* 3 bytes of in, 4 chars out */                                                             \
    const char alphabet[64] = alph;                                                                                         \
    const BYTE *cur = data;                                                                                                 \
    type *ptr = base64;                                                                                                     \
                                                                                                                            \
    TRACE( "dataSize %u, data %p, base64Size %u, base64 %p.\n", dataSize, data, base64Size, base64 );                       \
                                                                                                                            \
    size += pad ? rem : 0;                                                                                                  \
    if (size > base64Size) return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );                                          \
                                                                                                                            \
    while (div > 0)                                                                                                         \
    {                                                                                                                       \
        /* first 6 bits of byte 0 */                                                                                        \
        *ptr++ = alphabet[ (cur[0] >> 2) & 0x3f ];                                                                          \
        /* last 2 bits of byte 0, first 4 bits of byte 1 */                                                                 \
        *ptr++ = alphabet[ ((cur[0] << 4) & 0x30) | ((cur[1] >> 4) & 0x0f) ];                                               \
        /* last 4 bits of byte 1, first 2 bits of byte 2 */                                                                 \
        *ptr++ = alphabet[ ((cur[1] << 2) & 0x3c) | ((cur[2] >> 6) & 0x03) ];                                               \
        /* last 6 bits of byte 2 */                                                                                         \
        *ptr++ = alphabet[ cur[2] & 0x3f ];                                                                                 \
        cur += 3;                                                                                                           \
        div--;                                                                                                              \
    }                                                                                                                       \
                                                                                                                            \
    switch (rem)                                                                                                            \
    {                                                                                                                       \
        case 1:                                                                                                             \
            /* first 6 bits of byte 0 */                                                                                    \
            *ptr++ = alphabet[ (cur[0] >> 2) & 0x3f ];                                                                      \
            /* last 2 bits of byte 0, first 4 bits of byte 1 */                                                             \
            *ptr++ = alphabet[ ((cur[0] << 4) & 0x30) | ((cur[1] >> 4) & 0x0f) ];                                           \
            /* last 4 bits of byte 1, rest 0 */                                                                             \
            *ptr++ = alphabet[ ((cur[1] << 2) & 0x3c) ];                                                                    \
            /* padding */                                                                                                   \
            if (pad) *ptr++ = '=';                                                                                          \
            break;                                                                                                          \
        case 2:                                                                                                             \
            /* first 6 bits of byte 0 */                                                                                    \
            *ptr++ = alphabet[ (cur[0] >> 2) & 0x3f ];                                                                      \
            /* last 2 bits of byte 0, rest 0 */                                                                             \
            *ptr++ = alphabet[ ((cur[0] << 4) & 0x30) ];                                                                    \
            /* padding */                                                                                                   \
            if (pad)                                                                                                        \
            {                                                                                                               \
                *ptr++ = '=';                                                                                               \
                *ptr++ = '=';                                                                                               \
            }                                                                                                               \
            break;                                                                                                          \
    }                                                                                                                       \
    return S_OK;                                                                                                            \
}

encode_base64_(_utf16,WCHAR,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")
encode_base64_(_url,char,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_")
encode_base64_(,char,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")

#define get_json_(sfx,typ,ret)                                                                  \
HRESULT get_json_##sfx( IJsonObject *object, const WCHAR *key, ret value )                      \
{                                                                                               \
    HSTRING_HEADER hdr;                                                                         \
    HSTRING str;                                                                                \
    HRESULT hr;                                                                                 \
                                                                                                \
    if (FAILED(hr = WindowsCreateStringReference( key, wcslen( key ), &hdr, &str ))) return hr; \
    return IJsonObject_GetNamed##typ( object, str, value );                                     \
}

get_json_(object,Object,IJsonObject**)
get_json_(array,Array,IJsonArray**)
get_json_(boolean,Boolean,boolean*)
get_json_(value,Value,IJsonValue**)
get_json_(string,String,HSTRING*)
get_json_(number,Number,DOUBLE*)
