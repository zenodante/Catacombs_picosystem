#pragma once

#include <stdint.h>
#include "Defines.h"

#define FONT_WIDTH 4
#define FONT_HEIGHT 6

class Font
{
public:
	static constexpr int glyphWidth = 4;
	static constexpr int glyphHeight = 8;
	static constexpr int firstGlyphIndex = 32;

	static void PrintString(const char* str, uint8_t line, uint8_t x, uint8_t fgColour = COLOUR_BLACK, uint8_t bgColour = 0xf);
	static void PrintInt(uint16_t value, uint8_t line, uint8_t x, uint8_t fgColour = COLOUR_BLACK, uint8_t bgColour = 0xf);

private:
	static void DrawChar(int x, int y, char c, uint8_t fgColour, uint8_t bgColour);
};
