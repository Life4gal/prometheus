// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <mutex>
#include <unordered_map>
#include <string>
#include <filesystem>

#include <prometheus/macro.hpp>

#include <concurrency/thread.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)

#include <Windows.h>

#elif defined(GAL_PROMETHEUS_PLATFORM_LINUX)

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

// this_thread::get_id ==> syscall(SYS_gettid)
// #include <sys/syscall.h>

#elif defined(GAL_PROMETHEUS_PLATFORM_DARWIN)

#include <pthread.h>

#else

#error "Unsupported platform"

#endif

namespace
{
	using gal::prometheus::concurrency::thread_id;

	std::mutex thread_names_mutex;
	std::unordered_map<thread_id, std::string> thread_names;
}

namespace gal::prometheus::concurrency
{
	namespace this_process
	{
		[[nodiscard]] auto get_id() noexcept -> process_id
		{
			#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)

			// return GetCurrentProcessId();

			#if defined(_WIN64)
			// win64 => 0x40
			constexpr DWORD nt_thread_information_block_current_process_id = 0x40;
			return __readgsdword(nt_thread_information_block_current_process_id);
			#else
            // win32 => 0x20
            constexpr DWORD nt_thread_information_block_current_process_id = 0x20;
            return __readfsdword(nt_thread_information_block_current_process_id);
			#endif

			#elif defined(GAL_PROMETHEUS_PLATFORM_LINUX) or defined(GAL_PROMETHEUS_PLATFORM_DARWIN)

			return static_cast<process_id>(getpid());

			#else

            GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();

			#endif
		}
	}

	namespace this_thread
	{
		[[nodiscard]] auto get_id() noexcept -> thread_id
		{
			#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)

			// return GetCurrentThreadId();

			#if defined(_WIN64)
			// win64 => 0x48
			constexpr DWORD nt_thread_information_block_current_thread_id = 0x48;
			return __readgsdword(nt_thread_information_block_current_thread_id);
			#else
            // win32 => 0x24
            constexpr DWORD nt_thread_information_block_current_thread_id = 0x24;
            return __readfsdword(nt_thread_information_block_current_thread_id);
			#endif

			#elif defined(GAL_PROMETHEUS_PLATFORM_LINUX)

			return static_cast<thread_id>(pthread_self());

			#elif defined(GAL_PROMETHEUS_PLATFORM_DARWIN)

			uint64_t tid;
			pthread_threadid_np(nullptr, &tid);
			return static_cast<thread_id>(tid);

			#else

            GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();

			#endif
		}

		auto set_name(const std::string_view name) noexcept -> void
		{
			const auto id = get_id();

			#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)

			// fixme: UTF8 => UTF16
			const auto path = std::filesystem::path{name};
			const auto real_name = path.string();

			const auto result = SetThreadDescription(GetCurrentThread(), path.c_str());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(SUCCEEDED(result));

			#elif defined(GAL_PROMETHEUS_PLATFORM_LINUX)

			// fixme: truncated?
			const auto real_name = name;

			const auto result = pthread_setname_np(id, name.data());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result == 0);

			#elif defined(GAL_PROMETHEUS_PLATFORM_DARWIN)

			// fixme: truncated?
			const auto real_name = name;

			const auto result = pthread_setname_np(name.data());
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result == 0);

			#else

            GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();

			#endif

			const auto lock = std::scoped_lock{thread_names_mutex};

			if (const auto it = thread_names.find(id);
				it != thread_names.end())
			{
				it->second = real_name;
			}
			else
			{
				thread_names.emplace_hint(it, id, real_name);
			}
		}

		auto get_name(const thread_id id) noexcept -> std::optional<std::string_view>
		{
			const auto lock = std::scoped_lock{thread_names_mutex};

			if (const auto it = thread_names.find(id);
				it != thread_names.end())
			{
				return it->second;
			}

			return std::nullopt;
		}
	}
}
