/*
 * Game Input Library
 *  -> Game Input Devices
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

#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

#include "private.h"

struct device_callback
{
    v2_GameInputDeviceCallback callback;
    GameInputDeviceStatus filter;
    GameInputKind kind;
    v2_IGameInputDevice *device;
    PVOID context;
    GameInputCallbackToken token;
    struct list entry;
};

struct game_input_device
{
    v2_IGameInputDevice v2_IGameInputDevice_iface;
    v2_GameInputDeviceInfo deviceInfo;

    GameInputHapticInfo hapticInfo;
    GameInputDeviceStatus previousDeviceStatus;
    GameInputDeviceStatus deviceStatus;
    GameInputRumbleParams rumbleState;

    // Private members:
    struct list entry;

    HDEVINFO *hidInfo;
    SP_DEVINFO_DATA hidData;
    PHIDP_PREPARSED_DATA preparsed;
    HANDLE deviceHandle;

    OVERLAPPED readOv;
    PCHAR readBuffer;
    ULONG readBufferLen;
    BOOLEAN readPending;
    CRITICAL_SECTION readLock;
    LPWSTR pnpPath;

    LPDIRECTINPUTDEVICE8W pDevice;

    LONG ref;
};

HRESULT game_input_device_AcquireDInputDevice( IN v2_IGameInputDevice *iface, OUT LPDIRECTINPUTDEVICE8W *device );
HRESULT game_input_device_SetDInputDevice( IN v2_IGameInputDevice *iface, IN LPDIRECTINPUTDEVICE8W device );
HRESULT game_input_device_Create( v2_IGameInputDevice **device );
HRESULT game_input_device_AddGameHID( IN v2_IGameInputDevice *iface, IN LPWSTR devicePath );
HRESULT game_input_device_OpenDevice( IN v2_IGameInputDevice *iface );
HRESULT game_input_device_CloseDevice( IN v2_IGameInputDevice *iface );
HRESULT game_input_device_QueryGameHIDButtonCaps( IN v2_IGameInputDevice *iface, OUT HIDP_BUTTON_CAPS **btnCaps, OUT USHORT *nBtnCaps );
HRESULT game_input_device_QueryGameHIDValueCaps( IN v2_IGameInputDevice *iface, OUT HIDP_VALUE_CAPS **valueCaps, OUT USHORT *nValueCaps );
HRESULT game_input_device_PollHIDDevice( IN v2_IGameInputDevice *iface );
HRESULT game_input_device_CurrentButtons( IN v2_IGameInputDevice *iface, OUT USAGE usageList[128], OUT ULONG *usageLength );
HRESULT game_input_device_CurrentValue( IN v2_IGameInputDevice *iface, IN USAGE usagePage, IN USAGE usage, OUT PLONG value );

HRESULT WINAPI RegisterDeviceCallback( v2_IGameInputDevice *device, GameInputKind kind, GameInputDeviceStatus filter, void *context, v2_GameInputDeviceCallback callback, GameInputCallbackToken *token );
DWORD WINAPI DeviceMonitorThread( PVOID lpParam );

#endif