/*
 * Xbox Game runtime Library
 *  GDK Component: Internal Initialization
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

#include "InitInternalGDKC.h"
#include "../WineCoreUAP/Foundation/IWineAsync.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <ntstatus.h>
#include <shlwapi.h>

WINE_DEFAULT_DEBUG_CHANNEL(gdkct);

using namespace ABI::Windows::Foundation;

static BOOLEAN initializeCalled = FALSE;

LPCSTR msaAppId;
UINT32 titleId;
BOOLEAN fullTrust;

static HRESULT WINAPI
InitializeXodusService()
{
    DWORD async;
    HRESULT hr;

    IAsyncAction *pingAction = nullptr;

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

static HRESULT WINAPI
ObtainMsaAppId( INITIALIZE_OPTIONS *options )
{
    HRESULT hr = S_OK;

    CHAR filename[MAX_PATH], *last;

    xmlNodePtr root, child;
    xmlDocPtr config;

    TRACE( "options %p.\n", options );

    if ( options )
    {
        if ( options->isInlineConfig && !( config = xmlReadMemory( options->gameConfig, strlen( options->gameConfig ), NULL, NULL, 0 ) ) )
            return E_GAMERUNTIME_GAMECONFIG_BAD_FORMAT;
        else if ( !( config = xmlReadFile( options->gameConfig, NULL, 0 ) ) )
            return E_GAMERUNTIME_GAMECONFIG_BAD_FORMAT; 
        else 
        {
            hr = E_GAMERUNTIME_GAMECONFIG_BAD_FORMAT;
            goto _CLEANUP;
        }
    } 
    else 
    {
        if ( !GetModuleFileNameA( NULL, filename, MAX_PATH ) ) return HRESULT_FROM_WIN32( GetLastError() );
        while ( ( last = strrchr( filename, '\\' ) ) )
        {
            *( last + 1 ) = 0;
            if ( strlen( filename ) + strlen( "MicrosoftGame.config" ) < MAX_PATH )
                strcat( filename, "MicrosoftGame.config" );
            else return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
            if ( PathFileExistsA( filename ) ) break;
            *last = 0;
            if (!strrchr( filename, '\\' )) return E_GAME_MISSING_GAME_CONFIG;
        }
        if ( !( config = xmlReadFile( filename, NULL, 0 ) ) ) return E_GAMERUNTIME_GAMECONFIG_BAD_FORMAT;
    }

    if ( !( root = xmlDocGetRootElement( config ) ) ) 
    {
        hr = E_GAMERUNTIME_GAMECONFIG_BAD_FORMAT;
        goto _CLEANUP;
    }

    if ( !strcmp( (LPSTR)root->name, "Game" ) )
    {
        for ( child = root->children; child; child = child->next )
            if ( child->type == XML_ELEMENT_NODE )
            {
                if ( !strcmp( (LPSTR)child->name, "MSAAppId" ) )
                    msaAppId = (LPSTR)xmlNodeGetContent( child );
                else if ( !strcmp( (LPSTR)child->name, "TitleId" ) )
                {
                    LPSTR value = (LPSTR)xmlNodeGetContent( child );
                    /* MicrosoftGame.Config writes the title id in hex. */
                    titleId = strtoul( value, NULL, 16 );
                    free( value );
                }
                else if ( !strcmp( (LPSTR)child->name, "MSAFullTrust" ) )
                {
                    LPSTR value = (LPSTR)xmlNodeGetContent( child );
                    fullTrust = !strcmp( value, "true" );
                    free( value );
                }
            }
    }

_CLEANUP:
    xmlFreeDoc( config );
    return hr;
}

HRESULT WINAPI
InitializeGDKComponent( INITIALIZE_OPTIONS *options )
{
    HRESULT hr = S_OK;
    TRACE( "options %p.\n", options );

    if ( initializeCalled )
        return S_OK;

    initializeCalled = TRUE;
#if XODUS_INTEROP
    hr = InitializeXodusService();
    if ( FAILED( hr ) ) return hr;
#endif
    hr = ObtainMsaAppId( options );
    if ( FAILED( hr ) ) return hr;
    TRACE("got msaAppId %s\n", debugstr_a(msaAppId));

    return hr;
}
