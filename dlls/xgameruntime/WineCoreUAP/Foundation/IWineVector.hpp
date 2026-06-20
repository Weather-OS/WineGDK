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

#include "../../private.h"
#include "provider.h"

#include <atomic>
#include <mutex>

#ifndef IWINEVECTOR_HPP
#define IWINEVECTOR_HPP

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;

template<typename T>
class Iterator final :
    public IIterator<T>
{
public:
    Iterator() = default;
    virtual ~Iterator() = default;

    Iterator( IVectorView<T> *view, UINT32 size ) noexcept;

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

    /* IIterator<T> Methods */
    HRESULT WINAPI
    get_Current( T *value ) override;

    HRESULT WINAPI
    get_HasCurrent( boolean *value ) override;

    HRESULT WINAPI
    MoveNext( boolean *value ) override;

    HRESULT WINAPI
    GetMany( UINT32 items_size, T *items, UINT *count ) override;

private:
    IVectorView<T> *view;
    UINT32 index{ 0 };
    UINT32 size;
    std::atomic_long ref{ 1 };
};

template<typename T>
class VectorView final :
    public IVectorView<T>,
    public IIterable<T>
{
public:
    VectorView() = default;
    virtual ~VectorView() = default;

    VectorView( T* elems, UINT32 size ) noexcept;

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

    /* IVectorView<T> Method */
    HRESULT WINAPI
    GetAt( UINT32 index, T *value ) override;

    HRESULT WINAPI
    get_Size( UINT32 *value ) override;

    HRESULT WINAPI
    IndexOf( T element, UINT32 *index, BOOLEAN *found ) override;

    HRESULT WINAPI
    GetMany( UINT32 start_index, UINT32 items_size, T *items, UINT *count ) override;

    /* IIterable<T> Method */
    HRESULT WINAPI
    First( IIterator<T> **value ) override;

private:
    T *elements;
    UINT32 size;
    std::atomic_long ref{ 1 };
};

template<typename T>
class Vector final :
    public IVector<T>,
    public IIterable<T>
{
public:
    Vector() = default;
    virtual ~Vector() = default;

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

    /* Vector<T> Methods */
    HRESULT WINAPI
    GetAt( UINT32 index, T *value ) override;

    HRESULT WINAPI
    get_Size( UINT32 *value ) override;

    HRESULT WINAPI
    GetView( IVectorView<T> **value ) override;

    HRESULT WINAPI
    IndexOf( T element, UINT32 *index, BOOLEAN *found ) override;

    HRESULT WINAPI
    SetAt( UINT32 index, T value ) override;

    HRESULT WINAPI
    InsertAt( UINT32 index, T value ) override;

    HRESULT WINAPI
    RemoveAt( UINT32 index ) override;

    HRESULT WINAPI
    Append( T value ) override;

    HRESULT WINAPI
    RemoveAtEnd() override;

    HRESULT WINAPI
    Clear() override;

    HRESULT WINAPI
    GetMany( UINT32 start_index, UINT32 items_size, T *items, UINT *count ) override;

    HRESULT WINAPI
    ReplaceAll( UINT32 count, T *items ) override;

    /* IIterable<T> Method */
    HRESULT WINAPI
    First( IIterator<T> **value ) override;

    /* Internal methods */
    static HRESULT WINAPI
    Create( IVector<T> **out );

private:
    UINT32 capacity;
    UINT32 size;
    T *elements;
    std::atomic_long ref{ 1 };
};

#endif