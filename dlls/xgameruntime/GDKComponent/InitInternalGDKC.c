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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static BOOLEAN WINAPI GDKCIgnoreVersionMismatch( void )
{
    LSTATUS result;
    DWORD valueSize;
    INT32 value;

    result = RegGetValueW( HKEY_LOCAL_MACHINE,
                           L"Software\\Microsoft\\GamingServices",
                           L"IgnoreVersionMismatch",
                           RRF_RT_REG_DWORD,
                           NULL,
                           &value,
                           &valueSize );
    
    if ( FAILED( result ) ) return FALSE;
    if ( value == 0 )
        return FALSE;
    else
        return TRUE;
}

HRESULT WINAPI GDKC_InitAPI( 
    ULONG gdkVer, 
    ULONG gsVer, 
    CHAR mode,
    INITIALIZE_OPTIONS *options
) {
    HRESULT status = S_OK;

    TRACE("gdkVer %ld, gsVer %ld, mode %d, options %p\n", gdkVer, gsVer, mode, options);

    if ( !GDKCIgnoreVersionMismatch() && ( gdkVer >= GDKC_VERSION && gsVer > GAMING_SERVICES_VERSION ) )
    {
        ERR("GDKComponent version mismatch with the requested parameters!\n");
        ERR("Target GDK Version: %ld, Current Version: %ld\n", gdkVer, GDKC_VERSION );
        ERR("Target Gaming Services Version: %ld, Current Version: %ld\n", gsVer, GAMING_SERVICES_VERSION );

        // Uncomment once MS Store Services has been implemented within Wine.
        // ShellExecuteW( NULL, NULL, L"ms-windows-store://pdp?productId=9MWPM2CQNLHN", NULL, NULL, 1 );

        return E_GAMERUNTIME_VERSION_MISMATCH;
    }

    return status;

    // TODO: Game Specific Initialization
}