/*
 * WaitTimer Implementation
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

#ifndef __WAITTIMER_H__
#define __WAITTIMER_H__

#include "../../../private.h"

#include <atomic>

namespace OS
{
    using WaitTimerCallback = void(_In_opt_ void*);

    class WaitTimerImpl;

    // A wait timer holds a single timeout expressed as a monotonic due time.
    // Calling Start will reset any pending timeout.
    class WaitTimer
    {
    public:
        WaitTimer() noexcept;
        ~WaitTimer() noexcept;

        HRESULT Initialize(_In_opt_ void* context, _In_ WaitTimerCallback* callback) noexcept;
        void Terminate() noexcept;

        // Arms the one-shot timer for the provided monotonic due time.
        void Start(_In_ uint64_t dueTime) noexcept;
        void Cancel() noexcept;

        // Returns the current monotonic time used for delayed-callback
        // ordering and stale-timer validation.
        uint64_t GetCurrentTime() noexcept;

        // Returns a monotonic due time msFromNow milliseconds in the future.
        uint64_t GetDueTime(_In_ uint32_t msFromNow) noexcept;

    private:
        std::atomic<WaitTimerImpl*> m_impl;
    };
}


#endif