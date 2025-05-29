// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/internal/texture.hpp>
#include <gfx/render_list.hpp>
#include <gfx/gfx.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace
{
	using namespace gal::prometheus;
	using namespace gfx;

	[[nodiscard]] auto make_texture(const Texture::size_type size) noexcept -> Texture
	{
		auto data = std::make_unique_for_overwrite<Texture::element_type[]>(static_cast<std::size_t>(size.width) * size.height);

		return {
				.data = std::move(data),
				.size = size,
				.uv_scale = {1.f / static_cast<Texture::uv_scale_type::value_type>(size.width), 1.f / static_cast<Texture::uv_scale_type::value_type>(size.height)},
				.id = invalid_texture_id
		};
	}
}

namespace gal::prometheus::gfx
{
	auto TextureContext::root_id() const noexcept -> texture_atlas_id_type
	{
		std::ignore = this;
		return 0;
	}

	auto TextureContext::active_id() const noexcept -> texture_atlas_id_type
	{
		return static_cast<texture_atlas_id_type>(atlas_list_.size() - 1);
	}

	auto TextureContext::select_atlas(const texture_atlas_id_type texture_atlas_id) noexcept -> atlas_type&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture_atlas_id < atlas_list_.size());

		return atlas_list_[texture_atlas_id];
	}

	auto TextureContext::select_atlas(const texture_atlas_id_type texture_atlas_id) const noexcept -> const atlas_type&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture_atlas_id < atlas_list_.size());

		return atlas_list_[texture_atlas_id];
	}

	auto TextureContext::new_atlas(const size_type size) noexcept -> atlas_type&
	{
		atlas_type atlas
		{
				.texture = make_texture(size),
				.pending_update_data = {},
				.rp_context = gfx::RectPackContext{size},
		};
		return atlas_list_.emplace_back(std::move(atlas));
	}

	auto TextureContext::write(const texture_atlas_id_type texture_atlas_id, const size_type size) noexcept -> TextureWriter
	{
		auto& [texture, pending_data_list, rp_context] = select_atlas(texture_atlas_id);

		gfx::RectPackContext::rect_type new_rect{.size = size, .point = {}};
		if (rp_context.pack({&new_rect, 1}))
		{
			const auto point = new_rect.point;
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(point != gfx::RectPackContext::invalid_point);

			auto* address = texture.data.get() + (point.y * size.width + point.x);
			const auto mapping = TextureWriter::borrow_data_type::mapping_type{
					std::dextents<size_type::value_type, 2>{size.height, size.width},
					std::array<size_type::value_type, 2>{texture.size.width, 1},
			};
			const auto data = TextureWriter::borrow_data_type{address, mapping};

			TextureViewer viewer{point, data};
			pending_data_list.emplace_back(viewer);

			return {point, data};
		}

		return {TextureWriter::invalid_point, {}};
	}

	TextureContext::TextureContext() noexcept
	{
		atlas_list_.reserve(2);

		// root atlas
		constexpr size_type root_texture_atlas_size{128, 128};
		new_atlas(root_texture_atlas_size);

		// first
		constexpr size_type first_texture_atlas_size{2048, 2048};
		new_atlas(first_texture_atlas_size);
	}

	auto TextureContext::initialize(RenderListSharedData& shared_data) noexcept -> void
	{
		const auto atlas_id = root_id();
		const auto& texture = select_texture(atlas_id);

		// ========================================
		// BAKE LINES (AA)
		// ========================================
		{
			constexpr std::uint32_t white_color = 0xff'ff'ff'ff;
			constexpr auto aa_width = static_cast<size_type::value_type>(RenderListSharedData::baked_line_uv_count);
			constexpr auto aa_height = static_cast<size_type::value_type>(RenderListSharedData::baked_line_uv_count);
			constexpr auto aa_size = size_type{aa_width, aa_height};

			// baked line rect area:
			// white pixel + ◿
			const auto texture_write = write(atlas_id, aa_size);
			texture_write.fill(0);

			const auto aa_point = texture_write.position();
			const auto aa_uv_scale = texture.uv_scale;

			// white pixel
			{
				// LINE 0, 2 pixels
				texture_write.fill(0, 2, white_color);
				// LINE 1, 2 pixels
				texture_write.fill(1, 2, white_color);

				const auto uv_x = static_cast<gfx::point_type::value_type>(static_cast<float>(aa_point.x) + 1.0f) * aa_uv_scale.width;
				const auto uv_y = static_cast<gfx::point_type::value_type>(static_cast<float>(aa_point.y) + 1.0f) * aa_uv_scale.height;

				shared_data.white_pixel_uv = {uv_x, uv_y};
			}

			// ◿
			for (size_type::value_type y = 1; y < aa_height; ++y)
			{
				const auto line_width = y;
				const auto offset = aa_width - line_width;

				texture_write.fill(y, offset, line_width, white_color);

				const auto p_x = aa_point.x + offset;
				const auto p_y = aa_point.y + y;
				const auto width = line_width;
				constexpr auto height = .5f;

				const auto uv_x = static_cast<gfx::point_type::value_type>(p_x) * aa_uv_scale.width;
				const auto uv_y = static_cast<gfx::point_type::value_type>(p_y) * aa_uv_scale.height;
				const auto uv_width = static_cast<gfx::point_type::value_type>(width) * aa_uv_scale.width;
				const auto uv_height = static_cast<gfx::point_type::value_type>(height) * aa_uv_scale.height;

				shared_data.baked_line_uvs[y] = {uv_x, uv_y, uv_width, uv_height};
			}
		}

		// ========================================
		// 
		// ========================================
		{
			std::ignore = texture;
		}
	}

	auto TextureContext::update_all_atlas(Renderer& renderer) noexcept -> void
	{
		std::ranges::for_each(
			atlas_list_,
			[&renderer](auto& atlas) noexcept -> void
			{
				if (atlas.texture.id == invalid_texture_id)
				{
					atlas.texture.id = renderer.create_texture(atlas.texture.data, atlas.texture.size);
				}
				else if (not atlas.pending_update_data.empty())
				{
					renderer.update_texture(atlas.texture.id, atlas.pending_update_data);

					atlas.pending_update_data.clear();
				}
			}
		);
	}

	auto TextureContext::root_texture() noexcept -> Texture&
	{
		return select_texture(root_id());
	}

	auto TextureContext::root_texture() const noexcept -> const Texture&
	{
		return select_texture(root_id());
	}

	auto TextureContext::select_texture(const texture_atlas_id_type texture_atlas_id) noexcept -> Texture&
	{
		return select_atlas(texture_atlas_id).texture;
	}

	auto TextureContext::select_texture(const texture_atlas_id_type texture_atlas_id) const noexcept -> const Texture&
	{
		return select_atlas(texture_atlas_id).texture;
	}

	auto TextureContext::write(const size_type size) noexcept -> write_result
	{
		const auto id = active_id();
		const auto writer = write(id, size);

		return {.writer = writer, .texture_atlas_id = id};
	}
}
