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
#include "robuffer.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Streams;

class ABI::Windows::Storage::Streams::InMemoryRandomAccessStream final
    : public IRandomAccessStream
    , public IOutputStream
    , public IInputStream
    , public IClosable
{
public:
    InMemoryRandomAccessStream() = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) noexcept override
    {
        TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );
        if (!out) return E_POINTER;
        *out = nullptr;

        if (IsEqualIID(iid, __uuidof( IUnknown )) ||
            IsEqualIID(iid, IID_IInspectable) ||
            IsEqualIID(iid, IID_IAgileObject) ||
            IsEqualIID(iid, __uuidof( IRandomAccessStream )))
        {
            *out = static_cast<IRandomAccessStream*>(this);
        }
        else if ( iid == __uuidof( IOutputStream ) )
        {
            *out = static_cast<IOutputStream*>(this);
        }
        else if ( iid == __uuidof( IInputStream ) )
        {
            *out = static_cast<IInputStream*>(this);
        }
        else if ( iid == __uuidof( IClosable ) )
        {
            *out = static_cast<IClosable*>(this);
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

    // IRandomAccessStream
    HRESULT WINAPI get_Size( UINT64 *value ) override
    {
        TRACE( "iface %p, value %p.\n", this, value );

        if (closed)
        {
            *value = 0;
            return RO_E_CLOSED;
        }

        *value = size;
        return S_OK;
    }

    HRESULT WINAPI put_Size( UINT64 value ) override
    {
        HRESULT hr;

        TRACE( "iface %p, value %I64u.\n", this, value );

        if (closed)
            return RO_E_CLOSED;

        /* Native truncates the size to 32 bits, which is not replicated here if size_t is 64 bits. */
        if (FAILED(hr = memory_stream_require_capacity( reinterpret_cast<IUnknown*>(this), value )))
            return hr;

        size = value;
        return S_OK;
    }

    HRESULT WINAPI GetInputStreamAt( UINT64 position, IInputStream **stream ) override
    {
        FIXME( "iface %p, position %I64u, stream %p stub!\n", this, position, stream );

        *stream = NULL;
        return E_NOTIMPL;
    }

    HRESULT WINAPI GetOutputStreamAt( UINT64 position, IOutputStream **stream ) override
    {
        FIXME( "iface %p, position %I64u, stream %p stub!\n", this, position, stream );

        *stream = NULL;
        return E_NOTIMPL;
    }

    HRESULT WINAPI get_Position( UINT64 *value ) override
    {
        TRACE( "iface %p, value %p.\n", this, value );

        *value = pos;
        return closed ? RO_E_CLOSED : S_OK;
    }

    HRESULT WINAPI Seek( UINT64 position ) override
    {
        TRACE( "iface %p, position %I64u.\n", this, position );

        if (closed)
            return RO_E_CLOSED;

        pos = position;
        return S_OK;
    }

    HRESULT WINAPI CloneStream( IRandomAccessStream **stream ) override
    {
        *stream = NULL;
        return E_NOTIMPL;
    }

    HRESULT WINAPI get_CanRead( boolean *value ) override
    {
        *value = TRUE;
        return S_OK;
    }

    HRESULT WINAPI get_CanWrite( boolean *value ) override
    {
        *value = TRUE;
        return S_OK;
    }

    // IClosable
    HRESULT WINAPI Close() override
    {
        TRACE( "iface %p.\n", this );

        closed = TRUE;
        return S_OK;
    }

    // IInputStream
    HRESULT WINAPI ReadAsync( IBuffer *buffer, UINT32 count, InputStreamOptions options, IAsyncOperationWithProgress<IBuffer*, UINT32> **operation ) override
    {
        // TODO: Implement the new async parameters macro within IWineAsync.
        FIXME( "iface %p, buffer %p, count %u, options %d, operation %p.\n", this, buffer, count, (int)options, operation );
        return E_NOTIMPL;
    }

    // IOutputStream
    HRESULT WINAPI WriteAsync( IBuffer *buffer, IAsyncOperationWithProgress<UINT32, UINT32> **operation ) override
    {
        // TODO: Implement async with progress within IWineAsync.
        FIXME( "iface %p, buffer %p, operation %p.\n", this, buffer, operation );
        return E_NOTIMPL;
    }

    HRESULT WINAPI FlushAsync( IAsyncOperation<boolean> **operation ) override
    {
        FIXME( "iface %p, operation %p stub!\n", this, operation );

        *operation = NULL;

        if (closed)
            return RO_E_CLOSED;

        return E_NOTIMPL;
    }

private:
    static HRESULT memory_stream_require_capacity( IUnknown *iface, size_t capacity )
    {
        auto *impl = reinterpret_cast<InMemoryRandomAccessStream *>(iface);
        BYTE *new_buffer;

        if (capacity <= impl->capacity)
            return S_OK;

        capacity = max( capacity, impl->capacity + impl->capacity / 2u );
        capacity = max( capacity, 0x1000 );
        new_buffer = (PBYTE)realloc( impl->buffer, capacity );

        if (!new_buffer)
            return HRESULT_FROM_WIN32( ERROR_DISK_FULL );

        /* Zero memory for security and to silence address sanitisers */
        memset( &new_buffer[impl->capacity], 0, capacity - impl->capacity );
        impl->capacity = capacity;
        impl->buffer = new_buffer;

        return S_OK;
    }

    void memory_stream_read( IBuffer *buffer, UINT32 count )
    {
        IBufferByteAccess *access;
        BYTE *data;

        buffer->QueryInterface<IBufferByteAccess>( &access );

        access->Buffer( &data );
        access->Release();

        count = ( size >= pos ) ? min( count, size - pos ) : 0;
        memcpy( data, &buffer[pos], count );
        pos += count;
        buffer->put_Length( count );
    }

    BYTE *buffer;
    size_t capacity;
    size_t size;
    size_t pos;
    BOOL closed;
    std::atomic_long ref_{ 0 };
};


class RandomAccessStreamReferenceImpl final
    : public IActivationFactory
    , public IRandomAccessStreamReferenceStatics
{
public:
    RandomAccessStreamReferenceImpl() = default;

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

// Class is auto-instantiable.
class InMemoryRandomAccessStreamImpl final
    : public IActivationFactory
{
public:
    InMemoryRandomAccessStreamImpl() = default;

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
        HRESULT hr;

        auto out = new InMemoryRandomAccessStream();

        TRACE( "instance %p.\n", instance );

        *instance = NULL;

        hr = out->QueryInterface( IID_PPV_ARGS( instance ) );
        out->Release();

        if (SUCCEEDED(hr))
            TRACE( "created InMemoryRandomAccessStream %p.\n", *instance );

        return hr;
    }

private:
    std::atomic_long ref_{ 0 }; // static singleton-style object
};

static RandomAccessStreamReferenceImpl g_random_access_stream_reference_statics;
static InMemoryRandomAccessStreamImpl g_in_memory_random_access_stream_statics;

IActivationFactory* random_access_stream_reference_factory =
    static_cast<IActivationFactory*>(&g_random_access_stream_reference_statics);
IActivationFactory* memory_stream_activation_factory = 
    static_cast<IActivationFactory*>(&g_in_memory_random_access_stream_statics);