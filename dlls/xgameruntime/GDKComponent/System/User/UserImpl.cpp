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

#include "UserImpl.h"
#include "../../../WineCoreUAP/Foundation/IWineAsync.hpp"

#include <memory>
#include <mutex>

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

using namespace ABI::Xodus;

UserImpl::UserImpl( HSTRING token )
{
    WindowsDuplicateString( token, &m_token );
}

UserImpl::UserImpl( HSTRING token, HSTRING xuid, HSTRING gamertag, HSTRING xstsToken )
{
    UINT32 length = 0;
    LPCWSTR raw;

    WindowsDuplicateString( token, &m_token );
    WindowsDuplicateString( gamertag, &m_gamertag );
    WindowsDuplicateString( xstsToken, &m_xstsToken );

    /* The XUID arrives as a decimal string; callers want it as a number. */
    raw = WindowsGetStringRawBuffer( xuid, &length );
    if ( raw && length )
        m_xuid = _wcstoui64( raw, nullptr, 10 );
}

UINT64 WINAPI
UserImpl::GetXuid()
{
    TRACE( "iface %p\n", this );
    return m_xuid;
}

HRESULT WINAPI
UserImpl::GetGamertag( HSTRING *out )
{
    TRACE( "iface %p, out %p\n", this, out );
    WindowsDuplicateString( m_gamertag, out );
    return S_OK;
}

HRESULT WINAPI
UserImpl::GetXstsToken( HSTRING *out )
{
    TRACE( "iface %p, out %p\n", this, out );
    WindowsDuplicateString( m_xstsToken, out );
    return S_OK;
}

HRESULT WINAPI
UserImpl::GetPlayfabToken( HSTRING *out )
{
    TRACE( "iface %p, out %p\n", this, out );
    WindowsDuplicateString( m_playfabToken, out );
    return S_OK;
}

void WINAPI
UserImpl::SetPlayfabToken( HSTRING token )
{
    TRACE( "iface %p, token %s\n", this, debugstr_hstring( token ) );
    WindowsDeleteString( m_playfabToken );
    m_playfabToken = nullptr;
    WindowsDuplicateString( token, &m_playfabToken );
}

HRESULT WINAPI
UserImpl::QueryInterface( REFIID iid, void **out ) noexcept
{
    TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( IUser ) )
    {
        AddRef();
        *out = static_cast<IUser *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI 
UserImpl::AddRef() noexcept
{
    ULONG curr = static_cast<ULONG>(++ref);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

ULONG WINAPI 
UserImpl::Release() noexcept
{
    ULONG curr = static_cast<ULONG>(--ref);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr )
    {
        WindowsDeleteString( m_token );
        delete this;
    }
    return curr;
}

HRESULT WINAPI
UserImpl::GetMsaToken( HSTRING *out )
{
    TRACE( "iface %p, out %p\n", this, out );
    WindowsDuplicateString( m_token, out );
    return S_OK;
}

/* The signed-in user is the same for the whole session, and callers ask for it
 * repeatedly. Fetching a fresh MSA token every time serializes those calls behind
 * a multi-second round trip, so keep the first one. */
static std::mutex g_defaultUserLock;
static XUser *g_defaultUser;

static HRESULT WINAPI 
XUserAddProvider( XAsyncOp op, const XAsyncProviderData *data )
{
    struct XUserAddContext *context;
    XUser *user;

    IXThreadingImpl *xthreading;
    HRESULT hr;
    HSTRING msaAppIdStr;
    HSTRING msaToken;
    HSTRING xuid = nullptr;
    HSTRING gamertag = nullptr;
    HSTRING xstsToken = nullptr;
    LPWSTR msaAppIdW; 
    DWORD async;
    INT32 msaAppIdLen;

    IMsaTokenResponse* token_response = nullptr;
    IAsyncOperation<IMsaTokenResponse *> *token_operation = nullptr;

    TRACE( "op %d, data %p.\n", (int)op, data );

    if ( FAILED( hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading ) ) ) return hr;
    context = (struct XUserAddContext *)data->context;

    switch (op)
    {
        case XAsyncOp::Begin:
            hr = xthreading->XAsyncSchedule( data->async, 0 );
            break;

        case XAsyncOp::GetResult:
            memcpy( data->buffer, &context->user, sizeof(XUserHandle) );
            break;

        case XAsyncOp::DoWork:
#if XODUS_INTEROP
            {
                /* None (0) asks for the account picker, which xodus has no UI for -
                 * there is a single signed-in user, so serve it like a silent add. */
                if ( context->options == XUserAddOptions::None ||
                     static_cast<UINT32>(context->options) & static_cast<UINT32>(XUserAddOptions::AddDefaultUserSilently) ||
                     static_cast<UINT32>(context->options) & static_cast<UINT32>(XUserAddOptions::AddDefaultUserAllowingUI ) )
                {
                    msaAppIdLen = MultiByteToWideChar( CP_UTF8, 0, msaAppId, -1, NULL, 0 );
                    if ( msaAppIdLen == 0 )
                    {
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                        goto _FALLBACK;
                    }
                    msaAppIdW = (LPWSTR)CoTaskMemAlloc( msaAppIdLen * sizeof(WCHAR) );
                    if ( !msaAppIdW )
                    {
                        hr = E_OUTOFMEMORY;
                        goto _FALLBACK;
                    }
                    if ( !MultiByteToWideChar( CP_UTF8, 0, msaAppId, -1, msaAppIdW, msaAppIdLen ) )
                    {
                        CoTaskMemFree( msaAppIdW );
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                        goto _FALLBACK;
                    }
                    HRESULT hr = WindowsCreateString( msaAppIdW, lstrlenW( msaAppIdW), &msaAppIdStr );
                    CoTaskMemFree( msaAppIdW );
                    if ( FAILED( hr ) ) 
                        goto _FALLBACK;

                    {
                        std::lock_guard<std::mutex> cached( g_defaultUserLock );
                        if ( g_defaultUser )
                        {
                            TRACE( "reusing the signed-in user %p.\n", g_defaultUser );
                            context->user = g_defaultUser;
                            hr = S_OK;
                            goto _COMPLETE;
                        }
                    }

                    if ( FAILED( hr = xodus_service->MsaTokenRequest( msaAppIdStr, static_cast<UINT32>(context->options) & static_cast<UINT32>(XUserAddOptions::AddDefaultUserAllowingUI), fullTrust, &token_operation ) ) ) 
                        goto _FALLBACK;
                    context->userAddEvent = CreateEventW( nullptr, FALSE, FALSE, nullptr );

                    if ( ( async = AsyncOperationCompletedHandler<IMsaTokenResponse *>::await_CancellableAsyncOperation( token_operation, context->userAddEvent, INFINITE ) ) )
                    {
                        if ( async == STATUS_TIMEOUT )
                            WARN( "Timeout while waiting for MSA_TOKEN_RESPONSE response.\n" );
                        else
                            WARN( "Async action await failed. Status was %ld.\n", async );
                        hr = E_ABORT;
                    }
                    else
                    {
                        hr = token_operation->GetResults( &token_response );
                        if ( FAILED( hr ) )
                            goto _FALLBACK;
                        token_response->get_Token( &msaToken );
                        token_response->get_Xuid( &xuid );
                        token_response->get_Gamertag( &gamertag );
                        token_response->get_XstsToken( &xstsToken );

                        /* The handle outlives this call, so it cannot live on the stack. */
                        user = new (std::nothrow) XUser;
                        if ( !user )
                        {
                            WindowsDeleteString( msaToken );
                            token_response->Release();
                            hr = E_OUTOFMEMORY;
                            goto _FALLBACK;
                        }
                        user->m_signature = X_USER_SIGNATURE;
                        user->m_user = new UserImpl( msaToken, xuid, gamertag, xstsToken );
                        {
                            std::lock_guard<std::mutex> cached( g_defaultUserLock );
                            if ( !g_defaultUser ) g_defaultUser = user;
                        }

                        /* Titles authenticate to PlayFab against its own relying
                         * party, so settle that token here too rather than at the
                         * first request. */
                        {
                            HSTRING playfabParty;
                            IAsyncOperation<IMsaTokenResponse *> *playfab_op = nullptr;
                            IMsaTokenResponse *playfab_response = nullptr;
                            HSTRING playfabToken = nullptr;

                            if ( SUCCEEDED( WindowsCreateString( L"http://playfab.xboxlive.com/",
                                                                 lstrlenW( L"http://playfab.xboxlive.com/" ),
                                                                 &playfabParty ) ) )
                            {
                                if ( SUCCEEDED( xodus_service->XstsTokenRequest( msaAppIdStr, playfabParty, &playfab_op ) ) &&
                                     !AsyncOperationCompletedHandler<IMsaTokenResponse *>::await_AsyncOperation( playfab_op, INFINITE ) &&
                                     SUCCEEDED( playfab_op->GetResults( &playfab_response ) ) &&
                                     playfab_response )
                                {
                                    playfab_response->get_XstsToken( &playfabToken );
                                    user->m_user->SetPlayfabToken( playfabToken );
                                    WindowsDeleteString( playfabToken );
                                    playfab_response->Release();
                                }
                                else
                                {
                                    WARN( "No PlayFab token; that sign-in will fail.\n" );
                                }
                                WindowsDeleteString( playfabParty );
                            }
                        }
                        WindowsDeleteString( msaToken );
                        WindowsDeleteString( xuid );
                        WindowsDeleteString( gamertag );
                        WindowsDeleteString( xstsToken );
                        token_response->Release();

                        context->user = user;
                    }
                }
                else 
                    hr = E_ABORT;
                
                goto _COMPLETE;

            _FALLBACK:
                WARN( "Failed to request msa token from xodus. hr was %#lx\n", hr );
            }
#endif
        _COMPLETE:
            xthreading->XAsyncComplete( data->async, hr, SUCCEEDED(hr) ? sizeof(XUserHandle) : 0 );
            /* Completed here; returning success would make the framework complete
             * the operation a second time. */
            hr = E_PENDING;
            break;

        case XAsyncOp::Cleanup:
            free( context );
            break;

        case XAsyncOp::Cancel:
            if ( context->userAddEvent )
                SetEvent( context->userAddEvent );
            break;
    }

    xthreading->Release();
    return hr;
}

/**
 * xgameruntime XUser methods
 */
HRESULT XUserAddAsync( 
    XUserAddOptions options,
    XAsyncBlock *async
) {
    std::unique_ptr<XUserAddContext> context;
    IXThreadingImpl *xthreading = nullptr;
    HRESULT hr;

    if ( !async ) 
        return E_POINTER;

    context = std::make_unique<XUserAddContext>();
    
    hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading );
    if ( FAILED( hr ) ) return hr;

    context->options = options;
    // ownership of context is handed to XUserAddProvider.
    hr = xthreading->XAsyncBegin( async, context.get(), (void *)XUserAddProvider, __func__, XUserAddProvider );
    if ( SUCCEEDED( hr ) )
    {
        context.release();
    }

    return hr;
}

HRESULT XUserAddResult(
    XAsyncBlock *async,
    XUserHandle *newUser
) {
    IXThreadingImpl *xthreading = nullptr;
    HRESULT hr;

    if ( !async || !newUser )
        return E_POINTER;

    hr = QueryApiImpl( &CLSID_XThreadingImpl, IID_IXThreadingImpl, (void **)&xthreading );
    if ( FAILED( hr ) ) return hr;

    /* The identity has to match the one XUserAddAsync began the operation with. */
    hr = xthreading->XAsyncGetResult( async, (void *)XUserAddProvider,
                                      sizeof(*newUser), newUser, nullptr );
    xthreading->Release();

    return hr;
}
