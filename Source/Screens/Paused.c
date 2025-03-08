/****************************/
/*   	PAUSED.C			*/
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NavigatePausedMenu(void);
static void DrawPaused(void);



/****************************/
/*    CONSTANTS             */
/****************************/


#define	PAUSED_FRAME_WIDTH	190

#define	LETTER_SIZE			25.0f
#define	LETTER_SPACING_Y	(LETTER_SIZE * 1.0f)


/*********************/
/*    VARIABLES      */
/*********************/

static short	gPausedMenuSelection;

static const char *gPausedStrings[3] =
{
	"Resume Game",
	"Retire Game",
	"Quit Application",
};


/********************** DO PAUSED **************************/

void DoPaused(void)
{
	Boolean hadWindowGrab = SDL_GetWindowMouseGrab(gSDLWindow);
	Boolean hadRelativeMouse = SDL_GetWindowRelativeMouseMode(gSDLWindow);
	Boolean wasCursorVisible = SDL_CursorVisible();

	SDL_SetWindowMouseGrab(gSDLWindow, false);
	SDL_SetWindowRelativeMouseMode(gSDLWindow, false);
	SDL_ShowCursor();

	gPausedMenuSelection = 0;
	
	PauseAllChannels(true);

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();		
	
	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (NavigatePausedMenu())
			break;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();		
		KeepTerrainAlive();		// need to call this to keep supertiles active
		OGL_DrawScene(DrawPaused);
	}

	PauseAllChannels(false);

	SDL_SetWindowMouseGrab(gSDLWindow, hadWindowGrab);
	SDL_SetWindowRelativeMouseMode(gSDLWindow, hadRelativeMouse);
	if (!wasCursorVisible)
		SDL_HideCursor();
}


/*********************** DRAW PAUSED ***************************/

static void DrawPaused(void)
{
float	x = 0;
float	y = 0;
float	leftX = 0;
float	dotX = 0;
float	dotY = -100;	// offscreen when we're not selecting anything
static float	dotAlpha = 1.0f;

			/* DRAW THE BACKGROUND */
	
	DefaultDrawCallback();

	
			/*************************/
			/* DRAW THE PAUSED STUFF */
			/*************************/

	OGL_PushState();
	SetInfobarSpriteState(3);
			
	SetColor4f(1,1,1,1);
	gGlobalTransparency = 1.0f;


			/* DRAW FRAME FIRST */
			
	x = (640-PAUSED_FRAME_WIDTH)/2;
	y = 210;
	DrawInfobarSprite2(x, y, PAUSED_FRAME_WIDTH, SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_PausedFrame);


				/* DRAW TEXT */
		
	switch (gCurrentArea)
	{
		case AREA_TOWN_SHOOTOUT:
		case AREA_SWAMP_SHOOTOUT:
//			DrawFontString("SHOOTOUT", 320, 370, 30, true);
			DrawFontString("DUCK with Ctrl or Right Click", 320, 400, 20, true);
			DrawFontString("TURN with Arrows or A/D", 320, 420, 20, true);
//			DrawFontString("hold Shift to turn FASTER", 320, 440, 20, true);
			break;

		case AREA_TOWN_STAMPEDE:
		case AREA_SWAMP_STAMPEDE:
//			DrawFontString("STAMPEDE", 320, 370, 30, true);
//			DrawFontString("get to the finish line before the kangacows!", 320, 400, 20, true);
			DrawFontString("TURN with Arrows or A/D", 320, 420, 20, true);
			DrawFontString("JUMP with Spacebar", 320, 440, 20, true);
			break;
	}

		
	leftX = x + 15.0f;
	y += 9; 
			
	for (int j = 0; j < 3; j++)										// 3 lines of text
	{
		x = leftX;
	
		if (j == gPausedMenuSelection)								// remember where to put dot
		{
			gGlobalColorFilter.r = 1;
			gGlobalColorFilter.g = 1;
			gGlobalColorFilter.b = .2;
			
			dotX = x - 13.0f;
			dotY = y + 5.0f;		
		}	
		else
		{
			gGlobalColorFilter.r = 1;
			gGlobalColorFilter.g = 1;
			gGlobalColorFilter.b = 1;
		}

		int i = 0;
		while (gPausedStrings[j][i] != 0x00)
		{
			char	c = gPausedStrings[j][i];	// get char
			int		texNum = CharToSprite(c);						// convert to texture #
			if (texNum != -1)
				DrawInfobarSprite2(x, y, LETTER_SIZE, SPRITE_GROUP_FONT, texNum);

			x += GetCharSpacing(c, LETTER_SIZE);
			i++;
		}
		y += LETTER_SPACING_Y;
	}

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;

			/* DRAW SELECT DOT */

	dotAlpha += gFramesPerSecondFrac * 18.0f;					// occilate the dot
	if (dotAlpha > PI2)
		dotAlpha -= PI2;		
	gGlobalTransparency = (1.0f + sin(dotAlpha)) * .8f;

	DrawInfobarSprite2(dotX, dotY, 16, SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_PausedDot);

	gGlobalTransparency = 1.0f;

			
	OGL_PopState();			
}








/*********************** NAVIGATE PAUSED MENU **************************/

static Boolean NavigatePausedMenu(void)
{
Boolean	continueGame = false;
int	oldSelection = gPausedMenuSelection;

		/* SEE IF CHANGE SELECTION */
		
	if (GetNewNeedState(kNeed_UIUp) && (gPausedMenuSelection > 0))
		gPausedMenuSelection--;
	else
	if (GetNewNeedState(kNeed_UIDown) && (gPausedMenuSelection < 2))
		gPausedMenuSelection++;
	
			/* SEE IF USE MOUSE DELTAS */
			
	else if (gMouseDeltaY != 0)
	{
		gPausedMenuSelection = -1;

		OGLPoint2D mouse = GetLogicalMouseCoord();

		float YY = 210 + (LETTER_SPACING_Y) * 0.5f;
		float HH = LETTER_SPACING_Y;
		if (mouse.x > 320 - PAUSED_FRAME_WIDTH / 2 && mouse.x < 320 + PAUSED_FRAME_WIDTH / 2)
		{
			if (mouse.y > YY && mouse.y < YY + HH)
				gPausedMenuSelection = 0;
			else if (mouse.y > YY + HH && mouse.y < YY + 2 * HH)
				gPausedMenuSelection = 1;
			else if (mouse.y > YY + 2 * HH && mouse.y < YY + 3 * HH)
				gPausedMenuSelection = 2;
		}
	}
	
	
	if (gPausedMenuSelection != oldSelection)
		PlayEffect_Parms(EFFECT_SPURS1,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
		
	
			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	// Only check mouse clicks if we have focus to prevent misclicks
	bool gotMouseFocus = SDL_GetWindowFlags(gSDLWindow) & SDL_WINDOW_MOUSE_FOCUS;

	if (GetNewNeedState(kNeed_UIConfirm)
		|| (gotMouseFocus && GetNewClickState(1)))
	{
		PlayEffect_Parms(EFFECT_SPURS2,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);

		switch(gPausedMenuSelection)
		{
			case	0:								// RESUME
					continueGame = true;
					break;
					
			case	1:								// EXIT
					gGameOver = true;
					continueGame = true;
					break;

			case	2:								// QUIT
					CleanQuit();
					break;
										
		}
	}	
	
	
			/*****************************/
			/* SEE IF CANCEL A SELECTION */
			/*****************************/

	else
	if (GetNewNeedState(kNeed_UIPause)
		|| GetNewNeedState(kNeed_UIBack)
		|| GetNewClickState(3))
	{
		continueGame = true;
	}	
	
	
	return(continueGame);
}
