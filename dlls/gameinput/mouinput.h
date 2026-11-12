/*
 * Game Input Library
 *  -> Keyboard Input Events
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

#ifndef _MOUINPUT_H
#define _MOUINPUT_H

#include "private.h"

HRESULT mouse_input_device_InitDevice( IN v2_IGameInputDevice *device );
HRESULT mouse_input_device_InitDInput8Device( IN v2_IGameInputDevice *device );
HRESULT mouse_input_device_ReadCurrentStateFromHID( IN v2_IGameInputDevice *device, IN uint64_t timestamp, OUT v2_IGameInputReading **reading );
HRESULT mouse_input_device_ReadCurrentStateFromDInput8( IN v2_IGameInputDevice *device, IN uint64_t timestamp, OUT v2_IGameInputReading **reading );

struct mouse_sample
{
    UINT64 seq;
    UINT64 ts_us;
    GameInputMouseButtons buttons;
    LONG rel_x, rel_y, wheel_y;
};

UINT64 mouse_input_now_us( void );
void mouse_input_push_button( DWORD message, DWORD mouse_data );
BOOL mouse_input_find_sample( UINT64 ts_us, int direction, struct mouse_sample *out );
HRESULT mouse_input_device_CreateReadingForSample( IN v2_IGameInputDevice *device, IN const struct mouse_sample *sample, OUT v2_IGameInputReading **reading );

#endif