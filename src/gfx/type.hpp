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

	// DX11: ID3D11ShaderResourceView
	// DX12: D3D12_GPU_DESCRIPTOR_HANDLE::ptr
	using texture_id_type = std::uintptr_t;
	constexpr texture_id_type invalid_texture_id{0};

	class Texture;
	class BorrowTexture;

	// =========================================================
	// FONT
	// =========================================================

	using texture_atlas_id_type = std::uint32_t;
	constexpr texture_atlas_id_type invalid_texture_atlas_id{std::numeric_limits<texture_atlas_id_type>::max()};

	using font_id_type = std::uint32_t;
	constexpr font_id_type invalid_font_id{std::numeric_limits<font_id_type>::max()};

	class GlyphKey;
	class GlyphInfo;

	class FontPendingLoadData;

	class GlyphParsedInfo;
	class GlyphParser;

	class FontFace;

	// =========================================================
	// RENDERER LIST
	// =========================================================

	class RenderListSharedData;
	class RenderList;

	// =========================================================
	// RENDERER
	// =========================================================

	class Renderer;

	// =========================================================
	// CONTEXT
	// =========================================================

	class TextureContext;
	class RendererContext;
} // namespace gal::prometheus::gfx
