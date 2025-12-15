/*
 * Game Input Library
 *  -> Game Pad Input Events
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

#include "mouinput.h"

#include <time.h>

WINE_DEFAULT_DEBUG_CHANNEL(ginput);

HRESULT WINAPI gamepad_input2_device_QueryDeviceInformation( v2_GameInputDeviceInfo *info )
{
    v2_GameInputControllerAxisInfo *axis_info;
    v2_GameInputControllerButtonInfo *button_info;
    GameInputGamepadInfo *gamePadInfo;
    v2_GameInputDeviceInfo device_info;

    if (!(gamePadInfo = calloc( 1, sizeof(*gamePadInfo) ))) return E_OUTOFMEMORY;

    mouse_info->supportedButtons = 0x7F;
    mouse_info->sampleRate = 500;
    mouse_info->hasWheelX = TRUE;
    mouse_info->hasWheelY = TRUE;

    device_info.supportedInput = GameInputKindMouse;
    device_info.mouseInfo = mouse_info;
    device_info.deviceFamily = GameInputFamilyHid;

    *info = device_info;

    return S_OK;
}

static inline struct game_input_device *impl_from_v2_IGameInputDevice( v2_IGameInputDevice *iface )
{
    return CONTAINING_RECORD( iface, struct game_input_device, v2_IGameInputDevice_iface );
}

static HRESULT WINAPI gamepad_input2_device_QueryInterface( v2_IGameInputDevice *iface, REFIID iid, void **out )
{
    struct game_input_device *impl = impl_from_v2_IGameInputDevice( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_v2_IGameInputDevice ))
    {
        *out = &impl->v2_IGameInputDevice_iface;
        impl->ref++;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI gamepad_input2_device_AddRef( v2_IGameInputDevice *iface )
{
    struct game_input_device *impl = impl_from_v2_IGameInputDevice( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI gamepad_input2_device_Release( v2_IGameInputDevice *iface )
{
    struct game_input_device *impl = impl_from_v2_IGameInputDevice( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
};

static HRESULT WINAPI gamepad_input2_device_GetDeviceInfo( v2_IGameInputDevice *iface, const v2_GameInputDeviceInfo **info )
{
    struct game_input_device *impl = impl_from_v2_IGameInputDevice( iface );
    TRACE( "iface %p, info %p.\n", iface, info );
    *info = &impl->device_info_v2;
    return S_OK;
}

static HRESULT WINAPI gamepad_input2_device_GetHapticInfo( v2_IGameInputDevice *iface, GameInputHapticInfo *info )
{
    FIXME( "iface %p, info %p stub!\n", iface, info );
    return E_NOTIMPL;
}

static GameInputDeviceStatus WINAPI gamepad_input2_device_GetDeviceStatus( v2_IGameInputDevice *iface )
{
    FIXME( "iface %p, stub!\n", iface );
    return 0;
}

static HRESULT WINAPI gamepad_input2_device_CreateForceFeedbackEffect( v2_IGameInputDevice *iface, uint32_t motor, const GameInputForceFeedbackParams *params, v2_IGameInputForceFeedbackEffect **effect )
{
    FIXME( "iface %p, motor %d, params %p, effect %p stub!\n", iface, motor, params, effect );
    return E_NOTIMPL;
}

static bool WINAPI gamepad_input2_device_IsForceFeedbackMotorPoweredOn( v2_IGameInputDevice *iface, uint32_t motor )
{
    FIXME( "iface %p, motor %d stub!\n", iface, motor );
    return FALSE;
}

static VOID WINAPI gamepad_input2_device_SetForceFeedbackMotorGain( v2_IGameInputDevice *iface, uint32_t motor, float gain )
{
    FIXME( "iface %p, motor %d, gain %f stub!\n", iface, motor, gain );
    return;
}

static VOID WINAPI gamepad_input2_device_SetRumbleState( v2_IGameInputDevice *iface, const GameInputRumbleParams *params )
{
    FIXME( "iface %p, params %p stub!\n", iface, params );
    return;
}

static HRESULT WINAPI gamepad_input2_device_DirectInputEscape( v2_IGameInputDevice *iface, uint32_t command, const void *input, uint32_t in_size, void *output, uint32_t out_size, uint32_t *size )
{
    FIXME( "iface %p, stub!\n", iface );
    return E_NOTIMPL;
}

const struct v2_IGameInputDeviceVtbl gamepad_input2_device_vtbl =
{
    /* IUnknown methods */
    gamepad_input2_device_QueryInterface,
    gamepad_input2_device_AddRef,
    gamepad_input2_device_Release,
    /* v2_IGameInputDevice methods */
    gamepad_input2_device_GetDeviceInfo,
    gamepad_input2_device_GetHapticInfo,
    gamepad_input2_device_GetDeviceStatus,
    gamepad_input2_device_CreateForceFeedbackEffect,
    gamepad_input2_device_IsForceFeedbackMotorPoweredOn,
    gamepad_input2_device_SetForceFeedbackMotorGain,
    gamepad_input2_device_SetRumbleState,
    gamepad_input2_device_DirectInputEscape
};

static inline struct game_input_reading *impl_from_v2_IGameInputReading( v2_IGameInputReading *iface )
{
    return CONTAINING_RECORD( iface, struct game_input_reading, v2_IGameInputReading_iface );
}

static HRESULT WINAPI mouse_input2_reading_QueryInterface( v2_IGameInputReading *iface, REFIID iid, void **out )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_v2_IGameInputReading ))
    {
        *out = &impl->v2_IGameInputReading_iface;
        impl->ref++;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI mouse_input2_reading_AddRef( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI mouse_input2_reading_Release( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
};

static GameInputKind WINAPI mouse_input2_reading_GetInputKind( v2_IGameInputReading *iface )
{
    TRACE( "iface %p.\n", iface );
    return GameInputKindGamepad;
}

static uint64_t WINAPI mouse_input2_reading_GetTimestamp( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    TRACE( "iface %p.\n", iface );
    return impl->timestamp;
}

static void WINAPI mouse_input2_reading_GetDevice( v2_IGameInputReading *iface, v2_IGameInputDevice **device )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    TRACE( "iface %p, device %p.\n", iface, device );
    if (device) *device = impl->device;
    return;
}

static uint32_t WINAPI mouse_input2_reading_GetControllerAxisCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI mouse_input2_reading_GetControllerAxisState( v2_IGameInputReading *iface, uint32_t count, float *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI mouse_input2_reading_GetControllerButtonCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI mouse_input2_reading_GetControllerButtonState( v2_IGameInputReading *iface, uint32_t count, bool *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI mouse_input2_reading_GetControllerSwitchCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI mouse_input2_reading_GetControllerSwitchState( v2_IGameInputReading *iface, uint32_t count, GameInputSwitchPosition *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI mouse_input2_reading_GetKeyCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI mouse_input2_reading_GetKeyState( v2_IGameInputReading *iface, uint32_t count, GameInputKeyState *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static bool WINAPI mouse_input2_reading_GetMouseState( v2_IGameInputReading *iface, v2_GameInputMouseState *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    *state = impl->mouseState;
    return true;
}

static bool WINAPI mouse_input2_reading_GetSensorsState( v2_IGameInputReading *iface, GameInputSensorsState *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI mouse_input2_reading_GetArcadeStickState( v2_IGameInputReading *iface, GameInputArcadeStickState *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI mouse_input2_reading_GetFlightStickState( v2_IGameInputReading *iface, GameInputFlightStickState *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI mouse_input2_reading_GetGamepadState( v2_IGameInputReading *iface, GameInputGamepadState *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI mouse_input2_reading_GetRacingWheelState( v2_IGameInputReading *iface, GameInputRacingWheelState *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI mouse_input2_reading_GetUiNavigationState( v2_IGameInputReading *iface, GameInputUiNavigationState *state )
{
    struct game_input_reading *impl = impl_from_v2_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

const struct v2_IGameInputReadingVtbl mouse_input2_reading_vtbl =
{
    /* IUnknown methods */
    mouse_input2_reading_QueryInterface,
    mouse_input2_reading_AddRef,
    mouse_input2_reading_Release,
    /* v2_IGameInputReading */
    mouse_input2_reading_GetInputKind,
    mouse_input2_reading_GetTimestamp,
    mouse_input2_reading_GetDevice,
    mouse_input2_reading_GetControllerAxisCount,
    mouse_input2_reading_GetControllerAxisState,
    mouse_input2_reading_GetControllerButtonCount,
    mouse_input2_reading_GetControllerButtonState,
    mouse_input2_reading_GetControllerSwitchCount,
    mouse_input2_reading_GetControllerSwitchState,
    mouse_input2_reading_GetKeyCount,
    mouse_input2_reading_GetKeyState,
    mouse_input2_reading_GetMouseState,
    mouse_input2_reading_GetSensorsState,
    mouse_input2_reading_GetArcadeStickState,
    mouse_input2_reading_GetFlightStickState,
    mouse_input2_reading_GetGamepadState,
    mouse_input2_reading_GetRacingWheelState,
    mouse_input2_reading_GetUiNavigationState
};
