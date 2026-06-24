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

    HRESULT WINAPI
    BuildMsaTokenRequestXml( const char *clientId, boolean allowUi, boolean fullTrust, LPSTR *xml_string ) override
    {
        xmlNodePtr root = xmlNewNode( nullptr, BAD_CAST "MsaTokenRequest" );
        xmlDocPtr doc = xmlNewDoc( BAD_CAST "1.0" );
        xmlChar *buffer = nullptr;
        INT bufferSize;

        TRACE( "clientId %s, xml_string %p.\n", debugstr_a( clientId ), xml_string );

        xmlDocSetRootElement( doc, root );
        xmlNewChild( root, nullptr, BAD_CAST "ClientId", BAD_CAST clientId );
        xmlNewChild( root, nullptr, BAD_CAST "AllowUi", BAD_CAST (allowUi ? "true" : "false") );
        xmlNewChild( root, nullptr, BAD_CAST "MsaFullTrust", BAD_CAST (fullTrust ? "true" : "false") );

        xmlDocDumpFormatMemory( doc, &buffer, &bufferSize, 1 );
        xmlFreeDoc( doc );
        *xml_string = reinterpret_cast<char *>(buffer);
        return S_OK;
    }

    HRESULT WINAPI
    FromMsaTokenResponseXml( LPCSTR xml_string, IMsaTokenResponse **response ) override
    {
        xmlNodePtr child, root;
        char *token = nullptr;
        xmlDocPtr doc;

        TRACE( "xml_string %s, response %p.\n", debugstr_a( xml_string ), response );

        if (!(doc = xmlReadMemory( xml_string, strlen( xml_string ), nullptr, nullptr, 0 ))) return E_FAIL;
        if (!(root = xmlDocGetRootElement( doc )))
        {
            xmlFreeDoc( doc );
            return E_FAIL;
        }

        if (!strcmp( reinterpret_cast<const char *>(root->name), "MSATokenResponse" ))
            for (child = root->children; child != nullptr; child = child->next)
                if (child->type == XML_ELEMENT_NODE && !strcmp( reinterpret_cast<const char *>(child->name), "Token" ))
                {
                    token = reinterpret_cast<char *>(xmlNodeGetContent( child ));
                    break;
                }

        *response = new MsaTokenResponse( token );
        xmlFreeDoc( doc );
        return S_OK;
    }

private:
    std::atomic_long ref{ 1 };
};

static XodusXMLBuilder g_xodus_xml_builder;
IXodusXMLBuilder *xodus_xml_builder = static_cast<IXodusXMLBuilder*>(&g_xodus_xml_builder);
