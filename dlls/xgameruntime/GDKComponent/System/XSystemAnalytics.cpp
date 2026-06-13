/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XSystemAnalytics
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

#include <cstring>
#include <atomic>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

const INT32 XSystemConsoleIdBytes = 39;
const INT32 XSystemXboxLiveSandboxIdMaxBytes = 16;
const INT32 XSystemAppSpecificDeviceIdBytes = 45;

using namespace ABI::Windows::System::Profile;

class XSystemAnalyticsImpl : 
    public IXSystemAnalyticsImpl
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out )
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXSystemAnalyticsImpl ) )
        {
            AddRef();
            *out = static_cast<IXSystemAnalyticsImpl *>(this);
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

    XSystemAnalyticsInfo WINAPI XSystemGetAnalyticsInfo() override
    {
        // For Windows, XSystemAnalyticsInfo->form is always "Desktop"
        XSystemAnalyticsInfo info{};    
        ULONGLONG version;
        LPCWSTR analytics_info_str = RuntimeClass_Windows_System_Profile_AnalyticsInfo;
        HSTRING analytics_info_class;
        HSTRING deviceFamilyVersion;
        HSTRING deviceFamily;
        LPCWSTR deviceFamilyVersionStr;
        LPCWSTR deviceFamilyStr;
        HRESULT status;
        UINT32 strSize;
        LPSTR str;
        PSTR splitter;

        IAnalyticsVersionInfo *analytics_version_info = nullptr;
        IAnalyticsInfoStatics *analytics_info_statics = nullptr;

        TRACE( "iface %p.\n", this );

        status = WindowsCreateString( analytics_info_str, lstrlenW( analytics_info_str ), &analytics_info_class );
        if ( FAILED( status ) ) return info;

        status = RoGetActivationFactory( analytics_info_class, __uuidof( IAnalyticsVersionInfo ), (void **)&analytics_info_statics );
        WindowsDeleteString( analytics_info_class );
        if ( FAILED( status ) ) return info;

        status = analytics_info_statics->get_VersionInfo( &analytics_version_info );
        analytics_info_statics->Release();
        if ( FAILED( status ) ) return info;

        status = analytics_version_info->get_DeviceFamilyVersion( &deviceFamilyVersion );
        if ( FAILED( status ) )
        {
            analytics_version_info->Release();
            return info;
        }

        status = analytics_version_info->get_DeviceFamily( &deviceFamily );
        analytics_version_info->Release();
        if ( FAILED( status ) )
        {
            WindowsDeleteString( deviceFamilyVersion );
            return info;
        }

        deviceFamilyStr = WindowsGetStringRawBuffer( deviceFamily, nullptr );
        strSize = WideCharToMultiByte( CP_UTF8, 0, deviceFamilyStr, -1, nullptr, 0, nullptr, nullptr );

        str = (LPSTR)malloc( strSize );
        if ( !str )
        {
            WindowsDeleteString( deviceFamilyVersion );
            WindowsDeleteString( deviceFamily );
            return info;
        }

        if ( !WideCharToMultiByte( CP_UTF8, 0, deviceFamilyStr, -1, str, strSize, nullptr, nullptr ) )
        {
            WindowsDeleteString( deviceFamilyVersion );
            WindowsDeleteString( deviceFamily );
            free( str );
            return info;
        }

        splitter = strchr( str, '.' );
        if ( splitter )
        {
            *splitter = '\0';

            lstrcpyA( info.family, str );
            lstrcpyA( info.form, splitter + 1 );
        }

        WindowsDeleteString( deviceFamily );
        free( str );

        deviceFamilyVersionStr = WindowsGetStringRawBuffer( deviceFamilyVersion, nullptr );
        strSize = WideCharToMultiByte( CP_UTF8, 0, deviceFamilyVersionStr, -1, nullptr, 0, nullptr, nullptr );

        str = (LPSTR)malloc( strSize );
        if ( !str )
        {
            WindowsDeleteString( deviceFamilyVersion );
            return info;
        }

        if ( !WideCharToMultiByte( CP_UTF8, 0, deviceFamilyVersionStr, -1, str, strSize, nullptr, nullptr ) )
        {
            WindowsDeleteString( deviceFamilyVersion );
            free( str );
            return info;
        }

        version = strtoull( str, nullptr, 10 );
        info.osVersion.major = (UINT16)(version >> 48);
        info.osVersion.minor = (UINT16)((version >> 32) & 0xFFFF);
        info.osVersion.build = (UINT16)((version >> 16) & 0xFFFF);
        info.osVersion.revision = (UINT16)(version & 0xFFFF);

        WindowsDeleteString( deviceFamilyVersion );
        free( str );

        info.hostingOsVersion = info.osVersion;

        return info;
    }

private:
    std::atomic_long ref{ 1 };
};

static XSystemAnalyticsImpl g_x_system_analytics;

IXSystemAnalyticsImpl *x_system_analytics = static_cast<IXSystemAnalyticsImpl*>(&g_x_system_analytics);