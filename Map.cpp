#include "Defines.h"
#include "Map.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"
#include "Platform.h"
#include "Enemy.h"

uint8_t Map::level[Map::width * Map::height];
uint8_t Map::lightMap[Map::width * Map::height];
uint8_t Map::staticLightMap[Map::width * Map::height];

const uint8_t Map::menuMap[Map::menuMapWidth * Map::menuMapHeight] =
{
    0xd,0xd,0xd,0xd,0xd,0xd,0xd,0xd,
    0x0,0x0,0xa,0xd,0x0,0x0,0x0,0x0,
    0x0,0x0,0x2,0xd,0x0,0x2,0xd,0x0,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0x0,0x0,0x2,0xd,0x0,0x2,0xd,0x0,
    0x0,0x0,0x0,0xd,0x0,0x0,0x0,0x9,
    0xd,0xd,0xd,0xd,0xd,0xd,0xd,0xd,
    0xd,0xd,0xd,0xd,0xd,0xd,0xd,0xd,
};

void Map::LoadMenuMap()
{
    for(int y = 0; y < menuMapHeight; y++)
    {
        for(int x = 0; x < width; x++)
        {
            level[y * width + x] = menuMap[y * menuMapWidth + (x % menuMapWidth)];
        }
    }
    
    GenerateLightMap();
}

bool Map::IsBlocked(uint8_t x, uint8_t y)
{
	return GetCellSafe(x, y) >= CellType::FirstCollidableCell;
}

bool Map::IsSolid(uint8_t x, uint8_t y)
{
	return GetCellSafe(x, y) >= CellType::FirstSolidCell;
}

CellType Map::GetCell(uint8_t x, uint8_t y) 
{
	int index = y * Map::width + x;
	return (CellType) level[index];
}

CellType Map::GetCellSafe(uint8_t x, uint8_t y) 
{
	if(x >= Map::width || y >= Map::height)
		return CellType::BrickWall;
	
	int index = y * Map::width + x;
	return (CellType)level[index];
}

void Map::SetCell(uint8_t x, uint8_t y, CellType type)
{
	if (x >= Map::width || y >= Map::height)
	{
		return;
	}

	int index = (y * Map::width + x);
	level[index] = (uint8_t) type;
}

void Map::DebugDraw()
{
	for(int y = 0; y < Map::height; y++)
	{
		for(int x = 0; x < Map::width; x++)
		{
			Platform::PutPixel(x, y, GetCell(x, y) == CellType::BrickWall ? 1 : 0);

			if (x == Renderer::camera.cellX && y == Renderer::camera.cellY && (Game::globalTickFrame & 8) != 0)
			{
				Platform::PutPixel(x, y, 1);
			}
		}
	}

	if ((Game::globalTickFrame & 2) != 0)
	{
		for (uint8_t n = 0; n < EnemyManager::maxEnemies; n++)
		{
			Enemy& enemy = EnemyManager::enemies[n];

			if (enemy.IsValid())
			{
				Platform::PutPixel(enemy.x / CELL_SIZE, enemy.y / CELL_SIZE, 1);
			}
		}
	}
}

bool Map::IsClearLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int cellX1 = x1 / CELL_SIZE;
	int cellX2 = x2 / CELL_SIZE;
	int cellY1 = y1 / CELL_SIZE;
	int cellY2 = y2 / CELL_SIZE;

    int xdist = ABS(cellX2 - cellX1);

	int partial, delta;
	int deltafrac;
	int xfrac, yfrac;
	int xstep, ystep;
	int32_t ltemp;
	int x, y;

    if (xdist > 0)
    {
        if (cellX2 > cellX1)
        {
            partial = (CELL_SIZE * (cellX1 + 1) - x1);
            xstep = 1;
        }
        else
        {
            partial = (x1 - CELL_SIZE * (cellX1));
            xstep = -1;
        }

        deltafrac = ABS(x2 - x1);
        delta = y2 - y1;
        ltemp = ((int32_t)delta * CELL_SIZE) / deltafrac;
        if (ltemp > 0x7fffl)
            ystep = 0x7fff;
        else if (ltemp < -0x7fffl)
            ystep = -0x7fff;
        else
            ystep = ltemp;
        yfrac = y1 + (((int32_t)ystep*partial) / CELL_SIZE);

        x = cellX1 + xstep;
        cellX2 += xstep;
        do
        {
            y = (yfrac) / CELL_SIZE;
            yfrac += ystep;

			if (IsSolid(x, y))
				return false;

            x += xstep;

            //
            // see if the door is open enough
            //
            /*value &= ~0x80;
            intercept = yfrac-ystep/2;

            if (intercept>doorposition[value])
                return false;*/

        } while (x != cellX2);
    }

    int ydist = ABS(cellY2 - cellY1);

    if (ydist > 0)
    {
        if (cellY2 > cellY1)
        {
            partial = (CELL_SIZE * (cellY1 + 1) - y1);
            ystep = 1;
        }
        else
        {
            partial = (y1 - CELL_SIZE * (cellY1));
            ystep = -1;
        }

        deltafrac = ABS(y2 - y1);
        delta = x2 - x1;
        ltemp = ((int32_t)delta * CELL_SIZE)/deltafrac;
        if (ltemp > 0x7fffl)
            xstep = 0x7fff;
        else if (ltemp < -0x7fffl)
            xstep = -0x7fff;
        else
            xstep = ltemp;
        xfrac = x1 + (((int32_t)xstep*partial) / CELL_SIZE);

        y = cellY1 + ystep;
        cellY2 += ystep;
        do
        {
            x = (xfrac) / CELL_SIZE;
            xfrac += xstep;

			if (IsSolid(x, y))
				return false;
            y += ystep;

            //
            // see if the door is open enough
            //
            /*value &= ~0x80;
            intercept = xfrac-xstep/2;

            if (intercept>doorposition[value])
                return false;*/
        } while (y != cellY2);
    }

    return true;
}

void Map::DrawMinimap()
{
	constexpr uint8_t minimapWidth = 24;
	constexpr uint8_t minimapHeight = 18;
	constexpr uint8_t minimapX = 0; //DISPLAY_WIDTH / 2 - minimapWidth / 2;
	constexpr uint8_t minimapY = 0; //DISPLAY_HEIGHT - minimapHeight;
	uint8_t playerCellX = Game::player.x / CELL_SIZE;
	uint8_t playerCellY = Game::player.y / CELL_SIZE;
	uint8_t startCellX = playerCellX - minimapWidth / 2;
	uint8_t startCellY = playerCellY - minimapHeight / 2;

	uint8_t outX = minimapX;
	uint8_t cellX = startCellX;

	for (uint8_t x = 0; x < minimapWidth; x++)
	{
		uint8_t outY = minimapY;
		uint8_t cellY = startCellY;

		for (uint8_t y = 0; y < minimapHeight; y++)
		{
			if (cellX == playerCellX && cellY == playerCellY)
			{
				Platform::PutPixel(outX, outY, (Game::globalTickFrame & 3) ? COLOUR_BLACK : COLOUR_WHITE);
			}
			else
			{
			    uint8_t colour = COLOUR_BLACK;
			    if(cellX < width && cellY < height)
			    {
			        //if(IsSolid(cellX, cellY))
			        {
			          //  colour = 0x8f;
			        }
			        //else
			        {
			            //colour = 2;
			            colour = GetLightingAtCell(cellX, cellY) | 0xf0;
			        }
			    }
				Platform::PutPixel(outX, outY, colour);
			}
			outY++;
			cellY++;
		}

		outX++;
		cellX++;
	}
}

void Map::GenerateLightMap()
{
    memset(lightMap, 1, width * height);
    
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            if(GetCell(x, y) == CellType::Torch)
            {
                constexpr int torchReach = 5;
        		if (Map::IsSolid(x - 1, y))
        		{
        			AddLight(x * CELL_SIZE + CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2, torchReach);
        		}
        		else if (Map::IsSolid(x + 1, y))
        		{
        			AddLight(x * CELL_SIZE + 6 * CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2, torchReach);
        		}
        		else if (Map::IsSolid(x, y - 1))
        		{
        			AddLight(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 7, torchReach);
        		}
        		else if (Map::IsSolid(x, y + 1))
        		{
        			AddLight(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + 6 * CELL_SIZE / 7, torchReach);
        		}
            }
            else if(IsSolid(x, y))
            {
                lightMap[y * width + x] = 0;
            }
        }
    }

    memcpy(staticLightMap, lightMap, width * height);
}

void increment(uint8_t& a, uint8_t b, uint8_t max)
{
    a += b;
    if(a > max)
        a = max;
}


void Map::AddDynamicLight(int x, int y, int brightness)
{
    if(brightness > 15)
        brightness = 15;
        
    int reach = 3;
    
    int tileX = x >> 8;
    int tileY = y >> 8;
    for(int i = -reach; i <= reach; i++)
    {
        int tx = tileX + i;
        if(tx < 0 || tx >= width)
            continue;
        
        int sampleX = tx * CELL_SIZE + (CELL_SIZE / 2);
        int diffXSqr = (sampleX - x) * (sampleX - x);
        for(int j = -reach; j <= reach; j++)
        {
            int ty = tileY + j;
            
            if(ty < 0 || ty >= height)
                continue;
                
            if(lightMap[ty * width + tx] != 0)
            {
                int sampleY = ty * CELL_SIZE + (CELL_SIZE / 2);
                int diffYSqr = (sampleY - y) * (sampleY - y);
                int sqrDist = (diffXSqr + diffYSqr) >> 8;
                int intensity = 384 / sqrDist;
                intensity = (brightness * intensity) >> 4;
                
                if(lightMap[ty * width + tx] + intensity > 15)
                {
                    lightMap[ty * width + tx] = 15;
                }
                else
                {
                    lightMap[ty * width + tx] += intensity;
                }
            }
        }
    }
}

void Map::AddLight(int lx, int ly, int reach)
{
    int x = (lx >> 8);
    int y = (ly >> 8);

    for(int j = -reach; j <= reach; j++)
    {
        for(int i = -reach; i <= reach; i++)
        {
            int tx = x + i;
            int ty = y + j;
            if(tx >= 0 && ty >= 0 && tx < width && ty < height && !IsSolid(tx, ty))
            {
                int16_t kx = tx * CELL_SIZE + CELL_SIZE / 2;
                int16_t ky = ty * CELL_SIZE + CELL_SIZE / 2;
                
                if(IsClearLine(lx, ly, kx, ky))
                {
                    int sqrDist = (i * i) + (j * j);
                    int intensity = sqrDist > 0 ? 14 / sqrDist : 15;
                    lightMap[ty * width + tx] += intensity;
                    if(lightMap[ty * width + tx] > 15)
                    {
                        lightMap[ty * width + tx] = 15;
                    }
                }
            }
        }
    }
}

uint8_t Map::GetLightingAtCell(int x, int y)
{
    return lightMap[y * width + x];
}

uint8_t Map::SampleWorldLighting(int x, int y)
{
    x -= CELL_SIZE / 2;
    y -= CELL_SIZE / 2;
    int tx = x / CELL_SIZE;
    int ty = y / CELL_SIZE;
    
    if(tx >= 0 && ty >= 0 && tx < width - 1 && ty < height - 1)
    {
        int u = x & 0xff;
        int v = y & 0xff;
        uint8_t topLeft = GetLightingAtCell(tx, ty);
        uint8_t topRight = GetLightingAtCell(tx + 1, ty);
        uint8_t bottomLeft = GetLightingAtCell(tx, ty + 1);
        uint8_t bottomRight = GetLightingAtCell(tx + 1, ty + 1);
        
        uint8_t top, bottom, mid;
        
        if(topLeft == 0)
        {
            top = topRight;
        }
        else if(topRight == 0)
        {
            top = topLeft;
        }
        else
        {
            //top = u < 128 ? topLeft : topRight;
            top = (topRight * u + topLeft * (256 - u)) >> 8;
        }
        
        if(bottomLeft == 0)
        {
            bottom = bottomRight;
        }
        else if(bottomRight == 0)
        {
            bottom = bottomLeft;
        }
        else
        {
            //bottom = u < 128 ? bottomLeft : bottomRight;
            bottom = (bottomRight * u + bottomLeft * (256 - u)) >> 8;
        }
        
        if(top == 0)
        {
            mid = bottom;
        }
        else if(bottom == 0)
        {
            mid = top;
        }
        else
        {
            //mid = v < 128 ? top : bottom;
            mid = (bottom * v + top * (256 - v)) >> 8;
        }
        
        return mid;
    }
    return 0;
}

void Map::RevertToStaticLightMap()
{
    memcpy(lightMap, staticLightMap, width * height);
}
