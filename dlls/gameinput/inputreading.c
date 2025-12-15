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
    TRACE( "iface %p.\n", iface );
    return GameInputKindMouse;
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
    if (device) *device = impl->device;
    return;
}

static uint32_t WINAPI game_input_reading_GetControllerAxisCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI game_input_reading_GetControllerAxisState( v2_IGameInputReading *iface, uint32_t count, float *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI game_input_reading_GetControllerButtonCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI game_input_reading_GetControllerButtonState( v2_IGameInputReading *iface, uint32_t count, bool *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI game_input_reading_GetControllerSwitchCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI game_input_reading_GetControllerSwitchState( v2_IGameInputReading *iface, uint32_t count, GameInputSwitchPosition *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI game_input_reading_GetKeyCount( v2_IGameInputReading *iface )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static uint32_t WINAPI game_input_reading_GetKeyState( v2_IGameInputReading *iface, uint32_t count, GameInputKeyState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return 0;
}

static bool WINAPI game_input_reading_GetMouseState( v2_IGameInputReading *iface, v2_GameInputMouseState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    TRACE( "iface %p, state %p.\n", iface, state );
    *state = impl->mouseState;
    return true;
}

static bool WINAPI game_input_reading_GetSensorsState( v2_IGameInputReading *iface, GameInputSensorsState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI game_input_reading_GetArcadeStickState( v2_IGameInputReading *iface, GameInputArcadeStickState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI game_input_reading_GetFlightStickState( v2_IGameInputReading *iface, GameInputFlightStickState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI game_input_reading_GetGamepadState( v2_IGameInputReading *iface, GameInputGamepadState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    FIXME( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI game_input_reading_GetRacingWheelState( v2_IGameInputReading *iface, GameInputRacingWheelState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
}

static bool WINAPI game_input_reading_GetUiNavigationState( v2_IGameInputReading *iface, GameInputUiNavigationState *state )
{
    struct game_input_reading *impl = impl_from_IGameInputReading( iface );
    ERR( "Not for this device %p!\n", impl->device );
    return false;
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
    impl->ref = 1;

    *out = &impl->v2_IGameInputReading_iface;
    TRACE( "created v2_IGameInputReading %p\n", &impl->v2_IGameInputReading_iface );

    return S_OK;
}