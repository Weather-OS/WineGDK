/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XNetworking
 * 
 * Written by Weather
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

#ifndef USERIMPL_H
#define USERIMPL_H

#include "../../../private.h"

#include <atomic>

struct DECLSPEC_UUID("5F280469-FB5A-44AA-9F14-D77C7C90A5DC") IUser : IUnknown
{
    virtual HRESULT WINAPI GetMsaToken( HSTRING *token ) = 0;
    /* Xbox Live identity settled alongside the MSA token. */
    virtual UINT64 WINAPI GetXuid() = 0;
    virtual HRESULT WINAPI GetGamertag( HSTRING *gamertag ) = 0;
    virtual HRESULT WINAPI GetXstsToken( HSTRING *token ) = 0;
    /* Token for the PlayFab relying party, which titles authenticate against
     * separately from plain Xbox Live. */
    virtual HRESULT WINAPI GetPlayfabToken( HSTRING *token ) = 0;
    virtual void WINAPI SetPlayfabToken( HSTRING token ) = 0;
};
//5F280469-FB5A-44AA-9F14-D77C7C90A5DC
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IUser, 0x5F280469, 0xFB5A, 0x44AA, 0x9F,0x14, 0xD7,0x7C,0x7C,0x90,0xA5,0xDC)
#endif

struct XUser
{
    UINT32 m_signature;
    IUser* m_user;
};

struct XUserAddContext
{
    XUserAddOptions options;
    XUserHandle user;
    HANDLE userAddEvent;
};

/* Implemented in UserImpl.cpp; XUserImpl forwards its XUser API to these. */
HRESULT XUserAddAsync( XUserAddOptions options, XAsyncBlock *async );
HRESULT XUserAddResult( XAsyncBlock *async, XUserHandle *newUser );

class UserImpl : 
    public IUser
{
public:
    UserImpl( HSTRING token );
    UserImpl( HSTRING token, HSTRING xuid, HSTRING gamertag, HSTRING xstsToken );
    virtual ~UserImpl() = default;

    HRESULT WINAPI QueryInterface( REFIID iid, void **out ) noexcept override;
    ULONG WINAPI AddRef() noexcept override;
    ULONG WINAPI Release() noexcept override;

    HRESULT WINAPI GetMsaToken( HSTRING *out ) override;
    UINT64 WINAPI GetXuid() override;
    HRESULT WINAPI GetGamertag( HSTRING *out ) override;
    HRESULT WINAPI GetXstsToken( HSTRING *out ) override;
    HRESULT WINAPI GetPlayfabToken( HSTRING *out ) override;
    void WINAPI SetPlayfabToken( HSTRING token ) override;

private:
    HSTRING m_token;
    HSTRING m_gamertag = nullptr;
    HSTRING m_xstsToken = nullptr;
    HSTRING m_playfabToken = nullptr;
    UINT64 m_xuid = 0;
    std::atomic_long ref{ 1 };
};

// Signatures can be anything because these are not exposed to the client.
static uint32_t const X_USER_SIGNATURE = 0xa7f3c92d;

#endif