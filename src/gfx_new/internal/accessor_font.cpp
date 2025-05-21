// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx_new/internal/accessor_font.hpp>

#include <gfx_new/internal/renderer_context.hpp>

namespace gal::prometheus::gfx_new
{
	auto FontContext::set_glyph_parser(GlyphParser& parser) noexcept -> GlyphParser*
	{
		return std::exchange(parser_, std::addressof(parser));
	}

	auto FontContext::set_fallback_glyph() noexcept -> void
	{
		if (fallback_glyph_ == nullptr)
		{
			{
				constexpr GlyphKey key{u'\xFFFD', 16u, GlyphFlag::NONE};
				fallback_glyph_ = glyph_of(key);
			}

			if (fallback_glyph_ == nullptr)
			{
				constexpr GlyphKey key{u'?', 16u, GlyphFlag::NONE};
				fallback_glyph_ = glyph_of(key);
			}

			if (fallback_glyph_ == nullptr)
			{
				constexpr GlyphKey key{u' ', 16u, GlyphFlag::NONE};
				fallback_glyph_ = glyph_of(key);
			}
		}
	}

	auto FontContext::add_font(const std::filesystem::path& path) noexcept -> bool
	{
		return font_load_queue_.push(path);
	}

	auto FontContext::load_all_font() noexcept -> void
	{
		const auto loader = [this](Font&& font) noexcept -> void
		{
			font_list_.emplace_back(std::move(font));
		};

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parser_ != nullptr, "Use Renderer::set_glyph_parser to set a valid glyph parser first!");
		font_load_queue_.upload(*parser_, loader);
	}

	auto FontContext::glyph_of(const GlyphKey& key) const noexcept -> const GlyphInfo*
	{
		for (const auto& font: font_list_)
		{
			if (const auto* info = font.get_glyph(key); info != nullptr)
			{
				return info;
			}
		}

		return nullptr;
	}

	auto FontContext::glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) const noexcept -> const GlyphInfo*
	{
		return glyph_of({codepoint, size, flag});
	}

	auto FontContext::glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) const noexcept -> std::vector<const GlyphInfo*>
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

	auto FontContext::glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo*
	{
		for (const auto& font: font_list_)
		{
			if (const auto* info = font.get_glyph(key); info != nullptr)
			{
				return info;
			}
		}

		return fallback_glyph_;
	}

	auto FontContext::glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) const noexcept -> const GlyphInfo*
	{
		return this->glyph_of_or_fallback({codepoint, size, flag});
	}

	auto FontContext::glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag) const noexcept -> std::vector<const GlyphInfo*>
	{
		std::vector<const GlyphInfo*> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of_or_fallback(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto FontContext::glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		if (const auto* info = std::as_const(*this).glyph_of(key); info != nullptr)
		{
			return info;
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parser_ != nullptr);
		for (auto& font: font_list_)
		{
			if (auto result = parser_->parse(font.descriptor.id, key); result.valid())
			{
				auto& queue = glyph_upload_queue_list_[std::addressof(font)];
				auto& inserted_info = font.set_glyph(key, result);

				queue.push(inserted_info, std::move(result));
				return std::addressof(inserted_info);
			}
		}

		return nullptr;
	}

	auto FontContext::glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		return this->glyph_of({codepoint, size, flag});
	}

	auto FontContext::glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
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

	auto FontContext::glyph_of_or_fallback(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		if (const auto* info = glyph_of(key); info != nullptr)
		{
			return info;
		}

		return fallback_glyph_;
	}

	auto FontContext::glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		return this->glyph_of_or_fallback({codepoint, size, flag});
	}

	auto FontContext::glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		std::vector<const GlyphInfo*> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of_or_fallback(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto FontContext::upload_all_glyph(TextureContext& context) noexcept -> void
	{
		std::ranges::for_each(
			glyph_upload_queue_list_ | std::views::values,
			[&context](auto& queue) noexcept -> void
			{
				queue.upload(context);
			}
		);
		glyph_upload_queue_list_.clear();
	}

	Renderer::AccessorFont::AccessorFont(Renderer& renderer) noexcept
		: renderer_{renderer} {}

	auto Renderer::AccessorFont::context() noexcept -> FontContext&
	{
		return renderer_.get().context_->font_context;
	}
}
