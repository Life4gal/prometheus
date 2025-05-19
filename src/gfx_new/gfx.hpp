// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <span>

#include <primitive/circle.hpp>
#include <primitive/color.hpp>
#include <primitive/ellipse.hpp>
#include <primitive/extent.hpp>
#include <primitive/point.hpp>
#include <primitive/rect.hpp>
#include <primitive/vertex.hpp>

#include <memory/unique_ptr.hpp>

namespace gal::prometheus::gfx_new
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

	// DX11: ID3D11ShaderResourceView
	// DX12: D3D12_GPU_DESCRIPTOR_HANDLE::ptr / HEAP index + constant offset
	using texture_id_type = std::uintptr_t;
	constexpr texture_id_type invalid_texture_id{0};

	// =========================================================
	// RENDERER
	// =========================================================

	class Renderer
	{
	public:
		class RendererContext;

		class AccessorTexture;
		class AccessorFont;
		class AccessorRender;

		class Texture final
		{
		public:
			// 32-bits (RGBA)
			using element_type = std::uint32_t;
			// size.width * size.height
			using data_type = std::unique_ptr<element_type[]>;
			using data_view_type = std::span<element_type>;

			using size_type = primitive::basic_extent_2d<std::uint32_t>;

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

	private:
		memory::UniquePointer<RendererContext> context_;

	public:
		Renderer(const Renderer&) noexcept = delete;
		Renderer(Renderer&&) noexcept = default;
		auto operator=(const Renderer&) noexcept -> Renderer& = delete;
		auto operator=(Renderer&&) noexcept -> Renderer& = default;

		virtual ~Renderer() noexcept;

	protected:
		Renderer() noexcept;

	public:
		/**
		 * @brief Constructs the renderer, which assumes that all the parameters needed for the renderer have been set
		 */
		[[nodiscard]] auto construct() noexcept -> bool;

		/**
		 * @brief Destructs the renderer
		 */
		auto destruct() noexcept -> void;

		/**
		 * @brief Whether the current renderer is ready (@c construct was called and succeeded)
		 */
		[[nodiscard]] auto ready() const noexcept -> bool;

		/**
		 * @brief
		 * @note @c new_frame -> @c present -> @c end_frame
		 */
		auto new_frame() noexcept -> void;

		/**
		 * @brief
		 * @note @c new_frame -> @c present -> @c end_frame
		 */
		auto present() noexcept -> void;

		/**
		 * @brief
		 * @note @c new_frame -> @c present -> @c end_frame
		 */
		auto end_frame() noexcept -> void;

	private:
		virtual auto do_construct() noexcept -> bool = 0;
		virtual auto do_destruct() noexcept -> void = 0;
		[[nodiscard]] virtual auto do_ready() const noexcept -> bool = 0;

		[[nodiscard]] virtual auto do_texture_create(Texture::data_view_type data, Texture::size_type size) noexcept -> texture_id_type = 0;
		virtual auto do_texture_update(const Texture& texture) noexcept -> void = 0;
		virtual auto do_texture_destroy(texture_id_type texture_id) noexcept -> void = 0;

		virtual auto do_before_new_frame() noexcept -> void;
		virtual auto do_after_new_frame() noexcept -> void;
		virtual auto do_before_present() noexcept -> void;
		virtual auto do_after_present() noexcept -> void;
		virtual auto do_before_end_frame() noexcept -> void;
		virtual auto do_after_end_frame() noexcept -> void;
	};
}
