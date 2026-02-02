/*
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

/*
 * Xbox Game runtime Library
 * GDK Component: System API -> XLauncher
 */

#include "XLauncher.h"
#include "shellapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_launcher *impl_from_IXLauncherImpl(IXLauncherImpl *iface)
{
    return CONTAINING_RECORD(iface, struct x_launcher, IXLauncherImpl_iface);
}

static HRESULT WINAPI x_launcher_QueryInterface(IXLauncherImpl *iface, REFIID iid, void **out)
{
    struct x_launcher *impl = impl_from_IXLauncherImpl(iface);

    TRACE("iface %p, iid %s, out %p\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IXLauncherImpl))
    {
        *out = &impl->IXLauncherImpl_iface;
        impl->IXLauncherImpl_iface.lpVtbl->AddRef(*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_launcher_AddRef(IXLauncherImpl *iface)
{
    struct x_launcher *impl = impl_from_IXLauncherImpl(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p increasing refcount to %lu\n", iface, ref);
    return ref;
}

static ULONG WINAPI x_launcher_Release(IXLauncherImpl *iface)
{
    struct x_launcher *impl = impl_from_IXLauncherImpl(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p decreasing refcount to %lu\n", iface, ref);
    return ref;
}

static HRESULT WINAPI x_launcher_XLaunchUri(IXLauncherImpl *iface, XUserHandle user, LPCSTR uri)
{
    TRACE("iface %p, user %p launching uri %s\n", iface, user, uri);
    return (SIZE_T)ShellExecuteA(NULL, "open", uri, NULL, NULL, SW_SHOW) > 32 ? S_OK : E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER;
}

static const struct IXLauncherImplVtbl x_launcher_vtbl = {
    x_launcher_QueryInterface,
    x_launcher_AddRef,
    x_launcher_Release,
    x_launcher_XLaunchUri
};

static struct x_launcher x_launcher = {
    {&x_launcher_vtbl},
    0
};

IXLauncherImpl *x_launcher_impl = &x_launcher.IXLauncherImpl_iface;