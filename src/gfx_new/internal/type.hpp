// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <gfx_new/gfx.hpp>

namespace gal::prometheus::gfx_new
{
	// =========================================================
	// FONT
	// =========================================================

	// index
	using texture_atlas_id_type = std::uint32_t;
	constexpr texture_atlas_id_type invalid_texture_atlas_id{std::numeric_limits<texture_atlas_id_type>::max()};
}
