#include <stdint.h>
#include "Draw.h"
#include "Defines.h"
#include "Game.h"
#include "Particle.h"
#include "FixedMath.h"
#include "Map.h"
#include "Projectile.h"
#include "Platform.h"
#include "Enemy.h"
#include "Font.h"
#include "Palette.inc.h"

#include "LUT.h"
#include "SpriteData.inc.h"

#if WITH_VECTOR_TEXTURES
#include "Textures.h"
#endif

#if WITH_SPRITE_OUTLINES
#define DrawScaledInner DrawScaledOutline
#else
#define DrawScaledInner DrawScaledNoOutline
#endif

#include "Textures.inc.h"

Camera Renderer::camera;
uint8_t Renderer::wBuffer[DISPLAY_WIDTH];
int8_t Renderer::horizonBuffer[DISPLAY_WIDTH];
uint8_t Renderer::globalRenderFrame = 0;
uint8_t Renderer::numBufferSlicesFilled = 0;
QueuedDrawable Renderer::queuedDrawables[MAX_QUEUED_DRAWABLES];
uint8_t Renderer::numQueuedDrawables = 0;

const uint8_t scaleDrawWriteMasks[] PROGMEM =
{
	(1),
	(1 << 1),
	(1 << 2),
	(1 << 3),
	(1 << 4),
	(1 << 5),
	(1 << 6),
	(1 << 7)
};

const uint16_t scaleDrawReadMasks[] PROGMEM =
{
	(1),
	(1 << 1),
	(1 << 2),
	(1 << 3),
	(1 << 4),
	(1 << 5),
	(1 << 6),
	(1 << 7),
	(1 << 8),
	(1 << 9),
	(1 << 10),
	(1 << 11),
	(1 << 12),
	(1 << 13),
	(1 << 14),
	(1 << 15)
};

const int bands[] = 
{
    DISPLAY_HEIGHT / 4,  
    DISPLAY_HEIGHT / 5,  
    DISPLAY_HEIGHT / 6,  
    DISPLAY_HEIGHT / 7,  
    DISPLAY_HEIGHT / 8,  
    DISPLAY_HEIGHT / 9,  
    DISPLAY_HEIGHT / 10,  
    DISPLAY_HEIGHT / 11,  
};

uint8_t CalculateDistanceLighting(int w)
{
    for(int n = 0; n < 8; n++)
    {
        if(w > bands[n])
        {
            return 8 - n;
        }
    }

    return 0;    
}

#if WITH_VECTOR_TEXTURES
void Renderer::DrawWallLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t clipLeft, uint8_t clipRight, uint8_t col)
{
	if(x1 > x2)
		return;
	
	if (y1 < 0)
	{
		if(y2 < 0)
			return;
		
		if(y2 != y1)
			x1 += (0 - y1) * (x2 - x1) / (y2 - y1);
		y1 = 0;
	}
	if (y2 > DISPLAY_HEIGHT - 1)
	{
		if(y1 > DISPLAY_HEIGHT - 1)
			return;
		
		if(y2 != y1)
			x2 += (((DISPLAY_HEIGHT - 1) - y2) * (x1 - x2)) / (y1 - y2);
		y2 = DISPLAY_HEIGHT - 1;
	}
	
	if (x1 < clipLeft)
	{
		if(x2 != x1)
		{
			y1 += ((clipLeft - x1) * (y2 - y1)) / (x2 - x1);
		}
		x1 = clipLeft;
	}

	int16_t dx = x2 - x1;
	int16_t yerror = dx / 2;
	int16_t y = y1;
	int16_t dy;
	int8_t ystep;

	if (y1 < y2)
	{
		dy = y2 - y1;
		ystep = 1;
	}
	else
	{
		dy = y1 - y2;
		ystep = -1;
	}

	for (int x = x1; x <= x2 && x <= clipRight; x++)
	{
		int8_t horizon = horizonBuffer[x] - HORIZON;		

		Platform::PutPixel(x, horizon + y, col);

		yerror -= dy;

		while (yerror < 0)
		{
			y += ystep;
			
			//if(y < 0 || y >= DISPLAY_HEIGHT)
			//	return;
			
			yerror += dx;

			if (yerror < 0)
			{
				Platform::PutPixel(x, horizon + y, col);
			}

			if (x == x2 && y == y2)
				break;
		}
	}
}
#endif

#if WITH_IMAGE_TEXTURES
void Renderer::DrawWallSegment(const uint8_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, uint8_t lighting1, uint8_t lighting2)
#elif WITH_VECTOR_TEXTURES
void Renderer::DrawWallSegment(const uint8_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, bool edgeLeft, bool edgeRight, uint8_t lighting1, uint8_t lighting2)
#else
void Renderer::DrawWallSegment(int16_t x1, int16_t w1, int16_t x2, int16_t w2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#endif
{
	if (x1 < 0)
	{
#if WITH_TEXTURES
		u1clip += ((int32_t)(0 - x1) * (int32_t)(u2clip - u1clip)) / (x2 - x1);
#endif
		w1 += ((int32_t)(0 - x1) * (int32_t)(w2 - w1)) / (x2 - x1);
		x1 = 0;
#if !WITH_IMAGE_TEXTURES
		edgeLeft = false;
#endif
	}

	int16_t dx = x2 - x1;
	int16_t werror = dx / 2;
	int16_t w = w1;
	int16_t dw;
	int8_t wstep;

	if (w1 < w2)
	{
		dw = w2 - w1;
		wstep = 1;
	}
	else
	{
		dw = w1 - w2;
		wstep = -1;
	}

	constexpr uint8_t wallColour = COLOUR_WHITE;
	constexpr uint8_t edgeColour = COLOUR_BLACK;
	
	uint8_t segmentClipLeft = (uint8_t) x1;
	uint8_t segmentClipRight = x2 < DISPLAY_WIDTH ? (uint8_t) x2 : DISPLAY_WIDTH - 1;

	for (int x = x1; x < DISPLAY_WIDTH; x++)
	{
		bool drawSlice = x >= 0 && wBuffer[x] < w;
		//bool shadeSlice = shadeEdge;// && (x & 1) == 0;		
		
		int8_t horizon = horizonBuffer[x];

		if (drawSlice)
		{
			/*uint8_t sliceMask = 26; //0xff;

			if ((edgeLeft && x == x1) || (edgeRight && x == x2))
			{
				sliceMask = 0x00;
			}
			else if (shadeSlice)
			{
				sliceMask = 24; // 0x55;
			}*/
			
			/*
			int alpha = 256 * (x - x1) / (x2 - x1);
			int u = (alpha * u2clip + (256 - alpha) * u1clip) / 256;
			uint8_t sliceLighting = (u * lighting2 + (16 - u) * lighting1) / 16;
			
			int alpha2 = 256 * (x + 1 - x1) / (x2 - x1);
			int u2 = (alpha2 * u2clip + (256 - alpha2) * u1clip) / 256;
			uint8_t sliceLighting2 = (u2 * lighting2 + (16 - u2) * lighting1) / 16;
			*/
			
			int alpha = 256 * (x - x1) / (x2 - x1);
			int u = (alpha * u2clip + (256 - alpha) * u1clip) / 256;

			uint8_t sliceLighting = (u * lighting2 + (16 - u) * lighting1) / 16;

/*
			int dither1 = (x & 1) ? -1 : 2;
			int dither2 = (x & 1) ? 1 : 0;
			
			int alpha1 = 256 * (x + dither1 - x1) / (x2 - x1);
			int u1 = (alpha1 * u2clip + (256 - alpha1) * u1clip) / 256;
			u1 = u + dither1;
			uint8_t sliceLighting1 = (u1 * lighting2 + (16 - u1) * lighting1) / 16;
			
			int alpha2 = 256 * (x + dither2 - x1) / (x2 - x1);
			int u2 = (alpha2 * u2clip + (256 - alpha2) * u1clip) / 256;
			u2 = u + dither2;
			uint8_t sliceLighting2 = (u2 * lighting2 + (16 - u2) * lighting1) / 16;
*/			
			

#if WITH_IMAGE_TEXTURES
			{
				uint8_t y1 = w > horizon ? 0 : horizon - w;
				uint8_t y2 = horizon + w >= DISPLAY_HEIGHT ? DISPLAY_HEIGHT - 1 : horizon + w;

				//DrawVLine(x, y1, y2, sliceMask);
				u = u & 0xf;
				const uint8_t* texturePtr = texture + u * 16;
				int wallPos = y1 - (horizon - w);
				int wallSize = w * 2;
				uint8_t* screenPtr = Platform::GetScreenBuffer();
				screenPtr += x + (y1 * DISPLAY_WIDTH);
				for(int y = y1; y <= y2; y++)
				{
				    int v = (15 * wallPos) / wallSize;
				    uint8_t outColour = texturePtr[v];
				    
				    /*if(y & 1)
				        outColour |= sliceLighting1;
				    else
				        outColour |= sliceLighting2;
				      */
				    outColour |= sliceLighting; 

				    *screenPtr = outColour;
				    //Platform::PutPixel(x, y, outColour);
				    wallPos++;
				    screenPtr += DISPLAY_WIDTH;
				}
			}
#else
			int8_t extent = w > 64 ? 64 : w;
			Platform::DrawVLine(x, horizon - extent, horizon + extent, sliceColour);
			
			// disable outlines
			//Platform::PutPixel(x, horizon + extent, edgeColour);
			//Platform::PutPixel(x, horizon - extent, edgeColour);
#endif
			
			if(wBuffer[x] == 0)
			{
				numBufferSlicesFilled++;
			}
			
			if (w > 255)
				wBuffer[x] = 255;
			else
				wBuffer[x] = (uint8_t)w;
		}
		else
		{
			if(x == segmentClipLeft)
			{
				segmentClipLeft++;
			}
			else if(x < segmentClipRight)
			{
				segmentClipRight = x;
				break;
			}
		}

		if (x == x2)
			break;

		werror -= dw;

		while (werror < 0)
		{
			w += wstep;
			werror += dx;

			/*if (drawSlice && werror < 0 && w <= DISPLAY_HEIGHT / 2)
			{
				Platform::PutPixel(x, horizon + w - 1, edgeColour);
				Platform::PutPixel(x, horizon - w, edgeColour);
			}*/
		}
	}
	
	if(segmentClipLeft == segmentClipRight)
		return;
		
	// Disable outlines
	return;

#if WITH_VECTOR_TEXTURES
	if (w1 < MIN_TEXTURE_DISTANCE || w2 < MIN_TEXTURE_DISTANCE || !texture)
		return;
	if(u1clip == u2clip)
		return;

	const uint8_t* texPtr = texture;
	uint8_t numLines = pgm_read_byte(texPtr++);
	while (numLines)
	{
		numLines--;
		
		uint8_t u1 = pgm_read_byte(texPtr++);
		uint8_t v1 = pgm_read_byte(texPtr++);
		uint8_t u2 = pgm_read_byte(texPtr++);
		uint8_t v2 = pgm_read_byte(texPtr++);

		//if(u1clip != 0 || u2clip != 128)
		//	continue;
		
		if (u2 < u1clip || u1 > u2clip)
			continue;

		if (u1 < u1clip)
		{
			if(u2 != u1)
				v1 += (u1clip - u1) * (v2 - v1) / (u2 - u1);
			u1 = u1clip;
		}
		if (u2 > u2clip)
		{
			if (u2 != u1)
				v2 += (u2clip - u2) * (v1 - v2) / (u1 - u2);
			u2 = u2clip;
		}
		
		u1 = (128 * (u1 - u1clip)) / (u2clip - u1clip);
		u2 = (128 * (u2 - u1clip)) / (u2clip - u1clip);

		int16_t outU1 = (((int32_t)u1 * dx) >> 7) + x1;
		int16_t outU2 = (((int32_t)u2 * dx) >> 7) + x1;

		int16_t interpw1 = ((u1 * (w2 - w1)) >> 7) + w1;
		int16_t interpw2 = ((u2 * (w2 - w1)) >> 7) + w1;

		int16_t outV1 = (interpw1 * v1) >> 6;
		int16_t outV2 = (interpw2 * v2) >> 6;

		//uint8_t horizon = horizonBuffer[x]
		//DrawLine(ScreenSurface, outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, edgeColour, edgeColour, edgeColour);
		DrawWallLine(outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, segmentClipLeft, segmentClipRight, edgeColour);
		//DrawWallLine(outU1, -interpw1 + outV1, outU2, -interpw2 + outV2, edgeColour);
	}
#endif
}

bool Renderer::isFrustrumClipped(int16_t x, int16_t y)
{
	if ((camera.clipCos * (x - camera.cellX) - camera.clipSin * (y - camera.cellY)) < -512)
		return true;
	if ((camera.clipSin * (x - camera.cellX) + camera.clipCos * (y - camera.cellY)) < -512)
		return true;

	return false;
}

void Renderer::TransformToViewSpace(int16_t x, int16_t y, int16_t& outX, int16_t& outY)
{
	int32_t relX = x - camera.x;
	int32_t relY = y - camera.y;
	outY = (int16_t)((camera.rotCos * relX) >> 8) - (int16_t)((camera.rotSin * relY) >> 8);
	outX = (int16_t)((camera.rotSin * relX) >> 8) + (int16_t)((camera.rotCos * relY) >> 8);
}

void Renderer::TransformToScreenSpace(int16_t viewX, int16_t viewZ, int16_t& outX, int16_t& outW)
{
	// apply perspective projection
	outX = (int16_t)((int32_t)viewX * NEAR_PLANE * CAMERA_SCALE / viewZ);
	outW = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ);

	// transform into screen space
	outX = (int16_t)((DISPLAY_WIDTH / 2) + outX);
}

#if WITH_IMAGE_TEXTURES
void Renderer::DrawWall(const uint8_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
#elif WITH_VECTOR_TEXTURES
void Renderer::DrawWall(const uint8_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, uint8_t lighting1, uint8_t lighting2)
#else
void Renderer::DrawWall(int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#endif
{
	int16_t viewX1, viewZ1, viewX2, viewZ2;
#if WITH_VECTOR_TEXTURES
	uint8_t u1 = 0, u2 = 128;
#elif WITH_IMAGE_TEXTURES
	uint8_t u1 = 0, u2 = 16;
#endif

    uint8_t lighting1 = Map::SampleWorldLighting(x1, y1);
    uint8_t lighting2 = Map::SampleWorldLighting(x2, y2);
    
	TransformToViewSpace(x1, y1, viewX1, viewZ1);
	TransformToViewSpace(x2, y2, viewX2, viewZ2);

	// Frustum cull
	if (viewX2 < 0 && -2 * viewZ2 > viewX2)
		return;
	if (viewX1 > 0 && 2 * viewZ1 < viewX1)
		return;

	// clip to the front pane
	if ((viewZ1 < CLIP_PLANE) && (viewZ2 < CLIP_PLANE))
		return;

	if (viewZ1 < CLIP_PLANE)
	{
#if WITH_TEXTURES
		u1 += (CLIP_PLANE - viewZ1) * (u2 - u1) / (viewZ2 - viewZ1);
#endif
		viewX1 += (CLIP_PLANE - viewZ1) * (viewX2 - viewX1) / (viewZ2 - viewZ1);
		viewZ1 = CLIP_PLANE;
		//edgeLeft = false;
	}
	else if (viewZ2 < CLIP_PLANE)
	{
#if WITH_TEXTURES
		u2 += (CLIP_PLANE - viewZ2) * (u1 - u2) / (viewZ1 - viewZ2);
#endif
		viewX2 += (CLIP_PLANE - viewZ2) * (viewX1 - viewX2) / (viewZ1 - viewZ2);
		viewZ2 = CLIP_PLANE;
		//edgeRight = false;
	}

	// apply perspective projection
	int16_t vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	int16_t vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);

	// transform the end points into screen space
	int16_t sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	int16_t sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) - 1;

	if (sx1 >= sx2 || sx2 <= 0 || sx1 >= DISPLAY_WIDTH)
		return;

	int16_t w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	int16_t w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);

#if WITH_IMAGE_TEXTURES
    DrawWallSegment(texture, sx1, w1, sx2, w2, u1, u2, lighting1, lighting2);
#elif WITH_TEXTURES
	DrawWallSegment(texture, sx1, w1, sx2, w2, u1, u2, edgeLeft, edgeRight, lighting1, lighting2);
#else
	DrawWallSegment(sx1, w1, sx2, w2, edgeLeft, edgeRight, shadeEdge);
#endif
}

void swap(int16_t& a, int16_t& b)
{
	int16_t temp = a;
	a = b;
	b = temp;
}

void Renderer::DrawFloorLineInner(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	uint8_t color = COLOUR_BLACK;
	
	bool steep = ABS(y1 - y0) > ABS(x1 - x0);
	if (steep) 
	{
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1) 
	{
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = ABS(y1 - y0);

	int16_t err = dx / 2;
	int8_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if(steep)
		{
			if(y0 >= 0 && y0 < DISPLAY_WIDTH && x0 >= 0 && x0 < DISPLAY_HEIGHT && x0 > GetHorizon(y0) + wBuffer[y0] && x0 > GetHorizon(y0) + 8)
			{
				Platform::PutPixel((uint8_t)y0, (uint8_t)x0 + horizonBuffer[y0] - HORIZON, color);
			}
		}
		else
		{
			if(x0 >= 0 && x0 < DISPLAY_WIDTH && y0 >= 0 && y0 < DISPLAY_HEIGHT && y0 > GetHorizon(x0) + wBuffer[x0] && y0 > GetHorizon(x0) + 8)
			{
				Platform::PutPixel((uint8_t)x0, (uint8_t)y0 + horizonBuffer[x0] - HORIZON, color);
			}
		}

		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

void Renderer::DrawFloorLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int16_t viewX1, viewZ1, viewX2, viewZ2;

	TransformToViewSpace(x1, y1, viewX1, viewZ1);
	TransformToViewSpace(x2, y2, viewX2, viewZ2);
	
	//if(viewX1 > viewX2)
	//{
	//	swap(viewX1, viewX2);
	//	swap(viewZ1, viewZ2);
	//}

	// Frustum cull
//	if (viewX2 < 0 && -2 * viewZ2 > viewX2)
//		return;
//	if (viewX1 > 0 && 2 * viewZ1 < viewX1)
//		return;

	// clip to the front pane
	if ((viewZ1 < CLIP_PLANE) && (viewZ2 < CLIP_PLANE))
		return;

	if (viewZ1 < CLIP_PLANE)
	{
		viewX1 += (CLIP_PLANE - viewZ1) * (viewX2 - viewX1) / (viewZ2 - viewZ1);
		viewZ1 = CLIP_PLANE;
	}
	else if (viewZ2 < CLIP_PLANE)
	{
		viewX2 += (CLIP_PLANE - viewZ2) * (viewX1 - viewX2) / (viewZ1 - viewZ2);
		viewZ2 = CLIP_PLANE;
	}

	// apply perspective projection
	int16_t vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	int16_t vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);

	// transform the end points into screen space
	int16_t sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	int16_t sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) - 1;

	//if (sx2 <= 0 || sx1 >= DISPLAY_WIDTH)
	//	return;

	int16_t w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	int16_t w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);

	DrawFloorLineInner(sx1, HORIZON + w1, sx2, HORIZON + w2);
}

void Renderer::DrawFloorLines()
{
	constexpr int size = 10;
	int16_t baseX = (Game::player.x - CELL_SIZE * size / 2) & 0xff00;
	int16_t baseY = (Game::player.y - CELL_SIZE * size / 2) & 0xff00;
	
	for(int n = 0; n < 10; n++)
	{
		DrawFloorLine(baseX, baseY + n * CELL_SIZE, baseX + CELL_SIZE * 10 - n * CELL_SIZE, baseY + CELL_SIZE * 10);
		DrawFloorLine(baseX, baseY + n * CELL_SIZE, baseX + n * CELL_SIZE, baseY);
	}
	
	for(int n = 1; n < 10; n++)
	{
		DrawFloorLine(baseX + n * CELL_SIZE, baseY, baseX + CELL_SIZE * 10, baseY + CELL_SIZE * 10 - n * CELL_SIZE);
		DrawFloorLine(baseX + n * CELL_SIZE, baseY + 10 * CELL_SIZE, baseX + 10 * CELL_SIZE, baseY + n * CELL_SIZE);
	}
}

void Renderer::DrawCell(uint8_t x, uint8_t y)
{
	CellType cellType = Map::GetCellSafe(x, y);

	if (isFrustrumClipped(x, y))
	{
		return;
	}

	switch (cellType)
	{
	case CellType::Torch:
	{
		const uint8_t* torchSpriteData = Game::globalTickFrame & 4 ? torchSpriteData1 : torchSpriteData2;
		constexpr uint8_t torchScale = 75;

		if (Map::IsSolid(x - 1, y))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x + 1, y))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + 6 * CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x, y - 1))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 7, torchScale, AnchorType::Center);
		}
		else if (Map::IsSolid(x, y + 1))
		{
			DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + 6 * CELL_SIZE / 7, torchScale, AnchorType::Center);
		}
	}
	return;
	case CellType::Entrance:
		DrawObject(entranceSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 96, AnchorType::Ceiling);
		return;
	case CellType::Exit:
		DrawObject(exitSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 96);
		return;
	case CellType::Urn:
		DrawObject(urnSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 80);
		return;
	case CellType::Potion:
		DrawObject(potionSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Scroll:
		DrawObject(scrollSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Coins:
		DrawObject(coinsSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Crown:
		DrawObject(crownSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 64);
		return;
	case CellType::Sign:
		DrawObject(signSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 80);
		return;
	case CellType::Chest:
		DrawObject(chestSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 75);
		return;
	case CellType::ChestOpened:
		DrawObject(chestOpenSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 75);
		return;
	default:
		break;
	}

	if(numBufferSlicesFilled >= DISPLAY_WIDTH)
	{
		return;
	}

	if (!Map::IsSolid(x, y))
	{
		return;
	}
	
	int16_t x1 = x * CELL_SIZE;
	int16_t y1 = y * CELL_SIZE;
	int16_t x2 = x1 + CELL_SIZE;
	int16_t y2 = y1 + CELL_SIZE;

	bool blockedLeft = Map::IsSolid(x - 1, y);
	bool blockedRight = Map::IsSolid(x + 1, y);
	bool blockedUp = Map::IsSolid(x, y - 1);
	bool blockedDown = Map::IsSolid(x, y + 1);

#if WITH_IMAGE_TEXTURES
    int textureIndex = 2 + ((int)Map::GetCell(x, y) - (int)CellType::FirstSolidCell);
	const uint8_t* texture = ColourTextures + textureIndex * 256;
#elif WITH_VECTOR_TEXTURES
	const uint8_t* texture = vectorTexture0; // (const uint8_t*) pgm_read_ptr(&textures[(uint8_t)cellType - (uint8_t)CellType::FirstSolidCell]);
#endif


	if (!blockedLeft && camera.x < x1)
	{
#if WITH_TEXTURES
		DrawWall(texture, x1, y1, x1, y2);
#else
		DrawWall(x1, y1, x1, y2, !blockedUp && camera.y > y1, !blockedDown && camera.y < y2, true);
#endif
	}

	if (!blockedDown && camera.y > y2)
	{
#if WITH_TEXTURES
		DrawWall(texture, x1, y2, x2, y2);
#else
		DrawWall(x1, y2, x2, y2, !blockedLeft && camera.x > x1, !blockedRight && camera.x < x2, false);
#endif
	}

	if (!blockedRight && camera.x > x2)
	{
#if WITH_TEXTURES
		DrawWall(texture, x2, y2, x2, y1);
#else
		DrawWall(x2, y2, x2, y1, !blockedDown && camera.y < y2, !blockedUp && camera.y > y1, true);
#endif
	}

	if (!blockedUp && camera.y < y1)
	{
#if WITH_TEXTURES
		DrawWall(texture, x2, y1, x1, y1);
#else
		DrawWall(x2, y1, x1, y1, !blockedRight && camera.x < x2, !blockedLeft && camera.x > x1, false);
#endif
	}
}

void Renderer::DrawCells()
{
	constexpr int8_t MAP_BUFFER_WIDTH = 16;
	constexpr int8_t MAP_BUFFER_HEIGHT = 16;
	
	int16_t cosAngle = FixedCos(camera.angle);
	int16_t sinAngle = FixedSin(camera.angle);

	int8_t bufferX = (int8_t)((camera.x + cosAngle * 7) >> 8) - MAP_BUFFER_WIDTH / 2;
	int8_t bufferY = (int8_t)((camera.y + sinAngle * 7) >> 8) - MAP_BUFFER_WIDTH / 2;; 
	
	if(bufferX < 0)
		bufferX = 0;
	if(bufferY < 0)
		bufferY = 0;
	if(bufferX > Map::width - MAP_BUFFER_WIDTH)
		bufferX = Map::width - MAP_BUFFER_WIDTH;
	if(bufferY > Map::height - MAP_BUFFER_HEIGHT)
		bufferY = Map::height - MAP_BUFFER_HEIGHT;
	
	// This should make cells draw front to back
	
	int8_t xd, yd;
	int8_t x1, y1, x2, y2;

	if(camera.rotCos > 0)
	{
		x1 = bufferX;
		x2 = x1 + MAP_BUFFER_WIDTH;
		xd = 1;
	}
	else
	{
		x2 = bufferX - 1;
		x1 = x2 + MAP_BUFFER_WIDTH;
		xd = -1;
	}
	if(camera.rotSin < 0)
	{
		y1 = bufferY;
		y2 = y1 + MAP_BUFFER_HEIGHT;
		yd = 1;
	}
	else
	{
		y2 = bufferY - 1;
		y1 = y2 + MAP_BUFFER_HEIGHT;
		yd = -1;
	}

	if(ABS(camera.rotCos) < ABS(camera.rotSin))
	{
		for(int8_t y = y1; y != y2; y += yd)
		{
			for(int8_t x = x1; x != x2; x+= xd)
			{
				DrawCell(x, y);
			}
		}
	}
	else
	{
		for(int8_t x = x1; x != x2; x+= xd)
		{
			for(int8_t y = y1; y != y2; y += yd)
			{
				DrawCell(x, y);
			}
		}
	}	
}

void DrawScaledOutline(const uint8_t* data, int x, int y, uint8_t halfSize, uint8_t inverseCameraDistance, uint8_t colour)
{
	uint8_t size = 2 * halfSize;

	int i0 = x < 0 ? -x : 0;
	int i1 = x + size > DISPLAY_WIDTH ? DISPLAY_WIDTH - x : size;
	int j0 = y < 0 ? -y : 0;
	int j1 = y + size > DISPLAY_HEIGHT ? DISPLAY_HEIGHT - y : size;

	int outX = x >= 0 ? x : 0;

	bool wasVisible = false;
	
	for (int i = i0; i < i1; i++)
	{
		const bool isVisible = Renderer::wBuffer[outX] < inverseCameraDistance;

		if (isVisible)
		{
			uint16_t leftRightOutlineColumn = 0;

			int u = (i * 16) / size;
            int outY = y >= 0 ? y : 0;
			uint8_t* screenBuffer = Platform::GetScreenBuffer() + outX + outY * DISPLAY_WIDTH;
			const uint8_t* spriteColumn = data + 16 * u;

			for (uint8_t j = j0; j < j1; j++)
			{
				int v = (j * 16) / size;
				uint8_t pixel = spriteColumn[v];
				if(pixel != TRANSPARENT_INDEX)
				{
				    *screenBuffer = (pixel | colour);
				}
				screenBuffer += DISPLAY_WIDTH;
			}
		}

		outX++;
		wasVisible = isVisible;
	}
    
}

void DrawScaledOutlineMono(const uint16_t* data, int x, int y, uint8_t halfSize, uint8_t inverseCameraDistance, uint8_t colour)
{
	uint8_t size = 2 * halfSize;

	int i0 = x < 0 ? -x : 0;
	int i1 = x + size > DISPLAY_WIDTH ? DISPLAY_WIDTH - x : size;
	int j0 = y < 0 ? -y : 0;
	int j1 = y + size > DISPLAY_HEIGHT ? DISPLAY_HEIGHT - y : size;

	int outX = x >= 0 ? x : 0;

	uint16_t leftTransparencyAndColourColumn = 0;
	uint16_t middleTransparencyColumn = 0;
	uint16_t rightTransparencyColumn = 0;
	uint16_t middleColourColumn = 0;
	uint16_t rightColourColumn = 0;
	bool wasVisible = false;

	for (int i = i0; i < i1; i++)
	{
		const bool isVisible = Renderer::wBuffer[outX] < inverseCameraDistance;

		if (isVisible)
		{
			uint16_t leftRightOutlineColumn = 0;

			if(i >= i1 - 2)
			{
				if (wasVisible)
				{
					leftTransparencyAndColourColumn = middleColourColumn & middleTransparencyColumn;
					middleColourColumn = rightColourColumn;
					middleTransparencyColumn = rightTransparencyColumn;
					rightTransparencyColumn = 0;
					rightColourColumn = 0;
					leftRightOutlineColumn = leftTransparencyAndColourColumn;
				}
				else
				{
					break;
				}
			}
			else
			{
				int u = (i * 16) / size;

				if (wasVisible)
				{
					leftTransparencyAndColourColumn = middleColourColumn & middleTransparencyColumn;
					middleColourColumn = rightColourColumn;
					middleTransparencyColumn = rightTransparencyColumn;
					rightTransparencyColumn = pgm_read_word(&data[u * 2]);
					rightColourColumn = pgm_read_word(&data[u * 2 + 1]);
					leftRightOutlineColumn = leftTransparencyAndColourColumn | (rightColourColumn & rightTransparencyColumn);
				}
				else
				{
					leftTransparencyAndColourColumn = 0;
					rightTransparencyColumn = pgm_read_word(&data[u * 2]);
					rightColourColumn = pgm_read_word(&data[u * 2 + 1]);
					middleColourColumn = rightColourColumn;
					middleTransparencyColumn = rightTransparencyColumn;
					leftRightOutlineColumn = (rightColourColumn & rightTransparencyColumn);
				}
			}
			
			int8_t outY = y >= 0 ? y : 0;
			//uint8_t bufferPos = (outY & 7);
			//uint8_t* screenBuffer = Platform::GetScreenBuffer() + outX + ((outY & 0x38) << 4);
			uint8_t* screenBuffer = Platform::GetScreenBuffer() + outX + outY * DISPLAY_WIDTH;
			//uint8_t localBuffer = *screenBuffer;
			//uint8_t writeMask = pgm_read_byte(&scaleDrawWriteMasks[bufferPos]);
			
			bool upIsOpaqueAndWhite = false;
			bool middleIsOpaque = false;
			bool downIsOpaque = false;
			bool middleIsWhite = false;
			bool downIsWhite = false;
			bool leftOrRightIsOutline = false;
			bool downLeftOrRightIsOutline = false;
			
			for (uint8_t j = j0; j < j1; j++)
			{
				upIsOpaqueAndWhite = middleIsOpaque && middleIsWhite;
				middleIsOpaque = downIsOpaque;
				middleIsWhite = downIsWhite;
				leftOrRightIsOutline = downLeftOrRightIsOutline;

				if(j >= j1 - 2)
				{
					downIsOpaque = false;
					downIsWhite = false;
					downLeftOrRightIsOutline = false;
				}
				else
				{
					uint8_t v = (j * 16) / size;
					uint16_t mask = pgm_read_word(&scaleDrawReadMasks[v]);
					downLeftOrRightIsOutline = (leftRightOutlineColumn & mask) != 0;
					downIsOpaque = (middleTransparencyColumn & mask) != 0;
					downIsWhite = (middleColourColumn & mask) != 0;
				}
					
				if (middleIsOpaque && j < j1)
				{
					if(middleIsWhite)
					{
						*screenBuffer = colour;
						//localBuffer |= writeMask;
					}
					else
					{
						*screenBuffer = COLOUR_BLACK;
						//localBuffer &= ~writeMask;
					}
				}
				else if(leftOrRightIsOutline || (upIsOpaqueAndWhite) || (downIsOpaque && downIsWhite))
				{
					*screenBuffer = COLOUR_BLACK;
					//localBuffer &= ~writeMask;
				}
					
				outY++;
				/*bufferPos++;
				writeMask <<= 1;
					
				if(bufferPos == 8)
				{
					bufferPos = 0;
					writeMask = 1;
						
					*screenBuffer = localBuffer;
					if(outY < DISPLAY_HEIGHT)
					{
						screenBuffer += 128;
					}
					localBuffer = *screenBuffer;
				}*/
				screenBuffer += DISPLAY_WIDTH;
				
			}

			//*screenBuffer = localBuffer;
		}

		outX++;
		wasVisible = isVisible;
	}
    
}

void DrawScaledOutlineOld(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, uint8_t shiftAmount, bool invert)
{
	uint8_t size = 2 * halfSize;
	const uint8_t* lut = scaleLUT + (((halfSize - 1) >> shiftAmount) * ((halfSize - 1) >> shiftAmount));

	int i0 = x < 0 ? -x : 0;
	int i1 = x + size > DISPLAY_WIDTH ? DISPLAY_WIDTH - x : size;
	int j0 = y < 0 ? -y : 0;
	int j1 = y + size > DISPLAY_HEIGHT ? DISPLAY_HEIGHT - y : size;

	int outX = x >= 0 ? x : 0;
	uint16_t invertMask = invert ? 0xffff : 0;
	
	uint16_t leftTransparencyAndColourColumn = 0;
	uint16_t middleTransparencyColumn = 0;
	uint16_t rightTransparencyColumn = 0;
	uint16_t middleColourColumn = 0;
	uint16_t rightColourColumn = 0;
	bool wasVisible = false;

	for (int i = i0; i < i1; i++)
	{
		const bool isVisible = Renderer::wBuffer[outX] < inverseCameraDistance;

		if (isVisible)
		{
			uint16_t leftRightOutlineColumn = 0;

			if(i >= i1 - 2)
			{
				if (wasVisible)
				{
					leftTransparencyAndColourColumn = middleColourColumn & middleTransparencyColumn;
					middleColourColumn = rightColourColumn;
					middleTransparencyColumn = rightTransparencyColumn;
					rightTransparencyColumn = 0;
					rightColourColumn = 0;
					leftRightOutlineColumn = leftTransparencyAndColourColumn;
				}
				else
				{
					break;
				}
			}
			else
			{
				const uint8_t u = pgm_read_byte(&lut[i >> shiftAmount]);

				if (wasVisible)
				{
					leftTransparencyAndColourColumn = middleColourColumn & middleTransparencyColumn;
					middleColourColumn = rightColourColumn;
					middleTransparencyColumn = rightTransparencyColumn;
					rightTransparencyColumn = pgm_read_word(&data[u * 2]);
					rightColourColumn = pgm_read_word(&data[u * 2 + 1]) ^ invertMask;
					leftRightOutlineColumn = leftTransparencyAndColourColumn | (rightColourColumn & rightTransparencyColumn);
				}
				else
				{
					leftTransparencyAndColourColumn = 0;
					rightTransparencyColumn = pgm_read_word(&data[u * 2]);
					rightColourColumn = pgm_read_word(&data[u * 2 + 1]) ^ invertMask;
					middleColourColumn = rightColourColumn;
					middleTransparencyColumn = rightTransparencyColumn;
					leftRightOutlineColumn = (rightColourColumn & rightTransparencyColumn);
				}
			}
			
			int8_t outY = y >= 0 ? y : 0;
			//uint8_t bufferPos = (outY & 7);
			//uint8_t* screenBuffer = Platform::GetScreenBuffer() + outX + ((outY & 0x38) << 4);
			uint8_t* screenBuffer = Platform::GetScreenBuffer() + outX + outY * DISPLAY_WIDTH;
			//uint8_t localBuffer = *screenBuffer;
			//uint8_t writeMask = pgm_read_byte(&scaleDrawWriteMasks[bufferPos]);
			
			bool upIsOpaqueAndWhite = false;
			bool middleIsOpaque = false;
			bool downIsOpaque = false;
			bool middleIsWhite = false;
			bool downIsWhite = false;
			bool leftOrRightIsOutline = false;
			bool downLeftOrRightIsOutline = false;
			
			for (uint8_t j = j0; j < j1; j++)
			{
				upIsOpaqueAndWhite = middleIsOpaque && middleIsWhite;
				middleIsOpaque = downIsOpaque;
				middleIsWhite = downIsWhite;
				leftOrRightIsOutline = downLeftOrRightIsOutline;

				if(j >= j1 - 2)
				{
					downIsOpaque = false;
					downIsWhite = false;
					downLeftOrRightIsOutline = false;
				}
				else
				{
					uint8_t v = pgm_read_byte(&lut[j >> shiftAmount]);
					uint16_t mask = pgm_read_word(&scaleDrawReadMasks[v]);
					downLeftOrRightIsOutline = (leftRightOutlineColumn & mask) != 0;
					downIsOpaque = (middleTransparencyColumn & mask) != 0;
					downIsWhite = (middleColourColumn & mask) != 0;
				}
					
				if (middleIsOpaque && j < j1)
				{
					if(middleIsWhite)
					{
						*screenBuffer = COLOUR_WHITE;
						//localBuffer |= writeMask;
					}
					else
					{
						*screenBuffer = COLOUR_BLACK;
						//localBuffer &= ~writeMask;
					}
				}
				else if(leftOrRightIsOutline || (upIsOpaqueAndWhite) || (downIsOpaque && downIsWhite))
				{
					*screenBuffer = COLOUR_BLACK;
					//localBuffer &= ~writeMask;
				}
					
				outY++;
				/*bufferPos++;
				writeMask <<= 1;
					
				if(bufferPos == 8)
				{
					bufferPos = 0;
					writeMask = 1;
						
					*screenBuffer = localBuffer;
					if(outY < DISPLAY_HEIGHT)
					{
						screenBuffer += 128;
					}
					localBuffer = *screenBuffer;
				}*/
				screenBuffer += DISPLAY_WIDTH;
				
			}

			//*screenBuffer = localBuffer;
		}

		outX++;
		wasVisible = isVisible;
	}
}

template<int scaleMultiplier>
inline void DrawScaledNoOutline(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance)
{
	uint8_t size = 2 * halfSize;
	const uint8_t* lut = scaleLUT + ((halfSize / scaleMultiplier) * (halfSize / scaleMultiplier));

	uint8_t i0 = x < 0 ? -x : 0;
	uint8_t i1 = x + size > DISPLAY_WIDTH ? DISPLAY_WIDTH - x : size;
	uint8_t j0 = y < 0 ? -y : 0;
	uint8_t j1 = y + size > DISPLAY_HEIGHT ? DISPLAY_HEIGHT - y : size;

	int8_t outX = x >= 0 ? x : 0;

	for (uint8_t i = i0; i < i1; i++)
	{
		const bool isVisible = Renderer::wBuffer[outX] < inverseCameraDistance;

		if (isVisible)
		{
			const uint8_t u = pgm_read_byte(&lut[i / scaleMultiplier]);
			int8_t outY = y >= 0 ? y : 0;
			uint8_t bufferPos = (outY & 7);
			uint8_t* screenBuffer = Platform::GetScreenBuffer() + outX + ((outY & 0x38) << 4);
			uint8_t localBuffer = *screenBuffer;
			uint8_t writeMask = pgm_read_byte(&scaleDrawWriteMasks[bufferPos]);
			uint16_t transparencyColumn = pgm_read_word(&data[u * 2]);
			uint16_t colourColumn = pgm_read_word(&data[u * 2 + 1]);

			for (uint8_t j = j0; j < j1; j += scaleMultiplier)
			{
				uint8_t v = pgm_read_byte(&lut[j / scaleMultiplier]);
				uint16_t mask = pgm_read_word(&scaleDrawReadMasks[v]);

				for (uint8_t k = 0; k < scaleMultiplier; k++)
				{
					bool isOpaque = (transparencyColumn & mask) != 0;

					if (isOpaque)
					{
						bool isWhite = (colourColumn & mask) != 0;

						if (isWhite)
						{
							localBuffer |= writeMask;
						}
						else
						{
							localBuffer &= ~writeMask;
						}
					}

					outY++;
					bufferPos++;
					writeMask <<= 1;

					if (bufferPos == 8)
					{
						bufferPos = 0;
						writeMask = 1;

						*screenBuffer = localBuffer;
						if (outY < DISPLAY_HEIGHT)
						{
							screenBuffer += 128;
						}
						localBuffer = *screenBuffer;
					}
				}
			}

			*screenBuffer = localBuffer;
		}

		outX++;
	}
}

void Renderer::DrawScaled(const uint8_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, uint8_t colour)
{
	DrawScaledOutline(data, x, y, halfSize, inverseCameraDistance, colour);

/*	if (halfSize > 2)
	{
		DrawScaledOutline(data, x, y, halfSize, inverseCameraDistance, colour);
	}
	else if (halfSize == 2)
	{
		if (Renderer::wBuffer[x] < inverseCameraDistance)
		{
			Platform::PutPixel(x, y, COLOUR_BLACK);
			Platform::PutPixel(x, y + 1, COLOUR_BLACK);
		}
		if (Renderer::wBuffer[x + 1] < inverseCameraDistance)
		{
			Platform::PutPixel(x + 1, y, COLOUR_BLACK);
			Platform::PutPixel(x + 1, y + 1, COLOUR_BLACK);
		}
	}
	else
	{
		if (Renderer::wBuffer[x] < inverseCameraDistance)
		{
			Platform::PutPixel(x, y, COLOUR_BLACK);
		}
	}*/
}

QueuedDrawable* Renderer::CreateQueuedDrawable(uint8_t inverseCameraDistance)
{
	uint8_t insertionPoint = MAX_QUEUED_DRAWABLES;
	
	for(uint8_t n = 0; n < numQueuedDrawables; n++)
	{
		if(inverseCameraDistance < queuedDrawables[n].inverseCameraDistance)
		{
			if(numQueuedDrawables < MAX_QUEUED_DRAWABLES)
			{
				insertionPoint = n;
				numQueuedDrawables++;
				
				for (uint8_t i = numQueuedDrawables - 1; i > n; i--)
				{
					queuedDrawables[i] = queuedDrawables[i - 1];
				}
			}
			else
			{
				if(n == 0)
				{
					// List is full and this is smaller than the first element so just cull
					return nullptr;
				}
				
				// Drop the smallest element to make a space
				for (uint8_t i = 0; i < n - 1; i++)
				{
					queuedDrawables[i] = queuedDrawables[i + 1];
				}
				
				insertionPoint = n - 1;
			}
			
			break;
		}
	}
	
	if(insertionPoint == MAX_QUEUED_DRAWABLES)
	{
		if(numQueuedDrawables < MAX_QUEUED_DRAWABLES)
		{
			insertionPoint = numQueuedDrawables;
			numQueuedDrawables++;
		}
		else if (inverseCameraDistance > queuedDrawables[numQueuedDrawables - 1].inverseCameraDistance)
		{
			// Drop the smallest element to make a space
			for (uint8_t i = 0; i < numQueuedDrawables - 1; i++)
			{
				queuedDrawables[i] = queuedDrawables[i + 1];
			}
			insertionPoint = numQueuedDrawables - 1;
		}
		else
		{
			return nullptr;
		}
	}
	
	return &queuedDrawables[insertionPoint];
}

void Renderer::QueueSprite(const uint8_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance, uint8_t colour)
{
	if(x < -halfSize * 2)
		return;
	//if(x >= DISPLAY_WIDTH)
	//	return;
	//if(halfSize <= 2)
	//	return;

	QueuedDrawable* drawable = CreateQueuedDrawable(inverseCameraDistance);
	
	if(drawable != nullptr)
	{
		drawable->type = DrawableType::Sprite;
		drawable->spriteData = data;
		drawable->x = x;
		drawable->y = y;
		drawable->halfSize = halfSize;
		drawable->inverseCameraDistance = inverseCameraDistance;
		drawable->colour = colour;
	}
}

void Renderer::RenderQueuedDrawables()
{
	for(uint8_t n = 0; n < numQueuedDrawables; n++)
	{
		QueuedDrawable& drawable = queuedDrawables[n];
		
		if(drawable.type == DrawableType::Sprite)
		{
			DrawScaled(drawable.spriteData, drawable.x, drawable.y, drawable.halfSize, drawable.inverseCameraDistance, drawable.colour);
		}
		else
		{
			drawable.particleSystem->Draw(drawable.x, drawable.inverseCameraDistance);
		}
	}
}

int8_t Renderer::GetHorizon(int16_t x)
{
	if (x < 0)
		x = 0;
	if (x >= DISPLAY_WIDTH)
		x = DISPLAY_WIDTH - 1;
	return horizonBuffer[x];
}

bool Renderer::TransformAndCull(int16_t worldX, int16_t worldY, int16_t& outScreenX, int16_t& outScreenW)
{
	int16_t relX, relZ;
	TransformToViewSpace(worldX, worldY, relX, relZ);

	// Frustum cull
	if (relZ < CLIP_PLANE)
		return false;

	if (relX < 0 && -2 * relZ > relX)
		return false;
	if (relX > 0 && 2 * relZ < relX)
		return false;

	TransformToScreenSpace(relX, relZ, outScreenX, outScreenW);
	
	return true;
}

void Renderer::DrawObject(const uint8_t* spriteData, int16_t x, int16_t y, uint8_t scale, AnchorType anchor, uint8_t colour)
{
	int16_t screenX, screenW;

	if(TransformAndCull(x, y, screenX, screenW))
	{
		// Bit of a hack: nudge sorting closer to the camera
		uint8_t inverseCameraDistance = (uint8_t)(screenW + 1);
		int16_t spriteSize = (screenW * scale) / 128;
		int8_t outY = GetHorizon(screenX);

		switch (anchor)
		{
		case AnchorType::Floor:
			outY += screenW - 2 * spriteSize;
			break;
		case AnchorType::Center:
			outY -= spriteSize;
			break;
		case AnchorType::BelowCenter:
			break;
		case AnchorType::Ceiling:
			outY -= screenW;
			break;
		}
		
		uint8_t lighting = Map::SampleWorldLighting(x, y);
		
		QueueSprite(spriteData, screenX - spriteSize, outY, (uint8_t)spriteSize, inverseCameraDistance, lighting);
	}
}

void Renderer::DrawSprite(const uint8_t* spriteData, int x, int y, uint8_t lighting)
{
    uint8_t* screenBuffer = Platform::GetScreenBuffer();
    int width = spriteData[0];
    int height = spriteData[1];
    spriteData += 2;
    
    for(int j = 0; j < height; j++)
    {
        int outY = y + j;
        
        if(outY < 0 || outY >= DISPLAY_HEIGHT)
            continue;
            
        for(int i = 0; i < width; i++)
        {
            int outX = x + i;
            
            if(outX < 0 || outX >= DISPLAY_WIDTH)
                continue;
            
            uint8_t colour = spriteData[j * width + i];
            if(colour != TRANSPARENT_INDEX)
            {
                colour |= lighting;
                screenBuffer[outY * DISPLAY_WIDTH + outX] = colour;
            }
        }
    }
}

void Renderer::DrawWeapon()
{
	int x = DISPLAY_WIDTH / 2 + 22 + camera.tilt / 4;
	int y = DISPLAY_HEIGHT - 27 - 11 - camera.bob;
	uint8_t reloadTime = Game::player.reloadTime;
	
	uint8_t lighting = Map::SampleWorldLighting(camera.x, camera.y);
	
	if(reloadTime > 0)
	{
		DrawSprite(handSpriteData2, x - reloadTime / 3 - 1, y - reloadTime / 3 - 1, 0xf);
		//DrawSprite(x - reloadTime / 3 - 1, y - reloadTime / 3 - 1, handSpriteData2, handSpriteData2_mask, 0, 0);	
	}
	else
	{
		DrawSprite(handSpriteData1, x + 2, y + 2, lighting);
		//DrawSprite(x + 2, y + 2, handSpriteData1, handSpriteData1_mask, 0, 0);	
	}
	
}
/*
const int16_t floorLUT[] =
{
#include "FloorLUT.inc.h"  
};
*/

void Renderer::DrawBackground()
{
    const uint8_t* floorTexture = ColourTextures + (256 * 0);
    const uint8_t* ceilingTexture = ColourTextures + (256 * 1);
	int16_t rotCos = FixedCos(camera.angle); 
	int16_t rotSin = FixedSin(camera.angle);
	constexpr int lutColumnHeight = DISPLAY_HEIGHT / 2 + 4;

    int index = 0;

    for(int x = 0; x < DISPLAY_WIDTH; x++)
    {
	    uint8_t* screenPtr = Platform::GetScreenBuffer() + x;
	    
	    for(int w = wBuffer[x] + 1; w < lutColumnHeight; w++)
        //for(int y = 0; y < DISPLAY_HEIGHT; y++)
        {
            int y1 = horizonBuffer[x] - w;
            int y2 = horizonBuffer[x] + w;

            if(y1 < 0 && y2 >= DISPLAY_HEIGHT)
            {
                break;
            }

            index = (lutColumnHeight * x + w) * 2;
            //int viewX = floorLUT[index++];
            //int viewZ = floorLUT[index++];
            
            int wDepth = w;
    	    //int wDepth = horizonBuffer[x] - y; //(DISPLAY_HEIGHT / 2) - y;
    	    //if(wDepth < 0)
    	     //   wDepth = -wDepth;
    	    
    	    int viewZ = (CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / wDepth;

            int vx = x - (DISPLAY_WIDTH / 2);
            int viewX = (vx * viewZ) / (NEAR_PLANE * CAMERA_SCALE);
            
            int worldX = camera.x + ((rotCos * viewZ) >> FIXED_SHIFT) - ((rotSin * viewX) >> FIXED_SHIFT);
            int worldY = camera.y + ((rotSin * viewZ) >> FIXED_SHIFT) + ((rotCos * viewX) >> FIXED_SHIFT);
            
            const bool dither = true;
            
            const int quarter = 8;
            const int shift = 0;
            int tx = (worldX >> 4) & 0xf;
            int ty = (worldY >> 4) & 0xf;
            
            if(dither)
            {
                worldX += shift;
                worldY += shift;
                if(x & 1)
                {
                    if(w & 1)
                    {
                        worldY += quarter;
                    }
                    else
                    {
                        worldX += quarter * 2;
                        worldY += quarter * 3;
                    }
                }
                else
                {
                    if(w & 1)
                    {
                        worldX += quarter * 3;
                        worldY += quarter * 2;
                    }
                    else
                    {
                        worldX += quarter;
                    }
                }
            }


            uint8_t lighting = Map::SampleWorldLighting(worldX, worldY);

            if(y1 >= 0)
                screenPtr[y1 * DISPLAY_WIDTH] = ceilingTexture[ty * 16 + tx] | lighting;
            if(y2 < DISPLAY_HEIGHT)
                screenPtr[y2 * DISPLAY_WIDTH] = floorTexture[ ty * 16 + tx] | lighting;
            //Platform::PutPixel(x, y, lighting + 16);
/*	// apply perspective projection
	int16_t vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	int16_t vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);

	// transform the end points into screen space
	int16_t sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	int16_t sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) - 1;

	//if (sx2 <= 0 || sx1 >= DISPLAY_WIDTH)
	//	return;

	int16_t w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	int16_t w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);
  */          
        }
    }
    
    /*
	uint8_t* ptr = Platform::GetScreenBuffer();
	
	for(int y = 0; y < DISPLAY_HEIGHT; y++)
	{
	    int wDepth = (DISPLAY_HEIGHT / 2) - y;
	    if(wDepth < 0)
	        wDepth = -wDepth;
	    
	    uint8_t colour = 16 + CalculateDistanceLighting(wDepth);
	    
	    memset(ptr, colour, DISPLAY_WIDTH);
	    ptr += DISPLAY_WIDTH;
	}*/
	
	/*
	int counter = DISPLAY_WIDTH * DISPLAY_HEIGHT / 2;

	while (counter--)
	{
		*ptr++ = 28; //0x55;
	}

	counter = DISPLAY_WIDTH * DISPLAY_HEIGHT / 2;
	while (counter--)
	{
		*ptr++ = 22; //0xAA;
	}
	*/
}

void Renderer::DrawBar(int y, const uint8_t* iconData, uint8_t amount, uint8_t max)
{
	constexpr uint8_t iconWidth = 8;
	constexpr uint8_t barHeight = 8;
	constexpr uint8_t barWidth = 32;
	constexpr uint8_t unfilledBar = 0xfe;
	constexpr uint8_t filledBar = 0xc6;
	
	uint8_t fillAmount = (amount * barWidth) / max;
	uint8_t x = 0;

	while (x < iconWidth)
	{
	    uint8_t iconSlice = iconData[x];
		for(int j = 0; j < barHeight; j++)
		{
		    uint8_t colour = ((1 << j) & iconSlice) != 0 ? 1 : 0;
		    Platform::PutPixel(x, y + j, colour);
		}
		x++;
	}
/*
	while (fillAmount--)
	{
		screenPtr[x++] = filledBar;
	}

	while (x < barWidth + iconWidth)
	{
		screenPtr[x++] = unfilledBar;
	}

	screenPtr[x++] = unfilledBar;
	screenPtr[x] = 0;*/
}

void Renderer::DrawDamageIndicator()
{
	
	uint8_t* upper = Platform::GetScreenBuffer();
	uint8_t* lower = upper + DISPLAY_WIDTH * 7;

	for (int x = 1; x < DISPLAY_WIDTH - 1; x++)
	{
		upper[x] &= 0xfe;
		lower[x] &= 0x7f;
	}

	uint8_t* ptr = Platform::GetScreenBuffer();
	for (int y = 0; y < DISPLAY_HEIGHT / 8; y++)
	{
		*ptr = 0;
		ptr += (DISPLAY_WIDTH - 1);
		*ptr = 0;
		ptr++;
	}
}

#include <stdio.h>

void Renderer::DrawHUD()
{
	constexpr uint8_t barWidth = 40;

    DrawSprite(statusBarData, 0, DISPLAY_HEIGHT - 11);

    Font::PrintInt(Game::player.hp, (DISPLAY_HEIGHT/8)-1, 24, COLOUR_WHITE);
    Font::PrintInt(Game::player.mana, (DISPLAY_HEIGHT/8)-1, 59, COLOUR_WHITE);
    Font::PrintInt(Game::floor, (DISPLAY_HEIGHT/8)-1, 94, COLOUR_WHITE);
//	DrawBar(DISPLAY_HEIGHT - 16, heartSpriteData, Game::player.hp, Game::player.maxHP);
	//DrawBar(DISPLAY_HEIGHT - 8, manaSpriteData, Game::player.mana, Game::player.maxMana);

	if(Game::player.damageTime > 0)
		DrawDamageIndicator();

	if (Game::displayMessage)
		Font::PrintString(Game::displayMessage, 2, 5, COLOUR_WHITE, 0x7f);

    int offset = Game::player.damageTime / 3;
    if(offset > 3)
        offset = 3;
        
    Platform::SetPalette(GamePalette + offset * 256);
}

void Renderer::Init()
{
    Platform::SetPalette(GamePalette);
}

void Renderer::Render()
{
	globalRenderFrame++;

	numBufferSlicesFilled = 0;
	numQueuedDrawables = 0;
	
	for (uint8_t n = 0; n < DISPLAY_WIDTH; n++)
	{
		wBuffer[n] = 0;
		horizonBuffer[n] = HORIZON + (((DISPLAY_WIDTH / 2 - n) * camera.tilt) >> 8) + camera.bob;
	}

    Map::RevertToStaticLightMap();
    for(Projectile& proj : ProjectileManager::projectiles)
    {
        if(proj.life > 0)
        {
            Map::AddDynamicLight(proj.x, proj.y, 15);
        }
    }
    for(ParticleSystem& p : ParticleSystemManager::systems)
    {
        if(p.IsActive() && p.isLight)
        {
            Map::AddDynamicLight(p.worldX, p.worldY, p.life);
        }
    }


	camera.cellX = camera.x / CELL_SIZE;
	camera.cellY = camera.y / CELL_SIZE;

	camera.rotCos = FixedCos(-camera.angle);
	camera.rotSin = FixedSin(-camera.angle);
	camera.clipCos = FixedCos(-camera.angle + CLIP_ANGLE);
	camera.clipSin = FixedSin(-camera.angle + CLIP_ANGLE);

	DrawCells();
	//DrawFloorLines();
	DrawBackground();

	EnemyManager::Draw();
	ProjectileManager::Draw();
	ParticleSystemManager::Draw();
	
	RenderQueuedDrawables();
	
	if(Game::GetState() != Game::State::Menu)
	{
    	DrawWeapon();
    
    	DrawHUD();
	}

	//Map::DrawMinimap();
}

