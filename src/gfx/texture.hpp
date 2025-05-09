// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <mdspan>
#include <span>

#include <gfx/type.hpp>

#include <memory/unique_ptr.hpp>

namespace gal::prometheus::gfx
{
	class Texture final
	{
	public:
		using point_type = primitive::basic_point_2d<std::uint32_t>;
		using size_type = primitive::basic_extent_2d<std::uint32_t>;

		using element_type = std::uint32_t;
		// size.width * size.height (RGBA)
		using data_type = std::unique_ptr<element_type[]>;
		using data_view_type = std::span<element_type>;

		using uv_type = primitive::basic_extent_2d<float>;

	private:
		struct pack_context_type;
		memory::UniquePointer<pack_context_type> pack_context_;

		// ==============================
		// CPU side
		// ==============================

		data_type data_;
		size_type size_;

		static_assert(sizeof(texture_id_type) == sizeof(std::uint64_t));
		std::uint64_t dirty_ : 1;

		// ==============================
		// GPU side
		// ==============================

		std::uint64_t id_ : 63;

	public:
		Texture(const Texture&) noexcept = delete;
		Texture(Texture&&) noexcept;					// = default;
		auto operator=(const Texture&) noexcept -> Texture& = delete;
		auto operator=(Texture&&) noexcept -> Texture&; // = default;

		~Texture() noexcept;

		explicit Texture(size_type size) noexcept;

		auto upload(Renderer& renderer) noexcept -> void;

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
		[[nodiscard]] auto select(size_type size) noexcept -> BorrowTexture;
	};

	class BorrowTexture
	{
	public:
		using point_type = Texture::point_type;
		using size_type = Texture::size_type;

		using element_type = Texture::element_type;
		using data_type = std::mdspan<element_type, std::extents<size_type::value_type, std::dynamic_extent, std::dynamic_extent>, std::layout_stride>;
		using data_view_type = Texture::data_view_type;

		constexpr static point_type invalid_point{(std::numeric_limits<point_type::value_type>::max)(), (std::numeric_limits<point_type::value_type>::max)()};

	private:
		point_type point_;
		data_type data_;

	public:
		BorrowTexture(point_type point, data_type data) noexcept;

		[[nodiscard]] auto valid() const noexcept -> bool;

		[[nodiscard]] auto point() const noexcept -> point_type;

		auto fill(size_type::value_type y, size_type::value_type offset, size_type::value_type n, element_type element) const noexcept -> void;
		auto fill(size_type::value_type y, size_type::value_type offset, data_view_type data) const noexcept -> void;

		auto fill(size_type::value_type y, size_type::value_type n, element_type element) const noexcept -> void;
		auto fill(size_type::value_type y, data_view_type data) const noexcept -> void;

		auto fill(size_type::value_type y, element_type element) const noexcept -> void;

		auto fill(element_type element) const noexcept -> void;
		auto fill(data_view_type data) const noexcept -> void;

		auto operator[](size_type::value_type x, size_type::value_type y) const noexcept -> data_type::reference;
	};
} // namespace gal::prometheus::gfx
