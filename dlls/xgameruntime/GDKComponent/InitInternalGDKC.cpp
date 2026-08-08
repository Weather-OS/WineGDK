/*
 * Xbox Game runtime Library
 *  GDK Component: Internal Initialization
 * 
 * Written by Weather
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

#include "InitInternalGDKC.h"
#include "../WineCoreUAP/Foundation/IWineAsync.hpp"

#include <ntstatus.h>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

using namespace ABI::Windows::Foundation;

static BOOLEAN initializeCalled = FALSE;

static HRESULT WINAPI
InitializeXodusService()
{
    DWORD async;
    HRESULT hr;

    IAsyncAction *pingAction = nullptr;

    if ( initializeCalled )
        return S_OK;

    initializeCalled = TRUE;

    hr = xodus_ipclayer->InitializeSocket();
    if ( FAILED( hr ) ) 
    {
        WARN("Socket initialization failed with %#lx\n", hr);
        return hr;
    }
    hr = xodus_service->Ping( &pingAction );
    if ( FAILED( hr ) ) 
    {
        WARN("Xodus Ping Dispatch failed with %#lx\n", hr);
        return hr;
    }

    async = AsyncActionCompletedHandler::await_AsyncAction( pingAction, 10000 );
    if ( async )
    {
        if ( async == STATUS_TIMEOUT )
        {
            WARN("Timeout while waiting for PING response.\n");
            return HRESULT_FROM_WIN32( ERROR_TIMEOUT );
        }
            
        WARN("Async action await failed. Status was %ld\n", async);
        return E_FAIL;
    }

    hr = pingAction->GetResults();
    if ( FAILED( hr ) )
        WARN("PING response error. HR was %#lx\n", hr);

    return hr;
}

HRESULT WINAPI
InitializeGDKComponent( INITIALIZE_OPTIONS *options )
{
    HRESULT hr = S_OK;
    TRACE( "options %p.\n", options );
    
    if ( options )
    {
#if XODUS_INTEROP
        hr = InitializeXodusService();
        if ( FAILED( hr ) ) return hr;
#endif
    }

    return hr;
}
