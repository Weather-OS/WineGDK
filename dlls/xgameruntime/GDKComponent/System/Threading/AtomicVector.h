/*
 * AtomicVector Implementation
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

#ifndef __ATOMICVECTOR_H__
#define __ATOMICVECTOR_H__

#include "../../../private.h"

#include <mutex>
#include <atomic>
#include <vector>

template <class TElement>
class AtomicVector
{
public:

    template<typename TArg>
    HRESULT Add(_In_ TArg&& value) try
    {
        std::lock_guard<std::mutex> lock(m_lock);
        uint32_t bufferReadIdx = (m_indexAndRef & 0x80000000 ? 1 : 0);
        uint32_t bufferWriteIdx = 1 - bufferReadIdx;

        m_buffers[bufferWriteIdx] = m_buffers[bufferReadIdx];
        m_buffers[bufferWriteIdx].push_back(std::forward<TArg>(value));

        // Now spin wait to swap the active buffer.
        uint32_t expected = bufferReadIdx << 31;
        uint32_t desired = bufferWriteIdx << 31;

        for (;;)
        {
            uint32_t expectedSwap = expected;
            if (m_indexAndRef.compare_exchange_weak(expectedSwap, desired))
            {
                break;
            }
        }

        m_buffers[bufferReadIdx].clear();
        m_buffers[bufferReadIdx].shrink_to_fit();

        return S_OK;
    } catch (...)
    {
        // tracing not supported.
        return E_FAIL;
    }

    template <typename Func>
    void Remove(_In_ Func predicate)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        uint32_t bufferReadIdx = (m_indexAndRef & 0x80000000 ? 1 : 0);
        uint32_t bufferWriteIdx = 1 - bufferReadIdx;

        m_buffers[bufferWriteIdx] = m_buffers[bufferReadIdx];

        auto &buffer = m_buffers[bufferWriteIdx];
        auto it = std::find_if(buffer.begin(), buffer.end(), predicate);

        if (it != buffer.end())
        {
            buffer.erase(it);
        }

        uint32_t expected = bufferReadIdx << 31;
        uint32_t desired = bufferWriteIdx << 31;

        for (;;)
        {
            uint32_t expectedSwap = expected;
            if (m_indexAndRef.compare_exchange_weak(expectedSwap, desired))
            {
                break;
            }
        }

        m_buffers[bufferReadIdx].clear();
        m_buffers[bufferReadIdx].shrink_to_fit();
    }

    template <typename Func>
    void Visit(_In_ Func callback)
    {
        std::lock_guard<std::mutex> lock(m_lock);
        uint32_t indexAndRef = ++m_indexAndRef;
        uint32_t bufferIdx = (indexAndRef & 0x80000000 ? 1 : 0);

        for (auto const &it : m_buffers[bufferIdx])
        {
            callback(it);
        }

        m_indexAndRef--;
    }

private:

    std::mutex m_lock;
    std::vector<TElement> m_buffers[2];
    std::atomic<uint32_t> m_indexAndRef { 0 };
};

#endif