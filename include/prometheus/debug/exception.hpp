// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <atomic>
#include <prometheus/macro.hpp>
#include <stdexcept>

namespace gal::prometheus::debug
{
	inline std::atomic<const char*> terminate_reason{nullptr};

	[[nodiscard]] auto get_last_error_message() noexcept -> std::string;

	// fixme: Move this macro to macro.hpp and use `module`.
	#define GAL_PROMETHEUS_DEBUG_SET_TERMINATE_REASON(...) \
	::gal::prometheus::debug::terminate_reason.store("[" __FILE__ ":" GAL_PROMETHEUS_TO_STRING(__LINE__) "] -> " __VA_ARGS__, std::memory_order::relaxed)

	// fixme: Move this macro to macro.hpp and use `module`.
	#define GAL_PROMETHEUS_DEBUG_ASSERT(expression, ...)                                                                   \
	do {                                                                                               \
		if (not static_cast<bool>(expression))                                                         \
		{                                                                                              \
			GAL_PROMETHEUS_DEBUG_SET_TERMINATE_REASON("[ASSERT FAILED]: \"" __VA_ARGS__ "\" --> {" GAL_PROMETHEUS_TO_STRING(expression) "}"); \
			GAL_PROMETHEUS_DEBUG_TRAP(); \
	}                                                                                              \
	} while (false)

	// fixme: Move this macro to macro.hpp and use `module`.
	#define GAL_PROMETHEUS_DEBUG_ASSUME(expression, ...) GAL_PROMETHEUS_DEBUG_ASSERT(expression __VA_OPT__(, ) __VA_ARGS__)

	// fixme: Move this macro to macro.hpp and use `module`.
	#define GAL_PROMETHEUS_DEBUG_NOT_NULL(pointer, ...) \
	do {                                                                                              \
		if (pointer == nullptr)                                                                       \
		{                                                                                             \
			GAL_PROMETHEUS_DEBUG_SET_TERMINATE_REASON("[NOT-NULL FAILED]: \"" __VA_ARGS__ "\" --> {" GAL_PROMETHEUS_TO_STRING(pointer) "}"); \
			GAL_PROMETHEUS_DEBUG_TRAP(); \
	}                                                                                             \
	} while (false)

	// fixme: Move this macro to macro.hpp and use `module`.
	#define GAL_PROMETHEUS_DEBUG_NOT_IMPLEMENTED(...) \
		GAL_PROMETHEUS_DEBUG_SET_TERMINATE_REASON("[NOT IMPLEMENTED]: \"" __VA_ARGS__ "\""); \
		GAL_PROMETHEUS_DEBUG_TRAP()

	class RuntimeError : public std::runtime_error// NOLINT
	{
	public:
		using runtime_error::runtime_error;
	};
}// namespace gal::prometheus::debug
