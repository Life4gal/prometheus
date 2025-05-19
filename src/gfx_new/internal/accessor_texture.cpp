// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx_new/internal/accessor_texture.hpp>

namespace gal::prometheus::gfx_new
{
	auto TextureContext::root_id() const noexcept -> texture_atlas_id_type
	{
		std::ignore = this;
		return 0;
	}

	TextureContext::TextureContext() noexcept
	{
		// root atlas
		constexpr Texture::size_type root_texture_atlas_size{2048, 2048};
		texture_atlas_list_.emplace_back(root_texture_atlas_size);
	}

	auto TextureContext::root() noexcept -> Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture_atlas_list_.empty());

		return texture_atlas_list_[root_id()];
	}

	auto TextureContext::root() const noexcept -> const Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture_atlas_list_.empty());

		return texture_atlas_list_[root_id()];
	}

	auto TextureContext::select(const texture_atlas_id_type texture_atlas_id) noexcept -> Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture_atlas_id < texture_atlas_list_.size());

		return texture_atlas_list_[texture_atlas_id];
	}

	auto TextureContext::select(const texture_atlas_id_type texture_atlas_id) const noexcept -> const Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture_atlas_id < texture_atlas_list_.size());

		return texture_atlas_list_[texture_atlas_id];
	}

	auto TextureContext::write(
		const texture_atlas_id_type texture_atlas_id,
		const Texture::data_view_type data,
		const Texture::size_type size
	) noexcept -> primitive::basic_rect_2d<Texture::uv_type::value_type>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.size() >= static_cast<std::size_t>(size.width) * size.height);

		auto& texture = this->select(texture_atlas_id);
		const auto texture_uv = texture.uv();

		const auto borrowed_texture = texture.select(size);
		borrowed_texture.fill(data);

		const auto position = borrowed_texture.position();
		return {position.to<point_type>() * texture_uv, size.to<extent_type>() * texture_uv};
	}

	auto TextureContext::write(
		const Texture::data_view_type data,
		const Texture::size_type size
	) noexcept -> primitive::basic_rect_2d<Texture::uv_type::value_type>
	{
		// todo
		return this->write(root_id(), data, size);
	}

	auto TextureContext::upload(Renderer& renderer) noexcept -> void
	{
		std::ranges::for_each(
			texture_atlas_list_,
			[accessor = Renderer::AccessorTexture{renderer}](auto& texture) mutable noexcept -> void
			{
				if (not texture.uploaded())
				{
					accessor.upload(texture);
				}
				else
				{
					accessor.update_if_dirty(texture);
				}
			}
		);
	}

	Renderer::AccessorTexture::AccessorTexture(Renderer& renderer) noexcept
		: renderer_{renderer} {}

	auto Renderer::AccessorTexture::upload(texture_type& texture) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture.uploaded());

		auto& renderer = renderer_.get();

		texture.texture_.id = renderer.do_texture_create(texture.data(), texture.size());
		texture.texture_.dirty = false;
	}

	auto Renderer::AccessorTexture::update(texture_type& texture) noexcept -> void
	{
		auto& renderer = renderer_.get();

		renderer.do_texture_update(texture.texture_);
		texture.texture_.dirty = false;
	}

	auto Renderer::AccessorTexture::update_if_dirty(texture_type& texture) noexcept -> void
	{
		if (texture.dirty())
		{
			update(texture);
		}
	}
}
