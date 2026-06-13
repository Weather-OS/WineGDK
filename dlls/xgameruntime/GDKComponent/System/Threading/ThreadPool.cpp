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

#include "ThreadPool.h"
//FIXME: CallbackMayRunLong is implemented in code, but not in headers!
#include <winternl.h>

#include <memory>
#include <atomic>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

namespace OS
{
    // NOTE: ShutdownInProgress may not be necessary here.
    static inline BOOLEAN __stdcall RtlDllShutdownInProgress() noexcept
    {
        static decltype(RtlDllShutdownInProgress)* s_pfnRtlDllShutdownInProgress = nullptr;
        static HMODULE s_ntdllModuleHandle = nullptr;

        if (s_pfnRtlDllShutdownInProgress == nullptr)
        {
            if (s_ntdllModuleHandle == nullptr)
            {
                s_ntdllModuleHandle = GetModuleHandleA("ntdll.dll");
            }

            if (s_ntdllModuleHandle != nullptr)
            {
                s_pfnRtlDllShutdownInProgress = reinterpret_cast<decltype(RtlDllShutdownInProgress)*>(GetProcAddress(s_ntdllModuleHandle, "RtlDllShutdownInProgress"));
            }
        }

        return s_pfnRtlDllShutdownInProgress ? s_pfnRtlDllShutdownInProgress() : FALSE;
    }

    class ThreadPoolImpl
    {
    public:

        ~ThreadPoolImpl() noexcept
        {
            Terminate();
        }

        void AddRef()
        {
            m_refs++;
        }

        void Release()
        {
            if (--m_refs == 0)
            {
                delete this;
            }
        }

        HRESULT Initialize(
            _In_opt_ void* context,
            _In_ ThreadPoolCallback* callback) noexcept
        {
            m_context = context;
            m_callback = callback;
            m_work = CreateThreadpoolWork(TPCallback, this, nullptr);
            RETURN_LAST_ERROR_IF_NULL(m_work);

            return S_OK;
        }

        void Terminate() noexcept
        {
            if (m_work != nullptr)
            {
                WaitForThreadpoolWorkCallbacks(m_work, FALSE);
                CloseThreadpoolWork(m_work);
                m_work = nullptr;
            }
        }

        void Submit() noexcept
        {
            if (!RtlDllShutdownInProgress())
            {
                SubmitThreadpoolWork(m_work);
            }
        }

    private:

        static void CALLBACK TPCallback(
            _In_ PTP_CALLBACK_INSTANCE instance,
            _In_ void* context, PTP_WORK) noexcept
        {
            ThreadPoolImpl* pthis = static_cast<ThreadPoolImpl*>(context);
            ActionStatusImpl status(pthis, instance);
            pthis->AddRef();
            pthis->m_callback(pthis->m_context, status);

            if (!status.IsComplete)
            {
                status.Complete();
            }
            pthis->Release(); // May delete this
        }

        struct ActionStatusImpl : ThreadPoolActionStatus
        {
            ActionStatusImpl(ThreadPoolImpl* owner, PTP_CALLBACK_INSTANCE instance) :
                m_owner(owner),
                m_instance(instance)
            {
            }

            bool IsComplete = false;

            void Complete() override
            {
                IsComplete = true;
                DisassociateCurrentThreadFromCallback(m_instance);
            }

            void MayRunLong() override
            {
                if (!m_longRunning)
                {
                    m_longRunning = true;
                    //FIXME: CallbackMayRunLong is implemented in code, but not in headers!
                    TpCallbackMayRunLong(m_instance);
                }
            }

        private:
            ThreadPoolImpl* m_owner = nullptr;
            PTP_CALLBACK_INSTANCE m_instance = nullptr;
            bool m_longRunning = false;
        };

        std::atomic<uint32_t> m_refs{ 1 };
        PTP_WORK m_work = nullptr;
        void* m_context = nullptr;
        ThreadPoolCallback* m_callback = nullptr;
    };

    ThreadPool::ThreadPool() noexcept :
        m_impl(nullptr)
    {
    }

    ThreadPool::~ThreadPool() noexcept
    {
        Terminate();
    }

    HRESULT ThreadPool::Initialize(_In_opt_ void* context, _In_ ThreadPoolCallback* callback) noexcept
    {
        RETURN_HR_IF(E_UNEXPECTED, m_impl != nullptr);

        std::unique_ptr<ThreadPoolImpl> impl(new (std::nothrow) ThreadPoolImpl);
        RETURN_IF_NULL_ALLOC(impl);

        RETURN_IF_FAILED(impl->Initialize(context, callback));

        m_impl = impl.release();
        return S_OK;
    }

    void ThreadPool::Terminate() noexcept
    {
        if (m_impl != nullptr)
        {
            m_impl->Terminate();
            m_impl->Release();
            m_impl = nullptr;
        }
    }

    void ThreadPool::Submit()
    {
        m_impl->Submit();
    }

} // Namespace