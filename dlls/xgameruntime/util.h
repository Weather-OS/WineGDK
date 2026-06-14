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

#ifndef __WINE_XGAMERUNTIME_UTIL_H
#define __WINE_XGAMERUNTIME_UTIL_H

#define WIDL_using_Windows_Data_Json
#include "windows.data.json.h"

HRESULT http_request( const WCHAR *method, const WCHAR *hostName, const WCHAR *pathAndQuery, char *data,
                      const WCHAR *headers, const WCHAR **accept, UCHAR **buffer, SIZE_T *bufferSize );
HRESULT encode_base64( const UINT32 dataSize, const BYTE *data, const UINT32 base64Size, char *base64, BOOLEAN pad );
HRESULT encode_base64_url( const UINT32 dataSize, const BYTE *data, const UINT32 base64Size, char *base64, BOOLEAN pad );
HRESULT encode_base64_utf16( const UINT32 dataSize, const BYTE *data, const UINT32 base64Size, WCHAR *base64, BOOLEAN pad );
HRESULT get_json_object( IJsonObject *obj, const WCHAR *key, IJsonObject **value );
HRESULT get_json_array( IJsonObject *obj, const WCHAR *key, IJsonArray **value );
HRESULT get_json_boolean( IJsonObject *obj, const WCHAR *key, boolean *value );
HRESULT get_json_value( IJsonObject *obj, const WCHAR *key, IJsonValue **value );
HRESULT get_json_string( IJsonObject *obj, const WCHAR *key, HSTRING *value );
HRESULT get_json_number( IJsonObject *obj, const WCHAR *key, DOUBLE *value );

#endif
