// Compact font taken from
// https://hackaday.io/project/6309-vga-graphics-over-spi-and-serial-vgatonic/log/20759-a-tiny-4x6-pixel-font-that-will-fit-on-almost-any-microcontroller-license-mit

#include <stdint.h>
#include "Defines.h"
#include "Font.h"
#include "Platform.h"
#include "SpriteTypes.h"

void Font::PrintString(const char* str, uint8_t line, uint8_t x, uint8_t fgColour, uint8_t bgColour)
{
	uint8_t* screenPtr = Platform::GetScreenBuffer();
	int y = line * 8;

	for (;;)
	{
		char c = pgm_read_byte(str++);
		if (!c)
			break;

		DrawChar(x, y, c, fgColour, bgColour);
		x += glyphWidth;
	}
}

void Font::PrintInt(uint16_t val, uint8_t line, uint8_t x, uint8_t fgColour, uint8_t bgColour)
{
	uint8_t* screenPtr = Platform::GetScreenBuffer();
	int y = line * 8;

	if (val == 0)
	{
		DrawChar(x, y, '0', fgColour, bgColour);
		return;
	}

	constexpr int maxDigits = 5;
	char buffer[maxDigits];
	int bufCount = 0;

	for (int n = 0; n < maxDigits && val != 0; n++)
	{
		unsigned char c = val % 10;
		buffer[bufCount++] = '0' + c;
		val = val / 10;
	}

	for (int n = bufCount - 1; n >= 0; n--)
	{
		DrawChar(x, y, buffer[n], fgColour, bgColour);
		x += glyphWidth;
	}

}

void Font::DrawChar(int x, int y, char c, uint8_t fgColour, uint8_t bgColour)
{
	const uint8_t index = ((unsigned char)(c)) - firstGlyphIndex;
	const uint8_t* fontPtr = fontPageData + glyphWidth * index;

    for(int i = 0; i < glyphWidth; i++)
    {
        uint8_t slice = fontPtr[i];
        for(int j = 0; j < glyphHeight; j++)
        {
            uint8_t outColour = (slice & (1 << j)) == 0 ? fgColour : bgColour;
            if(outColour != 0xf)
            {
                Platform::PutPixel(x + i, y + j, outColour);
            }
        }
    }
}

