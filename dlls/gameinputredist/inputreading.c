/*
 * Game Input Library
 *  -> Game Input Reading
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

#include "inputreading.h"

WINE_DEFAULT_DEBUG_CHANNEL(ginput);

static inline struct game_input_reading *impl_from_IGameInputReading( v2_IGameInputReading *iface )
{
    return CONTAINING_RECORD( iface, struct game_input_reading, v2_IGameInputReading_iface );
}

static HRESULT WINAPI game_input_reading_QueryInterface( v2_IGameInputReading *iface, REFIID iid, void **out )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );

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

static ULONG WINAPI game_input_reading_AddRef( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI game_input_reading_Release( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
};

static GameInputKind WINAPI game_input_reading_GetInputKind( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );

    HRESULT status;
    const v2_GameInputDeviceInfo *device_info;

    TRACE( "iface %p.\n", iface );

    status = v2_IGameInputDevice_GetDeviceInfo( impl->device, &device_info );
    if ( FAILED( status ) ) return 0;

    return device_info->supportedInput;
}

static uint64_t WINAPI game_input_reading_GetTimestamp( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p.\n", iface );
    return impl->timestamp;
}

static void WINAPI game_input_reading_GetDevice( v2_IGameInputReading *iface, v2_IGameInputDevice **device )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, device %p.\n", iface, device );
    if ( device )
        v2_IGameInputDevice_QueryInterface( impl->device, &IID_v2_IGameInputDevice, (void **)device );
    return;
}

static uint32_t WINAPI game_input_reading_GetControllerAxisCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );

    HRESULT status;
    const v2_GameInputDeviceInfo *device_info;

    TRACE( "iface %p.\n", iface );

    status = v2_IGameInputDevice_GetDeviceInfo( impl->device, &device_info );
    if ( FAILED( status ) ) return 0;

    return device_info->controllerAxisCount;
}

static uint32_t WINAPI game_input_reading_GetControllerAxisState( v2_IGameInputReading *iface, uint32_t count, float *state )
{
    uint32_t copy;
    uint32_t numAxes;
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    
    TRACE( "iface %p, count %d, state %p!\n", iface, count, state );

    numAxes = v2_IGameInputReading_GetControllerAxisCount( iface );
    copy = ( count < numAxes ) ? count : numAxes;

    memcpy( state, impl->controllerAxisState, copy * sizeof( float ) );

    return copy;
}

static uint32_t WINAPI game_input_reading_GetControllerButtonCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );

    HRESULT status;
    const v2_GameInputDeviceInfo *device_info;

    TRACE( "iface %p.\n", iface );

    status = v2_IGameInputDevice_GetDeviceInfo( impl->device, &device_info );
    if ( FAILED( status ) ) return 0;

    return device_info->controllerButtonCount;
}

static uint32_t WINAPI game_input_reading_GetControllerButtonState( v2_IGameInputReading *iface, uint32_t count, bool *state )
{
    uint32_t copy;
    uint32_t numButtons;
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    
    TRACE( "iface %p, count %d, state %p!\n", iface, count, state );

    numButtons = v2_IGameInputReading_GetControllerButtonCount( iface );
    copy = ( count < numButtons ) ? count : numButtons;

    memcpy( state, impl->controllerButtonState, copy * sizeof( bool ) );

    return copy;
}

static uint32_t WINAPI game_input_reading_GetControllerSwitchCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );

    HRESULT status;
    const v2_GameInputDeviceInfo *device_info;

    TRACE( "iface %p.\n", iface );

    status = v2_IGameInputDevice_GetDeviceInfo( impl->device, &device_info );
    if ( FAILED( status ) ) return 0;

    return device_info->controllerSwitchCount;
}

static uint32_t WINAPI game_input_reading_GetControllerSwitchState( v2_IGameInputReading *iface, uint32_t count, GameInputSwitchPosition *state )
{
    uint32_t copy;
    uint32_t numSwitches;
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    
    TRACE( "iface %p, count %d, state %p!\n", iface, count, state );

    numSwitches = v2_IGameInputReading_GetControllerSwitchCount( iface );
    copy = ( count < numSwitches ) ? count : numSwitches;

    memcpy( state, impl->switchState, copy * sizeof( GameInputSwitchPosition ) );

    return copy;
}

// MSDN Error: GetKeyCount API is for Keyboards, not Game Controllers!
// Reference: https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/input/gameinput-v2/interfaces/igameinputreading/methods/igameinputreading_getkeycount-v2
static uint32_t WINAPI game_input_reading_GetKeyCount( v2_IGameInputReading *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return 0;
}

static uint32_t WINAPI game_input_reading_GetKeyState( v2_IGameInputReading *iface, uint32_t count, GameInputKeyState *state )
{
    FIXME( "iface %p, count %d, state %p stub!\n", iface, count, state );
    return 0;
}

static bool WINAPI game_input_reading_GetMouseState( v2_IGameInputReading *iface, v2_GameInputMouseState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    if( state ) *state = impl->mouseState;
    return !!state;
}

static bool WINAPI game_input_reading_GetSensorsState( v2_IGameInputReading *iface, GameInputSensorsState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    if( state ) *state = impl->sensorsState;
    return !!state;
}

static bool WINAPI game_input_reading_GetArcadeStickState( v2_IGameInputReading *iface, GameInputArcadeStickState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    if( state ) *state = impl->arcadeStickState;
    return !!state;
}

static bool WINAPI game_input_reading_GetFlightStickState( v2_IGameInputReading *iface, GameInputFlightStickState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    if( state ) *state = impl->flightStickState;
    return !!state;
}

static bool WINAPI game_input_reading_GetGamepadState( v2_IGameInputReading *iface, GameInputGamepadState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    if( state ) *state = impl->gamepadState;
    return !!state;
}

static bool WINAPI game_input_reading_GetRacingWheelState( v2_IGameInputReading *iface, GameInputRacingWheelState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    if( state ) *state = impl->racingWheelState;
    return !!state;
}

static bool WINAPI game_input_reading_GetUiNavigationState( v2_IGameInputReading *iface, GameInputUiNavigationState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    if( state ) *state = impl->uiNavigationState;
    return !!state;
}

const struct v2_IGameInputReadingVtbl game_input_reading_vtbl =
{
    /* IUnknown methods */
    game_input_reading_QueryInterface,
    game_input_reading_AddRef,
    game_input_reading_Release,
    /* v2_IGameInputReading */
    game_input_reading_GetInputKind,
    game_input_reading_GetTimestamp,
    game_input_reading_GetDevice,
    game_input_reading_GetControllerAxisCount,
    game_input_reading_GetControllerAxisState,
    game_input_reading_GetControllerButtonCount,
    game_input_reading_GetControllerButtonState,
    game_input_reading_GetControllerSwitchCount,
    game_input_reading_GetControllerSwitchState,
    game_input_reading_GetKeyCount,
    game_input_reading_GetKeyState,
    game_input_reading_GetMouseState,
    game_input_reading_GetSensorsState,
    game_input_reading_GetArcadeStickState,
    game_input_reading_GetFlightStickState,
    game_input_reading_GetGamepadState,
    game_input_reading_GetRacingWheelState,
    game_input_reading_GetUiNavigationState
};

HRESULT game_input_reading_CreateForMouseDevice( v2_IGameInputDevice *device, v2_GameInputMouseState state, uint64_t timestamp, v2_IGameInputReading **out )
{
    struct game_input_reading *impl;

    TRACE( "device %p\n", device );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->v2_IGameInputReading_iface.lpVtbl = &game_input_reading_vtbl;
    impl->mouseState = state;
    impl->device = device;
    impl->ref = 1;

    *out = &impl->v2_IGameInputReading_iface;
    TRACE( "created v2_IGameInputReading %p\n", &impl->v2_IGameInputReading_iface );

    return S_OK;
}

HRESULT game_input_reading_CreateForGamepadDevice( v2_IGameInputDevice *device, GameInputGamepadState state, uint64_t timestamp, v2_IGameInputReading **out )
{
    struct game_input_reading *impl;

    TRACE( "device %p\n", device );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->v2_IGameInputReading_iface.lpVtbl = &game_input_reading_vtbl;
    impl->gamepadState = state;
    impl->device = device;
    impl->ref = 1;

    *out = &impl->v2_IGameInputReading_iface;
    TRACE( "created v2_IGameInputReading %p\n", &impl->v2_IGameInputReading_iface );

    return S_OK;
}