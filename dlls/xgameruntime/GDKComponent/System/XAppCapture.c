/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XAppCapture
 *
 * Copyright 2026 Olivia Ryan
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

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

struct x_app_capture
{
    IXAppCaptureImpl IXAppCaptureImpl_iface;
    IXAppCaptureImpl5 IXAppCaptureImpl5_iface;
    LONG ref;
};

static inline struct x_app_capture *impl_from_IXAppCaptureImpl( IXAppCaptureImpl *iface )
{
    return CONTAINING_RECORD( iface, struct x_app_capture, IXAppCaptureImpl_iface );
}

static HRESULT WINAPI x_app_capture_QueryInterface( IXAppCaptureImpl *iface, REFIID iid, void **out )
{
    struct x_app_capture *impl = impl_from_IXAppCaptureImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown     ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl ))
    {
        IXAppCaptureImpl_AddRef( *out = &impl->IXAppCaptureImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_app_capture_AddRef( IXAppCaptureImpl *iface )
{
    struct x_app_capture *impl = impl_from_IXAppCaptureImpl( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_app_capture_Release( IXAppCaptureImpl *iface )
{
    struct x_app_capture *impl = impl_from_IXAppCaptureImpl( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static BOOLEAN WINAPI x_app_capture_XAppBroadcastIsAppBroadcasting( IXAppCaptureImpl *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return FALSE;
}

static HRESULT WINAPI x_app_capture_XAppBroadcastShowUI( IXAppCaptureImpl *iface, XUserHandle requestingUser )
{
    FIXME( "iface %p requestingUser %p stub!\n", iface, requestingUser );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppBroadcastGetStatus( IXAppCaptureImpl *iface, XUserHandle requestingUser, XAppBroadcastStatus *appBroadcastStatus )
{
    FIXME( "iface %p, requestingUser %p, appBroadcastStatus %p stub!\n", iface, requestingUser, appBroadcastStatus );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppBroadcastRegisterIsAppBroadcastingChanged( IXAppCaptureImpl *iface, XTaskQueueHandle queue, void *context, XAppBroadcastMonitorCallback *appBroadcastMonitorCallback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, appBroadcastMonitorCallback %p, token %p stub!\n", iface, queue, context, appBroadcastMonitorCallback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_app_capture_XAppBroadcastUnregisterIsAppBroadcastingChanged( IXAppCaptureImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return TRUE;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataAddStringEvent( IXAppCaptureImpl *iface, const char *name, const char *value, XAppCaptureMetadataPriority priority )
{
    FIXME( "iface %p, name %s, value %s, priority %d stub!\n", iface, debugstr_a( name ), debugstr_a( value ), priority );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataAddInt32Event( IXAppCaptureImpl *iface, const char *name, INT32 value, XAppCaptureMetadataPriority priority )
{
    FIXME( "iface %p, name %s, value %d, priority %d stub!\n", iface, debugstr_a( name ), value, priority );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataAddDoubleEvent( IXAppCaptureImpl *iface, const char *name, double value, XAppCaptureMetadataPriority priority )
{
    FIXME( "iface %p, name %s, value %e, priority %d stub!\n", iface, debugstr_a( name ), value, priority );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataStartStringState( IXAppCaptureImpl *iface, const char *name, const char *value, XAppCaptureMetadataPriority priority )
{
    FIXME( "iface %p, name %s, value %s, priority %d stub!\n", iface, debugstr_a( name ), debugstr_a( value ), priority );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataStartInt32State( IXAppCaptureImpl *iface, const char *name, INT32 value, XAppCaptureMetadataPriority priority )
{
    FIXME( "iface %p, name %s, value %d, priority %d stub!\n", iface, debugstr_a( name ), value, priority );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataStartDoubleState( IXAppCaptureImpl *iface, const char *name, double value, XAppCaptureMetadataPriority priority )
{
    FIXME( "iface %p, name %s, value %e, priority %d stub!\n", iface, debugstr_a( name ), value, priority );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataStopState( IXAppCaptureImpl *iface, const char *name )
{
    FIXME( "iface %p, name %s stub!\n", iface, debugstr_a( name ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataStopAllStates( IXAppCaptureImpl *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureMetadataRemainingStorageBytesAvailable( IXAppCaptureImpl *iface, UINT64 *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture_XAppCaptureRegisterMetadataPurged( IXAppCaptureImpl *iface, XTaskQueueHandle queue, void *context, XAppCaptureMetadataPurgedCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_app_capture_XAppCaptureUnRegisterMetadataPurged( IXAppCaptureImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return TRUE;
}

static const struct IXAppCaptureImplVtbl x_app_capture_vtbl =
{
    x_app_capture_QueryInterface,
    x_app_capture_AddRef,
    x_app_capture_Release,
    /* IXAppCaptureImpl methods */
    x_app_capture_XAppBroadcastIsAppBroadcasting,
    x_app_capture_XAppBroadcastShowUI,
    x_app_capture_XAppBroadcastGetStatus,
    x_app_capture_XAppBroadcastRegisterIsAppBroadcastingChanged,
    x_app_capture_XAppBroadcastUnregisterIsAppBroadcastingChanged,
    x_app_capture_XAppCaptureMetadataAddStringEvent,
    x_app_capture_XAppCaptureMetadataAddInt32Event,
    x_app_capture_XAppCaptureMetadataAddDoubleEvent,
    x_app_capture_XAppCaptureMetadataStartStringState,
    x_app_capture_XAppCaptureMetadataStartInt32State,
    x_app_capture_XAppCaptureMetadataStartDoubleState,
    x_app_capture_XAppCaptureMetadataStopState,
    x_app_capture_XAppCaptureMetadataStopAllStates,
    x_app_capture_XAppCaptureMetadataRemainingStorageBytesAvailable,
    x_app_capture_XAppCaptureRegisterMetadataPurged,
    x_app_capture_XAppCaptureUnRegisterMetadataPurged,
};

static inline struct x_app_capture *impl_from_IXAppCaptureImpl5( IXAppCaptureImpl5 *iface )
{
    return CONTAINING_RECORD( iface, struct x_app_capture, IXAppCaptureImpl5_iface );
}

static HRESULT WINAPI x_app_capture2_QueryInterface( IXAppCaptureImpl5 *iface, REFIID iid, void **out )
{
    struct x_app_capture *impl = impl_from_IXAppCaptureImpl5( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown      ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl2 ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl3 ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl4 ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl5 ))
    {
        IXAppCaptureImpl3_AddRef( *out = &impl->IXAppCaptureImpl5_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_app_capture2_AddRef( IXAppCaptureImpl5 *iface )
{
    struct x_app_capture *impl = impl_from_IXAppCaptureImpl5( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_app_capture2_Release( IXAppCaptureImpl5 *iface )
{
    struct x_app_capture *impl = impl_from_IXAppCaptureImpl5( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureTakeDiagnosticScreenshot( IXAppCaptureImpl5 *iface, BOOLEAN gamescreenOnly, XAppCaptureScreenshotFormatFlag x_app_captureFlags, const char *filenamePrefix, XAppCaptureDiagnosticScreenshotResult *result )
{
    FIXME( "iface %p, gamescreenOnly %d, x_app_captureFlags %x, filenamePrefix %s, result %p stub!\n", iface, gamescreenOnly, x_app_captureFlags, debugstr_a( filenamePrefix ), result );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureRecordDiagnosticClip( IXAppCaptureImpl5 *iface, time_t startTime, UINT32 durationInMs, const char *filenamePrefix, XAppCaptureRecordClipResult *result )
{
    FIXME( "iface %p, startTime %Iu, durationInMs %u, filenamePrefix %s, result %p stub!\n", iface, startTime, durationInMs, debugstr_a( filenamePrefix ), result );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureTakeScreenshot( IXAppCaptureImpl5 *iface, XUserHandle requestingUser, XAppCaptureTakeScreenshotResult *result )
{
    FIXME( "iface %p, requestingUser %p, result %p stub!\n", iface, requestingUser, result );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureOpenScreenshotStream( IXAppCaptureImpl5 *iface, const char *localId, XAppCaptureScreenshotFormatFlag screenshotFormat, XAppCaptureScreenshotStreamHandle *handle, UINT64 *totalBytes )
{
    FIXME( "iface %p, localId %s, screenshotFormat %d, handle %p, totalBytes %p stub!\n", iface, debugstr_a( localId ), screenshotFormat, handle, totalBytes );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureReadScreenshotStream( IXAppCaptureImpl5 *iface, XAppCaptureScreenshotStreamHandle handle, UINT64 startPosition, UINT32 bytesToRead, UINT8 *bytes, UINT32 *bytesWritten )
{
    FIXME( "iface %p, handle %p, startPosition %llu, bytesToRead %u, bytes %p, bytesWritten %p stub!\n", iface, handle, startPosition, bytesToRead, bytes, bytesWritten );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureCloseScreenshotStream( IXAppCaptureImpl5 *iface, XAppCaptureScreenshotStreamHandle handle )
{
    FIXME( "iface %p, handle %p stub!\n", iface, handle );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureEnableRecord( IXAppCaptureImpl5 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureDisableRecord( IXAppCaptureImpl5 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureGetVideoCaptureSettings( IXAppCaptureImpl5 *iface, XAppCaptureVideoCaptureSettings *userCaptureSettings )
{
    FIXME( "iface %p, userCaptureSettings %p stub!\n", iface, userCaptureSettings );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureRecordTimespan( IXAppCaptureImpl5 *iface, const SYSTEMTIME *startTimestamp, UINT64 durationInMilliseconds, XAppCaptureLocalResult *result )
{
    FIXME( "iface %p, startTimestamp %p, durationInMilliseconds %llu, result %p stub!\n", iface, startTimestamp, durationInMilliseconds, result );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureReadLocalStream( IXAppCaptureImpl5 *iface, XAppCaptureLocalStreamHandle handle, SIZE_T startPosition, UINT32 bytesToRead, UINT8 *bytes, UINT32 *bytesWritten )
{
    FIXME( "iface %p, handle %p, startPosition %Iu, bytesToRead %u, bytes %p, bytesWritten %p stub!\n", iface, handle, startPosition, bytesToRead, bytes, bytesWritten );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureCloseLocalStream( IXAppCaptureImpl5 *iface, XAppCaptureLocalStreamHandle handle )
{
    FIXME( "iface %p, handle %p stub!\n", iface, handle );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureStartUserRecord( IXAppCaptureImpl5 *iface, XUserHandle requestingUser, UINT32 localIdBufferLength, char *localIdBuffer )
{
    FIXME( "iface %p, requestingUser %p, localIdBufferLength %u, localIdBuffer %p stub!\n", iface, requestingUser, localIdBufferLength, localIdBuffer );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureStopUserRecord( IXAppCaptureImpl5 *iface, const char *localId, XAppCaptureUserRecordingResult *result )
{
    FIXME( "iface %p, localId %s, result %p stub!\n", iface, debugstr_a( localId ), result );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_app_capture2_XAppCaptureCancelUserRecord( IXAppCaptureImpl5 *iface, const char *localId )
{
    FIXME( "iface %p, localId %s stub!\n", iface, debugstr_a( localId ) );
    return E_NOTIMPL;
}

static const struct IXAppCaptureImpl5Vtbl x_app_capture2_vtbl =
{
    x_app_capture2_QueryInterface,
    x_app_capture2_AddRef,
    x_app_capture2_Release,
    /* IXAppCaptureImpl2 methods */
    x_app_capture2_XAppCaptureTakeDiagnosticScreenshot,
    x_app_capture2_XAppCaptureRecordDiagnosticClip,
    x_app_capture2_XAppCaptureTakeScreenshot,
    x_app_capture2_XAppCaptureOpenScreenshotStream,
    x_app_capture2_XAppCaptureReadScreenshotStream,
    x_app_capture2_XAppCaptureCloseScreenshotStream,
    x_app_capture2_XAppCaptureEnableRecord,
    x_app_capture2_XAppCaptureDisableRecord,
    /* IXAppCaptureImpl3 methods */
    x_app_capture2_XAppCaptureGetVideoCaptureSettings,
    x_app_capture2_XAppCaptureRecordTimespan,
    x_app_capture2_XAppCaptureReadLocalStream,
    x_app_capture2_XAppCaptureCloseLocalStream,
    /* IXAppCaptureImpl4 methods */
    x_app_capture2_XAppCaptureStartUserRecord,
    x_app_capture2_XAppCaptureStopUserRecord,
    /* IXAppCaptureImpl5 methods */
    x_app_capture2_XAppCaptureCancelUserRecord,
};

static struct x_app_capture x_app_capture =
{
    {&x_app_capture_vtbl},
    {&x_app_capture2_vtbl},
    0,
};

IXAppCaptureImpl *x_app_capture_impl = &x_app_capture.IXAppCaptureImpl_iface;
IXAppCaptureImpl5 *x_app_capture2_impl = &x_app_capture.IXAppCaptureImpl5_iface;
