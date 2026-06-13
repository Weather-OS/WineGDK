/*
 * ThreadPool Implementation
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

#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "../../../private.h"

namespace OS
{
    struct ThreadPoolActionStatus
    {
        virtual void Complete() = 0;
        virtual void MayRunLong() = 0;
    };

    using ThreadPoolCallback = void(_In_opt_ void*, _In_ ThreadPoolActionStatus& status);

    class ThreadPoolImpl;

    // A thread pool will invoke its callback on a pool of threads.
    class ThreadPool
    {
    public:
        ThreadPool() noexcept;
        ~ThreadPool() noexcept;

        // Initializes the thread pool.
        HRESULT Initialize(_In_opt_ void* context, _In_ ThreadPoolCallback* callback) noexcept;

        // Terminates the thread pool, waiting for any outstanding calls to drain
        // and and canceling any pending calls.
        void Terminate() noexcept;

        // Submits a new callback to the thread pool.  The callback passed to Initialize will
        // be invoked on a thread pool thread. May throw / crash if called after termination
        // or before init.
        void Submit();

    private:
        ThreadPoolImpl* m_impl;
    };
}

#endif