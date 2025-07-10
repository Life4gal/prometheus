// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <primitive/circle.hpp>
#include <primitive/color.hpp>
#include <primitive/ellipse.hpp>
#include <primitive/extent.hpp>
#include <primitive/point.hpp>
#include <primitive/rect.hpp>
#include <primitive/vertex.hpp>

namespace gal::prometheus::gfx
{
	// =========================================================
	// PRIMITIVE
	// =========================================================

	using point_type = primitive::basic_point_2d<float>;
	using uv_type = primitive::basic_point_2d<float>;
	using color_type = primitive::basic_color;
	using vertex_type = primitive::basic_vertex<point_type, uv_type, color_type>;
	using index_type = std::uint16_t;

	using extent_type = primitive::basic_extent_2d<float>;
	using rect_type = primitive::basic_rect_2d<float, float>;
	using circle_type = primitive::basic_circle_2d<float>;
	using ellipse_type = primitive::basic_ellipse_2d<float, float>;

	// =========================================================
	// TEXTURE
	// =========================================================

	// D3D11: ID3D11ShaderResourceView
	// D3D12: D3D12_GPU_DESCRIPTOR_HANDLE::ptr / HEAP index
	using texture_id_type = std::uintptr_t;
	constexpr texture_id_type invalid_texture_id{std::numeric_limits<texture_id_type>::max()};
}
