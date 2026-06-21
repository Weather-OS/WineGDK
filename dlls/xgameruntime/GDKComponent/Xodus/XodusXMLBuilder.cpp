/*
 * Xbox Game runtime Library
 *  Xodus Interopability Layer -> XodusXMLBuilder
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

#include "../../private.h"
#include "../../WineCoreUAP/Foundation/IWineAsync.hpp"
#include "../../WineCoreUAP/Foundation/IWineVector.hpp"
#include "Structs.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <atomic>
#include <windows.h>
#include <winstring.h>

#define WINDOWS_TICK 10000000
#define SEC_TO_UNIX_EPOCH 11644473600LL

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

using namespace ABI;
using namespace ABI::Xodus;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;

class ABI::Xodus::XodusXMLBuilder :
    public IXodusXMLBuilder
{
public:
    XodusXMLBuilder() noexcept
    {
        xmlInitParser();
        LIBXML_TEST_VERSION
    }

    ~XodusXMLBuilder()
    {
        xmlCleanupParser();
    }

    /* IUnknown Methods */
    HRESULT WINAPI 
    QueryInterface( REFIID iid, void **out )
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXodusXMLBuilder ) )
        {
            AddRef();
            *out = static_cast<IXodusXMLBuilder *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI 
    AddRef() noexcept override
    {
        ULONG curr = static_cast<ULONG>(++ref);
        TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
        return curr;
    }

    ULONG WINAPI 
    Release() noexcept override
    {
        ULONG curr = static_cast<ULONG>(--ref);
        TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

        // Polymorphic classes should not be deleted.
        /*
        if ( !curr )
            delete this;
        */

        return curr;
    }

    /* IInspectable Methods */
    HRESULT WINAPI
    GetIids( ULONG *iidCount, IID **iids ) override
    {
        FIXME("iface %p, iidCount %p, iids %p stub!\n", this, iidCount, iids);
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetRuntimeClassName( HSTRING *className ) override 
    {
        FIXME("iface %p, className %p stub!\n", this, className);
        return E_NOTIMPL;
    }

    HRESULT WINAPI
    GetTrustLevel( TrustLevel *trustLevel ) override
    {
        FIXME("iface %p, trustLevel %p stub!\n", this, trustLevel);
        return E_NOTIMPL;
    }

    /* IXodusXMLBuilder Methods */
    HRESULT WINAPI
    BuildXstsTokenRequestXml( HSTRING url, LPSTR *xml_string ) override
    {
        INT bufSize;
        INT urlStrSize;
        LPSTR urlStr;
        UINT32 urlStrLen;
        LPCWSTR urlStrW = WindowsGetStringRawBuffer( url, &urlStrLen );
        xmlChar *xmlBuff = nullptr;
        xmlDocPtr doc = xmlNewDoc( BAD_CAST "1.0" );
        xmlNodePtr root;

        TRACE("url %s, xml_string %p.\n", debugstr_hstring(url), xml_string);

        urlStrSize = WideCharToMultiByte( CP_UTF8, 0, urlStrW, urlStrLen, nullptr, 0, nullptr, nullptr );
        urlStr = (LPSTR)CoTaskMemAlloc( urlStrSize );
        WideCharToMultiByte( CP_UTF8, 0, urlStrW, urlStrLen, urlStr, urlStrSize, nullptr, nullptr );

        root = xmlNewNode( nullptr, BAD_CAST "XSTSTokenRequest" );
        xmlDocSetRootElement(doc, root);
        xmlNewChild( root, nullptr, BAD_CAST "Url", BAD_CAST urlStr );
        CoTaskMemFree( urlStr );
        xmlDocDumpFormatMemory(doc, &xmlBuff, &bufSize, 1);
        xmlFreeDoc(doc);

        *xml_string = (LPSTR)CoTaskMemAlloc( bufSize );
        lstrcpynA( *xml_string, reinterpret_cast<LPCSTR>(xmlBuff), bufSize );

        return S_OK;
    }

    HRESULT WINAPI
    FromXstsTokenResponseXml( LPCSTR xml_string, IXstsTokenResponse **response ) override
    {
        HRESULT hr;
        INT strLen;
        LPSTR str;
        LPWSTR strW;
        HSTRING tokenStr;
        HSTRING RelyingParty;
        HSTRING tempStr;
        DateTime expiry{};
        LONGLONG expiryUnix;
        LONGLONG maxBodyBytes = 0;
        xmlChar *childContent;
        xmlDocPtr doc = nullptr;
        xmlNodePtr root = nullptr;
        xmlNodePtr curr_child = nullptr;
        xmlNodePtr sp_child = nullptr;
        TitleMgtSignaturePolicy signaturePolicy{};

        IVector<HSTRING> *AlgorithmVec;
        IVectorView<HSTRING> *AlgorithmVecView;
        IVector<HSTRING> *SignatureVec;
        IVectorView<HSTRING> *SignatureVecView;

        TRACE("xml_string %s, response %p.\n", debugstr_a(xml_string), response);
        printf("xml string is %s\n", xml_string);
        int len = (int)lstrlenA(xml_string);

        doc = xmlReadMemory( xml_string, len, "noname.xml", nullptr, 0 );
        if ( !doc )
        {
            return E_FAIL;
        }

        root = xmlDocGetRootElement( doc );
        if ( !root )
        {
            ERR( "Failed to obtain root element for xmlDoc %p\n", doc );
            return E_FAIL;
        }

        for ( curr_child = root->children; curr_child != nullptr; curr_child = curr_child->next ) 
            if ( curr_child->type == XML_ELEMENT_NODE ) 
            {
                if ( !lstrcmpA( reinterpret_cast<LPCSTR>(curr_child->name), "Token" ) )
                {
                    childContent = xmlNodeGetContent( curr_child );
                    str = (LPSTR)CoTaskMemAlloc( sizeof(CHAR) * ( lstrlenA( reinterpret_cast<LPCSTR>(childContent) ) + 1 ) );
                    if ( !str )
                        return E_OUTOFMEMORY;

                    lstrcpyA( str, reinterpret_cast<LPCSTR>(childContent) );
                    strLen = MultiByteToWideChar( CP_UTF8, 0, str, -1, nullptr, 0 );
                    strW = (LPWSTR)CoTaskMemAlloc( strLen * sizeof(WCHAR) );
                    MultiByteToWideChar( CP_UTF8, 0, str, -1, strW, strLen );
                    hr = WindowsCreateString( strW, strLen - 1, &tokenStr );
                    CoTaskMemFree( strW );
                    CoTaskMemFree( str );
                    if ( FAILED( hr ) ) return hr;
                }

                else if ( !lstrcmpA( reinterpret_cast<LPCSTR>(curr_child->name), "Expiry" ) )
                {
                    childContent = xmlNodeGetContent( curr_child );
                    str = (LPSTR)CoTaskMemAlloc( sizeof(CHAR) * ( lstrlenA( reinterpret_cast<LPCSTR>(childContent) ) + 1 ) );
                    if ( !str )
                        return E_OUTOFMEMORY;

                    lstrcpyA( str, reinterpret_cast<LPCSTR>(childContent) );
                    expiryUnix = strtoll( str, nullptr, 10 );
                    expiry.UniversalTime = ((INT64)expiryUnix + SEC_TO_UNIX_EPOCH) * WINDOWS_TICK;
                    CoTaskMemFree( str );
                }

                else if ( !lstrcmpA( reinterpret_cast<LPCSTR>(curr_child->name), "RelyingParty" ) )
                {
                    childContent = xmlNodeGetContent( curr_child );
                    str = (LPSTR)CoTaskMemAlloc( sizeof(CHAR) * ( lstrlenA( reinterpret_cast<LPCSTR>(childContent) ) + 1 ) );
                    if ( !str )
                        return E_OUTOFMEMORY;

                    lstrcpyA( str, reinterpret_cast<LPCSTR>(childContent) );
                    strLen = MultiByteToWideChar( CP_UTF8, 0, str, -1, nullptr, 0 );
                    strW = (LPWSTR)CoTaskMemAlloc( strLen * sizeof(WCHAR) );
                    MultiByteToWideChar( CP_UTF8, 0, str, -1, strW, strLen );
                    hr = WindowsCreateString( strW, strLen - 1, &RelyingParty );
                    CoTaskMemFree( strW );
                    CoTaskMemFree( str );
                    if ( FAILED( hr ) ) return hr;
                }

                else if ( !lstrcmpA( reinterpret_cast<LPCSTR>(curr_child->name), "SignaturePolicy" ) )
                {
                    for ( sp_child = curr_child->children; sp_child != nullptr; sp_child = sp_child->next ) 
                        if ( sp_child->type == XML_ELEMENT_NODE ) 
                        {
                            if ( !lstrcmpA( reinterpret_cast<LPCSTR>(sp_child->name), "Algorithms" ) )
                            {
                                xmlNodePtr algorithms;

                                hr = Vector<HSTRING>::Create( &AlgorithmVec );
                                if ( FAILED( hr ) ) return hr;

                                for ( algorithms = sp_child->children; algorithms != NULL; algorithms = algorithms->next ) 
                                    if ( algorithms->type == XML_ELEMENT_NODE ) 
                                    {
                                        childContent = xmlNodeGetContent( algorithms );
                                        str = (LPSTR)CoTaskMemAlloc( sizeof(CHAR) * ( lstrlenA( reinterpret_cast<LPCSTR>(childContent) ) + 1 ) );
                                        if ( !str )
                                            return E_OUTOFMEMORY;

                                        lstrcpyA( str, reinterpret_cast<LPCSTR>(childContent) );
                                        strLen = MultiByteToWideChar( CP_UTF8, 0, str, -1, nullptr, 0 );
                                        strW = (LPWSTR)CoTaskMemAlloc( strLen * sizeof(WCHAR) );
                                        MultiByteToWideChar( CP_UTF8, 0, str, -1, strW, strLen );
                                        hr = WindowsCreateString( strW, strLen - 1, &tempStr );
                                        CoTaskMemFree( strW );
                                        CoTaskMemFree( str );
                                        if ( FAILED( hr ) )
                                        {
                                            AlgorithmVec->Release();
                                            return hr;
                                        } 

                                        hr = AlgorithmVec->Append( tempStr );
                                        if ( FAILED( hr ) )
                                        {
                                            AlgorithmVec->Release();
                                            return hr;
                                        }
                                        WindowsDeleteString( tempStr );
                                    }
                            }

                            else if ( !lstrcmpA( reinterpret_cast<LPCSTR>(sp_child->name), "MaxBodyBytes" ) )
                            {
                                childContent = xmlNodeGetContent( sp_child );
                                str = (LPSTR)CoTaskMemAlloc( sizeof(CHAR) * ( lstrlenA( reinterpret_cast<LPCSTR>(childContent) ) + 1 ) );
                                if ( !str )
                                    return E_OUTOFMEMORY;

                                lstrcpyA( str, reinterpret_cast<LPCSTR>(childContent) );
                                maxBodyBytes = strtoll( str, nullptr, 10 );
                                CoTaskMemFree( str );
                            }

                            else if ( !lstrcmpA( reinterpret_cast<LPCSTR>(sp_child->name), "SignatureTypes" ) )
                            {
                                xmlNodePtr signatures;

                                hr = Vector<HSTRING>::Create( &SignatureVec );
                                if ( FAILED( hr ) ) return hr;

                                for ( signatures = sp_child->children; signatures != NULL; signatures = signatures->next ) 
                                    if ( signatures->type == XML_ELEMENT_NODE ) 
                                    {
                                        childContent = xmlNodeGetContent( signatures );
                                        str = (LPSTR)CoTaskMemAlloc( sizeof(CHAR) * ( lstrlenA( reinterpret_cast<LPCSTR>(childContent) ) + 1 ) );
                                        if ( !str )
                                            return E_OUTOFMEMORY;

                                        lstrcpyA( str, reinterpret_cast<LPCSTR>(childContent) );
                                        strLen = MultiByteToWideChar( CP_UTF8, 0, str, -1, nullptr, 0 );
                                        strW = (LPWSTR)CoTaskMemAlloc( strLen * sizeof(WCHAR) );
                                        MultiByteToWideChar( CP_UTF8, 0, str, -1, strW, strLen );
                                        hr = WindowsCreateString( strW, strLen - 1, &tempStr );
                                        CoTaskMemFree( strW );
                                        CoTaskMemFree( str );
                                        if ( FAILED( hr ) )
                                        {
                                            SignatureVec->Release();
                                            return hr;
                                        } 

                                        hr = SignatureVec->Append( tempStr );
                                        if ( FAILED( hr ) )
                                        {
                                            SignatureVec->Release();
                                            return hr;
                                        }
                                        WindowsDeleteString( tempStr );
                                    }
                            }
                            TRACE("name was %s\n", sp_child->name);
                        }
                }
            }

        if ( AlgorithmVec ) AlgorithmVec->GetView( &AlgorithmVecView );
        if ( SignatureVec ) SignatureVec->GetView( &SignatureVecView );
        signaturePolicy.MaxBodyBytes = maxBodyBytes;
        signaturePolicy.SupportedAlgorithms = AlgorithmVecView;
        signaturePolicy.SupportedSignatureTypes = SignatureVecView;
        AlgorithmVec->Release();
        SignatureVec->Release();
        *response = new XstsTokenResponse( tokenStr, expiry, RelyingParty, signaturePolicy );
        WindowsDeleteString( tokenStr );
        WindowsDeleteString( RelyingParty );
        AlgorithmVecView->Release();
        SignatureVecView->Release();

        return S_OK;
    }

private:
    std::atomic_long ref{ 1 };
};

static XodusXMLBuilder g_xodus_xml_builder;
IXodusXMLBuilder *xodus_xml_builder = static_cast<IXodusXMLBuilder*>(&g_xodus_xml_builder);
