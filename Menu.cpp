#include "Defines.h"
#include "Platform.h"
#include "Menu.h"
#include "Font.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"
#include "SpriteTypes.h"

void Menu::Init()
{
	selection = 0;
}

void Menu::Draw()
{
	//Platform::FillScreen(COLOUR_BLACK);
	//Font::PrintString(PSTR("CATACOMBS OF THE DAMNED"), 2, 18, COLOUR_WHITE);
	Font::PrintString("Code by @jameshhoward", 9, 17, COLOUR_WHITE);
	Font::PrintString("Art by Stephane C", 10, 17, COLOUR_WHITE);
	//Font::PrintString(PSTR("Press A to Start"), 7, 24, COLOUR_WHITE);
	Font::PrintString(PSTR("Press (A) / (B) to Start"), 12, 12, COLOUR_WHITE);
	//Disabled sound option
	Font::PrintString(PSTR("<- -> Sound Volume:"), 13, 12, COLOUR_WHITE);
	uint32_t v = Platform::GetVolume();
	Font::PrintInt(v,13,95,COLOUR_WHITE);
	
	
	//Font::PrintString(PSTR("< >"), 4 + selection, 16, COLOUR_WHITE);
	
	
	/*const uint8_t* torchSprite = Game::globalTickFrame & 4 ? torchSpriteData1 : torchSpriteData2;
	Renderer::DrawScaled(torchSprite, 0, 10, 9, 255);
	Renderer::DrawScaled(torchSprite, DISPLAY_WIDTH - 18, 10, 9, 255);
	*/
	
	Renderer::DrawSprite(logoData, 23, 21);
}

void Menu::Tick()
{
	static uint8_t lastInput = 0;
	
	// Spin the RNG to have a unique(ish) starting level
	Random();

/*
    // Disabled sound option    
	if ((Platform::GetInput() & INPUT_DOWN) && !(lastInput & INPUT_DOWN))
	{
		selection = !selection;
	}
	if ((Platform::GetInput() & INPUT_UP) && !(lastInput & INPUT_UP))
	{
		selection = !selection;
	}
*/
	uint32_t v = Platform::GetVolume();
	if ((Platform::GetInput() & INPUT_LEFT )&& !(lastInput & INPUT_LEFT)){
		if (v>=10){
			v -= 10;
			Platform::SetVolume(v);
		}
	}
	if ((Platform::GetInput() &INPUT_RIGHT)&& !(lastInput & INPUT_RIGHT)){
		if (v<=90){
			v += 10;
			Platform::SetVolume(v);
		}
	}
	if ((Platform::GetInput() & (INPUT_A | INPUT_B)) && !(lastInput & (INPUT_A | INPUT_B)))
	{
		switch (selection)
		{
		case 0:			
			Game::StartGame();
			break;
		case 1:
			Platform::SetAudioEnabled(!Platform::IsAudioEnabled());
			break;
		}
	}

	lastInput = Platform::GetInput();
}

void Menu::ResetTimer()
{
	timer = 0;
}

void Menu::TickEnteringLevel()
{
	constexpr uint8_t showTime = 30;
	
	if(timer < showTime)
	{
		timer++;
	}
	
	if(timer == showTime && Platform::GetInput() == 0)
	{
		Game::StartLevel();
	}
}

void Menu::DrawEnteringLevel()
{
	Platform::FillScreen(COLOUR_BLACK);
	Font::PrintString(PSTR("Entering floor"), 5, 21, COLOUR_WHITE);
	Font::PrintInt(Game::floor, 5, 81, COLOUR_WHITE);
}	

void Menu::TickGameOver()
{
	constexpr uint8_t minShowTime = 30;
	
	if(timer < minShowTime)
	{
		timer++;
	}
	
	if(timer == minShowTime && (Platform::GetInput() & (INPUT_A | INPUT_B)))
	{
		timer++;
	}
	else if(timer == minShowTime + 1 && Platform::GetInput() == 0)
	{
		Game::SwitchState(Game::State::Menu);
	}
}

void Menu::DrawGameOver()
{
	Platform::FillScreen(COLOUR_BLACK);
	Font::PrintString(PSTR("GAME OVER"), 0, DISPLAY_WIDTH / 2 - 18, COLOUR_WHITE);

	switch (Game::stats.killedBy)
	{
	case EnemyType::None:
		Font::PrintString(PSTR("You escaped the catacombs!"), 1, 4, COLOUR_WHITE);
		break;
	case EnemyType::Mage:
		Font::PrintString(PSTR("Killed by a mage"), 1, 24, COLOUR_WHITE);
		break;
	case EnemyType::Skeleton:
		Font::PrintString(PSTR("Killed by a knight"), 1, 20, COLOUR_WHITE);
		break;
	case EnemyType::Bat:
		Font::PrintString(PSTR("Killed by a bat"), 1, 26, COLOUR_WHITE);
		break;
	case EnemyType::Spider:
		Font::PrintString(PSTR("Killed by a spider"), 1, 20, COLOUR_WHITE);
		break;
	}
	
	Font::PrintString(PSTR("LOOT:"), 2, 16, COLOUR_WHITE);

	constexpr uint8_t firstRow = 22;
	constexpr uint8_t secondRow = 40;
	constexpr int firstColumn = 0;
	constexpr int secondColumn = 28;
	constexpr int thirdColumn = 62;
	constexpr int fourthColumn = 88;

	Renderer::DrawScaled(chestSpriteData, firstColumn, firstRow, 8, 255);
	Font::PrintInt(Game::stats.chestsOpened, 4, firstColumn + 18, COLOUR_WHITE);

	Renderer::DrawScaled(crownSpriteData, firstColumn, secondRow, 8, 255);
	Font::PrintInt(Game::stats.crownsCollected, 6, firstColumn + 18, COLOUR_WHITE);

	Renderer::DrawScaled(scrollSpriteData, secondColumn, firstRow, 8, 255);
	Font::PrintInt(Game::stats.scrollsCollected, 4, secondColumn + 18, COLOUR_WHITE);

	Renderer::DrawScaled(coinsSpriteData, secondColumn, secondRow, 8, 255);
	Font::PrintInt(Game::stats.coinsCollected, 6, secondColumn + 18, COLOUR_WHITE);

	///
	int offset = (Game::globalTickFrame & 8) == 0 ? 256 : 0;
	Font::PrintString(PSTR("KILLS:"), 2, 76, COLOUR_WHITE);

	Renderer::DrawScaled(skeletonSpriteData + offset, thirdColumn, firstRow, 8, 255);
	Font::PrintInt(Game::stats.enemyKills[(int)EnemyType::Skeleton], 4, thirdColumn + 18, COLOUR_WHITE);

	Renderer::DrawScaled(mageSpriteData + offset, thirdColumn, secondRow, 8, 255);
	Font::PrintInt(Game::stats.enemyKills[(int)EnemyType::Mage], 6, thirdColumn + 18, COLOUR_WHITE);

	Renderer::DrawScaled(batSpriteData + offset, fourthColumn, firstRow, 8, 255, true);
	Font::PrintInt(Game::stats.enemyKills[(int)EnemyType::Bat], 4, fourthColumn + 18, COLOUR_WHITE);

	Renderer::DrawScaled(spiderSpriteData + offset, fourthColumn, secondRow, 8, 255);
	Font::PrintInt(Game::stats.enemyKills[(int)EnemyType::Spider], 6, fourthColumn + 18, COLOUR_WHITE);

	// Calculate final score here
	uint16_t finalScore = 0;
	constexpr int finishBonus = 500;
	constexpr int levelBonus = 20;
	constexpr int chestBonus = 15;
	constexpr int crownBonus = 10;
	constexpr int scrollBonus = 8;
	constexpr int coinsBonus = 4;
	constexpr int skeletonKillBonus = 10;
	constexpr int mageKillBonus = 10;
	constexpr int batKillBonus = 5;
	constexpr int spiderKillBonus = 4;

	finalScore += (Game::floor - 1) * levelBonus;

	if (Game::stats.killedBy == EnemyType::None)
		finalScore += finishBonus;
	finalScore += Game::stats.chestsOpened * chestBonus;
	finalScore += Game::stats.crownsCollected * crownBonus;
	finalScore += Game::stats.scrollsCollected * scrollBonus;
	finalScore += Game::stats.coinsCollected * coinsBonus;
	finalScore += Game::stats.enemyKills[(int)EnemyType::Skeleton] * skeletonKillBonus;
	finalScore += Game::stats.enemyKills[(int)EnemyType::Mage] * mageKillBonus;
	finalScore += Game::stats.enemyKills[(int)EnemyType::Bat] * batKillBonus;
	finalScore += Game::stats.enemyKills[(int)EnemyType::Spider] * spiderKillBonus;

    if(Game::stats.killedBy != EnemyType::None)
    {
    	Font::PrintString(PSTR("Reached level "), 8, 16, COLOUR_WHITE);
    	Font::PrintInt(Game::floor, 8, 76, COLOUR_WHITE);
    }
	
	Font::PrintString(PSTR("FINAL SCORE:"), 10, 20, COLOUR_WHITE);
	Font::PrintInt(finalScore, 10, 72, COLOUR_WHITE);
}

void Menu::FadeOut()
{
	constexpr uint16_t toggleMask = 0x1f6e;
	constexpr int fizzlesPerFrame = 128;
	constexpr uint16_t startValue = 1;

	if (fizzleFade == 0)
	{
		fizzleFade = startValue;
	}

	for (int n = 0; n < fizzlesPerFrame; n++)
	{
		bool lsb = (fizzleFade & 1) != 0;
		fizzleFade >>= 1;
		if (lsb)
		{
			fizzleFade ^= toggleMask;
		}

		uint8_t x = (uint8_t)(fizzleFade & 0x7f);
		uint8_t y = (uint8_t)(fizzleFade >> 7);
		if ((x < 120)&&(y<120)){
			Platform::PutPixel(x, y, 0x67);
		}
		//Platform::PutPixel(x, y, 0x67);
		if ((x < 120)&&(y<56)){
			Platform::PutPixel(x, y + 64, 0x67);
		}

		if (fizzleFade == startValue)
		{
			Game::SwitchState(Game::State::GameOver);
			return;
		}
	}

}
