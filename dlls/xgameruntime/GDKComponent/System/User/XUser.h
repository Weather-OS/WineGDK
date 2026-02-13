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

#ifndef XUSER_H
#define XUSER_H

#include "../../../private.h"
#include "Token.h"

struct x_user
{
    IXUserImpl IXUserImpl_iface;
    IXUserGamertag IXUserGamertag_iface;
    LONG ref;

    UINT64 xuid;
    XUserLocalId local_id;
    XUserAgeGroup age_group;

    time_t oauth_token_expiry;
    HSTRING refresh_token;
    HSTRING oauth_token;
    HSTRING user_token;
    HSTRING xsts_token;
};

#endif