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
static void DrawHighScoresCallback(void);
static void DrawScoreVerbage(void);
static void DrawHighScoresAndCursor(void);
static void SetHighScoresSpriteState(void);
static void StartEnterName(void);
static void MoveHighScoresCyc(ObjNode *theNode);
static Boolean IsThisScoreInList(u_long score);
static short AddNewScore(u_long newScore);
static void SaveHighScores(void);
static void DrawScoreText(const char* s, float x, float y, float scale);

/***************************/
/*    CONSTANTS            */
/***************************/

static const char* kEmptyScorePlaceholder = "--------";

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

static Str32	gHighScoresFileName = ":Billy:HighScores";

HighScoreType	gHighScores[NUM_SCORES];	

static	float	gFinalScoreTimer,gFinalScoreAlpha, gCursorFlux = 0;

static	short	gNewScoreSlot,gCursorIndex;

static	Boolean	gExitHighScores, gJustShowScores;

static	float	gShowScoreDelay;


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
			
	TurnOffISp();
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
		OGL_DrawScene(DrawHighScoresCallback);	
		
		
				/***********************/
				/* JUST SHOWING SCORES */
				/***********************/
		
		if (justShowScores || (gNewScoreSlot == -1))
		{
				ReadKeyboard();
			if (gShowScoreDelay <= 0.0f)
			{
			
				if (AreAnyNewKeysPressed() || Button())
					gExitHighScores = true;		
			}
		}
		
		
				/*****************************/
				/* SEE IF USER ENTERING NAME */
				/*****************************/
		
		else	
		{
			IMPLEMENT_ME();
#if 0
			EventRecord 	theEvent;
			
			GetNextEvent(keyDownMask|autoKeyMask, &theEvent);							// poll event queue
			if ((theEvent.what == keyDown) || (theEvent.what == autoKeyMask))			// see if key pressed
			{
				char	theChar = theEvent.message & charCodeMask;						// extract key
				int		i;
		
				switch(theChar)
				{
					case	CHAR_RETURN:
					case	CHAR_ENTER:
							gExitHighScores = true;
							break;
							
					case	CHAR_LEFT:
							if (gCursorIndex > 0)
								gCursorIndex--;
							break;

					case	CHAR_RIGHT:
							if (gCursorIndex < (MAX_NAME_LENGTH-1))
								gCursorIndex++;
							else
								gCursorIndex = MAX_NAME_LENGTH-1;
							break;

					case	CHAR_DELETE:
							if (gCursorIndex > 0)
							{
								gCursorIndex--;
								for (i = gCursorIndex+1; i < MAX_NAME_LENGTH; i++)
									gHighScores[gNewScoreSlot].name[i] = gHighScores[gNewScoreSlot].name[i+1];
								gHighScores[gNewScoreSlot].name[MAX_NAME_LENGTH] = ' ';
							}
							break;
							
					default:
							if (gCursorIndex < MAX_NAME_LENGTH)								// dont add anything more if maxxed out now
							{
								if ((theChar >= 'a') && (theChar <= 'z'))					// see if convert lower case to upper case a..z
									theChar = 'A' + (theChar-'a');
								gHighScores[gNewScoreSlot].name[gCursorIndex+1] = theChar;
								gCursorIndex++;
							}
				}
							
			}
#endif
		}
	}	


	OGL_FadeOutScene(DrawHighScoresCallback, MoveObjects);

		/* CLEANUP */
		
	if (gNewScoreSlot != -1)						// if a new score was added then update the high scores file			
		SaveHighScores();
		
	FreeScoreScreen();
	
	TurnOnISp();
	
	gAllowAudioKeys = true;
}


/********************* SETUP SCORE SCREEN **********************/

static void SetupScoreScreen(void)
{
FSSpec				spec;
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
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:font.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT);
		
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

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, 0, backgroundImagePath);
}




/********************** FREE SCORE SCREEN **********************/

static void FreeScoreScreen(void)
{				
	MyFlushEvents();
	DeleteAllObjects();
	if (gBackgoundPicture)
	{
		MO_DisposeObjectReference(gBackgoundPicture);	
		gBackgoundPicture = nil;
	}
	DisposeAllSpriteGroups();	
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup();
}



/***************** DRAW HIGHSCORES CALLBACK *******************/

static void DrawHighScoresCallback(void)
{
	if (gBackgoundPicture)
		MO_DrawObject(gBackgoundPicture);

	DrawObjects();


			/* DRAW SPRITES */
			
	OGL_PushState();

	SetHighScoresSpriteState();

	DrawHighScoresAndCursor();


	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;
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
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();					
}

/********************* DRAW SCORE VERBAGE ****************************/

static void DrawScoreVerbage(void)
{
char	s[32];
int		texNum;

				/* SEE IF DONE */
				
	gFinalScoreTimer -= gFramesPerSecondFrac;
	if (gFinalScoreTimer <= 0.0f)
	{
		if (gNewScoreSlot != -1)							// see if bail or if let player enter name for high score
		{
			StartEnterName();
		}
		else
			gExitHighScores = true;
		return;
	}
	if (gFinalScoreTimer < 1.0f)							// fade out
		gFinalScoreAlpha = gFinalScoreTimer;


			/****************************/
			/* DRAW BONUS TOTAL VERBAGE */
			/****************************/
			
	gGlobalTransparency = gFinalScoreAlpha;


			/**************/
			/* DRAW SCORE */
			/**************/

	int n = snprintf(s, sizeof(s), "%d", gScore);

	float x = 320.0f - ((float)n / 2.0f) * MYSCORE_DIGIT_SPACING - (MYSCORE_DIGIT_SPACING/2);	// calc starting x

	
	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 0;
		
	for (const char* cursor = s; *cursor; cursor++)
	{
		texNum = CharToSprite(*cursor);				// get texture #

		DrawInfobarSprite2(x, 240, MYSCORE_DIGIT_SPACING * 1.9f, SPRITE_GROUP_FONT, texNum);
		x += MYSCORE_DIGIT_SPACING;
	}
	
	
	gGlobalTransparency = 1.0f;
	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;
}


/****************** DRAW HIGH SCORES AND CURSOR ***********************/

static void DrawHighScoresAndCursor(void)
{
float	y = 0;
float	cursorY = 0;
float	cursorX = 0;
char	s[16];
float	fps = gFramesPerSecondFrac;

		/* DELAY THE DISPLAY? */
		
	if (gShowScoreDelay > 0.0f)
	{
		gShowScoreDelay -= fps;
		
		if (gShowScoreDelay <= 0.0f)
			MakeDarkenPane();
		return;
	}

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
			for (; *name; name++)
				cursorX += GetCharSpacing(*name, SCORE_TEXT_SPACING);	// calc cursor position
		}
		
				/* DRAW NAME */
				
		DrawScoreText(gHighScores[i].name, 150,y, SCORE_TEXT_SPACING);
	
				/* DRAW SCORE */
				
		snprintf(s, sizeof(s), SCORE_FMT, gHighScores[i].score);
		DrawScoreText(s, 400, y, SCORE_TEXT_SPACING);
		
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
}



/************************* START ENTER NAME **********************************/

static void StartEnterName(void)
{
	gCursorIndex = 0;
	MyFlushEvents();
}



/************************ MOVE HIGH SCORES CYC *******************************/

static void MoveHighScoresCyc(ObjNode *theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac * .07f;
	theNode->Rot.z += gFramesPerSecondFrac * .05f;

	UpdateObjectTransforms(theNode);
}


#pragma mark -


/*********************** LOAD HIGH SCORES ********************************/

void LoadHighScores(void)
{
OSErr				iErr;
short				refNum;
FSSpec				file;
long				count;

				/* OPEN FILE */
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);	
	if (iErr == fnfErr)
		ClearHighScores();
	else
	if (iErr)
		DoFatalAlert("LoadHighScores: Error opening High Scores file!");
	else
	{
		count = sizeof(HighScoreType) * NUM_SCORES;
		iErr = FSRead(refNum, &count, (Ptr) &gHighScores[0]);				// read data from file
		if (iErr)
		{
			FSClose(refNum);			
			FSpDelete(&file);												// file is corrupt, so delete
			return;
		}
		FSClose(refNum);			
	}	
}


/************************ SAVE HIGH SCORES ******************************/

static void SaveHighScores(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

				/* CREATE BLANK FILE */
				
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, kGameID, 'Skor', smSystemScript);					// create blank file
	if (iErr)
		goto err;


				/* OPEN FILE */
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
err:	
		DoAlert("Unable to Save High Scores file!");
		return;
	}

				/* WRITE DATA */
				
	count = sizeof(HighScoreType) * NUM_SCORES;
	FSWrite(refNum, &count, (Ptr) & gHighScores[0]);
	FSClose(refNum);			

}


/**************** CLEAR HIGH SCORES **********************/

void ClearHighScores(void)
{
	for (int i = 0; i < NUM_SCORES; i++)
	{
		snprintf(gHighScores[i].name, sizeof(gHighScores[i].name), "%s", kEmptyScorePlaceholder);
		gHighScores[i].score = 0;
	}

	SaveHighScores();
}


/*************************** ADD NEW SCORE ****************************/
//
// Returns high score slot that score was inserted to or -1 if none
//

static short AddNewScore(u_long newScore)
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
	snprintf(gHighScores[slot].name, sizeof(gHighScores[slot].name), "%s", kEmptyScorePlaceholder);		// clear name
	return(slot);
}





#pragma mark -

/****************** IS THIS SCORE IN LIST *********************/
//
// Returns True if this score value is anywhere in the high scores already
//

static Boolean IsThisScoreInList(u_long score)
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

