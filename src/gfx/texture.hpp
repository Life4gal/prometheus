// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>
#include <span>
#include <mdspan>

#include <gfx/type.hpp>

namespace gal::prometheus::gfx
{
	class Texture final
	{
	public:
		// 32-bits (RGBA)
		using element_type = std::uint32_t;
		// size.width * size.height
		using data_type = std::unique_ptr<element_type[]>;
		using data_view_type = std::span<const element_type>;

		// RectPackContext::point_type
		using point_type = primitive::basic_point_2d<std::uint32_t>;
		// RectPackContext::size_type
		using size_type = primitive::basic_extent_2d<std::uint32_t>;

		using uv_scale_type = primitive::basic_extent_2d<float>;

		// ==============================
		// CPU side
		// ==============================

		data_type data;
		size_type size;

		// It's much more cost-effective to keep a member variable than to compute it every time
		uv_scale_type uv_scale;

		// ==============================
		// GPU side
		// ==============================

		texture_id_type id;
	};

	class TextureContext;
	class TextureWriter;
	class TextureViewer;

	class TextureWriter final
	{
		friend TextureContext;
		friend TextureViewer;

	public:
		using element_type = Texture::element_type;
		using data_type = Texture::data_type;
		using data_view_type = Texture::data_view_type;

		using point_type = Texture::point_type;
		using size_type = Texture::size_type;

		using uv_scale_type = Texture::uv_scale_type;

		constexpr static point_type invalid_point{(std::numeric_limits<point_type::value_type>::max)(), (std::numeric_limits<point_type::value_type>::max)()};

	private:
		using borrow_data_type = std::mdspan<element_type, std::extents<size_type::value_type, std::dynamic_extent, std::dynamic_extent>, std::layout_stride>;

		point_type point_;
		borrow_data_type data_;

		TextureWriter(point_type point, const borrow_data_type& data) noexcept;

	public:
		[[nodiscard]] auto valid() const noexcept -> bool;

		[[nodiscard]] auto position() const noexcept -> point_type;

		auto fill(size_type::value_type y, size_type::value_type offset, size_type::value_type n, element_type element) const noexcept -> void;
		auto fill(size_type::value_type y, size_type::value_type offset, data_view_type data) const noexcept -> void;

		auto fill(size_type::value_type y, size_type::value_type n, element_type element) const noexcept -> void;
		auto fill(size_type::value_type y, data_view_type data) const noexcept -> void;

		auto fill(size_type::value_type y, element_type element) const noexcept -> void;

		auto fill(element_type element) const noexcept -> void;
		auto fill(data_view_type data) const noexcept -> void;

		auto operator[](size_type::value_type x, size_type::value_type y) const noexcept -> borrow_data_type::reference;
	};

	class TextureViewer final
	{
		friend TextureContext;

	public:
		using element_type = Texture::element_type;
		using data_type = Texture::data_type;
		using data_view_type = Texture::data_view_type;

		using point_type = Texture::point_type;
		using size_type = Texture::size_type;

		using uv_scale_type = Texture::uv_scale_type;

	private:
		using borrow_data_type = std::mdspan<const element_type, std::extents<size_type::value_type, std::dynamic_extent, std::dynamic_extent>, std::layout_stride>;

		point_type point_;
		borrow_data_type data_;

		TextureViewer(point_type point, const borrow_data_type& data) noexcept;

	public:
		[[nodiscard]] auto position() const noexcept -> point_type;

		[[nodiscard]] auto size() const noexcept -> size_type;

		[[nodiscard]] auto line(size_type::value_type y, size_type::value_type offset, size_type::value_type n) const noexcept -> data_view_type;

		[[nodiscard]] auto line(size_type::value_type y, size_type::value_type n) const noexcept -> data_view_type;

		[[nodiscard]] auto line(size_type::value_type y) const noexcept -> data_view_type;

		auto operator[](size_type::value_type x, size_type::value_type y) const noexcept -> borrow_data_type::reference;
	};
}
