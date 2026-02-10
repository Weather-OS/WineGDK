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

#ifndef TOKEN_H
#define TOKEN_H

#include "../../../private.h"

#include <errno.h>
#include <winhttp.h>
#include "time.h"

struct token
{
    time_t expiry;
    LPCSTR content;
    UINT32 size;
};

HRESULT RefreshOAuth(LPCSTR client_id, LPCSTR refresh_token, time_t *new_expiry, HSTRING *new_refresh_token, HSTRING *new_oauth_token);
HRESULT RequestUserToken(HSTRING oauth_token, HSTRING *token, XUserLocalId *localId);
HRESULT RequestXToken(LPCWSTR domain, LPCWSTR path, LPSTR data, HSTRING *token);
HRESULT HSTRINGToMultiByte(HSTRING hstr, LPSTR *str, UINT32 *str_len);

#endif