/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#include <stddef.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "initguid.h"
#include "unknwn.h"
#include "objidlbase.h"
#include "gameinput.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ginput);

struct game_input
{
    v0_IGameInput v0_IGameInput_iface;
    v1_IGameInput v1_IGameInput_iface;
    LONG ref;
};

static inline struct game_input *impl_from_v0_IGameInput( v0_IGameInput *iface )
{
    return CONTAINING_RECORD( iface, struct game_input, v0_IGameInput_iface );
}

static HRESULT WINAPI game_input_QueryInterface( v0_IGameInput *iface, REFIID iid, void **out )
{
    struct game_input *impl = impl_from_v0_IGameInput( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_v0_IGameInput ))
    {
        *out = &impl->v0_IGameInput_iface;
        impl->ref++;
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_v1_IGameInput ))
    {
        *out = &impl->v1_IGameInput_iface;
        impl->ref++;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI game_input_AddRef( v0_IGameInput *iface )
{
    struct game_input *impl = impl_from_v0_IGameInput( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI game_input_Release( v0_IGameInput *iface )
{
    struct game_input *impl = impl_from_v0_IGameInput( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static UINT64 WINAPI game_input_GetCurrentTimestamp( v0_IGameInput *iface )
{
    FIXME( "iface %p, stub GetCurrentTimestamp.\n", iface );
    return (UINT64)0;
}

static HRESULT WINAPI game_input_GetCurrentReading( v0_IGameInput *iface, GameInputKind kind,
                                                   v0_IGameInputDevice *device, v0_IGameInputReading **reading )
{
    FIXME( "iface %p GetCurrentReading kind=%d device=%p reading_out=%p\n", iface, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_GetNextReading( v0_IGameInput *iface, v0_IGameInputReading *reference,
                                                 GameInputKind kind, v0_IGameInputDevice *device,
                                                 v0_IGameInputReading **reading )
{
    FIXME( "iface %p GetNextReading reference=%p kind=%d device=%p reading_out=%p\n", iface, reference, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_GetPreviousReading( v0_IGameInput *iface, v0_IGameInputReading *reference,
                                                     GameInputKind kind, v0_IGameInputDevice *device,
                                                     v0_IGameInputReading **reading )
{
    FIXME( "iface %p GetPreviousReading reference=%p kind=%d device=%p reading_out=%p\n", iface, reference, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_GetTemporalReading( v0_IGameInput *iface, uint64_t timestamp,
                                                    v0_IGameInputDevice *device, v0_IGameInputReading **reading )
{
    FIXME( "iface %p GetTemporalReading timestamp=%llu device=%p reading_out=%p\n", iface, (unsigned long long)timestamp, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_RegisterReadingCallback( v0_IGameInput *iface, v0_IGameInputDevice *device,
                                                          GameInputKind kind, float threshold, void *context,
                                                          v0_GameInputReadingCallback callback, GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterReadingCallback device=%p kind=%d threshold=%f callback=%p token_out=%p\n",
           iface, device, (int)kind, threshold, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_RegisterDeviceCallback( v0_IGameInput *iface, v0_IGameInputDevice *device,
                                                         GameInputKind kind, GameInputDeviceStatus filter,
                                                         GameInputEnumerationKind enum_kind, void *context,
                                                         v0_GameInputDeviceCallback callback, GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterDeviceCallback device=%p kind=%d filter=%d enum_kind=%d callback=%p token_out=%p\n",
           iface, device, (int)kind, (int)filter, (int)enum_kind, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_RegisterSystemButtonCallback( v0_IGameInput *iface, v0_IGameInputDevice *device,
                                                               GameInputSystemButtons filter, void *context,
                                                               v0_GameInputSystemButtonCallback callback, GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterSystemButtonCallback device=%p filter=%u callback=%p token_out=%p\n",
           iface, device, (unsigned)filter, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_RegisterKeyboardLayoutCallback( v0_IGameInput *iface, v0_IGameInputDevice *device,
                                                                 void *context, v0_GameInputKeyboardLayoutCallback callback,
                                                                 GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterKeyboardLayoutCallback device=%p callback=%p token_out=%p\n",
           iface, device, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static void WINAPI game_input_StopCallback( v0_IGameInput *iface, GameInputCallbackToken token )
{
    FIXME( "iface %p StopCallback token=%llu\n", iface, (unsigned long long)token );
    /* no-op stub */
}

static bool WINAPI game_input_UnregisterCallback( v0_IGameInput *iface, GameInputCallbackToken token, uint64_t timeout_ms )
{
    FIXME( "iface %p UnregisterCallback token=%llu timeout_ms=%llu\n", iface, (unsigned long long)token, (unsigned long long)timeout_ms );
    return false;
}

static HRESULT WINAPI game_input_CreateDispatcher( v0_IGameInput *iface, IGameInputDispatcher **dispatcher )
{
    FIXME( "iface %p CreateDispatcher dispatcher_out=%p\n", iface, dispatcher );
    if (dispatcher) *dispatcher = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_CreateAggregateDevice( v0_IGameInput *iface, GameInputKind kind, v0_IGameInputDevice **device )
{
    FIXME( "iface %p CreateAggregateDevice kind=%d device_out=%p\n", iface, (int)kind, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_FindDeviceFromId( v0_IGameInput *iface, const APP_LOCAL_DEVICE_ID *value, v0_IGameInputDevice **device )
{
    FIXME( "iface %p FindDeviceFromId value=%p device_out=%p\n", iface, value, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_FindDeviceFromObject( v0_IGameInput *iface, IUnknown *value, v0_IGameInputDevice **device )
{
    FIXME( "iface %p FindDeviceFromObject unknown=%p device_out=%p\n", iface, value, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_FindDeviceFromPlatformHandle( v0_IGameInput *iface, HANDLE value, v0_IGameInputDevice **device )
{
    FIXME( "iface %p FindDeviceFromPlatformHandle handle=%p device_out=%p\n", iface, value, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_FindDeviceFromPlatformString( v0_IGameInput *iface, const WCHAR *value, v0_IGameInputDevice **device )
{
    FIXME( "iface %p FindDeviceFromPlatformString value=%p device_out=%p\n", iface, value, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input_EnableOemDeviceSupport( v0_IGameInput *iface, uint16_t vid, uint16_t pid,
                                                         uint8_t iface_num, uint8_t collection )
{
    FIXME( "iface %p EnableOemDeviceSupport vid=0x%04x pid=0x%04x iface=%u collection=%u\n",
           iface, (unsigned)vid, (unsigned)pid, (unsigned)iface_num, (unsigned)collection );
    return E_NOTIMPL;
}

static void WINAPI game_input_SetFocusPolicy( v0_IGameInput *iface, GameInputFocusPolicy policy )
{
    FIXME( "iface %p SetFocusPolicy policy=%d\n", iface, (int)policy );
    /* no-op stub */
}

static const struct v0_IGameInputVtbl game_input_vtbl =
{
    /* IUnknown methods */
    game_input_QueryInterface,
    game_input_AddRef,
    game_input_Release,
    /* v0_IGameInput methods */
    game_input_GetCurrentTimestamp,
    game_input_GetCurrentReading,
    game_input_GetNextReading,
    game_input_GetPreviousReading,
    game_input_GetTemporalReading,
    game_input_RegisterReadingCallback,
    game_input_RegisterDeviceCallback,
    game_input_RegisterSystemButtonCallback,
    game_input_RegisterKeyboardLayoutCallback,
    game_input_StopCallback,
    game_input_UnregisterCallback,
    game_input_CreateDispatcher,
    game_input_CreateAggregateDevice,
    game_input_FindDeviceFromId,
    game_input_FindDeviceFromObject,
    game_input_FindDeviceFromPlatformHandle,
    game_input_FindDeviceFromPlatformString,
    game_input_EnableOemDeviceSupport,
    game_input_SetFocusPolicy
};

static inline struct game_input *impl_from_v1_IGameInput( v1_IGameInput *iface )
{
    return CONTAINING_RECORD( iface, struct game_input, v1_IGameInput_iface );
}

static HRESULT WINAPI game_input1_QueryInterface( v1_IGameInput *iface, REFIID iid, void **out )
{
    struct game_input *impl = impl_from_v1_IGameInput( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_v0_IGameInput ))
    {
        *out = &impl->v0_IGameInput_iface;
        impl->ref++;
        return S_OK;
    }

    if (IsEqualGUID( iid, &IID_v1_IGameInput ))
    {
        *out = &impl->v1_IGameInput_iface;
        impl->ref++;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI game_input1_AddRef( v1_IGameInput *iface )
{
    struct game_input *impl = impl_from_v1_IGameInput( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI game_input1_Release( v1_IGameInput *iface )
{
    struct game_input *impl = impl_from_v1_IGameInput( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
};

static UINT64 WINAPI game_input1_GetCurrentTimestamp( v1_IGameInput *iface )
{
    TRACE( "iface %p, stub GetCurrentTimestamp.\n", iface );
    return (UINT64)0;
}

static HRESULT WINAPI game_input1_GetCurrentReading( v1_IGameInput *iface, GameInputKind kind,
                                                   v1_IGameInputDevice *device, v1_IGameInputReading **reading )
{
    FIXME( "iface %p GetCurrentReading kind=%d device=%p reading_out=%p\n", iface, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input1_GetNextReading( v1_IGameInput *iface, v1_IGameInputReading *reference,
                                                 GameInputKind kind, v1_IGameInputDevice *device,
                                                 v1_IGameInputReading **reading )
{
    FIXME( "iface %p GetNextReading reference=%p kind=%d device=%p reading_out=%p\n", iface, reference, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input1_GetPreviousReading( v1_IGameInput *iface, v1_IGameInputReading *reference,
                                                     GameInputKind kind, v1_IGameInputDevice *device,
                                                     v1_IGameInputReading **reading )
{
    FIXME( "iface %p GetPreviousReading reference=%p kind=%d device=%p reading_out=%p\n", iface, reference, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input1_RegisterReadingCallback( v1_IGameInput *iface, v1_IGameInputDevice *device,
                                                          GameInputKind kind, void *context,
                                                          v1_GameInputReadingCallback callback, GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterReadingCallback device=%p kind=%d callback=%p token_out=%p\n",
           iface, device, (int)kind, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input1_RegisterDeviceCallback( v1_IGameInput *iface, v1_IGameInputDevice *device,
                                                         GameInputKind kind, GameInputDeviceStatus filter,
                                                         GameInputEnumerationKind enum_kind, void *context,
                                                         v1_GameInputDeviceCallback callback, GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterDeviceCallback device=%p kind=%d filter=%d enum_kind=%d callback=%p token_out=%p\n",
           iface, device, (int)kind, (int)filter, (int)enum_kind, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input1_RegisterSystemButtonCallback( v1_IGameInput *iface, v1_IGameInputDevice *device,
                                                               GameInputSystemButtons filter, void *context,
                                                               v1_GameInputSystemButtonCallback callback, GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterSystemButtonCallback device=%p filter=%u callback=%p token_out=%p\n",
           iface, device, (unsigned)filter, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input1_RegisterKeyboardLayoutCallback( v1_IGameInput *iface, v1_IGameInputDevice *device,
                                                                 void *context, v1_GameInputKeyboardLayoutCallback callback,
                                                                 GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterKeyboardLayoutCallback device=%p callback=%p token_out=%p\n",
           iface, device, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static void WINAPI game_input1_StopCallback( v1_IGameInput *iface, GameInputCallbackToken token )
{
    FIXME( "iface %p StopCallback token=%llu\n", iface, (unsigned long long)token );
    /* no-op stub */
}

static bool WINAPI game_input1_UnregisterCallback( v1_IGameInput *iface, GameInputCallbackToken token )
{
    FIXME( "iface %p UnregisterCallback token=%llu\n", iface, (unsigned long long)token );
    return false;
}

static HRESULT WINAPI game_input1_CreateDispatcher( v1_IGameInput *iface, IGameInputDispatcher **dispatcher )
{
    FIXME( "iface %p CreateDispatcher dispatcher_out=%p\n", iface, dispatcher );
    if (dispatcher) *dispatcher = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input1_FindDeviceFromId( v1_IGameInput *iface, const APP_LOCAL_DEVICE_ID *value, v1_IGameInputDevice **device )
{
    FIXME( "iface %p FindDeviceFromId value=%p device_out=%p\n", iface, value, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input1_FindDeviceFromPlatformString( v1_IGameInput *iface, const WCHAR *value, v1_IGameInputDevice **device )
{
    FIXME( "iface %p FindDeviceFromPlatformString value=%p device_out=%p\n", iface, value, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static void WINAPI game_input1_SetFocusPolicy( v1_IGameInput *iface, GameInputFocusPolicy policy )
{
    FIXME( "iface %p SetFocusPolicy policy=%d\n", iface, (int)policy );
    /* no-op stub */
}

static const struct v1_IGameInputVtbl game_input1_vtbl =
{
    /* IUnknown methods */
    game_input1_QueryInterface,
    game_input1_AddRef,
    game_input1_Release,
    /* v0_IGameInput methods */
    game_input1_GetCurrentTimestamp,
    game_input1_GetCurrentReading,
    game_input1_GetNextReading,
    game_input1_GetPreviousReading,
    game_input1_RegisterReadingCallback,
    game_input1_RegisterDeviceCallback,
    game_input1_RegisterSystemButtonCallback,
    game_input1_RegisterKeyboardLayoutCallback,
    game_input1_StopCallback,
    game_input1_UnregisterCallback,
    game_input1_CreateDispatcher,
    game_input1_FindDeviceFromId,
    game_input1_FindDeviceFromPlatformString,
    game_input1_SetFocusPolicy
};

HRESULT WINAPI GameInputCreate( v0_IGameInput **out )
{
    struct game_input *input;

    TRACE( "out %p\n", out );

    if (!(input = calloc( 1, sizeof(*input) ))) return E_OUTOFMEMORY;

    input->v0_IGameInput_iface.lpVtbl = &game_input_vtbl;
    input->v1_IGameInput_iface.lpVtbl = &game_input1_vtbl;
    input->ref = 1;

    *out = &input->v0_IGameInput_iface;

    return S_OK;
}

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}
