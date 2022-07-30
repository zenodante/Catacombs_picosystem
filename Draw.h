#pragma once
#include "Defines.h"

#define WITH_IMAGE_TEXTURES 1
#define WITH_VECTOR_TEXTURES 0
#define WITH_TEXTURES (WITH_IMAGE_TEXTURES || WITH_VECTOR_TEXTURES)
#define WITH_SPRITE_OUTLINES 1
#define TRANSPARENT_INDEX 0xf

struct Camera
{
	int16_t x, y;
	uint8_t angle;
	int16_t rotCos, rotSin;
	int16_t clipCos, clipSin;
	uint8_t cellX, cellY;
	int8_t tilt;
	int8_t bob;
	uint8_t shakeTime;
};

enum class DrawableType : uint8_t
{
	Sprite = 0,
	ParticleSystem = 1
};

enum class AnchorType : uint8_t
{
	Floor,
	Center,
	BelowCenter,
	Ceiling
};

struct QueuedDrawable
{
	union
	{
		const uint8_t* spriteData;
		struct ParticleSystem* particleSystem;
	};
	
	DrawableType type;
	uint8_t colour;
	int8_t x;
	int8_t y;
	uint8_t halfSize;
	uint8_t inverseCameraDistance;
};

class Renderer
{
public:
	static Camera camera;
	static uint8_t wBuffer[DISPLAY_WIDTH];
	static uint8_t globalRenderFrame;

    static void Init();
	static void Render();

    static void DrawSprite(const uint8_t* spriteData, int x, int y, uint8_t lighting = 0xf);

	static void DrawObject(const uint8_t* spriteData, int16_t x, int16_t y, uint8_t scale = 128, AnchorType anchor = AnchorType::Floor, uint8_t colour = 0xf);
	static QueuedDrawable* CreateQueuedDrawable(uint8_t inverseCameraDistance);
	static int8_t GetHorizon(int16_t x);
	
	static bool TransformAndCull(int16_t worldX, int16_t worldY, int16_t& outScreenX, int16_t& outScreenW);

	static void DrawScaled(const uint8_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, uint8_t colour = 0xf);
	
private:
	static int8_t horizonBuffer[DISPLAY_WIDTH];
	static QueuedDrawable queuedDrawables[MAX_QUEUED_DRAWABLES];
	static uint8_t numQueuedDrawables;
	static uint8_t numBufferSlicesFilled;

	static void DrawBackground();

#if WITH_IMAGE_TEXTURES
	static void DrawWallSegment(const uint8_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, uint8_t lighting1, uint8_t lighting2);
	static void DrawWall(const uint8_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
#elif WITH_VECTOR_TEXTURES
	static void DrawWallLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t clipLeft, uint8_t clipRight, uint8_t col);
	static void DrawWallSegment(const uint8_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, bool edgeLeft, bool edgeRight, uint8_t lighting1, uint8_t lighting2);
	static void DrawWall(const uint8_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, uint8_t lighting1, uint8_t lighting2);
#else
	static void DrawWallSegment(int16_t x1, int16_t w1, int16_t x2, int16_t w2, bool edgeLeft, bool edgeRight, bool shadeEdge);
	static void DrawWall(int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge);
#endif
	static void DrawFloorLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
	static void DrawFloorLineInner(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
	static void DrawFloorLines();

	static bool isFrustrumClipped(int16_t x, int16_t y);
	static void TransformToViewSpace(int16_t x, int16_t y, int16_t& outX, int16_t& outY);
	static void TransformToScreenSpace(int16_t viewX, int16_t viewZ, int16_t& outX, int16_t& outW);
	
	static void DrawCell(uint8_t x, uint8_t y);
	static void DrawCells();
	static void DrawWeapon();
	static void DrawHUD();
	static void DrawBar(int y, const uint8_t* iconData, uint8_t amount, uint8_t max);
	static void DrawDamageIndicator();
	
	static void QueueSprite(const uint8_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, uint8_t colour = 0xf);
	static void RenderQueuedDrawables();
		

};
