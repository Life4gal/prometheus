// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>
#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <Windows.h>
#else

#endif

export module gal.prometheus.concurrency:thread.impl;

import std;
import gal.prometheus.error;

import :thread;

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
			#error "Architecture Not Supported"
			// win32 => 0x20
			constexpr DWORD nt_thread_information_block_current_process_id = 0x20;
			return __readfsdword(nt_thread_information_block_current_process_id);
			#endif

			#else
			GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();
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
			#error "Architecture Not Supported"
			constexpr DWORD nt_thread_information_block_current_thread_id = 0x24;
			return __readfsdword(nt_thread_information_block_current_thread_id);
			#endif

			#else
			GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();
			#endif
		}

		auto set_name(const std::string_view name) noexcept -> void
		{
			#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
			// fixme
			const auto n = std::filesystem::path{name};
			const auto result = SetThreadDescription(GetCurrentThread(), n.c_str());
			GAL_PROMETHEUS_DEBUG_ASSUME(SUCCEEDED(result));

			const auto lock = std::scoped_lock{thread_names_mutex};
			const auto id = get_id();
			if (const auto it = thread_names.find(id);
				it != thread_names.end()) { it->second = n.string(); }
			else { thread_names.emplace_hint(it, id, n.string()); }
			#else
			GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();
			#endif
		}

		auto get_name(const thread_id id) noexcept -> std::optional<std::string_view>
		{
			const auto lock = std::scoped_lock{thread_names_mutex};
			if (const auto it = thread_names.find(id); it != thread_names.end()) { return it->second; }

			return std::nullopt;
		}
	} // namespace this_thread
} // namespace gal::prometheus::concurrency
