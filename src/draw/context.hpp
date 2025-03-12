// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <draw/def.hpp>
#include <draw/font.hpp>
#include <draw/shared_data.hpp>

namespace gal::prometheus::draw
{
	class Context;

	class Context
	{
	public:
		using font_type = std::shared_ptr<Font>;

	private:
		DrawListSharedData default_draw_list_shared_data_;
		DrawListSharedData* current_draw_list_shared_data_;

		font_type default_font_;
		Font* current_font_;

		Context() noexcept;

	public:
		// ---------------------------------------------
		// SINGLETON

		[[nodiscard]] static auto instance() noexcept -> Context&;

		// ---------------------------------------------
		// DRAW LIST SHARED DATA

		[[nodiscard]] auto draw_list_shared_data() const noexcept -> const DrawListSharedData&;

		// ---------------------------------------------
		// FONT

		auto set_default_font(font_type font) noexcept -> void;

		[[nodiscard]] auto font() const noexcept -> const Font&;
	};
}
