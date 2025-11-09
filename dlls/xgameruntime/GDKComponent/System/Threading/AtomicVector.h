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

#ifndef ATOMIC_VECTOR_H
#define ATOMIC_VECTOR_H

#include "../../../private.h"

typedef BOOLEAN CALLBACK (*AtomicVector_Predicate)( const void *element, void *ctx );
typedef void CALLBACK (*AtomicVector_Visitor)(void *ctx);

typedef struct IAtomicVector IAtomicVector;

typedef struct IAtomicVectorVtbl {
    /* IUnknown methods */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IAtomicVector* This, 
        REFIID riid, 
        void** ppvObject);
        
    ULONG   (STDMETHODCALLTYPE *AddRef)(
        IAtomicVector* This);

    ULONG   (STDMETHODCALLTYPE *Release)(
        IAtomicVector* This);

    /* IAtomicVector Methods */
    HRESULT (STDMETHODCALLTYPE *Add)(
        IAtomicVector* This,
        PVOID value);

    VOID    (STDMETHODCALLTYPE *Remove)(
        IAtomicVector* This,
        AtomicVector_Predicate predicate,
        PVOID predicateContext);

    VOID    (STDMETHODCALLTYPE *Visit)(
        IAtomicVector* This,
        AtomicVector_Visitor visitor);

} IAtomicVectorVtbl;

struct IAtomicVector {
    const IAtomicVectorVtbl* lpVtbl;
};

struct atomic_vector
{
    IAtomicVector IAtomicVector_iface;
    SRWLOCK lock;
    PVOID *items;
    SIZE_T size, capacity;
    LONG ref;
};

HRESULT CreateAtomicVector( IAtomicVector **out );

#endif