// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/context.hpp>

#include <gfx/render_list.hpp>

// #include <chars/chars.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gfx
{
	auto TextureContext::root_atlas() noexcept -> Texture&
	{
		return texture_atlases_.front();
	}

	auto TextureContext::root_atlas() const noexcept -> const Texture&
	{
		return texture_atlases_.front();
	}

	auto TextureContext::select_atlas(const texture_atlas_id_type id) noexcept -> Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id < texture_atlases_.size());

		return texture_atlases_[id];
	}

	auto TextureContext::select_atlas(const texture_atlas_id_type id) const noexcept -> const Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id < texture_atlases_.size());

		return texture_atlases_[id];
	}

	auto TextureContext::select_atlas(const Texture::size_type size) const noexcept -> texture_atlas_id_type
	{
		// width/height == 0 ==> whitespace
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size.width >= 0 and size.height >= 0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture_atlases_.empty());

		std::ignore = size;

		// todo: root only?
		return 0;
	}

	auto TextureContext::make_territory(const Texture::size_type size) noexcept -> BorrowTexture
	{
		// width/height == 0 ==> whitespace
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size.width >= 0 and size.height >= 0);

		const auto atlas_id = select_atlas(size);
		auto& atlas = select_atlas(atlas_id);

		auto texture = atlas.select(size);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture.valid());

		Territory territory{.id = atlas_id, .point = texture.point(), .size = size};
		territories_.emplace_back(territory);

		return texture;
	}

	TextureContext::TextureContext() noexcept
	{
		// root atlas
		constexpr Texture::size_type root_texture_atlas_size{2048, 2048};
		texture_atlases_.emplace_back(root_texture_atlas_size);
	}

	auto TextureContext::initialize(RenderListSharedData& shared_data) noexcept -> void
	{
		// ========================================
		// BAKE LINES (AA)
		// ========================================
		{
			constexpr std::uint32_t white_color = 0xff'ff'ff'ff;
			constexpr auto aa_width = static_cast<Texture::size_type::value_type>(RenderListSharedData::baked_line_uv_count);
			constexpr auto aa_height = static_cast<Texture::size_type::value_type>(RenderListSharedData::baked_line_uv_count);
			constexpr auto aa_size = Texture::size_type{aa_width, aa_height};

			const auto atlas_id = select_atlas(aa_size);
			auto& atlas = select_atlas(atlas_id);

			const auto atlas_uv_scale = atlas.uv();

			const auto& aa_texture = make_territory(aa_size);
			const auto aa_point = aa_texture.point();

			// baked line rect area:
			// white pixel
			// ◿
			aa_texture.fill(0);

			// white pixel
			{
				aa_texture[0, 0] = white_color;
				aa_texture[1, 0] = white_color;
				aa_texture[0, 1] = white_color;
				aa_texture[1, 1] = white_color;

				const auto uv_x = static_cast<point_type::value_type>(static_cast<float>(aa_point.x) + .5f) * atlas_uv_scale.width;
				const auto uv_y = static_cast<point_type::value_type>(static_cast<float>(aa_point.y) + .5f) * atlas_uv_scale.height;

				shared_data.white_pixel_uv = {uv_x, uv_y};
			}

			// ◿
			for (Texture::size_type::value_type y = 1; y < aa_height; ++y)
			{
				const auto line_width = y;
				const auto offset = aa_width - line_width;

				aa_texture.fill(y, offset, line_width, white_color);

				const auto p_x = aa_point.x + offset;
				const auto p_y = aa_point.y + y;
				const auto width = line_width;
				constexpr auto height = .5f;

				const auto uv_x = static_cast<point_type::value_type>(p_x) * atlas_uv_scale.width;
				const auto uv_y = static_cast<point_type::value_type>(p_y) * atlas_uv_scale.height;
				const auto uv_width = static_cast<point_type::value_type>(width) * atlas_uv_scale.width;
				const auto uv_height = static_cast<point_type::value_type>(height) * atlas_uv_scale.height;

				shared_data.baked_line_uvs[y] = {uv_x, uv_y, uv_width, uv_height};
			}
		}

		// // ========================================
		// // set fallback glyph
		// // ========================================
		// fonts_.set_fallback_glyph();
	}

	auto TextureContext::root_texture() const noexcept -> texture_id_type
	{
		return root_atlas().id();
	}

	auto TextureContext::atlas_of(const GlyphInfo& info) const noexcept -> const Texture&
	{
		return select_atlas(info.texture_atlas_id);
	}

	auto TextureContext::bind_parser(GlyphParser& parser) noexcept -> void
	{
		fonts_.bind_parser(parser);
	}

	auto TextureContext::add_font(const std::filesystem::path& path) noexcept -> bool
	{
		return fonts_.add_font(path);
	}

	auto TextureContext::load_all_font() noexcept -> void
	{
		fonts_.load_all_font();
	}

	auto TextureContext::set_fallback_glyph() noexcept -> void
	{
		fonts_.set_fallback_glyph();
	}

	auto TextureContext::set_fallback_glyph(const GlyphKey& key) noexcept -> void
	{
		fonts_.set_fallback_glyph(key);
	}

	auto TextureContext::set_fallback_glyph(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> void
	{
		fonts_.set_fallback_glyph(codepoint, size, flag);
	}

	auto TextureContext::glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		return fonts_.glyph_of(key);
	}

	auto TextureContext::glyph_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		return fonts_.glyph_of(codepoint, size, flag);
	}

	auto TextureContext::glyph_of(const std::u32string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		return fonts_.glyph_of(text, size, flag);
	}

	// auto TextureContext::glyph_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	// {
	// 	const auto utf32_text = chars::convert<chars::CharsType::UTF8_CHAR, chars::CharsType::UTF32>(text);
	//
	// 	return this->glyph_of(utf32_text, size, flag);
	// }

	auto TextureContext::glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo&
	{
		return fonts_.glyph_of_or_fallback(key);
	}

	auto TextureContext::glyph_of_or_fallback(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) const noexcept -> const GlyphInfo&
	{
		return fonts_.glyph_of_or_fallback(codepoint, size, flag);
	}

	auto TextureContext::glyph_of_or_fallback(const std::u32string_view text, const std::uint32_t size, const GlyphFlag flag) const noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>
	{
		return fonts_.glyph_of_or_fallback(text, size, flag);
	}

	// auto TextureContext::size_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	// {
	// 	const auto* info = this->glyph_of(codepoint, size, flag);
	//
	// 	if (info == nullptr)
	// 	{
	// 		return {0, 0};
	// 	}
	//
	// 	return {info->advance_x, info->rect.height()};
	// }

	// auto TextureContext::size_of(const std::u32string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	// {
	// 	const auto infos = this->glyph_of(text, size, flag);
	//
	// 	return std::ranges::fold_left(
	// 		infos,
	// 		extent_type{0, 0},
	// 		[](const extent_type total, const auto* info) noexcept -> extent_type
	// 		{
	// 			if (info == nullptr)
	// 			{
	// 				return total;
	// 			}
	//
	// 			return {total.width + info->advance_x, std::ranges::max(total.height, info->rect.height())};
	// 		}
	// 	);
	// }
	//
	// auto TextureContext::size_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	// {
	// 	const auto infos = glyph_of(text, size, flag);
	//
	// 	return std::ranges::fold_left(
	// 		infos,
	// 		extent_type{0, 0},
	// 		[](const extent_type total, const auto* info) noexcept -> extent_type
	// 		{
	// 			if (info == nullptr)
	// 			{
	// 				return total;
	// 			}
	//
	// 			return {total.width + info->advance_x, std::ranges::max(total.height, info->rect.height())};
	// 		}
	// 	);
	// }

	auto TextureContext::load_all_glyph() noexcept -> void
	{
		fonts_.load_all_glyph(*this);
	}

	auto TextureContext::upload_parsed_info_to_texture(const GlyphParser::ParseResult& result) noexcept -> parsed_info_upload_result_type
	{
		const auto width = static_cast<Texture::size_type::value_type>(result.rect.width());
		const auto height = static_cast<Texture::size_type::value_type>(result.rect.height());
		const auto size = Texture::size_type{width, height};

		const auto atlas_id = select_atlas(size);
		const auto& atlas = select_atlas(atlas_id);

		const auto atlas_uv_scale = atlas.uv();

		const auto& texture = make_territory(size);
		texture.fill({result.data.get(), static_cast<std::size_t>(width) * height});

		const auto texture_point = texture.point();
		return {
				.texture_atlas_id = atlas_id,
				.uv = {texture_point.to<point_type>() * atlas_uv_scale, size.to<extent_type>() * atlas_uv_scale}
		};
	}

	auto TextureContext::upload_all_texture(Renderer& renderer) noexcept -> void
	{
		std::ranges::for_each(
			texture_atlases_,
			[&renderer](auto& texture) noexcept -> void
			{
				if (not texture.uploaded())
				{
					texture.create(renderer);
				}
				else
				{
					texture.upload_if_required(renderer);
				}
			}
		);
	}

	RenderContext::~RenderContext() noexcept = default;

	RenderContext::RenderContext() noexcept = default;

	auto RenderContext::initialize() noexcept -> void
	{
		texture_context_.initialize(render_list_shared_data_);
	}

	auto RenderContext::begin_frame(Renderer& renderer) noexcept -> void
	{
		texture_context_.load_all_font();
		texture_context_.upload_all_texture(renderer);
	}
	
	auto RenderContext::end_frame(Renderer& renderer) noexcept -> void
	{
		std::ignore = renderer;
		texture_context_.load_all_glyph();
	}

	auto RenderContext::root_texture() const noexcept -> texture_id_type
	{
		return texture_context_.root_texture();
	}

	auto RenderContext::atlas_of(const GlyphInfo& info) const noexcept -> const Texture&
	{
		return texture_context_.atlas_of(info);
	}

	auto RenderContext::bind_parser(GlyphParser& parser) noexcept -> void
	{
		texture_context_.bind_parser(parser);
	}

	auto RenderContext::add_font(const std::filesystem::path& path) noexcept -> bool
	{
		return texture_context_.add_font(path);
	}

	auto RenderContext::load_all_font() noexcept -> void
	{
		texture_context_.load_all_font();
	}

	auto RenderContext::set_fallback_glyph() noexcept -> void
	{
		texture_context_.set_fallback_glyph();
	}

	auto RenderContext::set_fallback_glyph(const GlyphKey& key) noexcept -> void
	{
		texture_context_.set_fallback_glyph(key);
	}

	auto RenderContext::set_fallback_glyph(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> void
	{
		texture_context_.set_fallback_glyph(codepoint, size, flag);
	}

	auto RenderContext::glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		return texture_context_.glyph_of(key);
	}

	auto RenderContext::glyph_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		return texture_context_.glyph_of(codepoint, size, flag);
	}

	auto RenderContext::glyph_of(const std::u32string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		return texture_context_.glyph_of(text, size, flag);
	}

	// auto RenderContext::glyph_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	// {
	// 	return texture_context_.glyph_of(text, size, flag);
	// }

	auto RenderContext::glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo&
	{
		return texture_context_.glyph_of_or_fallback(key);
	}

	auto RenderContext::glyph_of_or_fallback(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) const noexcept -> const GlyphInfo&
	{
		return texture_context_.glyph_of_or_fallback(codepoint, size, flag);
	}

	auto RenderContext::glyph_of_or_fallback(const std::u32string_view text, const std::uint32_t size, const GlyphFlag flag) const noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>
	{
		return texture_context_.glyph_of_or_fallback(text, size, flag);
	}

	// auto RenderContext::size_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	// {
	// 	return texture_context_.size_of(codepoint, size, flag);
	// }
	//
	// auto RenderContext::size_of(const std::u32string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	// {
	// 	return texture_context_.size_of(text, size, flag);
	// }
	//
	// auto RenderContext::size_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	// {
	// 	return texture_context_.size_of(text, size, flag);
	// }

	auto RenderContext::load_all_glyph() noexcept -> void
	{
		texture_context_.load_all_glyph();
	}

	auto RenderContext::upload_all_texture(Renderer& renderer) noexcept -> void
	{
		texture_context_.upload_all_texture(renderer);
	}

	auto RenderContext::render_list_shared_data() const noexcept -> const RenderListSharedData&
	{
		return render_list_shared_data_;
	}

	auto RenderContext::render_data() const noexcept -> std::vector<RenderData>
	{
		std::vector<RenderData> all_render_data{};
		all_render_data.reserve(render_lists_.size());

		std::ranges::for_each(
			render_lists_,
			[&all_render_data](const auto& render_list) noexcept -> void
			{
				all_render_data.emplace_back(render_list.render_data());
			}
		);

		return all_render_data;
	}

	auto RenderContext::test_render_list() noexcept -> RenderList&
	{
		if (render_lists_.empty())
		{
			render_lists_.emplace_back(RenderListFlag::ANTI_ALIASED_LINE | RenderListFlag::ANTI_ALIASED_LINE_USE_TEXTURE | RenderListFlag::ANTI_ALIASED_FILL, *this);
		}

		return render_lists_.front();
	}
} // namespace gal::prometheus::gfx
