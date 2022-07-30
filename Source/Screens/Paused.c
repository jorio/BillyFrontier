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
static void DrawPaused(OGLSetupOutputType *setupInfo);



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

static const char *gPausedStrings[MAX_LANGUAGES][3] =
{
		/* ENGLISH */
	{	
		"Resume Game",
		"Retire Game",
		"Quit Application",
	},
	
		/* FRENCH */
	{	
		"Continuer",
		"Abondonner",
		"Quitter",
	},

		/* GERMAN */
	{	
		"Spiel Fortsetzen",
		"Spiel Abbrechen",
		"Beenden",
	},

		/* SPANISH */
	{	
		"Reanudar",
		"Retirarse",
		"Cerrar el Juego",
	},

		/* ITALIAN */
	{	
		"Continua",
		"Annulla",
		"Esci dal Gioco",
	},

		/* SWEDISH */
	{	
		"�teruppta Spelet",
		"Dra dig ur Spelet",
		"Avsluta",
	},

		/* DUTCH */
	{	
		"Hervat Spel",
		"Stop Spel",
		"Stop Programma",
	}
	
};


/********************** DO PAUSED **************************/

void DoPaused(void)
{
Boolean	oldMute = gMuteMusicFlag;


	
	gPausedMenuSelection = 0;
	
	if (!gMuteMusicFlag)							// see if pause music
		ToggleMusic();

	

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
		OGL_DrawScene(gGameViewInfoPtr, DrawPaused);

	}	
	
	
	
	if (!oldMute)									// see if restart music
		ToggleMusic();

	
}


/*********************** DRAW PAUSED ***************************/

static void DrawPaused(OGLSetupOutputType *setupInfo)
{
float	x,y,leftX;
float	dotX,dotY;
int		i,j;
static float	dotAlpha = 1.0f;

			/* DRAW THE BACKGROUND */
	
	DefaultDrawCallback(setupInfo);

	
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
		
	
		
	leftX = x + 15.0f;
	y += 9; 
			
	for (j = 0; j < 3; j++)											// 3 lines of text
	{		
		x = leftX;
		i = 0;
	
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
	
		while (gPausedStrings[gGamePrefs.language][j][i] != 0x00)
		{
			char	c = gPausedStrings[gGamePrefs.language][j][i];	// get char
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
		
	if (GetNewKeyState_Real(KEY_UP) && (gPausedMenuSelection > 0))
		gPausedMenuSelection--;
	else
	if (GetNewKeyState_Real(KEY_DOWN) && (gPausedMenuSelection < 2))
		gPausedMenuSelection++;
	
			/* SEE IF USE MOUSE DELTAS */
			
	else
	{
		if (gMouseDeltaY != 0)
		{
			Point	pt;
			GetMouseCoord(&pt);
			gPausedMenuSelection = ((float)pt.v / ((float)gGameWindowHeight * .2f)) * 3.0f;
			if (gPausedMenuSelection < 0)
				gPausedMenuSelection = 0; 
			if (gPausedMenuSelection > 2)
				gPausedMenuSelection = 2; 
		}	
	}
	
	
	if (gPausedMenuSelection != oldSelection)
		PlayEffect_Parms(EFFECT_SPURS1,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
		
	
			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/
			
	if (GetNewKeyState_Real(KEY_RETURN) || GetNewKeyState_Real(KEY_SPACE) || gMouseNewButtonState)
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
	if (GetNewKeyState_Real(KEY_ESC))
	{
		continueGame = true;
	}	
	
	
	return(continueGame);
}



