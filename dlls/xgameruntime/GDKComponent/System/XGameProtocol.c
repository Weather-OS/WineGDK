/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XGameProtocol
 *
 * Written by LukasPAH
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

#include "XGameProtocol.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static inline struct x_gameprotocol *impl_from_IXGameProtocolImpl(IXGameProtocolImpl *iface)
{
    return CONTAINING_RECORD(iface, struct x_gameprotocol, IXGameProtocolImpl_iface);
}

static HRESULT WINAPI x_gameprotocol_QueryInterface(IXGameProtocolImpl *iface, REFIID iid, void **out)
{
    struct x_gameprotocol *impl = impl_from_IXGameProtocolImpl(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IXGameProtocolImpl))
    {
        *out = &impl->IXGameProtocolImpl_iface;
        impl->IXGameProtocolImpl_iface.lpVtbl->AddRef(*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_gameprotocol_AddRef(IXGameProtocolImpl *iface)
{
    struct x_gameprotocol *impl = impl_from_IXGameProtocolImpl(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p increasing refcount to %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI x_gameprotocol_Release(IXGameProtocolImpl *iface)
{
    struct x_gameprotocol *impl = impl_from_IXGameProtocolImpl(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p decreasing refcount to %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI x_gameprotocol_XGameProtocolRegisterForActivation(IXGameProtocolImpl *iface, XTaskQueueHandle queue, void *context, void *callback, XTaskQueueRegistrationToken *token)
{
    FIXME("iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token);
    return S_OK;
}

static void WINAPI x_gameprotocol_XGameProtocolActivationCallback(IXGameProtocolImpl *iface, void *context, const LPSTR protocolUri)
{
    FIXME("iface %p, context %p, protocolUri %p stub!\n", iface, context, protocolUri);
}

static BOOLEAN WINAPI x_gameprotocol_XGameProtocolUnregisterForActivation(IXGameProtocolImpl *iface, XTaskQueueRegistrationToken *token, boolean wait)
{
    FIXME("iface %p, token %p, wait %d stub!\n", iface, token, wait);
    return TRUE;
}

static const struct IXGameProtocolImplVtbl x_gameprotocol_vtbl =
    {
        x_gameprotocol_QueryInterface,
        x_gameprotocol_AddRef,
        x_gameprotocol_Release,
        /* IXGameProtocolImpl methods */
        x_gameprotocol_XGameProtocolRegisterForActivation,
        x_gameprotocol_XGameProtocolUnregisterForActivation,
        x_gameprotocol_XGameProtocolActivationCallback,
};

static struct x_gameprotocol x_gameprotocol =
    {
        {&x_gameprotocol_vtbl},
        0};

IXGameProtocolImpl *x_gameprotocol_impl = &x_gameprotocol.IXGameProtocolImpl_iface;
