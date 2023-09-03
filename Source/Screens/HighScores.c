/****************************/
/*   	HIGHSCORES.C    	*/
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

static void SetupScoreScreen(void);
static void FreeScoreScreen(void);
static void DrawHighScoresAndCursor(ObjNode* theNode);
static void SetHighScoresSpriteState(void);
static Boolean IsThisScoreInList(uint32_t score);
static short AddNewScore(uint32_t newScore);
static void SaveHighScores(void);
static void DrawScoreText(const char* s, float x, float y, float scale);

/***************************/
/*    CONSTANTS            */
/***************************/

static const char* kScoreCharset = " .-?!'0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define	NUM_SCORES		10
#define	MAX_NAME_LENGTH	19

#define	SCORE_TEXT_SPACING	25.0f

enum
{
	HIGHSCORES_SObjType_EnterNameText,
	HIGHSCORES_SObjType_ScoreText
};


#define MYSCORE_DIGIT_SPACING 	40.0f

typedef struct
{
	char		name[MAX_NAME_LENGTH+1];
	uint32_t	score;
}HighScoreType;


/***************************/
/*    VARIABLES            */
/***************************/

HighScoreType	gHighScores[NUM_SCORES];	

static	float	gFinalScoreTimer,gFinalScoreAlpha, gCursorFlux = 0;

static	short	gNewScoreSlot = -1;
static	short	gCursorIndex = 0;

static	Boolean	gExitHighScores = false;
static	Boolean	gJustShowScores = false;

static	float	gShowScoreDelay;



static void ClearHighScoreName(char name[MAX_NAME_LENGTH + 1])
{
	memset(name, ' ', MAX_NAME_LENGTH);
	name[MAX_NAME_LENGTH] = '\0';
}

/*********************** NEW SCORE ***********************************/
//
// INPUT: justShowScores = true if we just want to see the scores, not really add a new one
//

void NewScore(Boolean justShowScores)
{
	gJustShowScores = justShowScores;
		
	gAllowAudioKeys = false;					// dont interfere with name editing
	
	gCursorIndex = 0;

	if (gLostGame)
		PlaySong(SONG_LOSE, true);
	else
	if (gCurrentSong != SONG_THEME)
		PlaySong(SONG_THEME, true);

	
			/* INIT */
			

	LoadHighScores();										// make sure current scores are loaded
	SetupScoreScreen();										// setup OGL
	MakeFadeEvent(true);



			/* LOOP */
			
	MyFlushEvents();
	CalcFramesPerSecond();
		
	while(!gExitHighScores)
	{
		CalcFramesPerSecond();
		MoveObjects();				
		OGL_DrawScene(DrawObjects);
		

		ReadKeyboard();

				/***********************/
				/* JUST SHOWING SCORES */
				/***********************/
		
		if (justShowScores || (gNewScoreSlot == -1))
		{
			if (gShowScoreDelay <= 0.0f)
			{
				if (UserWantsOut() || GetNewClickState(1))
					gExitHighScores = true;		
			}
		}
				/*****************************/
				/* SEE IF USER ENTERING NAME */
				/*****************************/
		else if (gShowScoreDelay > 0.0f)
		{
			InvalidateAllInputs();
		}
		else
		{
			if (GetNewKeyState(SDL_SCANCODE_RETURN) || GetNewKeyState(SDL_SCANCODE_KP_ENTER))
			{
				gExitHighScores = true;
			}
			else if (GetNewKeyState(SDL_SCANCODE_LEFT))
			{
				if (gCursorIndex > 0)
					gCursorIndex--;
			}
			else if (GetNewKeyState(SDL_SCANCODE_RIGHT))
			{
				if (gCursorIndex < (MAX_NAME_LENGTH - 1))
					gCursorIndex++;
				else
					gCursorIndex = MAX_NAME_LENGTH - 1;
			}
			else if (GetNewKeyState(SDL_SCANCODE_BACKSPACE))
			{
				if (gCursorIndex > 0)
				{
					gCursorIndex--;
					for (int i = gCursorIndex; i < MAX_NAME_LENGTH; i++)
						gHighScores[gNewScoreSlot].name[i] = gHighScores[gNewScoreSlot].name[i + 1];
					gHighScores[gNewScoreSlot].name[MAX_NAME_LENGTH-1] = ' ';
				}
			}
			else if (GetNewKeyState(SDL_SCANCODE_DELETE))
			{
				if (gCursorIndex < MAX_NAME_LENGTH)
				{
					for (int i = gCursorIndex; i < MAX_NAME_LENGTH - 1; i++)
						gHighScores[gNewScoreSlot].name[i] = gHighScores[gNewScoreSlot].name[i + 1];
					gHighScores[gNewScoreSlot].name[MAX_NAME_LENGTH - 1] = ' ';
				}
			}
			else if (gTextInput[0])
			{
				char theChar = gTextInput[0];
				if (gCursorIndex < MAX_NAME_LENGTH)								// dont add anything more if maxxed out now
				{
					if ((theChar >= 'a') && (theChar <= 'z'))					// see if convert lower case to upper case a..z
						theChar = 'A' + (theChar - 'a');

					if (NULL == strchr(kScoreCharset, theChar))
						theChar = '-';

					gHighScores[gNewScoreSlot].name[gCursorIndex] = theChar;
					gCursorIndex++;
				}
			}
		}
	}	


	OGL_FadeOutScene(DrawObjects, MoveObjects);

		/* CLEANUP */
		
	if (gNewScoreSlot != -1)						// if a new score was added then update the high scores file			
		SaveHighScores();
		
	FreeScoreScreen();

	gAllowAudioKeys = true;
}


/********************* SETUP SCORE SCREEN **********************/

static void SetupScoreScreen(void)
{
OGLSetupInputType	viewDef;

	gExitHighScores = false;


		/* IF THIS WAS A SAVED GAME AND SCORE HASN'T CHANGED AND IS ALREADY IN LIST THEN DON'T ADD TO HIGH SCORES */
		
	if (!gJustShowScores)
	{	
		if (gPlayingFromSavedGame && (gScore == gLoadedScore) && IsThisScoreInList(gScore))
		{
			gFinalScoreTimer = 4.0f;
			gNewScoreSlot = -1;
		}
		
				/* NOT SAVED GAME OR A BETTER SCORE THAN WAS LOADED OR ISN'T IN LIST YET */
		else
		{
			gNewScoreSlot = AddNewScore(gScore);				// try to add new score to high scores list
			if (gNewScoreSlot == -1)							// -1 if not added
				gFinalScoreTimer = 4.0f;
			else
				gFinalScoreTimer = 3.0f;
		}
	}


			/**************/
			/* SETUP VIEW */
			/**************/
			
	OGL_NewViewDef(&viewDef);	

	viewDef.camera.fov 			= 1.0;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 5000;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.z		= 800;
	viewDef.camera.from.y		= -350;
	
	OGL_SetupWindow(&viewDef);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD SPRITES */

	LoadSpriteGroup(SPRITE_GROUP_FONT);

			/* CREATE BACKGROUND OBJECT */

	const char* backgroundImagePath = NULL;

	if (gJustShowScores)
	{
		backgroundImagePath = ":images:HighScores.jpg";
		gShowScoreDelay = 0;
		gFinalScoreAlpha = 1.0f;
	}
	else
	{
		gShowScoreDelay = 3.0f;
		gFinalScoreAlpha = 0.0f;
		
		if (gWonGame)
			backgroundImagePath = ":images:WinScreen.jpg";
		else
			backgroundImagePath = ":images:LoseScreen.jpg";
	}

	MakeBackgroundPictureObject(backgroundImagePath);

			/* SCENE DRIVER */

	NewObjectDefinitionType def =
	{
		.scale = 1,
		.genre = CUSTOM_GENRE,
		.slot = SPRITE_SLOT
	};
	ObjNode* scene = MakeNewObject(&def);
	scene->CustomDrawFunction = DrawHighScoresAndCursor;
}




/********************** FREE SCORE SCREEN **********************/

static void FreeScoreScreen(void)
{				
	MyFlushEvents();
	DeleteAllObjects();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup();
}


/********************* SET HIGHSCORES SPRITE STATE ***************/

static void SetHighScoresSpriteState(void)
{
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);							
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures
	

			/* INIT MATRICES */
					
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	OGL_Ortho2DLogicalSize();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();					
}


/****************** DRAW HIGH SCORES AND CURSOR ***********************/

static void DrawHighScoresAndCursor(ObjNode* theNode)
{
float	y = 0;
float	cursorY = 0;
float	cursorX = 0;
char	s[16];
float	fps = gFramesPerSecondFrac;

	(void) theNode;


		/* DELAY THE DISPLAY? */
		
	if (gShowScoreDelay > 0.0f)
	{
		gShowScoreDelay -= fps;
		
		if (gShowScoreDelay <= 0.0f)
			MakeDarkenPane();
		return;
	}




		/* DRAW SPRITES */

	OGL_PushState();

	SetHighScoresSpriteState();



	gFinalScoreAlpha += fps;						// fade in
	if (gFinalScoreAlpha > .99f)
		gFinalScoreAlpha = .99f;

	
 	gCursorFlux += gFramesPerSecondFrac * 10.0f;
 	
 	
			/****************************/
			/* DRAW ENTER NAME VERBAGE */
			/****************************/
	
			/* DRAW TEXT */
			
	gGlobalTransparency = gFinalScoreAlpha;
	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 0;
 	
			/*****************/
			/* DRAW THE TEXT */
			/*****************/
		
		
	y = 120;
	for (int i = 0; i < NUM_SCORES; i++)
	{
		if (i == gNewScoreSlot)								// see if cursor will go on this line
		{
			cursorY = y;
			cursorX = 150.0f;
			
			const char* name = gHighScores[i].name;						// get str len
			int rem = gCursorIndex;
			for (; *name && rem > 0; name++)
			{
				cursorX += GetCharSpacing(*name, SCORE_TEXT_SPACING);	// calc cursor position
				rem--;
			}
		}
		
				/* DRAW NAME */
				
		DrawScoreText(gHighScores[i].name, 150,y, SCORE_TEXT_SPACING);
	
				/* DRAW SCORE */
				
		snprintf(s, sizeof(s), SCORE_FMT, gHighScores[i].score);
		DrawScoreText(s, 450, y, SCORE_TEXT_SPACING);
		
		y += SCORE_TEXT_SPACING * 1.05f;
	}	
	
	
		/*****************************/
		/* DRAW CURRENT SCORE AT TOP */
		/*****************************/
		
	if (!gJustShowScores)
	{
		snprintf(s, sizeof(s), SCORE_FMT, gScore);
		DrawScoreText(s, 270, 56, SCORE_TEXT_SPACING * 2);
	}	
	
		/*******************/	
		/* DRAW THE CURSOR */
		/*******************/
			
	if ((!gJustShowScores) && (gNewScoreSlot != -1))
	{
		gGlobalTransparency = (.3f + ((sin(gCursorFlux) + 1.0f) * .5f) * .699f) * gFinalScoreAlpha;
		DrawInfobarSprite2(cursorX, cursorY, SCORE_TEXT_SPACING, SPRITE_GROUP_FONT, FONT_SObjType_Cursor);
	}	
			
			
		
			/***********/
			/* CLEANUP */
			/***********/

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;

	gGlobalTransparency = 1;

	OGL_PopState();
	gGlobalMaterialFlags = 0;
}



#pragma mark -


/*********************** LOAD HIGH SCORES ********************************/

void LoadHighScores(void)
{
	ClearHighScores();	// clear them beforehand in case loading fails

	LoadUserDataFile(SCORES_FILE_NAME, SCORES_MAGIC, sizeof(gHighScores), (Ptr) &gHighScores[0]);
}


/************************ SAVE HIGH SCORES ******************************/

static void SaveHighScores(void)
{
	SaveUserDataFile(SCORES_FILE_NAME, SCORES_MAGIC, sizeof(gHighScores), (Ptr) &gHighScores[0]);
}


/**************** CLEAR HIGH SCORES **********************/

void ClearHighScores(void)
{
	for (int i = 0; i < NUM_SCORES; i++)
	{
		ClearHighScoreName(gHighScores[i].name);
		gHighScores[i].score = 0;
	}

	SaveHighScores();
}


/*************************** ADD NEW SCORE ****************************/
//
// Returns high score slot that score was inserted to or -1 if none
//

static short AddNewScore(uint32_t newScore)
{
short	slot,i;

			/* FIND INSERT SLOT */
	
	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (newScore > gHighScores[slot].score)
			goto	got_slot;
	}
	return(-1);
	
	
got_slot:	
			/* INSERT INTO LIST */

	for (i = NUM_SCORES-1; i > slot; i--)						// make hole
		gHighScores[i] = gHighScores[i-1];
	gHighScores[slot].score = newScore;							// set score in structure
	ClearHighScoreName(gHighScores[slot].name);					// clear name
	return(slot);
}





#pragma mark -

/****************** IS THIS SCORE IN LIST *********************/
//
// Returns True if this score value is anywhere in the high scores already
//

static Boolean IsThisScoreInList(uint32_t score)
{
short	slot;

	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (gHighScores[slot].score == score)
			return(true);
	}

	return(false);
}




/***************** DRAW SCORE TEXT ***********************/

static void DrawScoreText(const char* s, float x, float y, float scale)
{
	for (const char* cursor = s; *cursor; cursor++)
	{
		char c = *cursor;

		int texNum = CharToSprite(c);				// get texture #
		if (texNum != -1)
			DrawInfobarSprite2(x, y, scale, SPRITE_GROUP_FONT, texNum);
			
		x += GetCharSpacing(c, scale);
	}
}

