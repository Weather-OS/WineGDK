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
#include "Structs.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <atomic>
#include <windows.h>
#include <winstring.h>

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

using namespace ABI;
using namespace ABI::Xodus;
using namespace ABI::Windows::Foundation;

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
        xmlDocPtr doc = nullptr;
        xmlNodePtr root = nullptr;
        xmlNodePtr curr_child = nullptr;

        TRACE("xml_string %s, response %p.\n", debugstr_a(xml_string), response);
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

        for ( curr_child = root->children; curr_child != NULL; curr_child = curr_child->next ) 
        {
            if ( curr_child->type == XML_ELEMENT_NODE ) 
            {
                TRACE("Child name is %s\n", curr_child->name );
            }
        }

        return E_NOTIMPL;
    }

private:
    std::atomic_long ref{ 1 };
};

static XodusXMLBuilder g_xodus_xml_builder;
IXodusXMLBuilder *xodus_xml_builder = static_cast<IXodusXMLBuilder*>(&g_xodus_xml_builder);
