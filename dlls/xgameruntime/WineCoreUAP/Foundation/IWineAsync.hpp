/* CryptoWinRT Implementation
 *
 * Copyright 2022 Bernhard Kölbl for CodeWeavers
 * Copyright 2022 Rémi Bernon for CodeWeavers
 * C++ port was done by Weather.
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
#include "provider.h"

#include <mutex>

#ifndef IWINEASYNC_HPP
#define IWINEASYNC_HPP

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Xodus;

class AsyncInfo final
    : public IAsyncInfo
    , public IWineAsyncInfoImpl
{
public:
    AsyncInfo() = default;

    /* IUnknown Methods (Shared) */
    HRESULT WINAPI
    QueryInterface( REFIID iid, void** out ) noexcept override;

    ULONG WINAPI
    AddRef() noexcept override;

    ULONG WINAPI
    Release() noexcept override;

    /* IInspectable Methods (WineAsyncInfoImpl) */
    HRESULT WINAPI
    GetIids( ULONG *iid_count, IID **iids ) noexcept override;

    HRESULT WINAPI
    GetRuntimeClassName( HSTRING *class_name ) noexcept override;

    HRESULT WINAPI
    GetTrustLevel( TrustLevel *trust_level ) noexcept override;

    /* IWineAsyncOperationCompletedHandler Methods */
    HRESULT WINAPI
    put_Completed( IWineAsyncOperationCompletedHandler *handler ) noexcept override;

    HRESULT WINAPI
    get_Completed( IWineAsyncOperationCompletedHandler **handler ) noexcept override;

    HRESULT WINAPI
    get_Result( PROPVARIANT *result ) noexcept override;

    HRESULT WINAPI
    Start() noexcept override;

    /* IAsyncInfo Methods */
    HRESULT WINAPI
    get_Id( UINT32 *id ) noexcept override;

    HRESULT WINAPI
    get_Status( AsyncStatus *status ) noexcept override;

    HRESULT WINAPI
    get_ErrorCode( HRESULT *error_code ) noexcept override;

    HRESULT WINAPI
    Cancel() noexcept override;

    HRESULT WINAPI
    Close() noexcept override;

    /* Internal methods */
    static HRESULT WINAPI
    Create( IUnknown *invoker, PVOID param, async_operation_callback callback,
                                  IInspectable *outer, IWineAsyncInfoImpl **out ) noexcept;

private:
    static void CALLBACK
    async_info_callback( TP_CALLBACK_INSTANCE *instance, void *iface, TP_WORK *work );

    std::atomic_long ref{ 1 };
    std::mutex mutex;

    IWineAsyncOperationCompletedHandler *handler;
    IInspectable *IInspectable_outer;
    IErrorInfo *errorInfo;
    IUnknown *invoker;

    async_operation_callback callback;
    AsyncStatus status;
    PROPVARIANT result;
    HRESULT hr;
    TP_WORK *async_run_work;
    PVOID param;
};

class AsyncOperation
{
public:
    class Inspectable final
    : public IAsyncOperation<IInspectable *>
    {
    public:
        Inspectable() = default;

        /* IUnknown Methods */
        HRESULT WINAPI
        QueryInterface( REFIID iid, void** out ) noexcept override;

        ULONG WINAPI
        AddRef() noexcept override;

        ULONG WINAPI
        Release() noexcept override;

        /* IInspectable Methods */
        HRESULT WINAPI
        GetIids( ULONG *iid_count, IID **iids ) noexcept override;

        HRESULT WINAPI
        GetRuntimeClassName( HSTRING *class_name ) noexcept override;

        HRESULT WINAPI
        GetTrustLevel( TrustLevel *trust_level ) noexcept override;

        /* IAsyncOperation<TResult> methods */
        HRESULT WINAPI
        put_Completed( IAsyncOperationCompletedHandler<IInspectable *> *inspectable_handler ) noexcept override;

        HRESULT WINAPI
        get_Completed( IAsyncOperationCompletedHandler<IInspectable *> **inspectable_handler ) noexcept override;

        HRESULT WINAPI
        GetResults( IInspectable **results ) noexcept override;

        /* Internal methods */
        static HRESULT WINAPI
        Create( IUnknown *invoker, PVOID param, async_operation_callback callback,
                                            IAsyncOperation<IInspectable *> **out );

    private:
        std::atomic_long ref{ 1 };
        struct async_operation_iids iids;
        IWineAsyncInfoImpl *info;
    };
};

class AsyncAction final
    : public IAsyncAction
{
public:
    AsyncAction() = default;

    /* IUnknown Methods */
    HRESULT WINAPI
    QueryInterface( REFIID iid, void** out ) noexcept override;

    ULONG WINAPI
    AddRef() noexcept override;

    ULONG WINAPI
    Release() noexcept override;

    /* IInspectable Methods */
    HRESULT WINAPI
    GetIids( ULONG *iid_count, IID **iids ) noexcept override;

    HRESULT WINAPI
    GetRuntimeClassName( HSTRING *class_name ) noexcept override;

    HRESULT WINAPI
    GetTrustLevel( TrustLevel *trust_level ) noexcept override;

    /* IAsyncOperation<TResult> methods */
    HRESULT WINAPI
    put_Completed( IAsyncActionCompletedHandler *inspectable_handler ) noexcept override;

    HRESULT WINAPI
    get_Completed( IAsyncActionCompletedHandler **inspectable_handler ) noexcept override;

    HRESULT WINAPI
    GetResults() noexcept override;

    /* Internal methods */
    static HRESULT WINAPI
    Create( IUnknown *invoker, PVOID param, async_operation_callback callback,
                                        IAsyncAction **out );

private:
    std::atomic_long ref{ 1 };
    IWineAsyncInfoImpl *info;
};

#endif