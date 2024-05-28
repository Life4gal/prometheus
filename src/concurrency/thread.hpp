// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>
#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
		#include <prometheus/windows.hpp>
#endif

export module gal.prometheus.concurrency:thread;

import std;
import gal.prometheus.error;

#else
#include <filesystem>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <array>
#include <ranges>
#include <algorithm>

#include <error/error.hpp>
#include <prometheus/macro.hpp>
#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
#include <prometheus/windows.hpp>
#endif
#endif

namespace gal::prometheus::concurrency
{
	using thread_id = std::uint32_t;
	using process_id = std::uint32_t;

	namespace thread_detail
	{
		inline std::mutex thread_names_mutex;
		inline std::unordered_map<thread_id, std::string> thread_names;

		using affinity_mask_type = std::array<bool, 64>;

		#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
		inline auto mask_p_to_array(const DWORD_PTR p) noexcept -> affinity_mask_type
		{
			static_assert(sizeof(std::declval<affinity_mask_type>().max_size()) >= sizeof(DWORD_PTR));

			affinity_mask_type result;

			std::ranges::for_each(
					std::views::iota(0ull, sizeof(DWORD_PTR) * std::numeric_limits<unsigned char>::digits),
					[&result, p](const auto i) noexcept { result[i] = p & (static_cast<DWORD_PTR>(1) << i); }
					);

			return result;
		}

		inline auto mask_array_to_p(const affinity_mask_type& array) noexcept -> DWORD_PTR
		{
			static_assert(sizeof(std::declval<affinity_mask_type>().max_size()) >= sizeof(DWORD_PTR));

			DWORD_PTR result;

			std::ranges::for_each(
					std::views::iota(0ull, sizeof(DWORD_PTR) * std::numeric_limits<unsigned char>::digits),
					[&result, &array](const auto i) noexcept { if (array[i]) { result |= (static_cast<DWORD_PTR>(1) << i); } }
					);

			return result;
		}
		#endif
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	constexpr thread_id invalid_thread_id = 0;

	namespace this_process
	{
		[[nodiscard]] inline auto get_id() noexcept -> process_id
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

		[[nodiscard]] inline auto get_affinity_mask() noexcept -> thread_detail::affinity_mask_type
		{
			#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
			DWORD_PTR process;
			DWORD_PTR system;
			if (not GetProcessAffinityMask(GetCurrentProcess(), &process, &system)) { error::OsError::panic(); }

			return thread_detail::mask_p_to_array(process);
			#else
			GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();
			#endif
		}
	} // namespace this_process

	namespace this_thread
	{
		[[nodiscard]] inline auto get_id() noexcept -> thread_id
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

		inline auto set_name(const std::string_view name) noexcept -> void
		{
			#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
			// fixme
			const auto n = std::filesystem::path{name};
			const auto result = SetThreadDescription(GetCurrentThread(), n.c_str());
			GAL_PROMETHEUS_DEBUG_ASSUME(SUCCEEDED(result));

			const auto lock = std::scoped_lock{thread_detail::thread_names_mutex};
			const auto id = get_id();
			if (const auto it = thread_detail::thread_names.find(id);
				it != thread_detail::thread_names.end()) { it->second = n.string(); }
			else { thread_detail::thread_names.emplace_hint(it, id, n.string()); }
			#else
			GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();
			#endif
		}

		inline auto get_name(const thread_id id) noexcept -> std::optional<std::string_view>
		{
			const auto lock = std::scoped_lock{thread_detail::thread_names_mutex};
			if (const auto it = thread_detail::thread_names.find(id);
				it != thread_detail::thread_names.end()) { return it->second; }

			return std::nullopt;
		}

		inline auto set_affinity_mask(const thread_detail::affinity_mask_type& mask) noexcept -> thread_detail::affinity_mask_type
		{
			#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
			const auto process = thread_detail::mask_array_to_p(mask);

			const auto old = SetThreadAffinityMask(GetCurrentThread(), process);
			if (old == 0) { error::OsError::panic(); }

			return thread_detail::mask_p_to_array(old);
			#else
			GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED();
			#endif
		}
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
