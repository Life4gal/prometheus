// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.concurrency:thread;

import std;

export namespace gal::prometheus::concurrency
{
	using thread_id = std::uint32_t;
	using process_id = std::uint32_t;

	constexpr thread_id invalid_thread_id = 0;

	namespace this_process
	{
		[[nodiscard]] auto get_id() noexcept -> process_id;
	}

	namespace this_thread
	{
		[[nodiscard]] auto get_id() noexcept -> thread_id;

		auto set_name(std::string_view name) noexcept -> void;

		auto get_name(thread_id id) noexcept -> std::optional<std::string_view>;
	}
}
