// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <draw/context.hpp>
#include <utility>

namespace {}

namespace gal::prometheus::draw
{
	Context::Context() noexcept
		: current_draw_list_shared_data_{std::addressof(default_draw_list_shared_data_)},
		  default_font_{nullptr},
		  current_font_{nullptr} {}

	auto Context::instance() noexcept -> Context&
	{
		static Context context{};
		return context;
	}

	auto Context::draw_list_shared_data() const noexcept -> const DrawListSharedData&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_draw_list_shared_data_ != nullptr);

		return *current_draw_list_shared_data_;
	}

	auto Context::set_default_font(font_type font) noexcept -> void
	{
		default_font_ = std::move(font);
		current_font_ = default_font_.get();
	}

	auto Context::font() const noexcept -> const Font&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_font_ != nullptr);
		return *current_font_;
	}
}
