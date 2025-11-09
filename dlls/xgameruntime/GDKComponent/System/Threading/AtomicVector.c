/*
 * Atomic Vector Implementation
 *  From https://github.com/microsoft/libHttpClient
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

#include "AtomicVector.h"

WINE_DEFAULT_DEBUG_CHANNEL(xtaskqueue);

static inline struct atomic_vector *impl_from_IAtomicVector( IAtomicVector *iface )
{
    return CONTAINING_RECORD( iface, struct atomic_vector, IAtomicVector_iface );
}

static HRESULT WINAPI atomic_vector_QueryInterface( IAtomicVector *iface, REFIID iid, void **out )
{
    struct atomic_vector *impl = impl_from_IAtomicVector( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAtomicVector ))
    {
        *out = &impl->IAtomicVector_iface;
        impl->IAtomicVector_iface.lpVtbl->AddRef( *out );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI atomic_vector_AddRef( IAtomicVector *iface )
{
    struct atomic_vector *impl = impl_from_IAtomicVector( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI atomic_vector_Release( IAtomicVector *iface )
{
    struct atomic_vector *impl = impl_from_IAtomicVector( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    if ( ref == 0 ) {
        if ( impl->items ) 
        {
            free( impl->items );
            impl->items = NULL;
        }
        free( impl );
        return 0;
    }
    return ref;
}

static HRESULT STDMETHODCALLTYPE atomic_vector_Add( IAtomicVector* iface, PVOID value )
{
    SIZE_T newCapacity;
    PVOID *newArray;

    struct atomic_vector *impl = impl_from_IAtomicVector( iface );

    TRACE( "iface %p, value %p.\n", iface, value );

    AcquireSRWLockExclusive( &impl->lock );

    if ( impl->size == impl->capacity ) 
    {
        newCapacity = impl->capacity == 0 ? 8 : impl->capacity * 2;
        newArray = (PVOID *)realloc( impl->items, newCapacity * sizeof( PVOID ) );

        if ( !newArray ) 
        {
            ReleaseSRWLockExclusive(&impl->lock);
            return E_OUTOFMEMORY;
        }

        impl->items = newArray;
        impl->capacity = newCapacity;
    }

    impl->items[impl->size++] = value;

    ReleaseSRWLockExclusive( &impl->lock );

    return S_OK;
}

static VOID STDMETHODCALLTYPE atomic_vector_Remove( IAtomicVector* iface, AtomicVector_Predicate predicate, PVOID predicateContext )
{
    SIZE_T write = 0;
    SIZE_T read;
    PVOID element;
    BOOLEAN shouldRemove;

    struct atomic_vector *impl = impl_from_IAtomicVector( iface );

    TRACE( "iface %p, predicate %p, predicateContext %p.\n", iface, predicate, predicateContext );

    AcquireSRWLockExclusive( &impl->lock );

    for ( read = 0; read < impl->size; ++read ) 
    {
        element = impl->items[read];
        shouldRemove = predicate( element, predicateContext );

        if ( !shouldRemove ) 
        {
            if ( write != read ) 
                impl->items[write] = impl->items[read];
            write++;
        } else 
        {
            // For now, do nothing.
        }
    }
    impl->size = write;

    ReleaseSRWLockExclusive( &impl->lock );
}

static VOID STDMETHODCALLTYPE atomic_vector_Visit( IAtomicVector* iface, AtomicVector_Visitor visitor ) 
{
    SIZE_T elementIterator;
    PVOID element;
    struct atomic_vector *impl = impl_from_IAtomicVector( iface );

    TRACE( "iface %p, visitor %p.\n", iface, visitor );

    AcquireSRWLockShared( &impl->lock );

    for ( elementIterator = 0; elementIterator < impl->size; elementIterator++ ) 
    {
        element = impl->items[elementIterator];
        /* call visitor with element pointer as ctx */
        visitor( element );
    }

    ReleaseSRWLockShared( &impl->lock );
}

static const struct IAtomicVectorVtbl atomic_vector_vtbl =
{
    /* IUnknown methods */
    atomic_vector_QueryInterface,
    atomic_vector_AddRef,
    atomic_vector_Release,
    /* IAtomicVector methods */
    atomic_vector_Add,
    atomic_vector_Remove,
    atomic_vector_Visit
};

HRESULT CreateAtomicVector( IAtomicVector **out )
{
    struct atomic_vector *impl;

    TRACE( "out %p.\n", out );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IAtomicVector_iface.lpVtbl = &atomic_vector_vtbl;
    impl->ref = 1;
    InitializeSRWLock( &impl->lock );
    
    *out = &impl->IAtomicVector_iface;
    return S_OK;
}