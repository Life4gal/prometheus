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
	class TextureDescriptor final
	{
	public:
		using point_type = primitive::basic_point_2d<std::uint32_t>;
		using size_type = primitive::basic_extent_2d<std::uint32_t>;

		using element_type = std::uint32_t;
		// size.width * size.height (RGBA)
		using data_type = std::unique_ptr<element_type[]>;
		using data_view_type = std::span<element_type>;

		// ==============================
		// CPU side
		// ==============================

		data_type data;
		size_type size;

		static_assert(sizeof(texture_id_type) == sizeof(std::uint64_t));
		std::uint64_t dirty : 1;

		// ==============================
		// GPU side
		// ==============================

		std::uint64_t id : 63;
	};

	class SubTexture
	{
	public:
		using point_type = TextureDescriptor::point_type;
		using size_type = TextureDescriptor::size_type;

		using element_type = TextureDescriptor::element_type;
		using data_type = std::mdspan<element_type, std::extents<size_type::value_type, std::dynamic_extent, std::dynamic_extent>, std::layout_stride>;
		using data_view_type = TextureDescriptor::data_view_type;

		constexpr static point_type invalid_point{(std::numeric_limits<point_type::value_type>::max)(), (std::numeric_limits<point_type::value_type>::max)()};

	private:
		point_type point_;
		data_type data_;

	public:
		SubTexture(point_type point, data_type data) noexcept;

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

	class Texture final
	{
	public:
		using point_type = SubTexture::point_type;
		using size_type = SubTexture::size_type;

		using element_type = SubTexture::element_type;
		using data_type = SubTexture::data_type;
		using data_view_type = SubTexture::data_view_type;

		using uv_type = primitive::basic_extent_2d<float>;

	private:
		struct pack_context_type;
		memory::UniquePointer<pack_context_type> pack_context_;

		TextureDescriptor descriptor_;

	public:
		explicit Texture(size_type size) noexcept;

		/**
		 * @brief Texture atlas data (for upload)
		 */
		[[nodiscard]] auto data() const noexcept -> data_view_type;

		/**
		 * @brief Set texture atlas id (GPU handle)
		 */
		auto bind_id(texture_id_type id) noexcept -> void;

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
		[[nodiscard]] auto select(size_type size) noexcept -> SubTexture;
	};

	// /**
	//  * @brief Texture atlas uploaded to the GPU
	//  */
	// class TextureAtlas final
	// {
	// public:
	// 	using value_type = std::uint32_t;
	// 	using point_type = primitive::basic_point_2d<value_type>;
	// 	using size_type = primitive::basic_extent_2d<value_type>;
	//
	// 	using uv_scale_type = extent_type;
	//
	// 	// size.width * size.height (RGBA)
	// 	using data_type = std::unique_ptr<std::uint32_t[]>;
	// 	using data_view_type = std::span<const std::uint32_t>;
	//
	// 	constexpr static point_type invalid_point{(std::numeric_limits<value_type>::max)(), (std::numeric_limits<value_type>::max)()};
	//
	// private:
	// 	struct pack_context_type;
	// 	memory::UniquePointer<pack_context_type> pack_context_;
	//
	// 	// Size of texture atlas
	// 	size_type size_;
	// 	// UV scale of texture atlas (1.0f / size.width, 1.0f / size.height)
	// 	uv_scale_type uv_scale_;
	// 	// Texture atlas data (CPU side)
	// 	data_type data_;
	//
	// 	static_assert(sizeof(texture_id_type) == sizeof(std::uint64_t));
	// 	// Does this texture need to be updated (re-uploaded)
	// 	std::uint64_t dirty_ : 1;
	// 	// GPU resource handle
	// 	std::uint64_t texture_id_ : 63;
	//
	// public:
	// 	TextureAtlas(value_type width, value_type height) noexcept;
	//
	// 	auto build(Renderer& renderer) noexcept -> void;
	// 	auto destroy(Renderer& renderer) noexcept -> void;
	//
	// 	/**
	// 	 * @brief Texture atlas size
	// 	 */
	// 	[[nodiscard]] auto size() const noexcept -> size_type;
	//
	// 	/**
	// 	 * @brief Texture atlas uv scale (1.0f / size.width, 1.0f / size.height)
	// 	 */
	// 	[[nodiscard]] auto uv_scale() const noexcept -> uv_scale_type;
	//
	// 	/**
	// 	 * @brief Texture atlas data (CPU side)
	// 	 */
	// 	[[nodiscard]] auto data() const noexcept -> data_view_type;
	//
	// 	/**
	// 	 * @brief Does this texture atlas is valid (uploaded to GPU)
	// 	 */
	// 	[[nodiscard]] auto valid() const noexcept -> bool;
	//
	// 	/**
	// 	 * @brief Does this texture atlas need to be re-uploaded to the GPU
	// 	 */
	// 	[[nodiscard]] auto dirty() const noexcept -> bool;
	//
	// 	/**
	// 	 * @brief Texture atlas id (usually a GPU resource handle)
	// 	 */
	// 	[[nodiscard]] auto id() const noexcept -> texture_id_type;
	//
	// 	/**
	// 	 * @brief Find a region that can hold a (piece of) texture of @c size
	// 	 * @param size texture size
	// 	 * @return texture coordinate
	// 	 * @note If such a region is not found, @c invalid_point is returned
	// 	 */
	// 	[[nodiscard]] auto seek(size_type size) noexcept -> point_type;
	//
	// 	/**
	// 	 * @brief Write a (piece of) texture @c data of @c size at the specified @c point of the current texture
	// 	 * @param point texture coordinate
	// 	 * @param size texture size
	// 	 * @param data texture data
	// 	 * @note Do not check the length of the @c data, assume it is at least @c size.width * @c size.height
	// 	 */
	// 	auto write(point_type point, size_type size, data_view_type data) noexcept -> void;
	//
	// 	/**
	// 	 * @brief Write a (piece of) texture @c data of @c size at the specified @c point of the current texture
	// 	 * @param point texture coordinate
	// 	 * @param size texture size
	// 	 * @param data texture data
	// 	 * @note Do not check the length of the @c data, assume it is at least @c size.width * @c size.height
	// 	 */
	// 	auto write(point_type point, size_type size, const data_type& data) noexcept -> void;
	// };
} // namespace gal::prometheus::gfx
