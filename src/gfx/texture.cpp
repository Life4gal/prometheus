// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/texture.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

namespace gal::prometheus::gfx
{
	SubTexture::SubTexture(const point_type point, const data_type data) noexcept
		: point_{point},
		  data_{data}
	{
	}

	auto SubTexture::valid() const noexcept -> bool
	{
		return point_ != invalid_point;
	}

	auto SubTexture::point() const noexcept -> point_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		return point_;
	}

	auto SubTexture::fill(const size_type::value_type y, const size_type::value_type offset, const size_type::value_type n, const element_type element) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < data_.extent(0));
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(offset + n < data_.extent(1));

		for (size_type::value_type x = offset; x < offset + n; ++x)
		{
			data_[y, x] = element;
		}
	}

	auto SubTexture::fill(const size_type::value_type y, const size_type::value_type offset, const data_view_type data) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < data_.extent(0));
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(offset + data.size() < data_.extent(1));

		for (size_type::value_type x = 0; x < data.size(); ++x)
		{
			data_[y, x + offset] = data[x];
		}
	}

	auto SubTexture::fill(const size_type::value_type y, const size_type::value_type n, const element_type element) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < data_.extent(0));
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(n < data_.extent(1));

		fill(y, 0, n, element);
	}

	auto SubTexture::fill(const size_type::value_type y, const data_view_type data) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < data_.extent(0));
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.size() < data_.extent(1));

		fill(y, 0, data);
	}

	auto SubTexture::fill(const size_type::value_type y, const element_type element) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < data_.extent(0));

		fill(y, data_.extent(1), element);
	}

	auto SubTexture::fill(const element_type element) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		for (size_type::value_type y = 0; y < data_.extent(0); ++y)
		{
			fill(y, element);
		}
	}

	auto SubTexture::fill(const data_view_type data) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		for (size_type::value_type y = 0; y < data_.extent(0); ++y)
		{
			const data_view_type sub{data.begin() + y * data_.extent(1), data_.extent(1)};

			fill(y, sub);
		}
	}

	auto SubTexture::operator[](size_type::value_type x, size_type::value_type y) const noexcept -> data_type::reference
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < data_.extent(0));
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(x < data_.extent(1));

		return data_[y, x];
	}

	struct Texture::pack_context_type
	{
		stbrp_context context{};
		std::vector<stbrp_node> nodes{};
	};

	Texture::Texture(const size_type size) noexcept
		: pack_context_{memory::make_unique<pack_context_type>()},
		  descriptor_{
			  .data = std::make_unique_for_overwrite<std::uint32_t[]>(static_cast<std::size_t>(size.width) * size.height),
			  .size = size,
			  .dirty = false,
			  .id = invalid_texture_id,
		  }
	{
		pack_context_->nodes.resize(size.width);

		stbrp_init_target(
				&pack_context_->context, static_cast<stbrp_coord>(size.width), static_cast<stbrp_coord>(size.height), pack_context_->nodes.data(), static_cast<int>(pack_context_->nodes.size())
		);
	}

	auto Texture::data() const noexcept -> data_view_type
	{
		const auto length = descriptor_.size.width * descriptor_.size.height;

		return {descriptor_.data.get(), length};
	}

	auto Texture::bind_id(const texture_id_type id) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id != invalid_texture_id);

		descriptor_.id = id;
	}

	auto Texture::size() const noexcept -> size_type
	{
		return descriptor_.size;
	}

	auto Texture::uv() const noexcept -> uv_type
	{
		const auto s = size();

		return {1.f / static_cast<uv_type::value_type>(s.width), 1.f / static_cast<uv_type::value_type>(s.height)};
	}

	auto Texture::dirty() const noexcept -> bool
	{
		return descriptor_.dirty;
	}

	auto Texture::uploaded() const noexcept -> bool
	{
		return descriptor_.id != invalid_texture_id;
	}

	auto Texture::id() const noexcept -> texture_id_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(uploaded());

		return descriptor_.id;
	}

	auto Texture::select(const size_type size) noexcept -> SubTexture
	{
		stbrp_rect rect{.id = -1, .w = static_cast<stbrp_coord>(size.width), .h = static_cast<stbrp_coord>(size.height), .x = 0, .y = 0, .was_packed = 0};

		if (stbrp_pack_rects(&pack_context_->context, &rect, 1))
		{
			const point_type point{static_cast<point_type::value_type>(rect.x), static_cast<point_type::value_type>(rect.y)};

			auto* address = descriptor_.data.get() + (point.y * descriptor_.size.width + point.x);
			const auto mapping = data_type::mapping_type{
				std::dextents<size_type::value_type, 2>{size.height, size.width},
				std::array<size_type::value_type, 2>{descriptor_.size.width, 1},
			};
			const auto data = data_type{address, mapping};

			descriptor_.dirty = true;
			return {point, data};
		}

		return {SubTexture::invalid_point, {}};
	}

	// struct TextureAtlas::pack_context_type
	// {
	// 	stbrp_context context;
	// 	std::vector<stbrp_node> nodes;
	// };

	// TextureAtlas::TextureAtlas(const value_type width, const value_type height) noexcept
	// 	: pack_context_{memory::make_unique<pack_context_type>()},
	// 	  size_{width, height},
	// 	  uv_scale_{1.f / static_cast<uv_scale_type::value_type>(width), 1.f / static_cast<uv_scale_type::value_type>(height)},
	// 	  data_{std::make_unique_for_overwrite<std::uint32_t[]>(static_cast<std::size_t>(width) * height)},
	// 	  dirty_{false},
	// 	  texture_id_{invalid_texture_id}
	// {
	// 	pack_context_->nodes.resize(width);
	//
	// 	stbrp_init_target(&pack_context_->context, static_cast<stbrp_coord>(width), static_cast<stbrp_coord>(height), pack_context_->nodes.data(), static_cast<int>(pack_context_->nodes.size()));
	// }
	//
	// auto TextureAtlas::build(Renderer& renderer) noexcept -> void
	// {
	// 	texture_id_ = renderer.create_texture();
	// }
	//
	// auto TextureAtlas::destroy(Renderer& renderer) noexcept -> void
	// {
	// 	data_.reset();
	// 	dirty_ = false;
	// 	renderer.destroy_texture(texture_id_);
	// 	texture_id_ = invalid_texture_id;
	// }
	//
	// auto TextureAtlas::valid() const noexcept -> bool
	// {
	// 	return texture_id_ != invalid_texture_id;
	// }
	//
	// auto TextureAtlas::dirty() const noexcept -> bool
	// {
	// 	return dirty_;
	// }
	//
	// auto TextureAtlas::id() const noexcept -> texture_id_type
	// {
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
	// 	return texture_id_;
	// }
	//
	// auto TextureAtlas::size() const noexcept -> size_type
	// {
	// 	return size_;
	// }
	//
	// auto TextureAtlas::uv_scale() const noexcept -> uv_scale_type
	// {
	// 	return uv_scale_;
	// }
	//
	// auto TextureAtlas::data() const noexcept -> data_view_type
	// {
	// 	const auto length = size_.width * size_.height;
	//
	// 	return {data_.get(), length};
	// }
	//
	// auto TextureAtlas::seek(const size_type size) noexcept -> point_type
	// {
	// 	stbrp_rect rect{.id = -1, .w = static_cast<stbrp_coord>(size.width), .h = static_cast<stbrp_coord>(size.height), .x = 0, .y = 0, .was_packed = 0};
	//
	// 	if (stbrp_pack_rects(&pack_context_->context, &rect, 1))
	// 	{
	// 		return {static_cast<point_type::value_type>(rect.x), static_cast<point_type::value_type>(rect.y)};
	// 	}
	//
	// 	return invalid_point;
	// }
	//
	// auto TextureAtlas::write(const point_type point, const size_type size, const data_view_type data) noexcept -> void
	// {
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(point.x + size.width <= size_.width);
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(point.y + size.height <= size_.height);
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.data() != nullptr);
	// 	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.size() >= (static_cast<std::size_t>(size.width) * size.height));
	//
	// 	for (value_type y = 0; y < size.height; ++y)
	// 	{
	// 		const auto offset_y = (point.y + y) * size_.width;
	//
	// 		for (value_type x = 0; x < size.width; ++x)
	// 		{
	// 			const auto offset_x = point.x + x;
	// 			const auto index = offset_x + offset_y;
	//
	// 			data_[index] = data[y * size.width + x];
	// 		}
	// 	}
	//
	// 	dirty_ = true;
	// }
	//
	// auto TextureAtlas::write(const point_type point, const size_type size, const data_type& data) noexcept -> void
	// {
	// 	return write(point, size, {data.get(), static_cast<std::size_t>(size.width) * size.height});
	// }

} // namespace gal::prometheus::gfx
