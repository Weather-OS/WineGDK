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

#include <time.h>

#include "mouinput.h"
#include "inputdevice.h"
#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(ginput);

extern HINSTANCE game_input;

static CRITICAL_SECTION provider_cs;
static CRITICAL_SECTION_DEBUG provider_cs_debug =
{
    0, 0, &provider_cs,
    { &provider_cs_debug.ProcessLocksList, &provider_cs_debug.ProcessLocksList },
      0, 0, { (ULONG_PTR)(__FILE__ ": provider_cs") }
};
static CRITICAL_SECTION provider_cs = { &provider_cs_debug, -1, 0, 0, 0, 0 };

DEFINE_GUID( device_path_guid, 0x00000000, 0x0000, 0x0000, 0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf8 );

static const DIOBJECTDATAFORMAT data_format_objs[] =
{
    {NULL,DIJOFS_X,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_Y,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_Z,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_RX,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_RY,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_RZ,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_SLIDER(0),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_SLIDER(1),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_POV(0),DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_POV(1),DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_POV(2),DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_POV(3),DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(0),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(1),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(2),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(3),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(4),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(5),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(6),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(7),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(8),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(9),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(10),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(11),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(12),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(13),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(14),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(15),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(16),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(17),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(18),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(19),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(20),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(21),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(22),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(23),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(24),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(25),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(26),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(27),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(28),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(29),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(30),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(31),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(32),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(33),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(34),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(35),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(36),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(37),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(38),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(39),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(40),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(41),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(42),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(43),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(44),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(45),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(46),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(47),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(48),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(49),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(50),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(51),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(52),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(53),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(54),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(55),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(56),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(57),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(58),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(59),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(60),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(61),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(62),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(63),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(64),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(65),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(66),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(67),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(68),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(69),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(70),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(71),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(72),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(73),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(74),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(75),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(76),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(77),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(78),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(79),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(80),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(81),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(82),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(83),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(84),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(85),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(86),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(87),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(88),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(89),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(90),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(91),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(92),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(93),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(94),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(95),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(96),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(97),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(98),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(99),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(100),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(101),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(102),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(103),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(104),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(105),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(106),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(107),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(108),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(109),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(110),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(111),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(112),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(113),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(114),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(115),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(116),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(117),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(118),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(119),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(120),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(121),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(122),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(123),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(124),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(125),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(126),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
    {NULL,DIJOFS_BUTTON(127),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE},
};

static const DIDATAFORMAT data_format =
{
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(DIJOYSTATE2),
    ARRAY_SIZE(data_format_objs),
    (LPDIOBJECTDATAFORMAT)data_format_objs
};

static struct list device_list = LIST_INIT( device_list );
static struct list callback_list = LIST_INIT( callback_list );
static UINT64 nextId = 0;

static inline struct game_input_device *impl_from_IGameInputDevice( v2_IGameInputDevice *iface )
{
    return CONTAINING_RECORD( iface, struct game_input_device, v2_IGameInputDevice_iface );
}

static HRESULT WINAPI game_input_device_QueryInterface( v2_IGameInputDevice *iface, REFIID iid, void **out )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

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

static ULONG WINAPI game_input_device_AddRef( v2_IGameInputDevice *iface )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI game_input_device_Release( v2_IGameInputDevice *iface )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
};

static HRESULT WINAPI game_input_device_GetDeviceInfo( v2_IGameInputDevice *iface, const v2_GameInputDeviceInfo **info )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    TRACE( "iface %p, info %p.\n", iface, info );
    *info = &impl->deviceInfo;
    return S_OK;
}

static HRESULT WINAPI game_input_device_GetHapticInfo( v2_IGameInputDevice *iface, GameInputHapticInfo *info )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

    TRACE( "iface %p, info %p.\n", iface, info );

    if ( !(impl->deviceStatus | GameInputDeviceHapticInfoReady) )
        return GAMEINPUT_E_HAPTIC_INFO_NOT_FOUND;

    *info = impl->hapticInfo;

    return S_OK;
}

static GameInputDeviceStatus WINAPI game_input_device_GetDeviceStatus( v2_IGameInputDevice *iface )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    TRACE( "iface %p.\n", iface );
    return impl->deviceStatus;
}

static HRESULT WINAPI game_input_device_CreateForceFeedbackEffect( v2_IGameInputDevice *iface, uint32_t motor, const GameInputForceFeedbackParams *params, v2_IGameInputForceFeedbackEffect **effect )
{
    FIXME( "iface %p, motor %d, params %p, effect %p stub!\n", iface, motor, params, effect );
    return E_NOTIMPL;
}

static bool WINAPI game_input_device_IsForceFeedbackMotorPoweredOn( v2_IGameInputDevice *iface, uint32_t motor )
{
    FIXME( "iface %p, motor %d stub!\n", iface, motor );
    return FALSE;
}

static VOID WINAPI game_input_device_SetForceFeedbackMotorGain( v2_IGameInputDevice *iface, uint32_t motor, float gain )
{
    FIXME( "iface %p, motor %d, gain %f stub!\n", iface, motor, gain );
    return;
}

static VOID WINAPI game_input_device_SetRumbleState( v2_IGameInputDevice *iface, const GameInputRumbleParams *params )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    TRACE( "iface %p, params %p.\n", iface, params );
    if ( params )
        impl->rumbleState = *params;
    return;
}

static HRESULT WINAPI game_input_device_DirectInputEscape( v2_IGameInputDevice *iface, uint32_t command, const void *input, uint32_t in_size, void *output, uint32_t out_size, uint32_t *size )
{    
    HRESULT hr;
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    
    DIEFFESCAPE dinputEscape = { .dwSize = sizeof(DIEFFESCAPE), .dwCommand = (DWORD)command, .lpvInBuffer = (const PVOID)input, 
                                 .cbInBuffer = (DWORD)in_size, .lpvOutBuffer = output, .cbOutBuffer = (DWORD)out_size };

    TRACE( "iface %p, command %d, input %p, in_size %d, output %p, out_size %d, size %p.\n", iface, command, input, in_size, output, out_size, size );

    if ( !impl->pDevice )
        return E_HANDLE;
    if ( size ) 
        *size = 0;
    
    hr = impl->pDevice->lpVtbl->Escape( impl->pDevice, &dinputEscape );

    if ( SUCCEEDED( hr ) )
        if ( size )
            *size = dinputEscape.cbOutBuffer;

    return hr;
}

HRESULT game_input_device_AcquireDInputDevice( IN v2_IGameInputDevice *iface, OUT LPDIRECTINPUTDEVICE8W *device )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    TRACE( "iface %p, device %p.\n", iface, device );
    if ( device )
        *device = impl->pDevice;
    return S_OK;
}

HRESULT game_input_device_SetDInputDevice( IN v2_IGameInputDevice *iface, IN LPDIRECTINPUTDEVICE8W device )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    TRACE( "iface %p, device %p.\n", iface, device );
    impl->pDevice = device;
    return S_OK;
}

HRESULT game_input_device_AddGameHID( IN v2_IGameInputDevice *iface, IN LPWSTR devicePath )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );
    
    HRESULT status = S_OK;

    CHAR nameBuf[512];
    DWORD request;
    DWORD interfaceDetailSizeNeeded = 0;
    HANDLE deviceFileHandle = NULL;
    HIDP_CAPS caps;
    DEVPROPTYPE propType;
    SP_DEVINFO_DATA deviceData = { .cbSize = sizeof(SP_DEVINFO_DATA) };
    PHIDP_PREPARSED_DATA preparsedData = NULL;
    SP_DEVICE_INTERFACE_DATA interfaceData = { .cbSize = sizeof(SP_DEVICE_INTERFACE_DATA) };
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A interfaceDetail = NULL;

    TRACE("iface %p, devicePath %s\n", iface, debugstr_w(devicePath));

    impl->pnpPath = (LPWSTR)malloc( (wcslen(devicePath) + 1) * sizeof(WCHAR));
    wcscpy( impl->pnpPath, devicePath );

    // Check if we can write to this device
    deviceFileHandle = CreateFileW( devicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    if ( deviceFileHandle != INVALID_HANDLE_VALUE )
    {
        impl->deviceStatus |= GameInputDeviceConnected;
        impl->deviceStatus |= GameInputDeviceInputEnabled;
        impl->deviceStatus |= GameInputDeviceOutputEnabled;
        CloseHandle( deviceFileHandle );
    }

    deviceFileHandle = CreateFileW( devicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    if ( deviceFileHandle == INVALID_HANDLE_VALUE )
    {
        // If we can't read from the device, we probably can't access it.
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

    if ( wcsstr( devicePath, L"BTHENUM" ) != NULL )
        impl->deviceStatus |= GameInputDeviceWireless;

    impl->deviceStatus |= GameInputDeviceConnected;
    impl->deviceStatus |= GameInputDeviceInputEnabled;

    if ( HidD_GetPreparsedData( deviceFileHandle, &preparsedData ) )
    {
        CloseHandle( deviceFileHandle );
        impl->preparsed = preparsedData;
        if ( HidP_GetCaps( preparsedData, &caps ) == HIDP_STATUS_SUCCESS )
        {
            impl->deviceInfo.usage.id = caps.Usage;
            impl->deviceInfo.usage.page = caps.UsagePage;
            switch ( caps.UsagePage )
            {
                // These are the ones we support for now
                case HID_USAGE_PAGE_GENERIC:
                {
                    switch ( caps.Usage )
                    {
                        case HID_USAGE_GENERIC_MOUSE:
                        case HID_USAGE_GENERIC_KEYBOARD:
                        {
                            impl->deviceInfo.supportedInput = GameInputKindKeyboard | GameInputKindMouse;
                            status = mouse_input_device_InitDevice( iface );
                            TRACE("mouse_input_device_InitDevice returned %#lx\n", status);
                            if ( FAILED( status ) ) goto _CLEANUP;
                            break;
                        }

                        case HID_USAGE_GENERIC_GAMEPAD:
                        {
                            impl->deviceInfo.supportedInput = GameInputKindGamepad;
                            break;
                        }

                        case HID_USAGE_GENERIC_JOYSTICK:
                        {
                            impl->deviceInfo.supportedInput = GameInputKindArcadeStick;
                            break;
                        }

                        default:
                            FIXME( "Unsupported usage %#x\n", caps.Usage ) ;
                            status = E_NOTIMPL;
                            goto _CLEANUP;
                    }
                    break;
                }
                
                default:
                    FIXME( "Unsupported usage page %#x\n", caps.UsagePage ) ;
                    status = E_NOTIMPL;
                    goto _CLEANUP;
            }
        } else
        {
            status = HRESULT_FROM_WIN32( GetLastError() );
            goto _CLEANUP;
        }
    } else
    {
        status = HRESULT_FROM_WIN32( GetLastError() );
        goto _CLEANUP;
    }

    impl->hidInfo = SetupDiCreateDeviceInfoList( NULL, NULL );
    if ( impl->hidInfo == INVALID_HANDLE_VALUE ) { status = HRESULT_FROM_WIN32( GetLastError() ); goto _CLEANUP; }

    if ( !SetupDiOpenDeviceInterfaceW( impl->hidInfo, devicePath, 0, &interfaceData ) )
    { status = HRESULT_FROM_WIN32( GetLastError() ); goto _CLEANUP; }

    SetupDiGetDeviceInterfaceDetailW( impl->hidInfo, &interfaceData, NULL, 0, &interfaceDetailSizeNeeded, NULL );
    if ( interfaceDetailSizeNeeded == 0 ) { status = HRESULT_FROM_WIN32( GetLastError() ); goto _CLEANUP; }

    interfaceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)malloc( interfaceDetailSizeNeeded );
    if ( !interfaceDetail ) { status = E_OUTOFMEMORY; goto _CLEANUP; }
    interfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

    if ( !SetupDiGetDeviceInterfaceDetailA( impl->hidInfo, &interfaceData, interfaceDetail, interfaceDetailSizeNeeded, NULL, &deviceData ) )
    { status = HRESULT_FROM_WIN32( GetLastError() ); goto _CLEANUP; }

    impl->hidData = deviceData;

    {
        // GameInputDeviceInfo::vendorId, GameInputDeviceInfo::productId & GameInputDeviceInfo::revisionNumber
        CHAR *hardwareId = NULL;
        DWORD hardwareIdSizeNeeded = 0;

        SetupDiGetDeviceRegistryPropertyA( impl->hidInfo, &impl->hidData, SPDRP_HARDWAREID, NULL, NULL, 0, &hardwareIdSizeNeeded );
        hardwareId = (CHAR *)malloc( hardwareIdSizeNeeded );
        if ( !hardwareId )
        {
            status = E_OUTOFMEMORY;
            goto _CLEANUP;
        }

        if ( SetupDiGetDeviceRegistryPropertyA( impl->hidInfo, &impl->hidData, SPDRP_HARDWAREID, NULL, 
            (PBYTE)hardwareId, hardwareIdSizeNeeded, NULL ) )
        {
            sscanf_s( hardwareId, "HID\\VID_%4hx&PID_%4hx", 
                &impl->deviceInfo.vendorId, &impl->deviceInfo.productId );
            FIXME( "hardwareId is %s\n", debugstr_a(hardwareId) );
            FIXME( "vendorId is %4hx, productId is %4hx\n", impl->deviceInfo.vendorId, impl->deviceInfo.productId );
        }
        free( hardwareId );
    }

    {
        // GameInputDeviceInfo::deviceFamily
        impl->deviceInfo.deviceFamily = GameInputFamilyHid;
    }
        
    {
        // GameInputDeviceInfo::containerId
        SetupDiGetDevicePropertyW( impl->hidInfo, &impl->hidData, &DEVPKEY_Device_ContainerId,
              &propType, (PBYTE)&impl->deviceInfo.containerId, sizeof(GUID), &request, 0 );
    }

    {
        // GameInputDeviceInfo::displayName
        if ( SetupDiGetDevicePropertyW( impl->hidInfo, &impl->hidData, &DEVPKEY_Device_FriendlyName, 
            &propType, (PBYTE)nameBuf, sizeof(nameBuf), &request, 0 ) ) 
        {
            impl->deviceInfo.displayName = nameBuf;
        }
    }

    impl->previousDeviceStatus = impl->deviceStatus;

_CLEANUP:
    if ( FAILED( status ) )
    {
        free( impl->pnpPath );
        SetupDiDeleteDeviceInterfaceData( impl->hidInfo, &interfaceData );
        SetupDiDestroyDeviceInfoList( impl->hidInfo );
    }
    if ( interfaceDetail )
        free( interfaceDetail );
    if ( deviceFileHandle && deviceFileHandle != INVALID_HANDLE_VALUE )
        CloseHandle( deviceFileHandle );
    return status;
}

HRESULT game_input_device_OpenDevice( IN v2_IGameInputDevice *iface )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

    NTSTATUS status;
    HIDP_CAPS caps;

    TRACE( "iface %p\n", iface );

    impl->deviceHandle = CreateFileW( impl->pnpPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL );
    if ( impl->deviceHandle == INVALID_HANDLE_VALUE )
        return HRESULT_FROM_WIN32( GetLastError() );

    status = HidP_GetCaps( impl->preparsed, &caps );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );

    impl->readBuffer = VirtualAlloc( NULL, caps.InputReportByteLength,
                                MEM_COMMIT | MEM_RESERVE,
                                PAGE_READWRITE );
    impl->readBufferLen = caps.InputReportByteLength;

    return S_OK;
}

HRESULT game_input_device_CloseDevice( IN v2_IGameInputDevice *iface )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

    TRACE( "iface %p\n", iface );

    if ( !CloseHandle( impl->deviceHandle ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    VirtualFree( impl->readBuffer, impl->readBufferLen, MEM_DECOMMIT );

    return S_OK;
}

HRESULT game_input_device_QueryGameHIDButtonCaps( IN v2_IGameInputDevice *iface, OUT HIDP_BUTTON_CAPS **btnCaps, OUT USHORT *nBtnCaps )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

    HIDP_BUTTON_CAPS *buttons;
    HIDP_CAPS caps;
    NTSTATUS status;
    USHORT got;

    TRACE( "iface %p, btnCaps %p, nBtnCaps %p.\n", iface, btnCaps, nBtnCaps );

    if ( !btnCaps || !nBtnCaps )
        return E_POINTER;

    status = HidP_GetCaps( impl->preparsed, &caps );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );
    
    got = caps.NumberInputButtonCaps;
    buttons = (HIDP_BUTTON_CAPS *)malloc( got * sizeof(HIDP_BUTTON_CAPS) );

    status = HidP_GetButtonCaps( HidP_Input, buttons, &got, impl->preparsed );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );

    *btnCaps = buttons;
    *nBtnCaps = got;

    return S_OK;
}

HRESULT game_input_device_QueryGameHIDValueCaps( IN v2_IGameInputDevice *iface, OUT HIDP_VALUE_CAPS **valueCaps, OUT USHORT *nValueCaps )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

    HIDP_VALUE_CAPS *values;
    HIDP_CAPS caps;
    NTSTATUS status;
    USHORT got;

    TRACE( "iface %p, valueCaps %p, nValueCaps %p.\n", iface, valueCaps, nValueCaps );

    if ( !valueCaps || !nValueCaps )
        return E_POINTER;

    status = HidP_GetCaps( impl->preparsed, &caps );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );
    
    got = caps.NumberInputValueCaps;
    values = (HIDP_VALUE_CAPS *)malloc( got * sizeof(HIDP_VALUE_CAPS) );

    status = HidP_GetValueCaps( HidP_Input, values, &got, impl->preparsed );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );

    *valueCaps = values;
    *nValueCaps = got;

    return S_OK;
}

HRESULT game_input_device_PollHIDDevice( IN v2_IGameInputDevice *iface )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

    HIDP_CAPS caps;
    NTSTATUS status;
    DWORD bytesRead;
    DWORD lastError;

    TRACE( "iface %p.\n", iface );

    status = HidP_GetCaps( impl->preparsed, &caps );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );

    if ( !impl->deviceHandle || impl->deviceHandle == INVALID_HANDLE_VALUE )
    {
        ERR( "Device %s is not opened yet!\n", debugstr_w(impl->pnpPath) );
        return E_HANDLE;
    }

    EnterCriticalSection( &impl->readLock );

    if ( !impl->readPending )
    {
        ZeroMemory( &impl->readOv, sizeof(impl->readOv) );

        if ( !ReadFile( impl->deviceHandle, impl->readBuffer, impl->readBufferLen, &bytesRead, &impl->readOv ) )
        {
            lastError = GetLastError();
            if ( lastError == ERROR_IO_PENDING )
            {
                // Pending read, don't block and forward IO.
                impl->readPending = TRUE;
                LeaveCriticalSection( &impl->readLock );
                return E_PENDING;
            }

            LeaveCriticalSection( &impl->readLock );
            if ( lastError == ERROR_DEVICE_NOT_CONNECTED || lastError == ERROR_OPERATION_ABORTED ) {
                ERR( "device %s is lost!\n", debugstr_w( impl->pnpPath ) );
                return E_PENDING;
            }
            ERR( "last error was %lu\n", lastError );
            return HRESULT_FROM_WIN32( lastError );
        }

        impl->readPending = FALSE;
    }
    else
    {
        // IO read is in progress. just check if it's completed or not.
        if ( !GetOverlappedResult( impl->deviceHandle, &impl->readOv, &bytesRead, FALSE ) )
        {
            lastError = GetLastError();
            if ( lastError == ERROR_IO_INCOMPLETE || lastError == ERROR_IO_PENDING )
            {
                LeaveCriticalSection(&impl->readLock);
                return E_PENDING;
            }

            LeaveCriticalSection(&impl->readLock);
            if ( lastError == ERROR_DEVICE_NOT_CONNECTED || lastError == ERROR_OPERATION_ABORTED ) {
                ERR( "device %s is lost!\n", debugstr_w( impl->pnpPath ) );
                return E_PENDING;
            }
            return HRESULT_FROM_WIN32( lastError );
        }

        impl->readPending = FALSE;
    }

    ZeroMemory( &impl->readOv, sizeof(impl->readOv) );

    if ( !ReadFile( impl->deviceHandle, impl->readBuffer, impl->readBufferLen, &bytesRead, &impl->readOv ) )
    {
        lastError = GetLastError();
        if ( lastError == ERROR_IO_PENDING )
        {
            impl->readPending = TRUE;
        }
        else
        {
            TRACE( "ReadFile re-issue failed with error %lu.\n", lastError );
        }
    }
    else
    {
        impl->readPending = FALSE;
    }
    
    LeaveCriticalSection(&impl->readLock);
    return S_OK;
}

HRESULT game_input_device_CurrentButtons( IN v2_IGameInputDevice *iface, OUT USAGE usageList[128], OUT ULONG *usageLength )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

    HIDP_CAPS caps;
    NTSTATUS status;

    TRACE( "iface %p.\n", iface );

    status = HidP_GetCaps( impl->preparsed, &caps );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );

    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usageList, usageLength, impl->preparsed, impl->readBuffer, impl->readBufferLen );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );

    return S_OK;
}

HRESULT game_input_device_CurrentValue( IN v2_IGameInputDevice *iface, IN USAGE usagePage, IN USAGE usage, OUT PLONG value )
{
    struct game_input_device *impl = impl_from_IGameInputDevice( iface );

    HIDP_CAPS caps;
    NTSTATUS status;

    TRACE( "iface %p.\n", iface );

    status = HidP_GetCaps( impl->preparsed, &caps );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );

    status = HidP_GetScaledUsageValue( HidP_Input, usagePage, 0, usage, value, impl->preparsed, impl->readBuffer, impl->readBufferLen );
    if ( status != HIDP_STATUS_SUCCESS ) return HRESULT_FROM_NT( status );

    return S_OK;
}

static const struct v2_IGameInputDeviceVtbl game_input_device_vtbl =
{
    /* IUnknown methods */
    game_input_device_QueryInterface,
    game_input_device_AddRef,
    game_input_device_Release,
    /* v2_IGameInputDevice methods */
    game_input_device_GetDeviceInfo,
    game_input_device_GetHapticInfo,
    game_input_device_GetDeviceStatus,
    game_input_device_CreateForceFeedbackEffect,
    game_input_device_IsForceFeedbackMotorPoweredOn,
    game_input_device_SetForceFeedbackMotorGain,
    game_input_device_SetRumbleState,
    game_input_device_DirectInputEscape
};

static HRESULT device_provider_create( LPWSTR device_path )
{
    HRESULT status;

    IDirectInputDevice8W *dinput_device;
    v2_IGameInputDevice *addedDevice;
    IDirectInput8W *dinput;
    BOOLEAN found;
    GUID guid = device_path_guid;

    struct game_input_device *impl, *entry;
    struct device_callback *callback;

    TRACE("device_path is %s\n", debugstr_w(device_path));

    *(LPCWSTR *)&guid = device_path;
    status = DirectInput8Create( game_input, DIRECTINPUT_VERSION, &IID_IDirectInput8W, (void **)&dinput, NULL );
    if ( FAILED( status ) ) return status;

    status = game_input_device_Create( &addedDevice );
    if ( FAILED( status ) ) goto _CLEANUP;

    status = IDirectInput8_CreateDevice( dinput, &guid, &dinput_device, NULL );
    IDirectInput8_Release( dinput );
    // HACK: Mouse device is manually set at mouinput.c
    if ( SUCCEEDED( status ) )
    {
        if ( FAILED(status = IDirectInputDevice8_SetCooperativeLevel( dinput_device, 0, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE ) ) ) goto _CLEANUP;
        if ( FAILED(status = IDirectInputDevice8_SetDataFormat( dinput_device, &data_format ) ) ) goto _CLEANUP;
        if ( FAILED(status = IDirectInputDevice8_Acquire( dinput_device ) ) ) goto _CLEANUP;
        status = game_input_device_SetDInputDevice( addedDevice, dinput_device );
        if ( FAILED( status ) ) goto _CLEANUP;
    }

    impl = impl_from_IGameInputDevice( addedDevice );

    status = game_input_device_AddGameHID( addedDevice, device_path );
    if ( FAILED( status ) ) goto _CLEANUP;

    list_init( &impl->entry );

    EnterCriticalSection( &provider_cs );
    LIST_FOR_EACH_ENTRY( entry, &device_list, struct game_input_device, entry )
    {
        // avoid registering 2 mouse devices at once.
        if ( (entry->deviceInfo.supportedInput & GameInputKindMouse) && (impl->deviceInfo.supportedInput & GameInputKindMouse) )
        {
            found = TRUE;
            break;
        }
        if ( (found = !wcscmp( entry->pnpPath, device_path ) ) ) break;
    }
    if ( !found ) list_add_tail( &device_list, &impl->entry );

    LIST_FOR_EACH_ENTRY( callback, &callback_list, struct device_callback, entry )
    {
        if ( impl->deviceInfo.supportedInput & callback->kind )
            if ( impl->deviceStatus & callback->filter )
            {
                callback->callback( callback->token, callback->context, addedDevice, time(NULL), impl->deviceStatus, impl->previousDeviceStatus );
            }
    }
    LeaveCriticalSection( &provider_cs );

_CLEANUP:
    if ( FAILED( status ) )
    {
        if ( dinput_device ) IDirectInputDevice_Release( dinput_device );
        if ( addedDevice ) v2_IGameInputDevice_Release( addedDevice );
    }

    return status;
}

static LRESULT CALLBACK DeviceNotifyWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    DEV_BROADCAST_DEVICEINTERFACE_W *iface;

    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg == WM_DEVICECHANGE)
    { 
        switch (wparam)
        {
        case DBT_DEVICEARRIVAL:
            iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)lparam;
            device_provider_create( iface->dbcc_name );
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)lparam;
            //provider_remove( iface->dbcc_name );
            break;
        default: break;
        }
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void initialize_devices( void )
{
    char buffer[offsetof( SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath[MAX_PATH] )];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = { .cbSize = sizeof(iface) };
    HRESULT status;
    HDEVINFO set;
    DWORD i = 0;
    GUID hidGuid;

    TRACE("initializing devices...\n");

    set = SetupDiGetClassDevsW( &hidGuid, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );

    while ( SetupDiEnumDeviceInterfaces( set, NULL, &hidGuid, i++, &iface ) )
    {
        detail->cbSize = sizeof(*detail);
        if ( !SetupDiGetDeviceInterfaceDetailW( set, &iface, detail, sizeof(buffer), NULL, NULL ) ) continue;
        device_provider_create( detail->DevicePath );
    }

    HidD_GetHidGuid( &hidGuid );

    while ( SetupDiEnumDeviceInterfaces( set, NULL, &hidGuid, i++, &iface ) )
    {
        detail->cbSize = sizeof(*detail);
        if ( !SetupDiGetDeviceInterfaceDetailW( set, &iface, detail, sizeof(buffer), NULL, NULL ) ) continue;
        status = device_provider_create( detail->DevicePath );
    }

    SetupDiDestroyDeviceInfoList( set );
}

DWORD WINAPI DeviceMonitorThread( LPVOID lpParam )
{
    DEV_BROADCAST_DEVICEINTERFACE_W filter =
    {
        .dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_W),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
    };
    WNDCLASSEXW wndclass =
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpszClassName = L"__wine_game_input_devnotify",
        .lpfnWndProc = DeviceNotifyWndProc,
    };

    HDEVNOTIFY devnotify;
    HMODULE module;
    HANDLE start_event = (HANDLE)lpParam;
    HWND hwnd;
    MSG msg;

    TRACE( "lpParam %p.\n", lpParam );

    SetThreadDescription( GetCurrentThread(), L"wine_game_input_worker" );

    GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (PVOID)game_input, &module );
    RegisterClassExW( &wndclass );

    hwnd = CreateWindowExW( 0, wndclass.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
    devnotify = RegisterDeviceNotificationW( hwnd, &filter, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES );

    initialize_devices();
    SetEvent( start_event );

    do
    {
        while ( PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
    } while ( !MsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE ) );

    UnregisterDeviceNotification( devnotify );
    DestroyWindow( hwnd );
    UnregisterClassW( wndclass.lpszClassName, NULL );

    FreeLibraryAndExitThread( module, 0 );
    return 0;
}

HRESULT WINAPI RegisterDeviceCallback( v2_IGameInputDevice *device, GameInputKind kind, GameInputDeviceStatus filter, void *context, v2_GameInputDeviceCallback callback, GameInputCallbackToken *token )
{
    struct device_callback *entry;
    struct game_input_device *input_device;

    TRACE( "device %p, kind %#x, filter %#x, context %p, callback %p, token %p.\n", device, kind, filter, context, callback, token );

    if ( !token )
        return E_POINTER;

    if (!(entry = calloc( 1, sizeof(*entry) ))) return E_OUTOFMEMORY;

    entry->device = device;
    entry->callback = callback;
    entry->context = context;
    entry->filter = filter;
    entry->kind = kind;

    list_init( &entry->entry );

    if ( !device )
    {
        EnterCriticalSection( &provider_cs );
        nextId++;
        entry->token = nextId;
        *token = nextId;
        list_add_tail( &callback_list, &entry->entry );

        LIST_FOR_EACH_ENTRY( input_device, &device_list, struct game_input_device, entry )
        {
            if ( input_device->deviceInfo.supportedInput & kind )
                if ( input_device->deviceStatus & filter )
                {
                    callback( *token, context, &input_device->v2_IGameInputDevice_iface, time(NULL), input_device->deviceStatus, input_device->previousDeviceStatus );
                }
        }
        LeaveCriticalSection( &provider_cs );
    }

    return S_OK;
}

HRESULT game_input_device_Create( v2_IGameInputDevice **device )
{
    struct game_input_device *impl;

    TRACE( "device %p\n", device );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->v2_IGameInputDevice_iface.lpVtbl = &game_input_device_vtbl;
    impl->ref = 1;

    InitializeCriticalSection( &impl->readLock );

    *device = &impl->v2_IGameInputDevice_iface;
    TRACE( "created v2_IGameInputDevice %p\n", &impl->v2_IGameInputDevice_iface );

    return S_OK;
}