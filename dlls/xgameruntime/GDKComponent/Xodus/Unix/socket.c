/*
 * Xbox Game runtime Library
 *  Xodus Interopability Layer -> Unix module -> Socket
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

#if 0
#pragma makedep unix
#endif

#define WINE_UNIX_LIB

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "initguid.h"

#include "wine/unixlib.h"
#include "wine/list.h"
#include "wine/unixlib.h"
#include "wine/debug.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
# define if_indextoname unix_if_indextoname
# define if_nametoindex unix_if_nametoindex
# include <net/if.h>
# undef if_indextoname
# undef if_nametoindex
#endif
#ifdef HAVE_IFADDRS_H
# include <ifaddrs.h>
#endif
#include <poll.h>

#ifdef HAVE_NETIPX_IPX_H
# include <netipx/ipx.h>
# define HAS_IPX
#elif defined(HAVE_LINUX_IPX_H)
# ifdef HAVE_ASM_TYPES_H
#  include <asm/types.h>
# endif
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/ipx.h>
# ifdef SOL_IPX
#  define HAS_IPX
# endif
#endif

#ifdef HAVE_LINUX_IRDA_H
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/irda.h>
# define HAS_IRDA
#endif

WINE_DEFAULT_DEBUG_CHANNEL(xodus);

// Persist connection
static int sockfd = 0;

static NTSTATUS conn_sock( void *args )
{
    struct sockaddr_un addr;
    LPCSTR socket_suffix = (LPCSTR)args;

#ifdef __linux__
    const char *runtime = getenv( "XDG_RUNTIME_DIR" );
    if ( !runtime )
        return E_NOT_VALID_STATE;
#elif defined(__APPLE__)
    const char *runtime = "/tmp";
#endif

    size_t len = strlen( runtime ) + strlen( socket_suffix ) + 1;
    char *socket_path = malloc( len );

    if ( !socket_path )
        return STATUS_NO_MEMORY;

    snprintf( socket_path, len, "%s/%s", runtime, socket_suffix );

    sockfd = socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sockfd < 0 ) 
        return STATUS_ABANDONED;

    memset( &addr, 0, sizeof(addr) );
    addr.sun_family = AF_UNIX;
    lstrcpynA( addr.sun_path, socket_path, sizeof(addr.sun_path) - 1 );


    if ( connect( sockfd, (struct sockaddr*)&addr, sizeof(addr) ) < 0 ) 
        return STATUS_CONNECTION_REFUSED;

    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    conn_sock
};