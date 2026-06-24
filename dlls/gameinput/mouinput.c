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

#include "mouinput.h"
#include "inputdevice.h"
#include "inputreading.h"

#include <time.h>

WINE_DEFAULT_DEBUG_CHANNEL(ginput);

extern HINSTANCE game_input;

static GameInputMouseButtons storedButtons = GameInputMouseNone;
static POINT relativePositionStore = {.x = 0, .y = 0};
static LONG relativeWheelStore = 0;

static VOID append_supported_mouse_buttons( GameInputMouseButtons *buttons, HIDP_BUTTON_CAPS *caps, USHORT nCaps )
{
    GameInputMouseButtons currentButtons = *buttons;
    HIDP_BUTTON_CAPS currentCap;
    USHORT capsIterator;

    TRACE( "buttons %p, caps %p, nCaps %d.\n", buttons, caps, nCaps );

    for ( capsIterator = 0; capsIterator < nCaps; capsIterator++ )
    {
        currentCap = caps[capsIterator];

        if ( currentCap.UsagePage != HID_USAGE_PAGE_BUTTON )
            continue;

        if ( currentCap.IsRange )
        {
            if ( 0x01 >= currentCap.Range.UsageMin && 0x01 <= currentCap.Range.UsageMax )
                currentButtons |= GameInputMouseLeftButton;
            if ( 0x02 >= currentCap.Range.UsageMin && 0x02 <= currentCap.Range.UsageMax )
                currentButtons |= GameInputMouseRightButton;
            if ( 0x03 >= currentCap.Range.UsageMin && 0x03 <= currentCap.Range.UsageMax )
                currentButtons |= GameInputMouseMiddleButton;
            if ( 0x04 >= currentCap.Range.UsageMin && 0x04 <= currentCap.Range.UsageMax )
                currentButtons |= GameInputMouseButton4;
            if ( 0x05 >= currentCap.Range.UsageMin && 0x05 <= currentCap.Range.UsageMax )
                currentButtons |= GameInputMouseButton5;
        }
        else
        {
            if ( 0x01 == currentCap.NotRange.Usage )
                currentButtons |= GameInputMouseLeftButton;
            if ( 0x02 == currentCap.NotRange.Usage )
                currentButtons |= GameInputMouseRightButton;
            if ( 0x03 == currentCap.NotRange.Usage )
                currentButtons |= GameInputMouseMiddleButton;
            if ( 0x04 == currentCap.NotRange.Usage )
                currentButtons |= GameInputMouseButton4;
            if ( 0x05 == currentCap.NotRange.Usage )
                currentButtons |= GameInputMouseButton5;
        }
    }

    *buttons = currentButtons;
}

static VOID append_supported_mouse_values( GameInputMouseButtons *buttons, v2_GameInputMouseInfo *mouse_info, HIDP_VALUE_CAPS *caps, USHORT nCaps )
{
    GameInputMouseButtons currentButtons = *buttons;
    HIDP_VALUE_CAPS currentCap;
    USHORT capsIterator;

    TRACE( "buttons %p, mouse_info %p, caps %p, nCaps %d.\n", buttons, mouse_info, caps, nCaps );

    for ( capsIterator = 0; capsIterator < nCaps; capsIterator++ )
    {
        currentCap = caps[capsIterator];

        if ( currentCap.UsagePage == HID_USAGE_PAGE_BUTTON && ( currentCap.IsRange ? (currentCap.Range.UsageMin <= 0x38 && currentCap.Range.UsageMax >= 0x38)
                                : currentCap.NotRange.Usage == 0x38 ) )
        {
            mouse_info->hasWheelX = TRUE;
            currentButtons |= GameInputMouseWheelTiltLeft;
            currentButtons |= GameInputMouseWheelTiltRight;
        }
        
        if ( currentCap.UsagePage == HID_USAGE_PAGE_CONSUMER && ( currentCap.IsRange ? (currentCap.Range.UsageMin <= 0x238 && currentCap.Range.UsageMax >= 0x238)
                                : currentCap.NotRange.Usage == 0x238 ) )
        {
            mouse_info->hasWheelY = TRUE;
        }
    }

    *buttons = currentButtons;
}

HRESULT mouse_input_device_InitDInput8Device( IN v2_IGameInputDevice *device )
{
    HRESULT hr;
    HWND hwnd;
    LPDIRECTINPUT8W g_pDI = NULL;
    LPDIRECTINPUTDEVICE8W g_pDevice;

    TRACE( "device %p.\n", device );

    hwnd = GetForegroundWindow();
    if ( !hwnd ) return E_FAIL;
                
    hr = DirectInput8Create( game_input, DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void**)&g_pDI, NULL );
    if ( FAILED( hr ) ) return hr;

    hr = g_pDI->lpVtbl->CreateDevice( g_pDI, &GUID_SysMouse, &g_pDevice, NULL );
    if ( FAILED( hr ) ) return hr;

    hr = g_pDevice->lpVtbl->SetDataFormat( g_pDevice, &c_dfDIMouse2 );
    if ( FAILED( hr ) ) return hr;

    hr = g_pDevice->lpVtbl->SetCooperativeLevel( g_pDevice, hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE );
    if ( FAILED( hr ) ) return hr;

    // Acquire device
    hr = g_pDevice->lpVtbl->Acquire( g_pDevice );

    game_input_device_SetDInputDevice( device, g_pDevice );

    return hr;
}

HRESULT mouse_input_device_InitDevice( IN v2_IGameInputDevice *device )
{
    USHORT nCaps;
    HRESULT hr = S_OK;
    HIDP_BUTTON_CAPS *buttonCaps = NULL;
    HIDP_VALUE_CAPS *valueCaps = NULL;

    v2_IGameInputDevice *dev = NULL;
    v2_GameInputMouseInfo mouse_info;
    v2_GameInputDeviceInfo *device_info;
    GameInputMouseButtons buttons = GameInputMouseNone;

    TRACE( "device %p.\n", device );

    if ( !device )
        return E_INVALIDARG;

    hr = game_input_device_OpenDevice( device );
    if ( FAILED( hr ) ) goto _CLEANUP;

    hr = game_input_device_QueryGameHIDButtonCaps( device, &buttonCaps, &nCaps );
    if ( FAILED( hr ) ) goto _CLEANUP;
    append_supported_mouse_buttons( &buttons, buttonCaps, nCaps );

    // BUG: QueryGameHIDValueCaps for mouse devices don't work within wine!!
    //hr = game_input_device_QueryGameHIDValueCaps( device, &valueCaps, &nCaps);
    //if ( FAILED( hr ) ) goto _CLEANUP;
    //append_supported_mouse_values( &buttons, &mouse_info, valueCaps, nCaps );

    // It is not possible to obtain sample rates from a HID device.
    // So we'll use a safe sample rate of 500 hz.
    mouse_info.sampleRate = 500;
    mouse_info.supportedButtons = buttons;

    // const override here!
    v2_IGameInputDevice_GetDeviceInfo( device, (const v2_GameInputDeviceInfo **)&device_info );
    device_info->mouseInfo = &mouse_info;

    // DInput device for mouse devices are manually acquired
    hr = mouse_input_device_InitDInput8Device( device );
    if ( FAILED( hr ) ) return hr;

_CLEANUP:
    if ( FAILED( hr ) )
    {
        if ( dev ) v2_IGameInputDevice_Release( dev );
    }
    if ( buttonCaps ) free( buttonCaps );
    if ( valueCaps ) free( valueCaps );

    return hr;
}

HRESULT mouse_input_device_ReadCurrentStateFromHID( IN v2_IGameInputDevice *device, IN uint64_t timestamp, OUT v2_IGameInputReading **reading )
{
    HRESULT hr = S_OK;
    USAGE usages[128];
    ULONG usageLength = sizeof(usages);
    ULONG usageIterator;
    POINT posStore;
    POINT absolutePos;
    LONG wheelStore;

    const v2_GameInputDeviceInfo *device_info;
    v2_GameInputMouseState state;

    TRACE( "device %p, timestamp %lld, reading %p.\n", device, timestamp, reading );

    state.buttons = GameInputMouseNone;

    v2_IGameInputDevice_GetDeviceInfo( device, &device_info );

    GetCursorPos( &absolutePos );

    hr = game_input_device_PollHIDDevice( device );
    if ( hr == E_PENDING )
    {
        // Pending call. consider previous buttons being pressed.
    }
    else if ( SUCCEEDED( hr ) )
    {
        storedButtons = GameInputMouseNone;
        game_input_device_CurrentValue( device, device_info->usage.page, 0x30, &posStore.x );
        game_input_device_CurrentValue( device, device_info->usage.page, 0x31 , &posStore.y );
        game_input_device_CurrentValue( device, device_info->usage.page, 0x38 , &wheelStore );

        game_input_device_CurrentButtons( device, usages, &usageLength );
        for ( usageIterator = 0; usageIterator < usageLength; usageIterator++ )
        {
            if ( usages[usageIterator] == 1 )
                storedButtons |= GameInputMouseLeftButton;
            if ( usages[usageIterator] == 2 )
                storedButtons |= GameInputMouseRightButton;
            if ( usages[usageIterator] == 3 )
                storedButtons |= GameInputMouseMiddleButton;
            if ( usages[usageIterator] == 4 )
                storedButtons |= GameInputMouseButton4;
            if ( usages[usageIterator] == 5 )
                storedButtons |= GameInputMouseButton5;
        }

        relativePositionStore.x += posStore.x;
        relativePositionStore.y += posStore.y;
        relativeWheelStore += wheelStore;
    } else
    {
        // Failed somewhere...
        return hr;
    }

    state.positionX = relativePositionStore.x * 5;
    state.positionY = relativePositionStore.y * 5;
    state.wheelY = relativeWheelStore * 100;

    state.buttons = storedButtons;
    state.positions = GameInputMouseRelativePosition;
    state.absolutePositionX = absolutePos.x;
    state.absolutePositionY = absolutePos.y;

    hr = game_input_reading_CreateForMouseDevice( device, state, timestamp, reading );

    return hr;
}

HRESULT mouse_input_device_ReadCurrentStateFromDInput8( IN v2_IGameInputDevice *device, IN uint64_t timestamp, OUT v2_IGameInputReading **reading )
{
    POINT absoluteP;
    HRESULT hr;
    DIMOUSESTATE2 state;
    LPDIRECTINPUTDEVICE8W g_pDevice;

    v2_GameInputMouseState mouseState;

    TRACE( "device %p, timestamp %lld, reading %p.\n", device, timestamp, reading );

    GetCursorPos( &absoluteP );

    hr = game_input_device_AcquireDInputDevice( device, &g_pDevice );
    if ( FAILED( hr ) ) return hr;

    hr = g_pDevice->lpVtbl->GetDeviceState( g_pDevice, sizeof(state), &state );
    if ( FAILED( hr ) ) 
    {
        if ( ( hr == DIERR_INPUTLOST ) || ( hr == DIERR_NOTACQUIRED ) ) {
            // Try to reacquire
            hr = g_pDevice->lpVtbl->Acquire( g_pDevice );
        }
    }
    else
    {
        mouseState.positions = GameInputMouseRelativePosition;
        mouseState.absolutePositionX = absoluteP.x;
        mouseState.absolutePositionY = absoluteP.y;
        relativePositionStore.x += state.lX;
        relativePositionStore.y += state.lY;
        relativeWheelStore += state.lZ;
        mouseState.positionX = relativePositionStore.x;
        mouseState.positionY = relativePositionStore.y;
        mouseState.wheelY = relativeWheelStore;
    }

    mouseState.buttons = GameInputMouseNone;
    if ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 )
        mouseState.buttons |= GameInputMouseLeftButton;
    if ( GetAsyncKeyState( VK_RBUTTON ) & 0x8000 )
        mouseState.buttons |= GameInputMouseRightButton;
    if ( GetAsyncKeyState( VK_MBUTTON ) & 0x8000 )
        mouseState.buttons |= GameInputMouseMiddleButton;
    if ( GetAsyncKeyState( VK_XBUTTON1 ) & 0x8000 )
        mouseState.buttons |= GameInputMouseButton4;
    if ( GetAsyncKeyState( VK_XBUTTON2 ) & 0x8000 )
        mouseState.buttons |= GameInputMouseButton5;

    hr = game_input_reading_CreateForMouseDevice( device, mouseState, timestamp, reading );

    return hr;
}
