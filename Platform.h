#pragma once

#include <stdint.h>

class Platform
{
public:
	static uint8_t GetInput(void);
	static uint8_t* GetScreenBuffer(); 

	static void PlaySound(const uint16_t* audioPatter);
	static uint32_t GetVolume();
	static void SetVolume(uint32_t v);
	static bool IsAudioEnabled();
	static void SetAudioEnabled(bool isEnabled);

	static void ExpectLoadDelay();
	
	static void FillScreen(uint8_t col);
	static void PutPixel(uint8_t x, uint8_t y, uint8_t colour);
	static void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame);	
	
	static void SetPalette(const uint16_t* palette);

	static void DrawVLine(uint8_t x, int y1, int y2, uint8_t pattern);

    // Not actually used so can just be stubbed
	static void SetLED(uint8_t r, uint8_t g, uint8_t b);
	static void DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
	static void DrawBackground();
	static void DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
	static void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, const uint8_t *mask, uint8_t frame, uint8_t mask_frame);
};