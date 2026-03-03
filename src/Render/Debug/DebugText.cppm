module;

#include <cstdint>

export module core:debug_text;

import std;

export namespace rendern::debugText
{
	enum class TextAlignH : std::uint8_t
	{
		Left,
		Center,
		Right,
	};

	enum class TextAlignV : std::uint8_t
	{
		Top,
		Middle,
		Bottom,
	};

	struct TextExtentPx
	{
		float width{ 0.0f };
		float height{ 0.0f };
	};

	// Packs 0..255 components into a uint32 that matches R8G8B8A8_UNORM.
	// Memory layout on little-endian is: RR GG BB AA.
	constexpr std::uint32_t PackRGBA8(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) noexcept
	{
		return static_cast<std::uint32_t>(r)
			| (static_cast<std::uint32_t>(g) << 8)
			| (static_cast<std::uint32_t>(b) << 16)
			| (static_cast<std::uint32_t>(a) << 24);
	}

	struct DebugTextItem
	{
		float xPx{ 0.0f };
		float yPx{ 0.0f };
		float scale{ 1.0f };
		std::uint32_t rgba{ 0xffffffffu };
		std::string text{};
	};

	struct DebugTextList
	{
		std::vector<DebugTextItem> items;

		void Clear()
		{
			items.clear();
		}

		void Reserve(std::size_t itemCount)
		{
			items.reserve(itemCount);
		}

		std::size_t ItemCount() const noexcept
		{
			return items.size();
		}

		bool Empty() const noexcept
		{
			return items.empty();
		}

		static constexpr float kGlyphCols = 5.0f;
		static constexpr float kGlyphRows = 7.0f;
		static constexpr float kGlyphAdvanceCols = 6.0f; // 5 cols + 1 spacing
		static constexpr float kLineAdvanceRows = 8.0f;  // 7 rows + 1 spacing

		static TextExtentPx MeasureTextPx(std::string_view text, float scale)
		{
			TextExtentPx e{};
			if (text.empty())
			{
				return e;
			}

			const float cell = std::max(0.25f, scale);
			std::size_t maxCols = 0;
			std::size_t curCols = 0;
			std::size_t lines = 1;

			for (char c : text)
			{
				if (c == '\n')
				{
					maxCols = std::max(maxCols, curCols);
					curCols = 0;
					++lines;
					continue;
				}
				++curCols;
			}
			maxCols = std::max(maxCols, curCols);

			if (maxCols == 0)
			{
				return e;
			}

			// For N characters: total advance is N*6*cell, but last character does not need trailing spacing.
			e.width = static_cast<float>(maxCols) * kGlyphAdvanceCols * cell - 1.0f * cell;
			e.height = static_cast<float>(lines) * kLineAdvanceRows * cell - 1.0f * cell;
			return e;
		}

		void AddTextPx(float xPx, float yPx, std::string_view text, std::uint32_t rgba = 0xffffffffu, float scale = 1.0f)
		{
			if (text.empty())
			{
				return;
			}

			DebugTextItem it{};
			it.xPx = xPx;
			it.yPx = yPx;
			it.scale = std::max(0.25f, scale);
			it.rgba = rgba;
			it.text = std::string(text);
			items.push_back(std::move(it));
		}

		void AddTextAlignedPx(float anchorXPx, float anchorYPx, std::string_view text,
			TextAlignH alignH, TextAlignV alignV,
			std::uint32_t rgba = 0xffffffffu, float scale = 1.0f)
		{
			if (text.empty())
			{
				return;
			}

			const float cell = std::max(0.25f, scale);
			const TextExtentPx ext = MeasureTextPx(text, cell);

			float x = anchorXPx;
			float y = anchorYPx;

			switch (alignH)
			{
			case TextAlignH::Left:   break;
			case TextAlignH::Center: x -= ext.width * 0.5f; break;
			case TextAlignH::Right:  x -= ext.width; break;
			}

			switch (alignV)
			{
			case TextAlignV::Top:    break;
			case TextAlignV::Middle: y -= ext.height * 0.5f; break;
			case TextAlignV::Bottom: y -= ext.height; break;
			}

			AddTextPx(x, y, text, rgba, cell);
		}

		void AddOutlinedTextAlignedPx(float anchorXPx, float anchorYPx, std::string_view text,
			TextAlignH alignH, TextAlignV alignV,
			std::uint32_t rgbaText,
			std::uint32_t rgbaOutline = PackRGBA8(0, 0, 0, 200),
			float scale = 1.0f,
			float outlinePx = 1.0f)
		{
			if (text.empty())
			{
				return;
			}

			const float cell = std::max(0.25f, scale);
			const float o = std::max(1.0f, outlinePx);

			// Outline first (so main text overdraws it).
			AddTextAlignedPx(anchorXPx - o, anchorYPx, text, alignH, alignV, rgbaOutline, cell);
			AddTextAlignedPx(anchorXPx + o, anchorYPx, text, alignH, alignV, rgbaOutline, cell);
			AddTextAlignedPx(anchorXPx, anchorYPx - o, text, alignH, alignV, rgbaOutline, cell);
			AddTextAlignedPx(anchorXPx, anchorYPx + o, text, alignH, alignV, rgbaOutline, cell);

			// Diagonals improve readability at tiny sizes.
			AddTextAlignedPx(anchorXPx - o, anchorYPx - o, text, alignH, alignV, rgbaOutline, cell);
			AddTextAlignedPx(anchorXPx + o, anchorYPx - o, text, alignH, alignV, rgbaOutline, cell);
			AddTextAlignedPx(anchorXPx - o, anchorYPx + o, text, alignH, alignV, rgbaOutline, cell);
			AddTextAlignedPx(anchorXPx + o, anchorYPx + o, text, alignH, alignV, rgbaOutline, cell);

			AddTextAlignedPx(anchorXPx, anchorYPx, text, alignH, alignV, rgbaText, cell);
		}
	};
}