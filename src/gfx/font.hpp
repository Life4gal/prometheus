// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>
#include <span>
#include <filesystem>

#include <gfx/type.hpp>
#include <gfx/draw_list_shared_data.hpp>

#include <functional/enumeration.hpp>
#include <memory/unique_ptr.hpp>
#include <memory/reference_wrapper.hpp>

namespace gal::prometheus::gfx
{
	/**
	 * @brief Texture atlas uploaded to the GPU
	 */
	class TextureAtlas final
	{
	public:
		using value_type = std::uint32_t;
		using point_type = primitive::basic_point_2d<value_type>;
		using size_type = primitive::basic_extent_2d<value_type>;

		using uv_scale_type = extent_type;

		// size.width * size.height (RGBA)
		using data_type = std::unique_ptr<std::uint32_t[]>;
		using data_view_type = std::span<const std::uint32_t>;

		constexpr static point_type invalid_point{(std::numeric_limits<value_type>::max)(), (std::numeric_limits<value_type>::max)()};

	private:
		struct pack_context_type;
		memory::UniquePointer<pack_context_type> pack_context_;

		// Size of texture atlas
		size_type size_;
		// UV scale of texture atlas (1.0f / size.width, 1.0f / size.height)
		uv_scale_type uv_scale_;
		// Texture atlas data (CPU side)
		data_type data_;

		static_assert(sizeof(texture_id_type) == sizeof(std::uint64_t));
		// Does this texture need to be updated (re-uploaded)
		mutable std::uint64_t dirty_ : 1;
		// GPU resource handle
		mutable std::uint64_t texture_id_ : 63;

	public:
		TextureAtlas(value_type width, value_type height) noexcept;

		auto build(Renderer& renderer) noexcept -> void;
		auto destroy(Renderer& renderer) noexcept -> void;

		/**
		 * @brief Texture atlas size
		 */
		[[nodiscard]] auto size() const noexcept -> size_type;

		/**
		 * @brief Texture atlas uv scale (1.0f / size.width, 1.0f / size.height)
		 */
		[[nodiscard]] auto uv_scale() const noexcept -> uv_scale_type;

		/**
		 * @brief Texture atlas data (CPU side)
		 */
		[[nodiscard]] auto data() const noexcept -> data_view_type;

		/**
		 * @brief Does this texture atlas is valid (uploaded to GPU)
		 */
		[[nodiscard]] auto valid() const noexcept -> bool;

		/**
		 * @brief Does this texture atlas need to be re-uploaded to the GPU
		 */
		[[nodiscard]] auto dirty() const noexcept -> bool;

		/**
		 * @brief Texture atlas id (usually a GPU resource handle)
		 */
		[[nodiscard]] auto id() const noexcept -> texture_id_type;

		/**
		 * @brief Find a region that can hold a (piece of) texture of @c size
		 * @param size texture size
		 * @return texture coordinate
		 * @note If such a region is not found, @c invalid_point is returned
		 */
		[[nodiscard]] auto seek(size_type size) noexcept -> point_type;

		/**
		 * @brief Write a (piece of) texture @c data of @c size at the specified @c point of the current texture
		 * @param point texture coordinate
		 * @param size texture size
		 * @param data texture data
		 * @note Do not check the length of the @c data, assume it is at least @c size.width * @c size.height
		 */
		auto write(point_type point, size_type size, data_view_type data) noexcept -> void;

		/**
		 * @brief Write a (piece of) texture @c data of @c size at the specified @c point of the current texture
		 * @param point texture coordinate
		 * @param size texture size
		 * @param data texture data
		 * @note Do not check the length of the @c data, assume it is at least @c size.width * @c size.height
		 */
		auto write(point_type point, size_type size, const data_type& data) noexcept -> void;
	};

	enum class GlyphFlag : std::uint8_t
	{
		NONE = 0,
		BOLD = 1 << 0,
		ITALIC = 1 << 1,
	};

	class GlyphKey final
	{
	public:
		std::uint32_t codepoint;
		std::uint32_t size : 26;
		GlyphFlag flag : 6;

		[[nodiscard]] constexpr auto operator==(const GlyphKey& other) const noexcept -> bool
		{
			return codepoint == other.codepoint and size == other.size and flag == other.flag;
		}

		struct hasher
		{
			[[nodiscard]] auto operator()(const GlyphKey& key) const noexcept -> std::size_t
			{
				return
						std::hash<std::uint32_t>{}(key.codepoint) ^
						std::hash<std::uint32_t>{}(key.size) ^
						std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(key.flag));
			}
		};
	};

	/**
	 * @brief Information about a glyph
	 */
	class GlyphInfo final
	{
	public:
		using value_type = extent_type::value_type;
		using uv_type = primitive::basic_rect_2d<uv_type::value_type>;

		// =============
		// Data filled when loading glyph
		// =============

		// Bitmap infos of this glyph
		rect_type rect;
		value_type advance_x;
		bool visible;
		bool colored;

		// =============
		// Data filled when writing texture
		// =============

		// The id of the texture atlas where the glyph is located
		// This id is present if and only if the glyph is in a texture atlas, otherwise it is invalid_texture_atlas_id
		texture_atlas_id_type texture_atlas_id;
		uv_type uv;
	};

	/**
	 * @brief Write @c data to texture according to @c info, and set texture_atlas_id and uv of @c info
	 */
	class GlyphUploadInfo final
	{
	public:
		memory::RefWrapper<GlyphInfo> info;
		TextureAtlas::data_type data;
	};

	/**
	 * @brief Getting glyph data from (binary) font data
	 */
	class GlyphParser
	{
	public:
		using binary_data_type = std::span<std::uint8_t>;

		class [[nodiscard]] LoadResult final
		{
		public:
			std::string name;
			font_id_type id;

			[[nodiscard]] explicit operator bool() const noexcept
			{
				return id != invalid_font_id;
			}

			[[nodiscard]] auto valid() const noexcept -> bool
			{
				return operator bool();
			}
		};

		class [[nodiscard]] ParseResult final
		{
		public:
			GlyphInfo info;
			TextureAtlas::data_type data;

			[[nodiscard]] explicit operator bool() const noexcept
			{
				return data == nullptr;
			}

			[[nodiscard]] auto valid() const noexcept -> bool
			{
				return operator bool();
			}
		};

		GlyphParser(const GlyphParser&) noexcept = delete;
		GlyphParser(GlyphParser&&) noexcept = default;
		auto operator=(const GlyphParser&) noexcept -> GlyphParser& = delete;
		auto operator=(GlyphParser&&) noexcept -> GlyphParser& = default;

		virtual ~GlyphParser() noexcept;

		GlyphParser() noexcept = default;

		[[nodiscard]] virtual auto initialize() noexcept -> bool = 0;

		/**
		 * @brief Load the font data, get all its glyph data, return the id of the font
		 */
		[[nodiscard]] virtual auto load(binary_data_type data) noexcept -> LoadResult = 0;

		/**
		 * @brief Determines whether the target font contains the glyphs of the specified codepoint
		 * @param id The id returned by loading the font from the previous load
		 * @param codepoint The codepoint of the glyph to be checked
		 * @return Exists or not
		 */
		[[nodiscard]] virtual auto has_glyph(font_id_type id, std::uint32_t codepoint) const noexcept -> bool = 0;

		/**
		 * @brief Get the glyph information of the specified size (and style) of the target codepoint, if it does not exist then data is a null pointer
		 * @param id The id returned by loading the font from the previous load
		 * @param key {codepoint, size, flag}
		 * @return the glyph information of the specified size (and style) of the target codepoint
		 */
		[[nodiscard]] virtual auto parse(font_id_type id, const GlyphKey& key) noexcept -> ParseResult = 0;

		/**
		 * @brief Get the glyph information of the specified size (and style) of the target codepoint, if it does not exist then data is a null pointer
		 * @param id The id returned by loading the font from the previous load
		 * @param codepoint The codepoint of the glyph
		 * @param size The size of the glyph
		 * @param flag The style of the glyph (bold, italic, etc.)
		 * @return the glyph information of the specified size (and style) of the target codepoint
		 */
		[[nodiscard]] auto parse(font_id_type id, std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> ParseResult;
	};

	/**
	 * @brief All the glyph data used in a font
	 */
	class FontFace final
	{
	public:
		using value_type = extent_type::value_type;
		using uv_type = primitive::basic_rect_2d<value_type>;

	private:
		memory::RefWrapper<TextureContext> context_;

		std::string name_;
		font_id_type id_;

		std::vector<GlyphUploadInfo> upload_queue_;

		std::unordered_map<GlyphKey, GlyphInfo, GlyphKey::hasher> glyphs_;
		const GlyphInfo* fallback_glyph_;

		[[nodiscard]] auto find_or_parse_glyph(const GlyphKey& key) noexcept -> GlyphInfo*;

	public:
		FontFace(const FontFace&) noexcept = delete;
		FontFace(FontFace&&) noexcept = default;
		auto operator=(const FontFace&) noexcept -> FontFace& = delete;
		auto operator=(FontFace&&) noexcept -> FontFace& = default;

		~FontFace() noexcept = default;

		FontFace(TextureContext& context, std::string name, font_id_type id) noexcept;

		FontFace(TextureContext& context, std::string_view name, font_id_type id) noexcept;

		/**
		 * @brief font name (obtained on GlyphParser::load)
		 */
		[[nodiscard]] auto name() const noexcept -> std::string_view;

		/**
		 * @brief font id (obtained on GlyphParser::load)
		 */
		[[nodiscard]] auto id() const noexcept -> font_id_type;

		/**
		 * @brief Initialization, usually used to load default glyph data (for fallbacks when the desired glyph is not found)
		 */
		auto initialize() noexcept -> void;

		/**
		 * @brief 
		 */
		auto begin_frame() noexcept -> void;

		/**
		 * @brief Upload the glyph data used in this frame and not uploaded to the texture before to the texture
		 */
		auto end_frame() noexcept -> void;

		/**
		 * @brief Get the glyph information of the specified codepoint, if it can't be found, then return the fallback glyph information
		 * @param key {codepoint, size, flag}
		 * @return The glyph information of the specified codepoint, or the fallback glyph information if it can't be found
		 */
		[[nodiscard]] auto find_glyph(const GlyphKey& key) noexcept -> const GlyphInfo&;

		/**
		 * @brief Get the glyph information of the specified codepoint, if it can't be found, then return a null pointer
		 * @param key {codepoint, size, flag}
		 * @return The glyph information of the specified codepoint, or a null pointer if it can't be found
		 */
		[[nodiscard]] auto find_glyph_no_fallback(const GlyphKey& key) noexcept -> const GlyphInfo*;
	};

	class TextureContext final
	{
	public:
		using texture_atlases_type = std::vector<TextureAtlas>;
		using font_faces_type = std::vector<FontFace>;

	private:
		class Territory final
		{
		public:
			using value_type = TextureAtlas::value_type;
			using point_type = TextureAtlas::point_type;
			using size_type = TextureAtlas::size_type;

			texture_atlas_id_type id{invalid_texture_atlas_id};

			point_type point{0, 0};
			size_type size{0, 0};
		};

		using territories_type = std::vector<Territory>;

		class FontFaceTask final
		{
		public:
			using value_type = GlyphParser::binary_data_type::element_type;

			std::unique_ptr<value_type[]> data;
			std::size_t size;
		};

		using font_face_tasks_type = std::vector<FontFaceTask>;

		GlyphParser* parser_;

		texture_atlases_type texture_atlases_;
		font_faces_type font_faces_;
		territories_type territories_;
		font_face_tasks_type font_face_tasks_;

		/**
		 * @brief Get the texture atlas for the specified id
		 * @param id texture atlas id
		 * @return texture atlas
		 */
		[[nodiscard]] auto select_atlas(texture_atlas_id_type id) noexcept -> TextureAtlas&;

		/**
		 * @brief Gets a texture atlas large enough to hold the specified @c size sub texture
		 * @param size sub texture size
		 * @return texture atlas id
		 */
		[[nodiscard]] auto select_atlas(TextureAtlas::size_type size) const noexcept -> texture_atlas_id_type;

		/**
		 * @brief Gets a texture atlas large enough to hold the specified @c {width, height} sub texture
		 * @param width sub texture width
		 * @param height sub texture height
		 * @return texture atlas id
		 */
		[[nodiscard]] auto select_atlas(TextureAtlas::value_type width, TextureAtlas::value_type height) const noexcept -> texture_atlas_id_type;

		/**
		 * @brief Find a suitable texture atlas based on @c size, and then find a suitable region on it (for writing sub texture)
		 * @param size sub texture size
		 * @return region on the texture atlas
		 */
		auto make_territory(TextureAtlas::size_type size) noexcept -> Territory&;

		/**
		 * @brief Find a suitable texture atlas based on @c {width, height}, and then find a suitable region on it (for writing sub texture)
		 * @param width sub texture width
		 * @param height sub texture height
		 * @return region on the texture atlas
		 */
		auto make_territory(TextureAtlas::value_type width, TextureAtlas::value_type height) noexcept -> Territory&;

	public:
		TextureContext(const TextureContext&) noexcept = delete;
		TextureContext(TextureContext&&) noexcept = default;
		auto operator=(const TextureContext&) noexcept -> TextureContext& = delete;
		auto operator=(TextureContext&&) noexcept -> TextureContext& = default;
		~TextureContext() noexcept = default;

		TextureContext() noexcept;

		/**
		 * @brief Initialization, usually used to set up DrawListSharedData's anti-aliased lines (uv) and initialize all font faces
		 */
		auto initialize(DrawListSharedData& shared_data) noexcept -> void;

		/**
		 * @brief Load all fonts to be loaded (and not loaded)
		 */
		auto begin_frame() noexcept -> void;

		/**
		 * @brief font faces -> end_frame
		 */
		auto end_frame() noexcept -> void;

		/**
		 * @brief Bind parser, default parser is null pointer, must bind parser before loading fonts
		 */
		auto bind_parser(GlyphParser& parser) noexcept -> void;

		/**
		 * @brief Get the current parser, usually used by FontFace to load glyph data
		 */
		[[nodiscard]] auto parser() const noexcept -> GlyphParser&;

		/**
		 * @brief Load fonts from the specified path, assuming the path is a valid font file
		 * @param path font path
		 */
		auto add_font(const std::filesystem::path& path) noexcept -> void;

		/**
		 * @brief Load fonts from the specified path, assuming the path is a valid font file
		 * @param path font path
		 */
		auto add_font(std::string_view path) noexcept -> void;

		/**
		 * @brief Write FontFace uploaded glyph data to texture, also set the texture atlas id and uv coordinates for this glyph data
		 */
		auto upload_glyph(GlyphUploadInfo& upload_info) noexcept -> void;

		[[nodiscard]] auto glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> const GlyphInfo*;
		[[nodiscard]] auto glyph_of(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> std::vector<const GlyphInfo*>;

		/**
		 * @brief The minimum space to be occupied if the specified codepoint is to be rendered in its entirety
		 * @param codepoint 
		 * @param size 
		 * @param flag 
		 * @return 
		 */
		[[nodiscard]] auto size_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;

		/**
		 * @brief The minimum space to be occupied if the specified text is to be rendered in its entirety
		 * @param text 
		 * @param size 
		 * @param flag 
		 * @return 
		 */
		[[nodiscard]] auto size_of(std::string_view text, std::uint32_t size, GlyphFlag flag = GlyphFlag::NONE) noexcept -> extent_type;
	};
} // namespace gal::prometheus::gfx

// ReSharper disable once CppRedundantNamespaceDefinition
namespace gal::prometheus::meta::user_defined
{
	template<>
	struct enum_is_flag<gfx::GlyphFlag> : std::true_type {};
}
