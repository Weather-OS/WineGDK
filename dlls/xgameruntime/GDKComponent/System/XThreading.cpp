/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XThread, XAsync and XTask
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

#include "../../private.h"

#include <atomic>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

class XThreadingImpl : 
    public IXThreadingImpl
{
public:
    HRESULT WINAPI QueryInterface( REFIID iid, void **out )
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

        if (!out) return E_POINTER;
        *out = nullptr;

        if ( iid == __uuidof( IUnknown ) ||
             iid == __uuidof( IInspectable ) ||
             iid == __uuidof( IAgileObject ) ||
             iid == __uuidof( IXThreadingImpl ) )
        {
            AddRef();
            *out = static_cast<IXThreadingImpl *>(this);
            return S_OK;
        }

        FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
        *out = nullptr;
        return E_NOINTERFACE;
    }

    ULONG WINAPI 
    AddRef() noexcept override
    {
        ULONG curr = static_cast<ULONG>(++ref);
        TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
        return curr;
    }

    ULONG WINAPI 
    Release() noexcept override
    {
        ULONG curr = static_cast<ULONG>(--ref);
        TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

        // Polymorphic classes should not be deleted.
        /*
        if ( !curr )
            delete this;
        */

        return curr;
    }

    HRESULT WINAPI XAsyncGetStatus( XAsyncBlock *asyncBlock, BOOLEAN wait ) override
    {
        TRACE("asyncBlock %p, wait %d\n", asyncBlock, wait);
        return ::XAsyncGetStatus( asyncBlock, wait );
    }

    HRESULT WINAPI XAsyncGetResultSize( XAsyncBlock *asyncBlock, SIZE_T *bufferSize ) override
    {
        TRACE("asyncBlock %p, bufferSize %p\n", asyncBlock, bufferSize);
        return ::XAsyncGetResultSize( asyncBlock, bufferSize );
    }

    void WINAPI XAsyncCancel( XAsyncBlock *asyncBlock ) override
    {
        TRACE("asyncBlock %p\n", asyncBlock);
        ::XAsyncCancel( asyncBlock );
    }

    HRESULT WINAPI XAsyncRun( XAsyncBlock *asyncBlock, XAsyncWork *work ) override
    {
        TRACE("asyncBlock %p, work %p\n", asyncBlock, work);
        return ::XAsyncRun( asyncBlock, work );
    }

    HRESULT WINAPI XAsyncBegin( XAsyncBlock *asyncBlock, void *context, const void *identity, const char *identityName, XAsyncProvider *provider ) override
    {
        TRACE("asyncBlock %p, context %p, identity %p, identityName %s, provider %p\n", asyncBlock, context, identity, identityName, provider);
        return ::XAsyncBegin( asyncBlock, context, identity, identityName, provider );
    }

    //  NOTE: Padding function here is most likely XAsyncBeginAlloc from libHttpClient. More testing needs to be conducted.
    // Since this is a private/undocumented function, implementation is not necessary.
    HRESULT WINAPI __PADDING__() override
    {
        WARN("padding function called!\n");
        return E_NOTIMPL;
    }

    HRESULT WINAPI XAsyncSchedule( XAsyncBlock *asyncBlock, UINT32 delayInMs ) override
    {
        TRACE("asyncBlock %p, UINT32 delayInMs %d\n", asyncBlock, delayInMs);
        return ::XAsyncSchedule( asyncBlock, delayInMs );
    }

    void WINAPI XAsyncComplete( XAsyncBlock *asyncBlock, HRESULT result, SIZE_T requiredBufferSize ) override
    {
        TRACE("asyncBlock %p, result %#lx, requiredBufferSize %lld\n", asyncBlock, result, requiredBufferSize);
        ::XAsyncComplete( asyncBlock, result, requiredBufferSize );
    }

    HRESULT WINAPI XAsyncGetResult( XAsyncBlock *asyncBlock, const void *identity, SIZE_T bufferSize, void *buffer, SIZE_T *bufferUsed ) override
    {
        TRACE("asyncBlock %p, identity %p, bufferSize %lld, buffer %p, bufferUsed %p\n", asyncBlock, identity, bufferSize, buffer, bufferUsed);
        return ::XAsyncGetResult( asyncBlock, identity, bufferSize, buffer, bufferUsed );
    }

    HRESULT WINAPI XTaskQueueCreate( XTaskQueueDispatchMode workDispatchMode, XTaskQueueDispatchMode completionDispatchMode, XTaskQueueHandle *queue ) override
    {
        TRACE("workDispatchMode %d, completionDispatchMode %d, queue %p\n", (int)workDispatchMode, (int)completionDispatchMode, queue);
        return ::XTaskQueueCreate( workDispatchMode, completionDispatchMode, queue );
    }

    HRESULT WINAPI XTaskQueueCreateComposite( XTaskQueuePortHandle workPort, XTaskQueuePortHandle completionPort, XTaskQueueHandle *queue ) override
    {
        TRACE("workPort %p, completionPort %p, queue %p\n", workPort, completionPort, queue);
        return ::XTaskQueueCreateComposite( workPort, completionPort, queue );
    }
    
    HRESULT WINAPI XTaskQueueGetPort( XTaskQueueHandle queue, XTaskQueuePort port, XTaskQueuePortHandle *portHandle ) override
    {
        TRACE("queue %p, port %d, portHandle %p\n", queue, (int)port, portHandle);
        return ::XTaskQueueGetPort( queue, port, portHandle );
    }

    HRESULT WINAPI XTaskQueueDuplicateHandle( XTaskQueueHandle queueHandle, XTaskQueueHandle *duplicatedHandle ) override
    {
        TRACE("queueHandle %p, duplicatedHandle %p\n", queueHandle, duplicatedHandle);
        return ::XTaskQueueDuplicateHandle( queueHandle, duplicatedHandle );
    }

    BOOLEAN WINAPI XTaskQueueDispatch( XTaskQueueHandle queue, XTaskQueuePort port, UINT32 timeoutInMs ) override
    {
        TRACE("queue %p, port %d, timeoutInMs %d\n", queue, (int)port, timeoutInMs);
        return ::XTaskQueueDispatch( queue, port, timeoutInMs );
    }

    void WINAPI XTaskQueueCloseHandle( XTaskQueueHandle queue ) override
    {
        TRACE("queue %p\n", queue);
        ::XTaskQueueCloseHandle( queue );
    }

    HRESULT WINAPI XTaskQueueSubmitCallback( XTaskQueueHandle queue, XTaskQueuePort port, void *callbackContext, XTaskQueueCallback *callback ) override
    {
        TRACE("queue %p, port %d, callbackContext %p, callback %p\n", queue, (int)port, callbackContext, callback);
        return ::XTaskQueueSubmitCallback( queue, port, callbackContext, callback );
    }

    HRESULT WINAPI XTaskQueueSubmitDelayedCallback( XTaskQueueHandle queue, XTaskQueuePort port, UINT32 delayMs, void *callbackContext, XTaskQueueCallback *callback ) override
    {
        TRACE("queue %p, port %d, delayMs %d, callbackContext %p, callback %p\n", queue, (int)port, delayMs, callbackContext, callback);
        return ::XTaskQueueSubmitDelayedCallback( queue, port, delayMs, callbackContext, callback );
    }

    HRESULT WINAPI XTaskQueueRegisterWaiter( XTaskQueueHandle queue, XTaskQueuePort port, HANDLE waitHandle, void *callbackContext, XTaskQueueCallback *callback, XTaskQueueRegistrationToken *token ) override
    {
        TRACE("queue %p, port %d, waitHandle %p, callbackContext %p, callback %p, token %p\n", queue, (int)port, waitHandle, callbackContext, callback, token);
        return ::XTaskQueueRegisterWaiter( queue, port, waitHandle, callbackContext, callback, token );
    }
    
    void WINAPI XTaskQueueUnregisterWaiter( XTaskQueueHandle queue, XTaskQueueRegistrationToken token ) override
    {
        TRACE("queue %p, token %lld\n", queue, token.token);
        ::XTaskQueueUnregisterWaiter( queue, token );
    }

    HRESULT WINAPI XTaskQueueTerminate( XTaskQueueHandle queue, BOOLEAN wait, void *callbackContext, XTaskQueueTerminatedCallback *callback ) override
    {
        TRACE("queue %p, wait %d, callbackContext %p, callback %p\n", queue, wait, callbackContext, callback);
        return ::XTaskQueueTerminate( queue, wait, callbackContext, callback );
    }

    HRESULT WINAPI XTaskQueueRegisterMonitor( XTaskQueueHandle queue, void *callbackContext, XTaskQueueMonitorCallback *callback, XTaskQueueRegistrationToken *token ) override
    {
        TRACE("queue %p, callbackContext %p, callback %p, token %p\n", queue, callbackContext, callback, token);
        return ::XTaskQueueRegisterMonitor( queue, callbackContext, callback, token );
    }

    void WINAPI XTaskQueueUnregisterMonitor( XTaskQueueHandle queue, XTaskQueueRegistrationToken token ) override
    {
        TRACE("queue %p, token %lld\n", queue, token.token);
        ::XTaskQueueUnregisterMonitor( queue, token );
    }

    BOOLEAN WINAPI XTaskQueueGetCurrentProcessTaskQueue( XTaskQueueHandle *queue ) override
    {
        TRACE("queue %p\n", queue);
        return ::XTaskQueueGetCurrentProcessTaskQueue( queue );
    }

    void WINAPI XTaskQueueSetCurrentProcessTaskQueue( XTaskQueueHandle queue ) override
    {
        TRACE("queue %p\n", queue);
        ::XTaskQueueSetCurrentProcessTaskQueue( queue );
    }

    HRESULT WINAPI XThreadSetTimeSensitive( BOOLEAN isTimeSensitiveThread ) override
    {
        this->isTimeSensitiveThread = isTimeSensitiveThread;
        return S_OK;
    }

    //  NOTE: Padding function here is most likely XTaskQueueUninitialize from libHttpClient. More testing needs to be conducted.
    // Since this is a private/undocumented function, implementation is not necessary.
    HRESULT WINAPI __PADDING_2__() override
    {
        WARN("padding function called!\n");
        return E_NOTIMPL;
    }

    void WINAPI XThreadAssertNotTimeSensitive() override
    {
        if ( isTimeSensitiveThread )
            assert( false );
    }

    BOOLEAN WINAPI XThreadIsTimeSensitive() override
    {
        return isTimeSensitiveThread;
    }

private:
    BOOLEAN isTimeSensitiveThread{ false };
    std::atomic_long ref{ 1 };
};

static XThreadingImpl g_x_threading;

IXThreadingImpl *x_threading_impl = static_cast<IXThreadingImpl*>(&g_x_threading);