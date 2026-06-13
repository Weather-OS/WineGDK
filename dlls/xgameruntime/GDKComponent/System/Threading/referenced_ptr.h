/*
 * SpinLock Implementation
 *  From https://github.com/microsoft/libHttpClient
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

#ifndef __REFERENCED_PTR_H__
#define __REFERENCED_PTR_H__

#include "../../../private.h"

template <typename TInterface>
struct referenced_ptr
{
    referenced_ptr() noexcept
        : _ptr(nullptr)
    {}

    explicit referenced_ptr(TInterface* ptr) noexcept
        : _ptr(ptr)
    {
        add_ref();
    }

    referenced_ptr(const referenced_ptr& other) noexcept
        : _ptr(other.get())
    {
        add_ref();
    }

    referenced_ptr(referenced_ptr&& other) noexcept
        : _ptr(other.release())
    {}

    referenced_ptr& operator=(const referenced_ptr& other) noexcept
    {
        if (this == &other) { return *this; }

        reset();
        _ptr = other.get();
        add_ref();

        return *this;
    }

    referenced_ptr& operator=(referenced_ptr&& other) noexcept
    {
        if (this == &other) { return *this; }

        reset();
        _ptr = other.release();

        return *this;
    }

    ~referenced_ptr() noexcept
    {
        reset();
    }
    
    TInterface* get() const noexcept
    {
        return _ptr;
    }
    
    TInterface* release() noexcept
    {
        TInterface* ptr = _ptr;
        _ptr = nullptr;
        return ptr;
    }

    void reset() noexcept
    {
        if (_ptr) _ptr->Release();
        _ptr = nullptr;
    }

    explicit operator bool() const noexcept
    {
        return _ptr != nullptr;
    }

    TInterface* operator->() const noexcept
    {
        return _ptr;
    }

    TInterface** address_of() noexcept
    {
        reset();
        return &_ptr;
    }

private:
    void add_ref() noexcept
    {
        if (_ptr) _ptr->AddRef();
    }

    TInterface* _ptr = nullptr;
};

template <typename TInterface>
bool operator==(referenced_ptr<TInterface>& p, nullptr_t)
{
    return p.get() == nullptr;
}

template <typename TInterface>
bool operator!=(referenced_ptr<TInterface>& p, nullptr_t)
{
    return p.get() != nullptr;
}

#endif