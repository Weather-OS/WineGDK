/*
 * StaticArray Implementation
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

#ifndef _STATICARRAY_H_
#define _STATICARRAY_H_

#include "../../../private.h"

template <typename TData, uint32_t size>
class StaticArray
{
public:

    StaticArray()
    {
    }

    StaticArray(const StaticArray& other)
    {
        m_count = other.m_count;
        for(uint32_t idx = 0; idx < m_count; idx++)
        {
            m_array[idx] = other.m_array[idx];
        }
    }

    StaticArray& operator=(const StaticArray& other)
    {
        m_count = other.m_count;
        for(uint32_t idx = 0; idx < m_count; idx++)
        {
            m_array[idx] = other.m_array[idx];
        }
        return *this;
    }

    uint32_t count() { return m_count; }
    uint32_t capacity() { return ARRAYSIZE(m_array) - m_count; }
    void clear() { m_count = 0; }
    TData* data() { return m_array; }
    TData& operator[](size_t idx) { return m_array[idx]; }

    void append(const TData& data)
    {
        assert(m_count != ARRAYSIZE(m_array));
        m_array[m_count++] = data;
    }

    void removeAt(uint32_t idx)
    {
        if (idx == m_count - 1)
        {
            m_count --;
        }
        else
        {
            for (uint32_t i = idx + 1; i < m_count; i++)
            {
                m_array[i - 1] = m_array[i];
            }
            m_count--;
        }
    }

private:

    uint32_t m_count = 0;
    TData m_array[size];

};

#endif