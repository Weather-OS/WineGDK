/* WinRT IVector implementation
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include "IWineVector.hpp"

#include <algorithm>

WINE_DEFAULT_DEBUG_CHANNEL(winevector);

template<typename T>
Iterator<T>::Iterator( IVectorView<T> *view, UINT32 size ) noexcept : 
    view( view ),
    size( size )
{
    view->AddRef();
}

template<typename T>
HRESULT WINAPI
Iterator<T>::QueryInterface( REFIID iid, void** out ) noexcept
{
    TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( IIterator<T> ) )
    {
        AddRef();
        *out = static_cast<IIterator<T> *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

template<typename T>
ULONG WINAPI
Iterator<T>::AddRef() noexcept
{
    ULONG curr = static_cast<ULONG>(++ref);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

template<typename T>
ULONG WINAPI
Iterator<T>::Release() noexcept
{
    ULONG curr = static_cast<ULONG>(--ref);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr )
    {
        view->Release();
        delete this;
    }

    return curr;
}

template<typename T>
HRESULT WINAPI
Iterator<T>::GetIids( ULONG *iid_count, IID **iids ) noexcept
{
    TRACE( "iface %p, iid_count %p, iids %p\n", this, iid_count, iids );

    if ( !iid_count || !iids )
        return E_POINTER;

    *iid_count = 1;
    IID* allocated = static_cast<IID*>( CoTaskMemAlloc( sizeof(IID) * (*iid_count) ) );

    if ( !allocated )
        return E_OUTOFMEMORY;

    allocated[0] = __uuidof( IIterator<T> );

    *iids = allocated;
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Iterator<T>::GetRuntimeClassName( HSTRING *class_name ) noexcept
{
    TRACE( "iface %p, class_name %p\n", this, class_name );
    return WindowsCreateString( (LPCWSTR)L"Windows.Foundation.Iterator`1<IInspectable>", 48, class_name );
}

template<typename T>
HRESULT WINAPI
Iterator<T>::GetTrustLevel( TrustLevel *trust_level ) noexcept
{
    FIXME( "iface %p, trust_level %p stub!\n", this, trust_level );
    return E_NOTIMPL;
}

template<typename T>
HRESULT WINAPI
Iterator<T>::get_Current( T *value )
{
    TRACE( "this %p, value %p.\n", this, value );
    return view->GetAt( index, value );
}

template<typename T>
HRESULT WINAPI
Iterator<T>::get_HasCurrent( boolean *value )
{
    TRACE( "this %p, value %p.\n", this, value );
    *value = index < size;
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Iterator<T>::MoveNext( boolean *value )
{
    TRACE( "iface %p, value %p.\n", this, value );

    if ( index < size ) index++;
    return get_HasCurrent( value );
}

template<typename T>
HRESULT WINAPI
Iterator<T>::GetMany( UINT32 items_size, T *items, UINT *count )
{
    TRACE( "iface %p, items_size %u, items %p, count %p.\n", this, items_size, items, count );
    return view->GetMany( index, items_size, items, count );
}

template<typename T>
VectorView<T>::VectorView( T* elems, UINT32 size ) noexcept :
    size( size )
{
    ULONG i;
    if constexpr ( std::is_base_of<IUnknown, T>() )
        for ( i = 0; i < size; ++i )
            elems[i]->AddRef();

    elements = (T *)malloc( size * sizeof(T) );

    if constexpr ( std::is_same_v<T, HSTRING> )
        for ( i = 0; i < size; ++i )
            WindowsDuplicateString( elems[i], &elements[i] );
    else
        memcpy( elements, elems, size * sizeof(T) );
}

template<typename T>
HRESULT WINAPI
VectorView<T>::QueryInterface( REFIID iid, void** out ) noexcept
{
    TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( IVectorView<T> ) )
    {
        AddRef();
        *out = static_cast<IVectorView<T> *>(this);
        return S_OK;
    }

    if ( iid == __uuidof( IIterable<T> ) )
    {
        AddRef();
        *out = static_cast<IIterable<T> *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

template<typename T>
ULONG WINAPI
VectorView<T>::AddRef() noexcept
{
    ULONG curr = static_cast<ULONG>(++ref);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

template<typename T>
ULONG WINAPI
VectorView<T>::Release() noexcept
{
    ULONG i, curr = static_cast<ULONG>(--ref);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr )
    {
        if constexpr ( std::is_base_of<IUnknown, T>() )
            for ( i = 0; i < size; ++i ) 
                IInspectable_Release( elements[i] );

        if constexpr ( std::is_same_v<T, HSTRING> )
            for ( i = 0; i < size; ++i )
                WindowsDeleteString( elements[i] );

        free( elements );
        delete this;
    }

    return curr;
}

template<typename T>
HRESULT WINAPI
VectorView<T>::GetIids( ULONG *iid_count, IID **iids ) noexcept
{
    TRACE( "iface %p, iid_count %p, iids %p\n", this, iid_count, iids );

    if ( !iid_count || !iids )
        return E_POINTER;

    *iid_count = 2;
    IID* allocated = static_cast<IID*>( CoTaskMemAlloc( sizeof(IID) * (*iid_count) ) );

    if ( !allocated )
        return E_OUTOFMEMORY;

    allocated[0] = __uuidof( IVectorView<T> );
    allocated[1] = __uuidof( IIterable<T> );

    *iids = allocated;
    return S_OK;
}

template<typename T>
HRESULT WINAPI
VectorView<T>::GetRuntimeClassName( HSTRING *class_name ) noexcept
{
    TRACE( "iface %p, class_name %p\n", this, class_name );
    return WindowsCreateString( (LPCWSTR)L"Windows.Foundation.VectorView`1<IInspectable>", 48, class_name );
}

template<typename T>
HRESULT WINAPI
VectorView<T>::GetTrustLevel( TrustLevel *trust_level ) noexcept
{
    FIXME( "iface %p, trust_level %p stub!\n", this, trust_level );
    return E_NOTIMPL;
}

template<typename T>
HRESULT WINAPI
VectorView<T>::GetAt( UINT32 index, T *value )
{
    TRACE( "iface %p, index %u, value %p.\n", this, index, value );

    *value = nullptr;
    if ( index >= size) return E_BOUNDS;

    if constexpr ( std::is_base_of<IUnknown, T>() )
        elements[index]->AddRef();

    if constexpr ( std::is_same_v<T, HSTRING> )
        WindowsDuplicateString( elements[index], value );
    else
        *value = elements[index];

    return S_OK;
}

template<typename T>
HRESULT WINAPI
VectorView<T>::get_Size( UINT32 *value )
{
    TRACE( "iface %p, value %p.\n", this, value );

    *value = size;
    return S_OK;
}

template<typename T>
HRESULT WINAPI
VectorView<T>::IndexOf( T element, UINT32 *index, BOOLEAN *found )
{
    ULONG i;

    TRACE( "iface %p, element %p, index %p, found %p.\n", this, element, index, found );

    for ( i = 0; i < size; ++i ) if ( elements[i] == element ) break;
    if ( ( *found = ( i < size ) ) ) *index = i;
    else *index = 0;

    return S_OK;
}

template<typename T>
HRESULT WINAPI
VectorView<T>::GetMany( UINT32 start_index, UINT32 items_size, T *items, UINT *count )
{
    UINT32 i;

    TRACE( "iface %p, start_index %u, items_size %u, items %p, count %p.\n",
           this, start_index, items_size, items, count );

    if ( start_index >= size ) return E_BOUNDS;

    for ( i = start_index; i < size; ++i )
    {
        if ( i - start_index >= items_size ) break;
        if constexpr ( std::is_base_of<IUnknown, T>() )
            elements[i]->AddRef();
        if constexpr ( std::is_same_v<T, HSTRING> )
            WindowsDuplicateString( elements[i], &items[i - start_index] );
        else
            items[i - start_index] = elements[i];
    }

    *count = i - start_index;

    return S_OK;
}

template<typename T>
HRESULT WINAPI
VectorView<T>::First( IIterator<T> **value )
{
    TRACE( "iface %p, value %p.\n", this, value );

    *value = new Iterator( this, size );
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::QueryInterface( REFIID iid, void** out ) noexcept
{
    TRACE( "iface %p, iid %s, out %p.\n", this, debugstr_guid( &iid ), out );

    if (!out) return E_POINTER;
    *out = nullptr;

    if ( iid == __uuidof( IUnknown ) ||
         iid == __uuidof( IInspectable ) ||
         iid == __uuidof( IAgileObject ) ||
         iid == __uuidof( IVector<T> ) )
    {
        AddRef();
        *out = static_cast<IVector<T> *>(this);
        return S_OK;
    }

    if ( iid == __uuidof( IIterable<T> ) )
    {
        AddRef();
        *out = static_cast<IIterable<T> *>(this);
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( &iid ) );
    *out = nullptr;
    return E_NOINTERFACE;
}

template<typename T>
ULONG WINAPI
Vector<T>::AddRef() noexcept
{
    ULONG curr = static_cast<ULONG>(++ref);
    TRACE( "iface %p increasing refcount to %lu.\n", this, curr );
    return curr;
}

template<typename T>
ULONG WINAPI
Vector<T>::Release() noexcept
{
    ULONG curr = static_cast<ULONG>(--ref);
    TRACE( "iface %p decreasing refcount to %lu.\n", this, curr );

    if ( !curr )
    {
        Clear();
        delete this;
    }

    return curr;
}

template<typename T>
HRESULT WINAPI
Vector<T>::GetIids( ULONG *iid_count, IID **iids ) noexcept
{
    TRACE( "iface %p, iid_count %p, iids %p\n", this, iid_count, iids );

    if ( !iid_count || !iids )
        return E_POINTER;

    *iid_count = 2;
    IID* allocated = static_cast<IID*>( CoTaskMemAlloc( sizeof(IID) * (*iid_count) ) );

    if ( !allocated )
        return E_OUTOFMEMORY;

    allocated[0] = __uuidof( IVector<T> );
    allocated[1] = __uuidof( IIterable<T> );

    *iids = allocated;
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::GetRuntimeClassName( HSTRING *class_name ) noexcept
{
    TRACE( "iface %p, class_name %p\n", this, class_name );
    return WindowsCreateString( (LPCWSTR)L"Windows.Foundation.Vector`1<IInspectable>", 48, class_name );
}

template<typename T>
HRESULT WINAPI
Vector<T>::GetTrustLevel( TrustLevel *trust_level ) noexcept
{
    FIXME( "iface %p, trust_level %p stub!\n", this, trust_level );
    return E_NOTIMPL;
}

template<typename T>
HRESULT WINAPI
Vector<T>::GetAt( UINT32 index, T *value )
{
    TRACE( "iface %p, index %u, value %p.\n", this, index, value );

    *value = nullptr;
    if ( index >= size) return E_BOUNDS;

    if constexpr ( std::is_base_of<IUnknown, T>() )
        elements[index]->AddRef();
    if constexpr ( std::is_same_v<T, HSTRING> )
        WindowsDuplicateString( elements[index], value );
    else
        *value = elements[index];

    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::get_Size( UINT32 *value )
{
    TRACE( "iface %p, value %p.\n", this, value );

    *value = size;
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::GetView( IVectorView<T> **value )
{
    TRACE( "iface %p, value %p.\n", this, value );

    *value = new VectorView( elements, size );
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::IndexOf( T element, UINT32 *index, BOOLEAN *found )
{
    ULONG i;

    TRACE( "iface %p, element %p, index %p, found %p.\n", this, element, index, found );

    for ( i = 0; i < size; ++i ) if ( elements[i] == element ) break;
    if ( ( *found = ( i < size ) ) ) *index = i;
    else *index = 0;

    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::SetAt( UINT32 index, T value )
{
    TRACE( "iface %p, index %u, value %p.\n", this, index, value );

    if ( index >= size ) return E_BOUNDS;
    if constexpr ( std::is_base_of<IUnknown, T>() )
    {
        elements[index]->Release();
        value->AddRef();
    }
    if constexpr ( std::is_same_v<T, HSTRING> )
    {
        WindowsDeleteString( elements[index] );
        WindowsDuplicateString( value, &elements[index] );
    } else
        elements[index] = value;
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::InsertAt( UINT32 index, T value )
{
    T *tmp = elements;

    TRACE( "iface %p, index %u, value %p.\n", this, index, value );

    if ( size == capacity )
    {
        capacity = max( 32, capacity * 3 / 2 );
        if ( !( elements = (T *)realloc( elements, capacity * sizeof(*elements) ) ) )
        {
            elements = tmp;
            return E_OUTOFMEMORY;
        }
    }

    memmove( elements + index + 1, elements + index, (size++ - index) * sizeof(*elements) );
    if constexpr ( std::is_base_of<IUnknown, T>() )
        value->AddRef();
    if constexpr ( std::is_same_v<T, HSTRING> )
        WindowsDuplicateString( value, &elements[index] );
    else
        elements[index] = value;
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::RemoveAt( UINT32 index )
{
    TRACE( "iface %p, index %u.\n", this, index );

    if ( index >= size ) return E_BOUNDS;
    if constexpr ( std::is_base_of<IUnknown, T>() )
        elements[index]->Release();
    if constexpr ( std::is_same_v<T, HSTRING> )
        WindowsDeleteString( elements[index] );

    memmove( elements + index, elements + index + 1, (--size - index) * sizeof(*elements) );
    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::Append( T value )
{
    TRACE( "iface %p, value %p.\n", this, value );

    return InsertAt( size, value );
}

template<typename T>
HRESULT WINAPI
Vector<T>::RemoveAtEnd()
{
    TRACE( "iface %p.\n", this );

    if ( size )
    {
        size--;
        if constexpr ( std::is_base_of<IUnknown, T>() )
            elements[size]->Release();
        if constexpr ( std::is_same_v<T, HSTRING> )
            WindowsDeleteString( elements[size] );
    }

    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::Clear()
{
    TRACE( "iface %p.\n", this );

    while ( size )
        RemoveAtEnd();

    free( elements );
    capacity = 0;
    elements = nullptr;

    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::GetMany( UINT32 start_index, UINT32 items_size, T *items, UINT *count )
{
    UINT32 i;

    TRACE( "iface %p, start_index %u, items_size %u, items %p, count %p.\n",
           this, start_index, items_size, items, count );

    if ( start_index >= size ) return E_BOUNDS;

    for ( i = start_index; i < size; ++i )
    {
        if ( i - start_index >= items_size ) break;
        if constexpr ( std::is_base_of<IUnknown, T>() )
            elements[i]->AddRef();
        if constexpr ( std::is_same_v<T, HSTRING> )
            WindowsDuplicateString( elements[i], &items[i - start_index] );
        else
            items[i - start_index] = elements[i];
    }

    *count = i - start_index;

    return S_OK;
}

template<typename T>
HRESULT WINAPI
Vector<T>::ReplaceAll( UINT32 count, T *items )
{
    HRESULT hr;
    ULONG i;

    TRACE( "iface %p, count %u, items %p.\n", this, count, items );

    hr = Clear();
    for ( i = 0; i < count && SUCCEEDED( hr ); ++i )
        hr = Append( items[i] );

    return hr;
}

template<typename T>
HRESULT WINAPI
Vector<T>::First( IIterator<T> **value )
{
    HRESULT hr;
    IVectorView<T> *view;
    IIterable<T> *iterable;

    TRACE( "iface %p, value %p.\n", this, value );

    if ( FAILED( hr = GetView( &view ) ) ) return hr;

    hr = view->template QueryInterface<IIterable<T>>( &iterable );
    view->Release();
    if ( FAILED( hr ) ) return hr;

    hr = iterable->First( value );
    iterable->Release();

    return hr;
}

template<typename T>
HRESULT WINAPI
Vector<T>::Create( IVector<T> **out )
{
    TRACE( "out %p.\n", out );
    *out = new Vector();
    TRACE( "created %p\n", *out );
    return S_OK;
}

template class Iterator<HSTRING>;
template class VectorView<HSTRING>;
template class Vector<HSTRING>;