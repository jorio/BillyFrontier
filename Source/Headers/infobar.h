//
// infobar.h
//

// Externals
#include "game.h"

void InitInfobar(void);
void DrawInfobar(void);
void DrawInfobarSprite(float x, float y, float size, short texNum);
void DrawInfobarSprite2_Centered(float x, float y, float size, short group, short texNum);
void DrawInfobarSprite2(float x, float y, float size, short group, short texNum);
void DrawInfobarSprite3(float x, float y, float size, short texNum);
void Infobar_DrawNumber(int number, float x, float y, float scale, int numDigits, Boolean showLeading);

void SetInfobarSpriteState(float anaglyphZ);
