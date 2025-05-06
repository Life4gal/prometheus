// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/font.hpp>

#include <fstream>

#include <gfx/renderer.hpp>
#include <chars/chars.hpp>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

namespace gal::prometheus::gfx
{
	struct TextureAtlas::pack_context_type
	{
		stbrp_context context;
		std::vector<stbrp_node> nodes;
	};

	TextureAtlas::TextureAtlas(const value_type width, const value_type height) noexcept
		: pack_context_{memory::make_unique<pack_context_type>()},
		  size_{width, height},
		  uv_scale_{1.f / static_cast<uv_scale_type::value_type>(width), 1.f / static_cast<uv_scale_type::value_type>(height)},
		  data_{std::make_unique_for_overwrite<std::uint32_t[]>(static_cast<std::size_t>(width) * height)},
		  dirty_{false},
		  texture_id_{invalid_texture_id}
	{
		pack_context_->nodes.resize(width);

		stbrp_init_target(
			&pack_context_->context,
			static_cast<stbrp_coord>(width),
			static_cast<stbrp_coord>(height),
			pack_context_->nodes.data(),
			static_cast<int>(pack_context_->nodes.size())
		);
	}

	auto TextureAtlas::build(Renderer& renderer) noexcept -> void
	{
		// todo
		std::ignore = renderer;
	}

	auto TextureAtlas::destroy(Renderer& renderer) noexcept -> void
	{
		data_.reset();
		dirty_ = false;
		renderer.destroy_texture(texture_id_);
	}

	auto TextureAtlas::valid() const noexcept -> bool
	{
		return texture_id_ != invalid_texture_id;
	}

	auto TextureAtlas::dirty() const noexcept -> bool
	{
		return dirty_;
	}

	auto TextureAtlas::id() const noexcept -> texture_id_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		return texture_id_;
	}

	auto TextureAtlas::size() const noexcept -> size_type
	{
		return size_;
	}

	auto TextureAtlas::uv_scale() const noexcept -> uv_scale_type
	{
		return uv_scale_;
	}

	auto TextureAtlas::data() const noexcept -> data_view_type
	{
		const auto length = size_.width * size_.height;

		return {data_.get(), length};
	}

	auto TextureAtlas::seek(const size_type size) noexcept -> point_type
	{
		stbrp_rect rect
		{
				.id = -1,
				.w = static_cast<stbrp_coord>(size.width),
				.h = static_cast<stbrp_coord>(size.height),
				.x = 0,
				.y = 0,
				.was_packed = 0
		};

		if (stbrp_pack_rects(&pack_context_->context, &rect, 1))
		{
			return {static_cast<point_type::value_type>(rect.x), static_cast<point_type::value_type>(rect.y)};
		}

		return invalid_point;
	}

	auto TextureAtlas::write(const point_type point, const size_type size, const data_view_type data) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(point.x + size.width <= size_.width);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(point.y + size.height <= size_.height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.data() != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.size() >= (static_cast<std::size_t>(size.width) * size.height));

		for (value_type y = 0; y < size.height; ++y)
		{
			const auto offset_y = (point.y + y) * size_.width;

			for (value_type x = 0; x < size.width; ++x)
			{
				const auto offset_x = point.x + x;
				const auto index = offset_x + offset_y;

				data_[index] = data[y * size.width + x];
			}
		}

		dirty_ = true;
	}

	auto TextureAtlas::write(const point_type point, const size_type size, const data_type& data) noexcept -> void
	{
		return write(point, size, {data.get(), static_cast<std::size_t>(size.width) * size.height});
	}

	GlyphParser::~GlyphParser() noexcept = default;

	auto GlyphParser::parse(const font_id_type id, const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> ParseResult
	{
		return this->parse(id, {.codepoint = codepoint, .size = size, .flag = flag});
	}

	auto FontFace::find_or_parse_glyph(const GlyphKey& key) noexcept -> GlyphInfo*
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id_ != invalid_font_id);

		if (const auto it = glyphs_.find(key);
			it != glyphs_.end())
		{
			return std::addressof(it->second);
		}

		auto& parser = context_.get().parser();
		if (not parser.has_glyph(id_, key.codepoint))
		{
			return nullptr;
		}

		auto result = parser.parse(id_, key);
		if (not result.valid())
		{
			return nullptr;
		}

		const auto [it, inserted] = glyphs_.emplace(key, result.info);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(inserted);
		upload_queue_.emplace_back(memory::ref(it->second), std::move(result.data));

		return std::addressof(it->second);
	}

	FontFace::FontFace(TextureContext& context, std::string name, const font_id_type id) noexcept
		: context_{context},
		  name_{std::move(name)},
		  id_{id},
		  fallback_glyph_{nullptr} {}

	FontFace::FontFace(TextureContext& context, const std::string_view name, const font_id_type id) noexcept
		: FontFace{context, std::string{name}, id} {}

	auto FontFace::name() const noexcept -> std::string_view
	{
		return name_;
	}

	auto FontFace::id() const noexcept -> font_id_type
	{
		return id_;
	}

	auto FontFace::initialize() noexcept -> void
	{
		GlyphKey key{.codepoint = u'\xFFFD', .size = 16u, .flag = GlyphFlag::NONE};
		fallback_glyph_ = find_glyph_no_fallback(key);

		if (fallback_glyph_ == nullptr)
		{
			key.codepoint = u'?';
			fallback_glyph_ = find_glyph_no_fallback(key);
		}
		if (fallback_glyph_ == nullptr)
		{
			key.codepoint = u' ';
			fallback_glyph_ = find_glyph_no_fallback(key);
		}

		if (fallback_glyph_ == nullptr)
		{
			// todo
			GAL_PROMETHEUS_ERROR_DEBUG_UNREACHABLE();
		}
	}

	auto FontFace::begin_frame() noexcept -> void
	{
		std::ignore = this;
	}

	auto FontFace::end_frame() noexcept -> void
	{
		std::ranges::for_each(
			upload_queue_,
			[&context = context_.get()](GlyphUploadInfo& upload_info) noexcept -> void
			{
				const auto& info = upload_info.info.get();

				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(info.texture_atlas_id == invalid_texture_atlas_id);
				context.upload_glyph(upload_info);
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(info.texture_atlas_id != invalid_texture_atlas_id);
			}
		);
		upload_queue_.clear();
	}

	auto FontFace::find_glyph(const GlyphKey& key) noexcept -> const GlyphInfo&
	{
		if (const auto* info = find_or_parse_glyph(key);
			info)
		{
			return *info;
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(fallback_glyph_ != nullptr);
		return *fallback_glyph_;
	}

	auto FontFace::find_glyph_no_fallback(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		if (const auto* info = find_or_parse_glyph(key); info)
		{
			return info;
		}

		return nullptr;
	}

	auto TextureContext::select_atlas(const texture_atlas_id_type id) noexcept -> TextureAtlas&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id < texture_atlases_.size());

		return texture_atlases_[id];
	}

	auto TextureContext::select_atlas(const TextureAtlas::size_type size) const noexcept -> texture_atlas_id_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size.width > 0 and size.height > 0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture_atlases_.empty());

		// todo: @see TextureContext::TextureContext
		std::ignore = size;

		return 0;
	}

	auto TextureContext::select_atlas(const TextureAtlas::value_type width, const TextureAtlas::value_type height) const noexcept -> texture_atlas_id_type
	{
		return select_atlas({width, height});
	}

	auto TextureContext::make_territory(const TextureAtlas::size_type size) noexcept -> Territory&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size.width > 0 and size.height > 0);

		const auto atlas_id = select_atlas(size);
		auto& atlas = select_atlas(atlas_id);

		const auto point = atlas.seek(size);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(point != TextureAtlas::invalid_point);

		Territory territory{.id = atlas_id, .point = point, .size = size};

		return territories_.emplace_back(territory);
	}

	auto TextureContext::make_territory(const TextureAtlas::value_type width, const TextureAtlas::value_type height) noexcept -> Territory&
	{
		return make_territory({width, height});
	}

	TextureContext::TextureContext() noexcept
		: parser_{nullptr}
	{
		// todo: @see select_atlas
		texture_atlases_.emplace_back(2048, 2048);
	}

	auto TextureContext::initialize(DrawListSharedData& shared_data) noexcept -> void
	{
		// ========================================
		// BAKE LINES (AA)
		// ========================================
		{
			constexpr std::uint32_t white_color = 0xff'ff'ff'ff;

			const auto atlas_id = select_atlas(DrawListSharedData::baked_line_uv_count, DrawListSharedData::baked_line_uv_count);
			auto& atlas = select_atlas(atlas_id);

			const auto atlas_uv_scale = atlas.uv_scale();

			const auto& aa = make_territory(DrawListSharedData::baked_line_uv_count, DrawListSharedData::baked_line_uv_count);

			// baked line rect area:
			// white pixel
			// ◿
			// auto data = std::make_unique_for_overwrite<TextureAtlas::data_type::element_type[]>(DrawListSharedData::baked_line_uv_count * DrawListSharedData::baked_line_uv_count);
			// auto data = std::make_unique<TextureAtlas::data_type::element_type[]>(DrawListSharedData::baked_line_uv_count * DrawListSharedData::baked_line_uv_count);
			TextureAtlas::data_type::element_type data[DrawListSharedData::baked_line_uv_count * DrawListSharedData::baked_line_uv_count];

			// white pixel
			{
				data[0 + 0] = white_color;
				data[0 + 1] = white_color;
				data[aa.size.width + 0] = white_color;
				data[aa.size.width + 1] = white_color;

				const auto uv_x = static_cast<point_type::value_type>(static_cast<float>(aa.point.x) + .5f) * atlas_uv_scale.width;
				const auto uv_y = static_cast<point_type::value_type>(static_cast<float>(aa.point.y) + .5f) * atlas_uv_scale.height;

				shared_data.white_pixel_uv = {uv_x, uv_y};
			}

			// ◿
			for (std::uint32_t y = 0; y < aa.size.height; ++y)
			{
				const auto line_width = y;

				for (std::uint32_t x = line_width; x > 0; --x)
				{
					const auto index = aa.size.width - x;

					data[index] = white_color;
				}

				const auto p_x = aa.point.x + (aa.size.width - line_width);
				const auto p_y = aa.point.y + y;
				const auto width = line_width;
				constexpr auto height = .5f;

				const auto uv_x = static_cast<point_type::value_type>(p_x) * atlas_uv_scale.width;
				const auto uv_y = static_cast<point_type::value_type>(p_y) * atlas_uv_scale.height;
				const auto uv_width = static_cast<point_type::value_type>(width) * atlas_uv_scale.width;
				const auto uv_height = static_cast<point_type::value_type>(height) * atlas_uv_scale.height;

				shared_data.baked_line_uvs[y] = {uv_x, uv_y, uv_width, uv_height};
			}

			// write texture
			atlas.write(aa.point, aa.size, data);
		}

		// ========================================
		// FontFace (load fallback glyph)
		// ========================================
		std::ranges::for_each(
			font_faces_,
			[](auto& font_face) noexcept -> void
			{
				font_face.initialize();
			}
		);
	}

	auto TextureContext::begin_frame() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parser_ != nullptr);

		std::ranges::for_each(
			font_face_tasks_,
			[this](const FontFaceTask& task) noexcept -> void
			{
				if (auto result = parser_->load({task.data.get(), task.size});
					result.valid())
				{
					font_faces_.emplace_back(*this, std::move(result.name), result.id);
				}
				else
				{
					// todo: error handling
				}
			}
		);
		font_face_tasks_.clear();

		std::ranges::for_each(font_faces_, &FontFace::begin_frame);
	}

	auto TextureContext::end_frame() noexcept -> void
	{
		std::ranges::for_each(font_faces_, &FontFace::end_frame);
	}

	auto TextureContext::bind_parser(GlyphParser& parser) noexcept -> void
	{
		parser_ = std::addressof(parser);
	}

	auto TextureContext::parser() const noexcept -> GlyphParser&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parser_ != nullptr);
		return *parser_;
	}

	auto TextureContext::add_font(const std::filesystem::path& path) noexcept -> void
	{
		std::ifstream file{path, std::ios::binary};
		if (not file.is_open())
		{
			// todo: error handling
			return;
		}

		file.seekg(0, std::ios::end);
		const auto size = file.tellg();

		auto data = std::make_unique_for_overwrite<FontFaceTask::value_type[]>(size);
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(data.get()), size);
		file.close();

		font_face_tasks_.emplace_back(std::move(data), size);
	}

	auto TextureContext::add_font(const std::string_view path) noexcept -> void
	{
		add_font(std::filesystem::path{path});
	}

	auto TextureContext::upload_glyph(GlyphUploadInfo& upload_info) noexcept -> void
	{
		auto& info = upload_info.info.get();
		const auto& data = upload_info.data;

		const auto width = static_cast<Territory::value_type>(info.rect.width());
		const auto height = static_cast<Territory::value_type>(info.rect.height());

		const auto atlas_id = select_atlas(width, height);
		auto& atlas = select_atlas(atlas_id);

		const auto atlas_uv_scale = atlas.uv_scale();

		const auto& territory = make_territory(width, height);

		info.texture_atlas_id = atlas_id;
		info.uv.point = territory.point.to<point_type>() * atlas_uv_scale;
		info.uv.extent = territory.size.to<extent_type>() * atlas_uv_scale;

		atlas.write(territory.point, territory.size, data);
	}

	auto TextureContext::glyph_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		for (auto& face: font_faces_)
		{
			if (const auto* info = face.find_glyph_no_fallback({.codepoint = codepoint, .size = size, .flag = flag});
				info != nullptr)
			{
				return info;
			}
		}

		return nullptr;
	}

	auto TextureContext::glyph_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		const auto utf32_text = chars::convert<chars::CharsType::UTF8_CHAR, chars::CharsType::UTF32>(text);

		std::vector<const GlyphInfo*> infos;
		infos.reserve(utf32_text.size());

		std::ranges::for_each(
			utf32_text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto TextureContext::size_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	{
		const auto* info = glyph_of(codepoint, size, flag);
		if (info == nullptr)
		{
			return {0, 0};
		}

		return {info->advance_x, info->rect.height()};
	}

	auto TextureContext::size_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	{
		const auto infos = glyph_of(text, size, flag);

		extent_type total_size{0, 0};
		std::ranges::for_each(
			infos,
			[&total_size](const auto* info) noexcept -> void
			{
				if (info == nullptr)
				{
					return;
				}

				total_size.width += info->advance_x;
				total_size.height = std::max(total_size.height, info->rect.height());
			}
		);

		return total_size;
	}
}
