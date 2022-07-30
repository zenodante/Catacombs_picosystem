#pragma once

#include <stdint.h>
#include "Defines.h"
#include "Draw.h"
#include "Game.h"

struct Particle
{
	int8_t x, y;
	int8_t velX, velY;

	inline bool IsActive() { return x != -128; }
};

struct ParticleSystem
{
	static constexpr int8_t gravity = 3;
	int16_t worldX, worldY;
	uint8_t life;
	uint8_t colour;
	bool isLight;
	Particle particles[PARTICLES_PER_SYSTEM];
	
	bool IsActive() { return life > 0; }

	void Init();
	void Step();
	void Draw(int x, int scale);
	void Explode();
};

class ParticleSystemManager
{
public:
	static constexpr int MAX_SYSTEMS = 3;
	static ParticleSystem systems[MAX_SYSTEMS];
	
	static void Init();
	static void Draw();
	static void Update();
	static ParticleSystem* CreateExplosion(int16_t x, int16_t y, uint8_t colour, bool isLight);
};
