/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XSystem
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

#include "HTTPClient.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static HRESULT httpclient_SendRequest( URL_COMPONENTS uc, HINTERNET *inetRequest )
{
    DWORD reqFlags;
    LPWSTR hostName = NULL;
    LPWSTR urlPath = NULL;
    HRESULT status = S_OK;
    HINTERNET inetSession = NULL;
    HINTERNET inetConnect = NULL;

    if (inetRequest) *inetRequest = NULL;

    TRACE( "uc %p, inetRequest %p\n", &uc, inetRequest );

    hostName = HeapAlloc( GetProcessHeap(), 0, (uc.dwHostNameLength + 1) * sizeof(WCHAR) );
    if ( !hostName ) 
    { 
        status = E_OUTOFMEMORY; goto _CLEANUP; 
    }
    wcsncpy_s( hostName, uc.dwHostNameLength + 1, uc.lpszHostName, uc.dwHostNameLength );
    hostName[uc.dwHostNameLength] = L'\0';

    if ( uc.dwUrlPathLength == 0 ) 
    {
        urlPath = HeapAlloc( GetProcessHeap(), 0, 2 * sizeof(WCHAR) );
        if ( !urlPath ) 
        { 
            status = E_OUTOFMEMORY; 
            goto _CLEANUP; 
        }
        wcscpy_s( urlPath, 2, L"/" );
    } else 
    {
        urlPath = HeapAlloc( GetProcessHeap(), 0, (uc.dwUrlPathLength + 1) * sizeof(WCHAR) );
        if ( !urlPath ) 
        { 
            status = E_OUTOFMEMORY; 
            goto _CLEANUP; 
        }
        wcsncpy_s( urlPath, uc.dwUrlPathLength + 1, uc.lpszUrlPath, uc.dwUrlPathLength );
        urlPath[uc.dwUrlPathLength] = L'\0';
    }

    inetSession = WinHttpOpen( L"curl/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 );
    if ( !inetSession )
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

    inetConnect = WinHttpConnect( inetSession, hostName, uc.nPort, 0 );
    if ( !inetConnect )
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

    reqFlags = ( uc.nScheme == INTERNET_SCHEME_HTTPS ) ? WINHTTP_FLAG_SECURE : 0;
    *inetRequest = WinHttpOpenRequest( inetConnect, L"GET",urlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, reqFlags );
    if ( !*inetRequest )
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

    if ( !WinHttpSendRequest( *inetRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0 ) ||
         !WinHttpReceiveResponse( *inetRequest, NULL ) )
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

_CLEANUP:
    if ( FAILED( status ) )
    {
        if ( *inetRequest ) WinHttpCloseHandle( *inetRequest );
        *inetRequest = NULL;
    }
    if ( inetSession ) WinHttpCloseHandle( inetSession );
    if ( inetConnect ) WinHttpCloseHandle( inetConnect );
    if ( hostName ) HeapFree( GetProcessHeap(), 0, hostName );
    if ( urlPath ) HeapFree( GetProcessHeap(), 0, urlPath );
    return status;
}

static HRESULT httpclient_ObtainSecurityProtocolFlags( HINTERNET inetRequest, UINT32 *flags )
{
    DWORD secInfoSize = sizeof( WINHTTP_SECURITY_INFO );
    HRESULT status = S_OK;
    PWINHTTP_SECURITY_INFO securityInfo = ( PWINHTTP_SECURITY_INFO )CoTaskMemAlloc( secInfoSize );

    TRACE( "inetRequest %p, flags %p\n", inetRequest, flags );

    if ( !WinHttpQueryOption( inetRequest, WINHTTP_OPTION_SECURITY_INFO, securityInfo, &secInfoSize ) )
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

    *flags = securityInfo->ConnectionInfo.dwProtocol;

_CLEANUP:
    CoTaskMemFree( securityInfo );
    return status;
}

static HRESULT httpclient_ObtainServerCertificate( HINTERNET inetRequest, PCERT_CONTEXT *context )
{
    DWORD certContextSize = sizeof( PCCERT_CONTEXT );
    HRESULT status = S_OK;

    TRACE( "inetRequest %p, context %p\n", inetRequest, context );

    if ( !WinHttpQueryOption( inetRequest, WINHTTP_OPTION_SERVER_CERT_CONTEXT, context, &certContextSize ) )
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

_CLEANUP:
    return status;
}

static HRESULT httpclient_ObtainThumbprints( HINTERNET inetRequest, SIZE_T *thumbprintCount, XNetworkingThumbprint **out )
{
    SIZE_T idx = 0;
    HRESULT status = S_OK;
    PCERT_CONTEXT certContext;
    CERT_CHAIN_PARA chainPara = { .cbSize = sizeof(CERT_CHAIN_PARA) };
    PCCERT_CHAIN_CONTEXT chainContext;
    XNetworkingThumbprint *thumbprints;

    TRACE( "inetRequest %p, thumbprintCount %p, out %p\n", inetRequest, thumbprintCount, out );

    status = httpclient_ObtainServerCertificate( inetRequest, &certContext );
    if ( FAILED( status ) ) goto _CLEANUP;

    if ( !CertGetCertificateChain( NULL, certContext, NULL, certContext->hCertStore, &chainPara, 0, NULL, &chainContext ) )
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

    thumbprints = (XNetworkingThumbprint *)calloc( 3, sizeof(XNetworkingThumbprint) );
    if ( !thumbprints )
    {
        status = E_OUTOFMEMORY;
        goto _CLEANUP;
    }

#define TRY_ADD_THUMBPRINT_FROM_CERT(pCertCtx, tp)                                                                  \
    do {                                                                                                            \
        if ( pCertCtx ) {                                                                                           \
            DWORD cb = 0;                                                                                           \
            /* Wine's WinHTTP only supports SHA1 */                                                                 \
            if ( CertGetCertificateContextProperty( pCertCtx, CERT_SHA1_HASH_PROP_ID, NULL, &cb ) )                 \
            {                                                                                                       \
                UINT8* buf = (UINT8*)malloc( cb );                                                                  \
                if ( buf && CertGetCertificateContextProperty( pCertCtx, CERT_SHA1_HASH_PROP_ID, buf, &cb ) )       \
                {                                                                                                   \
                    thumbprints[idx].thumbprintType = (tp);                                                         \
                    thumbprints[idx].thumbprintBufferByteCount = (SIZE_T)cb;                                        \
                    thumbprints[idx].thumbprintBuffer = buf;                                                        \
                    idx++;                                                                                          \
                } else { free( buf ); }                                                                             \
            }                                                                                                       \
        }                                                                                                           \
    } while( FALSE )

    if ( chainContext && chainContext->cChain > 0 && chainContext->rgpChain[0] ) 
    {
        PCERT_SIMPLE_CHAIN simple = chainContext->rgpChain[0];
        DWORD numElements = simple->cElement;
        if ( numElements >= 1 ) 
        {
            // leaf = element[0]
            TRY_ADD_THUMBPRINT_FROM_CERT( simple->rgpElement[0]->pCertContext, ThumbprintType_Leaf );
        }
        if ( numElements >= 2 ) 
        {
            // issuer = element[1] (immediate issuer of leaf)
            TRY_ADD_THUMBPRINT_FROM_CERT( simple->rgpElement[1]->pCertContext, ThumbprintType_Issuer );
        }
        if ( numElements >= 1 ) 
        {
            // root = last element
            TRY_ADD_THUMBPRINT_FROM_CERT( simple->rgpElement[numElements - 1]->pCertContext, ThumbprintType_Root );
        }
    } else 
    {
        // chain not available: at least try server cert (leaf)
        TRY_ADD_THUMBPRINT_FROM_CERT( certContext, ThumbprintType_Leaf );
    }

    if (idx > 0) 
    {
        *out = ( XNetworkingThumbprint* )realloc( thumbprints, idx * sizeof(XNetworkingThumbprint) );
        *thumbprintCount = idx;
    }

#undef TRY_ADD_THUMBPRINT_FROM_CERT

_CLEANUP:
    if ( chainContext ) CertFreeCertificateChain( chainContext );
    if ( certContext ) CertFreeCertificateContext( certContext );

    return status;
}

HRESULT httpclient_ObtainSecurityInformationForUrl( LPCWSTR url, BYTE **outBuffer, SIZE_T *outBufferByteCount, XNetworkingSecurityInformation **securityInformation )
{
    LPBYTE buffer = NULL;
    LPBYTE bufferLoc = NULL;
    SIZE_T iterator;
    SIZE_T totalBufferSize;
    SIZE_T thumbprintBytes = 0;
    HRESULT status = S_OK;
    HINTERNET inetRequest;
    URL_COMPONENTS uc = { .dwStructSize = sizeof(URL_COMPONENTS), 
        .dwSchemeLength = (DWORD)-1, .dwHostNameLength = (DWORD)-1, .dwUrlPathLength = (DWORD)-1, .dwExtraInfoLength = (DWORD)-1 };
    XNetworkingSecurityInformation *information = NULL;

    FIXME( "url %s, securityInformation %p\n", debugstr_w( url ), securityInformation );

    if ( !WinHttpCrackUrl( url, 0, 0, &uc ) )
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

    status = httpclient_SendRequest( uc, &inetRequest );
    if ( FAILED( status ) ) goto _CLEANUP;

    information = (XNetworkingSecurityInformation *)malloc( sizeof(*information) );

    status = httpclient_ObtainThumbprints( inetRequest, &information->thumbprintCount, &information->thumbprints );
    if ( FAILED( status ) ) goto _CLEANUP;

    // TODO: Security Protocol Flags are not supported by wine's WinHTTP
    status = httpclient_ObtainSecurityProtocolFlags( inetRequest, &information->enabledHttpSecurityProtocolFlags );
    if ( FAILED( status ) )
    {
        information->enabledHttpSecurityProtocolFlags = WINHTTP_FLAG_SECURE_PROTOCOL_ALL | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
        status = S_OK;
    }

    for ( iterator = 0; iterator < information->thumbprintCount; iterator++ )
        thumbprintBytes += information->thumbprints[iterator].thumbprintBufferByteCount;

    totalBufferSize = sizeof(XNetworkingSecurityInformation) + (information->thumbprintCount * sizeof(XNetworkingThumbprint)) + thumbprintBytes;

    buffer = (LPBYTE)malloc( totalBufferSize );
    if ( !buffer )
    {
        status = E_OUTOFMEMORY;
        goto _CLEANUP;
    }

    bufferLoc = buffer;

    memcpy( bufferLoc, information, sizeof(XNetworkingSecurityInformation) );
    bufferLoc += sizeof(XNetworkingSecurityInformation);

    memcpy( bufferLoc, information->thumbprints, (information->thumbprintCount * sizeof(XNetworkingThumbprint)) );
    bufferLoc += (information->thumbprintCount * sizeof(XNetworkingThumbprint));

    for ( iterator = 0; iterator < information->thumbprintCount; iterator++ )
    {
        // guard against empty thumbprints
        if ( information->thumbprints[iterator].thumbprintBuffer && information->thumbprints[iterator].thumbprintBufferByteCount > 0 )
        {
            memcpy( bufferLoc,
                    information->thumbprints[iterator].thumbprintBuffer,
                    (size_t)information->thumbprints[iterator].thumbprintBufferByteCount );
            bufferLoc += information->thumbprints[iterator].thumbprintBufferByteCount;
        }
    }

    *outBuffer = buffer;
    *outBufferByteCount = totalBufferSize;

_CLEANUP:
    if ( FAILED( status ) )
    {
        free( buffer );
    }
    if ( inetRequest ) WinHttpCloseHandle( inetRequest );
    return status;
}