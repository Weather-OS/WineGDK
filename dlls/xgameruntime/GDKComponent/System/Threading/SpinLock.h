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

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "../../../private.h"

#include <algorithm>
#include <atomic>
#include <thread>

//
// SpinLock: A spinlock implementation based on std::atomic_flag that
// prevents CPU starvation. SpinLock can be used as a RAII wrapper around
// an external flag, or its static Lock API may be used to lock an
// external flag.
//
class SpinLock
{
public:
    SpinLock(_In_ std::atomic_flag& flag) : m_lock(flag)
    {
        Lock(m_lock);
    }

    ~SpinLock()
    {
        m_lock.clear(std::memory_order_release);
    }

    static void Lock(_In_ std::atomic_flag& flag)
    {
        unsigned int backoff = 1;
        constexpr unsigned int maxBackoff = 1024;

        while (flag.test_and_set(std::memory_order_acquire)) {
            for (unsigned int i = 0; i < backoff; ++i) {
                cpu_pause();
            }
            
            // Exponential backoff with cap. If we are over the cap yield
            // this thread.

            backoff = backoff << 1;
            if (backoff >= maxBackoff)
            {
                backoff = maxBackoff;
                std::this_thread::yield();
            }
        }
    }

private:
    std::atomic_flag& m_lock;

    static inline void cpu_pause()
    {
        YieldProcessor();
    }
};

#endif