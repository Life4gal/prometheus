// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/font.hpp>

#include <fstream>

#include <gfx/context.hpp>

namespace gal::prometheus::gfx
{
	auto FontGlyphQueue::push(GlyphInfo& info, GlyphParser::ParseResult&& result) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result.valid());

		list_.emplace_back(memory::ref(info), std::move(result));
	}

	auto FontGlyphQueue::upload(TextureContext& context) noexcept -> void
	{
		std::ranges::for_each(
			list_,
			[&context](element_type& element) noexcept -> void
			{
				auto& info = element.info.get();
				const auto& result = element.result;

				const auto [texture_atlas_id, uv] = context.upload_parsed_info_to_texture(result);
				info.texture_atlas_id = texture_atlas_id;
				info.uv = uv;
			}
		);
		list_.clear();
	}

	Font::Font(name_type name, const font_id_type id) noexcept
		: name_{std::move(name)},
		  id_{id} {}

	auto Font::name() const noexcept -> std::string_view
	{
		return name_;
	}

	auto Font::id() const noexcept -> font_id_type
	{
		return id_;
	}

	auto Font::get_glyph(const GlyphKey& key) const noexcept -> const GlyphInfo*
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id_ != invalid_font_id);

		if (const auto it = glyphs_.find(key); it != glyphs_.end())
		{
			return std::addressof(it->second);
		}

		return nullptr;
	}

	auto Font::set_glyph(const GlyphKey& key, const GlyphParser::ParseResult& result) noexcept -> GlyphInfo&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id_ != invalid_font_id);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(result.valid());

		GlyphInfo info{};
		info.rect = result.rect;
		info.advance_x = result.advance_x;
		info.visible = result.visible;
		info.colored = result.colored;

		return glyphs_.insert_or_assign(key, info).first->second;
	}

	FontLoadQueue::FontLoadQueue() noexcept
		: new_font_index_{0} {}

	auto FontLoadQueue::push(const std::filesystem::path& path) noexcept -> bool
	{
		std::ifstream file{path, std::ios::binary};
		if (not file.is_open())
		{
			// todo: error handling
			return false;
		}

		file.seekg(0, std::ios::end);
		const auto size = file.tellg();

		auto* data = new font_data_type::element_type[size];
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(data), size);
		file.close();

		list_.emplace_back(std::unique_ptr<font_data_type::element_type>{data}, static_cast<font_data_type::size_type>(size));
		return true;
	}

	auto FontLoadQueue::upload(GlyphParser& parser, const functional::function_reference_wrapper<void(Font&&)> font_dest) noexcept -> void
	{
		std::ranges::for_each(
			std::ranges::subrange{list_.begin() + new_font_index_, list_.end()},
			[&](font_data_type& data) noexcept -> void
			{
				if (auto result = parser.load({data.data.get(), data.size}); result.valid())
				{
					font_dest(Font{std::move(result.name), result.id});
				}
				else
				{
					// todo: error handling
					GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
				}
			}
		);
		new_font_index_ = static_cast<list_type::difference_type>(list_.size());
	}

	Fonts::Fonts() noexcept
		: fallback_glyph_{nullptr},
		  parser_{nullptr} {}

	auto Fonts::bind_parser(GlyphParser& parser) noexcept -> GlyphParser*
	{
		return std::exchange(parser_, std::addressof(parser));
	}

	auto Fonts::add_font(const std::filesystem::path& path) noexcept -> bool
	{
		return font_load_queue_.push(path);
	}

	auto Fonts::load_all_font() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parser_ != nullptr);

		const auto loader = [this](Font&& font) noexcept -> void
		{
			font_list_.emplace_back(std::move(font));
		};

		font_load_queue_.upload(*parser_, loader);
	}

	auto Fonts::set_fallback_glyph() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not font_list_.empty());

		if (fallback_glyph_ == nullptr)
		{
			constexpr GlyphKey key{.codepoint = u'\xFFFD', .size = 16u, .flag = GlyphFlag::NONE};
			set_fallback_glyph(key);
		}

		if (fallback_glyph_ == nullptr)
		{
			constexpr GlyphKey key{.codepoint = u'?', .size = 16u, .flag = GlyphFlag::NONE};
			set_fallback_glyph(key);
		}

		if (fallback_glyph_ == nullptr)
		{
			constexpr GlyphKey key{.codepoint = u' ', .size = 16u, .flag = GlyphFlag::NONE};
			set_fallback_glyph(key);
		}

		if (fallback_glyph_ == nullptr)
		{
			// todo: error handling
			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
		}
	}

	auto Fonts::set_fallback_glyph(const GlyphKey& key) noexcept -> void
	{
		fallback_glyph_ = this->glyph_of(key);
	}

	auto Fonts::set_fallback_glyph(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> void
	{
		fallback_glyph_ = this->glyph_of(codepoint, size, flag);
	}

	auto Fonts::glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		for (const auto& font: font_list_)
		{
			if (const auto* info = font.get_glyph(key); info != nullptr)
			{
				return info;
			}
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parser_ != nullptr);

		for (auto& font: font_list_)
		{
			if (auto result = parser_->parse(font.id(), key); result.valid())
			{
				auto& queue = font_glyph_queue_[std::addressof(font)];
				auto& inserted_info = font.set_glyph(key, result);

				queue.push(inserted_info, std::move(result));
				return std::addressof(inserted_info);
			}
		}

		// todo: error handling
		return nullptr;
	}

	auto Fonts::glyph_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		return this->glyph_of({.codepoint = codepoint, .size = size, .flag = flag});
	}

	auto Fonts::glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		std::vector<const GlyphInfo*> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto Fonts::glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo&
	{
		for (const auto& font: font_list_)
		{
			if (const auto* info = font.get_glyph(key); info != nullptr)
			{
				return *info;
			}
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(fallback_glyph_ != nullptr);
		return *fallback_glyph_;
	}

	auto Fonts::glyph_of_or_fallback(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) const noexcept -> const GlyphInfo&
	{
		return this->glyph_of_or_fallback({.codepoint = codepoint, .size = size, .flag = flag});
	}

	auto Fonts::glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag) const noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>
	{
		std::vector<std::reference_wrapper<const GlyphInfo>> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto& info = this->glyph_of_or_fallback(codepoint, size, flag);
				infos.emplace_back(std::ref(info));
			}
		);

		return infos;
	}

	auto Fonts::load_all_glyph(TextureContext& context) noexcept -> void
	{
		std::ranges::for_each(
			font_glyph_queue_ | std::views::values,
			[&context](auto& queue) noexcept -> void
			{
				queue.upload(context);
			}
		);
		font_glyph_queue_.clear();
	}
} // namespace gal::prometheus::gfx
