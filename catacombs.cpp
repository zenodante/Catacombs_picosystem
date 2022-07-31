#include "picosystem.hpp"
#include "Platform.h"
#include "Defines.h"
#include "Platform.h"
#include "Game.h"
#include "Font.h"

using namespace picosystem;
uint32_t tempBuff[120*120/4]={};
color_t lut[256]={};
uint32_t gameVolume = 100;
//unsigned long lastTimingSample;
namespace picosystem {
    extern struct repeating_timer _audio_update_timer;
    extern uint32_t _volume;
}
void L8ToRGBA(uint32_t *source,color_t *fb, color_t *lut);
void playSimpleTones(uint16_t *seq, uint32_t v);
void L8ToRGBA(uint32_t *source,color_t *fb, color_t *lut, uint32_t c){
    c = c >>2;
    while(c--){
        uint32_t s = *source++;

        *fb = lut[s&0x000000FF];
        fb ++;
        *fb = lut[(s>>8)&0x000000FF];
        fb ++;
        *fb = lut[(s>>16)&0x000000FF];
        fb ++;
        *fb = lut[(s>>24)&0x000000FF];
        fb ++;
    }
}


uint8_t Platform::GetInput(void)
{
  uint8_t result = 0; 
  if(button(B)){
    result |= INPUT_B;  
  }
  if(button(A)){
    result |= INPUT_A;  
  }
  if(button(X)){
    result |= INPUT_START;  
  }
  if(button(UP)){
    result |= INPUT_UP;  
  }
  if(button(DOWN)){
    result |= INPUT_DOWN;  
  }
  if(button(LEFT)){
    result |= INPUT_LEFT;  
  }
  if(button(RIGHT)){
    result |= INPUT_RIGHT;  
  }
  return result;
}

void Platform::SetLED(uint8_t r, uint8_t g, uint8_t b)
{

}

uint8_t* Platform::GetScreenBuffer()
{
    return (uint8_t *)tempBuff;
}

void Platform::PlaySound(const uint16_t* audioPattern)
{

    playSimpleTones((uint16_t *)audioPattern, gameVolume);
}

uint32_t Platform::GetVolume(){
    return gameVolume;
}
void Platform::SetVolume(uint32_t v){
    gameVolume = v;
}
bool Platform::IsAudioEnabled()
{
    return true;
}

void Platform::SetAudioEnabled(bool isEnabled)
{
    
}

void Platform::ExpectLoadDelay()
{
    //lastTimingSample = time();
}

void Platform::FillScreen(uint8_t col)
{
    uint32_t col4 = (col<<24)|(col<<16)|(col<<8)|col;
    uint32_t i;
    for (i=0;i<(120*120/4);i++){
        tempBuff[i]=col4;
    }
}

void Platform::PutPixel(uint8_t x, uint8_t y, uint8_t colour)
{
    ((uint8_t *)tempBuff)[x+120*y]=colour;
}

void Platform::DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
    
}

void Platform::DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
    
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, const uint8_t *mask, uint8_t frame, uint8_t mask_frame)
{
    
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame){
	uint8_t w = bitmap[0];
	uint8_t h = bitmap[1];

	bitmap += 2;

	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			int blockY = j / 8;
			int blockIndex = (w * blockY + i) * 2;
			uint8_t pixels = bitmap[blockIndex];
			uint8_t maskPixels = bitmap[blockIndex + 1];
			uint8_t bitmask = 1 << (j % 8);

			if (maskPixels & bitmask)
			{
				if (x + i >= 0 && y + j >= 0)
				{
					if (pixels & bitmask)
					{
						//PutPixel(x + i, y + j, COLOUR_WHITE);
                        tempBuff[x+i+(y+j)*DISPLAY_WIDTH]=COLOUR_WHITE;
					}
					else
					{
						tempBuff[x+i+(y+j)*DISPLAY_WIDTH]=0;
                        //PutPixel(x + i, y + j, 0);
					}
				}
			}
		}
	}
//todo
}

void Platform::DrawVLine(uint8_t x, int y1, int y2, uint8_t colour){
    if(y1 < 0)
        y1 = 0;
    if(y2 >= DISPLAY_HEIGHT)
        y2 = DISPLAY_HEIGHT - 1;
    for(y1;y1<=y2;y1++){
        tempBuff[x+y1*DISPLAY_WIDTH]=colour;
    }
}

void Platform::SetPalette(const uint16_t* palette)
{
    uint32_t i=0;
    for(i=0;i<256;i++){
        lut[i] = palette[i];
    }
}
#ifndef TONES_END 
#define TONES_END 0x8000
#endif
uint16_t *_current_simple_tone_step;
int32_t  _current_simple_tone_ms_left=-1;
void update_audio() {
    // pitch bend
    if (_current_simple_tone_ms_left==-1){
        _play_note(10,0);//stop
        return;
    }
      if (_current_simple_tone_ms_left > 0){
        _current_simple_tone_ms_left --; //not finish current step yet
        return;
      }else{
        uint32_t freq = *_current_simple_tone_step++; //load step
        if (freq == TONES_END){
          _current_simple_tone_ms_left=-1; //stop statement
          _play_note(10,0);//stop
          return;
        }else{
          _current_simple_tone_ms_left = (*_current_simple_tone_step++)-1;
          _play_note(freq,_volume);//play the tone
          return;
        }
      }
    
    
  }
//#include "hardware/structs/systick.h"
bool audioCallback(struct repeating_timer *t) {
    update_audio();
    return true;
}

void playSimpleTones(uint16_t *seq, uint32_t v){
    _volume = v;
    _current_simple_tone_step = seq;
    _current_simple_tone_ms_left = 0;

}
void init() {
    //by pass the default SDK audio update function
    cancel_repeating_timer(&_audio_update_timer);
    //add new 1ms callback to process tones
    add_repeating_timer_ms(-1, audioCallback, NULL, &_audio_update_timer);

    Game::Init();
    const bool visualiseLighting = false;
    
    if(visualiseLighting)
    {
        for(int n = 0; n < 256; n++)
        {
            uint8_t intensity = (uint8_t) (((n & 0xf) * 255) / 15);
            lut[n] = rgb(intensity, intensity, intensity);
        }
    }
}

void update(uint32_t tick) {
    //uint32_t timingSample = time();
    //uint32_t tickAccum += (timingSample - lastTimingSample);
    //lastTimingSample = timingSample;
    //const int16_t frameDuration = 1000 / TARGET_FRAMERATE;
    //uint32_t tickAccum= stats.tick_us/1000;
    //while(tickAccum > frameDuration){
    //while(tickAccum > frameDuration){
    Game::Tick();
    //tickAccum -= frameDuration;
    		//tickAccum = 0;
    	//}
    //timingSample = time();
    
}

void draw(uint32_t tick) {
    Game::Draw();
    L8ToRGBA(tempBuff,SCREEN->data, lut, 120*120);   
}