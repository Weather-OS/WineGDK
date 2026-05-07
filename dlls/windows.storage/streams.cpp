/* WinRT Windows.Storage.Streams Implementation
 *
 * Copyright (C) 2025 Mohamad Al-Jaf
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

#include <atomic>

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Streams;

class ABI::Windows::Storage::Streams::RandomAccessStreamReference final
    : public IActivationFactory
    , public IRandomAccessStreamReferenceStatics
{
public:
    RandomAccessStreamReference() = default;

    // IUnknown / IInspectable / IActivationFactory
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) noexcept override
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );
        if (!out) return E_POINTER;
        *out = nullptr;

        if (IsEqualIID(iid, __uuidof( IUnknown )) ||
            IsEqualIID(iid, IID_IInspectable) ||
            IsEqualIID(iid, IID_IAgileObject) ||
            IsEqualIID(iid, IID_IActivationFactory))
        {
            *out = static_cast<IActivationFactory*>(this);
        }
        else if ( iid == __uuidof(IRandomAccessStreamReferenceStatics) )
        {
            *out = static_cast<IRandomAccessStreamReferenceStatics*>(this);
        }
        else
        {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() noexcept override
    {
        TRACE( "iface %p increasing refcount to %lu.\n", this, ref_.load() + 1 );
        return static_cast<ULONG>(++ref_);
    }

    ULONG STDMETHODCALLTYPE Release() noexcept override
    {
        ULONG n = static_cast<ULONG>(--ref_);
        TRACE( "iface %p decreasing refcount to %lu.\n", this, ref_.load() );
        if ( !n ) delete this;
        return n;
    }

    // IInspectable
    HRESULT STDMETHODCALLTYPE GetIids(ULONG* iid_count, IID** iids) noexcept override
    {
        if (!iid_count || !iids) return E_POINTER;
        *iid_count = 0;
        *iids = nullptr;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetRuntimeClassName(HSTRING* class_name) noexcept override
    {
        if (!class_name) return E_POINTER;
        *class_name = nullptr;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetTrustLevel(TrustLevel* trust_level) noexcept override
    {
        if (!trust_level) return E_POINTER;
        return E_NOTIMPL;
    }

    // IActivationFactory
    HRESULT STDMETHODCALLTYPE ActivateInstance(IInspectable** instance) noexcept override
    {
        if (!instance) return E_POINTER;
        *instance = nullptr;
        return E_NOTIMPL; // or create an actual instance
    }

    // IRandomAccessStreamReferenceStatics
    HRESULT STDMETHODCALLTYPE CreateFromFile(
        IStorageFile* file,
        IRandomAccessStreamReference** stream_reference) noexcept override
    {
        if (!stream_reference) return E_POINTER;
        *stream_reference = nullptr;
        if (!file) return E_POINTER;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CreateFromUri(
        IUriRuntimeClass* uri,
        IRandomAccessStreamReference** stream_reference) noexcept override
    {
        if (!stream_reference) return E_POINTER;
        *stream_reference = nullptr;
        if (!uri) return E_POINTER;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CreateFromStream(
        IRandomAccessStream* stream,
        IRandomAccessStreamReference** stream_reference) noexcept override
    {
        if (!stream_reference) return E_POINTER;
        *stream_reference = nullptr;
        if (!stream) return E_POINTER;
        return E_NOTIMPL;
    }

private:
    std::atomic_long ref_{ 0 }; // static singleton-style object
};

static RandomAccessStreamReference g_random_access_stream_reference_statics;

IActivationFactory* random_access_stream_reference_factory =
    static_cast<IActivationFactory*>(&g_random_access_stream_reference_statics);