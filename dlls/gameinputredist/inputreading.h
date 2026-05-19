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

#ifndef INPUT_READING_H
#define INPUT_READING_H

#include "private.h"

struct game_input_reading
{
    v2_IGameInputReading v2_IGameInputReading_iface;
    v2_IGameInputDevice *device;

    v2_GameInputMouseState mouseState;
    GameInputSensorsState sensorsState;
    GameInputArcadeStickState arcadeStickState;
    GameInputFlightStickState flightStickState;
    GameInputGamepadState gamepadState;
    GameInputRacingWheelState racingWheelState;
    GameInputUiNavigationState uiNavigationState;
    GameInputSwitchPosition *switchState;
    GameInputKeyState *keyState;
    FLOAT *controllerAxisState;
    BOOL *controllerButtonState;

    uint64_t timestamp;

    LONG ref;
};

HRESULT game_input_reading_CreateForMouseDevice( v2_IGameInputDevice *device, v2_GameInputMouseState state, uint64_t timestamp, v2_IGameInputReading **out );
HRESULT game_input_reading_CreateForGamepadDevice( v2_IGameInputDevice *device, GameInputGamepadState state, uint64_t timestamp, v2_IGameInputReading **out );

#endif