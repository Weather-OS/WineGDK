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

#include "initguid.h"
#include "private.h"

#include <time.h>
#include "ddk/hidclass.h"
#include "ntddkbd.h"
#include "ntddmou.h"

#include "mouinput.h"

WINE_DEFAULT_DEBUG_CHANNEL(ginput);

DEFINE_ASYNC_COMPLETED_HANDLER( device_information_collection_handler, IAsyncOperationCompletedHandler_DeviceInformationCollection, IAsyncOperation_DeviceInformationCollection )

extern const struct v2_IGameInputDeviceVtbl mouse_input2_device_vtbl;

struct game_input
{
    v0_IGameInput v0_IGameInput_iface;
    v1_IGameInput v1_IGameInput_iface;
    v2_IGameInput v2_IGameInput_iface;

    GInputDeviceEvents_v2 device_events;
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

    if (IsEqualGUID( iid, &IID_v2_IGameInput ))
    {
        *out = &impl->v2_IGameInput_iface;
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

static uint64_t WINAPI game_input_GetCurrentTimestamp( v0_IGameInput *iface )
{
    FIXME( "iface %p, stub GetCurrentTimestamp.\n", iface );
    return (uint64_t)0;
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

    if (IsEqualGUID( iid, &IID_v2_IGameInput ))
    {
        *out = &impl->v2_IGameInput_iface;
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

static uint64_t WINAPI game_input1_GetCurrentTimestamp( v1_IGameInput *iface )
{
    TRACE( "iface %p, stub GetCurrentTimestamp.\n", iface );
    return (uint64_t)0;
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

static inline struct game_input *impl_from_v2_IGameInput( v2_IGameInput *iface )
{
    return CONTAINING_RECORD( iface, struct game_input, v2_IGameInput_iface );
}

static HRESULT WINAPI game_input2_QueryInterface( v2_IGameInput *iface, REFIID iid, void **out )
{
    struct game_input *impl = impl_from_v2_IGameInput( iface );

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

    if (IsEqualGUID( iid, &IID_v2_IGameInput ))
    {
        *out = &impl->v2_IGameInput_iface;
        impl->ref++;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI game_input2_AddRef( v2_IGameInput *iface )
{
    struct game_input *impl = impl_from_v2_IGameInput( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI game_input2_Release( v2_IGameInput *iface )
{
    struct game_input *impl = impl_from_v2_IGameInput( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
};

static uint64_t WINAPI game_input2_GetCurrentTimestamp( v2_IGameInput *iface )
{
    TRACE( "iface %p, stub GetCurrentTimestamp.\n", iface );
    return (uint64_t)0;
}

static HRESULT WINAPI game_input2_GetCurrentReading( v2_IGameInput *iface, GameInputKind kind,
                                                   v2_IGameInputDevice *device, v2_IGameInputReading **reading )
{
    FIXME( "iface %p GetCurrentReading kind=%d device=%p reading_out=%p\n", iface, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input2_GetNextReading( v2_IGameInput *iface, v2_IGameInputReading *reference,
                                                 GameInputKind kind, v2_IGameInputDevice *device,
                                                 v2_IGameInputReading **reading )
{
    FIXME( "iface %p GetNextReading reference=%p kind=%d device=%p reading_out=%p\n", iface, reference, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input2_GetPreviousReading( v2_IGameInput *iface, v2_IGameInputReading *reference,
                                                     GameInputKind kind, v2_IGameInputDevice *device,
                                                     v2_IGameInputReading **reading )
{
    FIXME( "iface %p GetPreviousReading reference=%p kind=%d device=%p reading_out=%p\n", iface, reference, (int)kind, device, reading );
    if (reading) *reading = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input2_RegisterReadingCallback( v2_IGameInput *iface, v2_IGameInputDevice *device,
                                                          GameInputKind kind, void *context,
                                                          v2_GameInputReadingCallback callback, GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterReadingCallback device=%p kind=%d callback=%p token_out=%p\n",
           iface, device, (int)kind, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input2_RegisterDeviceCallback( v2_IGameInput *iface, v2_IGameInputDevice *device,
                                                         GameInputKind kind, GameInputDeviceStatus filter,
                                                         GameInputEnumerationKind enum_kind, void *context,
                                                         v2_GameInputDeviceCallback callback, GameInputCallbackToken *token )
{
    BOOLEAN exists;
    HRESULT status;
    HSTRING idStr;
    OLECHAR guidString[39];
    SIZE_T lowercaseIterator;
    DWORD asyncRes;

    struct game_input *impl = impl_from_v2_IGameInput( iface );
    struct game_input_device *input_device;

    IAsyncOperation_DeviceInformationCollection *device_information_collection_operation = NULL;
    IVectorView_DeviceInformation *device_information_collection = NULL;
    IIterable_DeviceInformation *device_information_iterable = NULL;
    IIterator_DeviceInformation *device_information_iterator = NULL;
    IDeviceInformation *device_information = NULL;

    FIXME( "iface %p device %p kind %d filter %d enum_kind %d callback %p token_out %p semi-stub.\n",
           iface, device, kind, filter, enum_kind, callback, token );

    status = IDeviceInformationStatics_FindAllAsync( impl->device_events.device_information_statics, &device_information_collection_operation );
    if ( FAILED( status ) ) return status;

    asyncRes = await_IAsyncOperation_DeviceInformationCollection( device_information_collection_operation, INFINITE );
    if ( asyncRes ) return E_FAIL;

    status = IAsyncOperation_DeviceInformationCollection_GetResults( device_information_collection_operation, &device_information_collection );
    if ( FAILED( status ) ) return status;

    status = IVectorView_DeviceInformation_QueryInterface( device_information_collection, &IID_IIterable_DeviceInformation, (void **)&device_information_iterable );
    if ( FAILED( status ) ) return status;

    status = IIterable_DeviceInformation_First( device_information_iterable, &device_information_iterator );
    if ( FAILED( status ) ) return status;

    IIterator_DeviceInformation_get_HasCurrent( device_information_iterator, &exists );

    while ( exists )
    {
        IIterator_DeviceInformation_get_Current( device_information_iterator, &device_information );
        IDeviceInformation_get_Id( device_information, &idStr );
        IIterator_DeviceInformation_MoveNext( device_information_iterator, &exists );

        if ( kind & GameInputKindMouse )
            StringFromGUID2( &GUID_DEVINTERFACE_MOUSE, guidString, ARRAYSIZE( guidString ) );

        for ( lowercaseIterator = 0; lowercaseIterator < wcslen( guidString ); lowercaseIterator++ )
            guidString[lowercaseIterator] = towlower( guidString[lowercaseIterator] );

        if ( wcsstr( WindowsGetStringRawBuffer( idStr, NULL ), guidString ) )
        {
            // Device found routine
            if (!(input_device = calloc( 1, sizeof(*input_device) ))) return E_OUTOFMEMORY;
            
            input_device->v2_IGameInputDevice_iface.lpVtbl = &mouse_input2_device_vtbl;
            input_device->ref = 1;

            mouse_input2_device_QueryDeviceInformation( &input_device->device_info_v2 );

            callback( 1, context, &input_device->v2_IGameInputDevice_iface, time(NULL), GameInputDeviceConnected, GameInputDeviceConnected );
        }
    }

    if (token) *token = (GameInputCallbackToken)1;
    return S_OK;
}

static HRESULT WINAPI game_input2_RegisterSystemButtonCallback( v2_IGameInput *iface, v2_IGameInputDevice *device,
                                                               GameInputSystemButtons filter, void *context,
                                                               v2_GameInputSystemButtonCallback callback, GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterSystemButtonCallback device=%p filter=%u callback=%p token_out=%p\n",
           iface, device, (unsigned)filter, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input2_RegisterKeyboardLayoutCallback( v2_IGameInput *iface, v2_IGameInputDevice *device,
                                                                 void *context, v2_GameInputKeyboardLayoutCallback callback,
                                                                 GameInputCallbackToken *token )
{
    FIXME( "iface %p RegisterKeyboardLayoutCallback device=%p callback=%p token_out=%p\n",
           iface, device, callback, token );
    if (token) *token = (GameInputCallbackToken)0;
    return E_NOTIMPL;
}

static void WINAPI game_input2_StopCallback( v2_IGameInput *iface, GameInputCallbackToken token )
{
    FIXME( "iface %p StopCallback token=%llu\n", iface, (unsigned long long)token );
    /* no-op stub */
}

static bool WINAPI game_input2_UnregisterCallback( v2_IGameInput *iface, GameInputCallbackToken token )
{
    FIXME( "iface %p UnregisterCallback token=%llu\n", iface, (unsigned long long)token );
    return TRUE;
}

static HRESULT WINAPI game_input2_CreateDispatcher( v2_IGameInput *iface, IGameInputDispatcher **dispatcher )
{
    FIXME( "iface %p CreateDispatcher dispatcher_out=%p\n", iface, dispatcher );
    if (dispatcher) *dispatcher = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input2_FindDeviceFromId( v2_IGameInput *iface, const APP_LOCAL_DEVICE_ID *value, v2_IGameInputDevice **device )
{
    FIXME( "iface %p FindDeviceFromId value=%p device_out=%p\n", iface, value, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI game_input2_FindDeviceFromPlatformString( v2_IGameInput *iface, const WCHAR *value, v2_IGameInputDevice **device )
{
    FIXME( "iface %p FindDeviceFromPlatformString value=%p device_out=%p\n", iface, value, device );
    if (device) *device = NULL;
    return E_NOTIMPL;
}

static void WINAPI game_input2_SetFocusPolicy( v2_IGameInput *iface, GameInputFocusPolicy policy )
{
    FIXME( "iface %p SetFocusPolicy policy=%d\n", iface, (int)policy );
    /* no-op stub */
}

static const struct v2_IGameInputVtbl game_input2_vtbl =
{
    /* IUnknown methods */
    game_input2_QueryInterface,
    game_input2_AddRef,
    game_input2_Release,
    /* v0_IGameInput methods */
    game_input2_GetCurrentTimestamp,
    game_input2_GetCurrentReading,
    game_input2_GetNextReading,
    game_input2_GetPreviousReading,
    game_input2_RegisterReadingCallback,
    game_input2_RegisterDeviceCallback,
    game_input2_RegisterSystemButtonCallback,
    game_input2_RegisterKeyboardLayoutCallback,
    game_input2_StopCallback,
    game_input2_UnregisterCallback,
    game_input2_CreateDispatcher,
    game_input2_FindDeviceFromId,
    game_input2_FindDeviceFromPlatformString,
    game_input2_SetFocusPolicy
};

HRESULT WINAPI GameInputCreate( v0_IGameInput **out )
{
    LPCWSTR device_information_str = RuntimeClass_Windows_Devices_Enumeration_DeviceInformation;
    HRESULT status;
    HSTRING device_information_class;

    struct game_input *input = NULL;

    TRACE( "out %p\n", out );

    if (!(input = calloc( 1, sizeof(*input) ))) return E_OUTOFMEMORY;

    input->v0_IGameInput_iface.lpVtbl = &game_input_vtbl;
    input->v1_IGameInput_iface.lpVtbl = &game_input1_vtbl;
    input->v2_IGameInput_iface.lpVtbl = &game_input2_vtbl;
    input->ref = 1;

    status = WindowsCreateString( device_information_str, wcslen( device_information_str ), &device_information_class );
    if ( FAILED( status ) ) goto _CLEANUP;

    status = RoGetActivationFactory( device_information_class, &IID_IDeviceInformationStatics, (void **)&input->device_events.device_information_statics );
    if ( FAILED( status ) ) goto _CLEANUP;

    status = IDeviceInformationStatics_CreateWatcher( input->device_events.device_information_statics, &input->device_events.watcher );
    if ( FAILED( status ) ) goto _CLEANUP;

    *out = &input->v0_IGameInput_iface;

_CLEANUP:
    if ( input->device_events.device_information_statics ) 
        IDeviceInformationStatics_Release( input->device_events.device_information_statics );
    if ( FAILED( status ) )
    {
        if ( input ) free( input );
    }
    WindowsDeleteString( device_information_class );

    return S_OK;
}

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}
