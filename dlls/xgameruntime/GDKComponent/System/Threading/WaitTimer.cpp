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

#include "WaitTimer.h"

#include <memory>
#include <chrono>

using Clock = std::chrono::steady_clock;
using Deadline = Clock::time_point;
using TimerDuration = std::chrono::nanoseconds;

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

namespace
{
    Deadline DeadlineFromDueTime(uint64_t dueTime) noexcept
    {
        return Deadline(std::chrono::duration_cast<Clock::duration>(TimerDuration(dueTime)));
    }

    uint64_t DueTimeFromDeadline(Deadline deadline) noexcept
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<TimerDuration>(deadline.time_since_epoch()).count());
    }
}

namespace OS
{
    class WaitTimerImpl
    {
    public:
        WaitTimerImpl()
            : m_timer(nullptr)
        {}

        ~WaitTimerImpl()
        {
            Terminate();
        }

        HRESULT Initialize(_In_opt_ void* context, _In_ WaitTimerCallback* callback)
        {
            m_context = context;
            m_callback = callback;

            m_timer = CreateThreadpoolTimer(WaitCallback, this, nullptr);
            RETURN_LAST_ERROR_IF_NULL(m_timer);

            return S_OK;
        }

        void Terminate()
        {
            if (m_timer != nullptr)
            {
                SetThreadpoolTimer(m_timer, nullptr, 0, 0);
                WaitForThreadpoolTimerCallbacks(m_timer, TRUE);
                CloseThreadpoolTimer(m_timer);
                m_timer = nullptr;
            }
        }

        void Start(_In_ uint64_t dueTime)
        {
            LARGE_INTEGER li;
            FILETIME ft;

            Deadline now = Clock::now();
            Deadline dueDeadline = DeadlineFromDueTime(dueTime);
            TimerDuration remaining = dueDeadline > now
                ? std::chrono::duration_cast<TimerDuration>(dueDeadline - now)
                : TimerDuration::zero();

            constexpr int64_t nanosPerTick = 100;
            int64_t relativeTicks =
                (static_cast<int64_t>(remaining.count()) + nanosPerTick - 1) /
                nanosPerTick;

            if (relativeTicks <= 0)
            {
                relativeTicks = 1;
            }

            li.QuadPart = -relativeTicks;
            ft.dwHighDateTime = li.HighPart;
            ft.dwLowDateTime = li.LowPart;

            SetThreadpoolTimer(m_timer, &ft, 0, 0);
        }

        void Cancel()
        {
            SetThreadpoolTimer(m_timer, nullptr, 0, 0);
        }

    private:

        PTP_TIMER m_timer;
        void* m_context;
        WaitTimerCallback* m_callback;

        static VOID CALLBACK WaitCallback(
            _Inout_ PTP_CALLBACK_INSTANCE,
            _Inout_opt_ void* context,
            _Inout_ PTP_TIMER)
        {
            if (context != nullptr)
            {
                WaitTimerImpl* pthis = static_cast<WaitTimerImpl*>(context);
                pthis->m_callback(pthis->m_context);
            }
        }
    };

    WaitTimer::WaitTimer() noexcept
        : m_impl(nullptr)
    {}

    WaitTimer::~WaitTimer() noexcept
    {
        Terminate();
    }

    HRESULT WaitTimer::Initialize(_In_opt_ void* context, _In_ WaitTimerCallback* callback) noexcept
    {
        if (m_impl.load() != nullptr || callback == nullptr)
        {
            assert(FALSE);
            return E_UNEXPECTED;
        }

        std::unique_ptr<WaitTimerImpl> timer(new (std::nothrow) WaitTimerImpl);
        RETURN_IF_NULL_ALLOC(timer.get());
        RETURN_IF_FAILED(timer->Initialize(context, callback));

        m_impl = timer.release();

        return S_OK;
    }

    void WaitTimer::Terminate() noexcept
    {
        std::unique_ptr<WaitTimerImpl> timer(m_impl.exchange(nullptr));
        if (timer != nullptr)
        {
            timer->Terminate();
        }
    }

    void WaitTimer::Start(_In_ uint64_t dueTime) noexcept
    {
        m_impl.load()->Start(dueTime);
    }

    void WaitTimer::Cancel() noexcept
    {
        m_impl.load()->Cancel();
    }

    uint64_t WaitTimer::GetCurrentTime() noexcept
    {
        return DueTimeFromDeadline(Clock::now());
    }

    uint64_t WaitTimer::GetDueTime(_In_ uint32_t msFromNow) noexcept
    {
        Deadline deadline = Clock::now() + std::chrono::milliseconds(msFromNow);
        return DueTimeFromDeadline(deadline);
    }
} // Namespace
