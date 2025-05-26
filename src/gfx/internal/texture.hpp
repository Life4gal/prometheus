// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <mdspan>

#include <gfx/gfx.hpp>

#include <stb_rect_pack.h>

namespace gal::prometheus::gfx
{
	class BorrowedTexture;

	class Texture final
	{
		// Texture::id and Texture::dirty
		friend Context;

	public:
		using element_type = TextureDescriptor::element_type;
		using data_type = TextureDescriptor::data_type;
		using data_view_type = TextureDescriptor::data_view_type;

		using size_type = TextureDescriptor::size_type;

		using point_type = primitive::basic_point_2d<size_type::value_type>;
		using uv_type = primitive::basic_extent_2d<float>;

	private:
		stbrp_context rp_context_;
		std::vector<stbrp_node> rp_nodes_;

		TextureDescriptor texture_;
		// It's much more cost-effective to keep a member variable than to compute it every time
		uv_type uv_;

	public:
		explicit Texture(size_type size) noexcept;

		/**
		 * @brief Texture atlas data (for upload)
		 */
		[[nodiscard]] auto data() const noexcept -> data_view_type;

		/**
		 * @brief Texture atlas area size
		 */
		[[nodiscard]] auto area_size() const noexcept -> std::size_t;

		/**
		 * @brief Texture atlas size
		 */
		[[nodiscard]] auto size() const noexcept -> size_type;

		/**
		 * @brief Texture atlas uv scale (1.0f / size.width, 1.0f / size.height)
		 */
		[[nodiscard]] auto uv() const noexcept -> uv_type;

		/**
		 * @brief Does this texture atlas need to be re-uploaded to the GPU
		 */
		[[nodiscard]] auto dirty() const noexcept -> bool;

		/**
		 * @brief Texture atlas uploaded (to GPU)
		 */
		[[nodiscard]] auto uploaded() const noexcept -> bool;

		/**
		 * @brief Texture atlas id (usually a GPU resource handle)
		 */
		[[nodiscard]] auto id() const noexcept -> texture_id_type;

		/**
		 * @brief Find a region that can hold a (piece of) texture of @c size
		 * @param size Texture size
		 */
		[[nodiscard]] auto select(size_type size) noexcept -> BorrowedTexture;
	};

	class BorrowedTexture final
	{
		friend Texture;

	public:
		using element_type = Texture::element_type;
		using data_type = Texture::data_type;
		using data_view_type = Texture::data_view_type;

		using size_type = Texture::size_type;

		using point_type = Texture::point_type;
		using uv_type = Texture::uv_type;

		constexpr static point_type invalid_point{(std::numeric_limits<point_type::value_type>::max)(), (std::numeric_limits<point_type::value_type>::max)()};

	private:
		using borrow_data_type = std::mdspan<element_type, std::extents<size_type::value_type, std::dynamic_extent, std::dynamic_extent>, std::layout_stride>;

		point_type point_;
		borrow_data_type data_;

		BorrowedTexture(point_type point, const borrow_data_type& data) noexcept;

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
}
