/****************************/
/*   	HIGHSCORES.C    	*/
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "infobar.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gGlobalTransparency;
extern	short	gPrefsFolderVRefNum, gCurrentSong;
extern	long	gPrefsFolderDirID;
extern	FSSpec	gDataSpec;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	u_long			gScore,gGlobalMaterialFlags,gLoadedScore;
extern	Boolean			gPlayingFromSavedGame,gAllowAudioKeys, gWonGame, gLostGame;
extern	AGLContext		gAGLContext;
extern	OGLColorRGB			gGlobalColorFilter;
extern	MOPictureObject 	*gBackgoundPicture;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupScoreScreen(void);
static void FreeScoreScreen(void);
static void DrawHighScoresCallback(OGLSetupOutputType *info);
static void DrawScoreVerbage(void);
static void DrawHighScoresAndCursor(void);
static void SetHighScoresSpriteState(void);
static void StartEnterName(void);
static void MoveHighScoresCyc(ObjNode *theNode);
static Boolean IsThisScoreInList(u_long score);
static short AddNewScore(u_long newScore);
static void SaveHighScores(void);
static void DrawScoreText(unsigned char *s, float x, float y, float scale);

/***************************/
/*    CONSTANTS            */
/***************************/


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
	unsigned char	name[MAX_NAME_LENGTH+1];
	unsigned long	score;
}HighScoreType;


/***************************/
/*    VARIABLES            */
/***************************/

static Str32	gHighScoresFileName = "\p:Billy:HighScores";

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
		const float fps = gFramesPerSecondFrac;
		
		CalcFramesPerSecond();
		MoveObjects();				
		OGL_DrawScene(gGameViewInfoPtr, DrawHighScoresCallback);	
		
		
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
		}
	}	


		/* CLEANUP */
		
	if (gNewScoreSlot != -1)						// if a new score was added then update the high scores file			
		SaveHighScores();
		
	FreeScoreScreen();
	
	GammaFadeOut();			
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
	
	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD SPRITES */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:font.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);
		
			/* CREATE BACKGROUND OBJECT */

	if (gJustShowScores)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:highscores", &spec);
		gShowScoreDelay = 0;
		gFinalScoreAlpha = 1.0f;
	}
	else
	{
		gShowScoreDelay = 3.0f;
		gFinalScoreAlpha = 0.0f;
		
		if (gWonGame)
			FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:WinScreen", &spec);
		else
			FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:LoseScreen", &spec);
	}
	
	
	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);
				
				

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
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}



/***************** DRAW HIGHSCORES CALLBACK *******************/

static void DrawHighScoresCallback(OGLSetupOutputType *info)
{
	if (gBackgoundPicture)
		MO_DrawObject(gBackgoundPicture, info);

	DrawObjects(info);


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
Str32	s;
int		texNum,n,i;
float	x;

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

	NumToString(gScore, s);
	n = s[0];										// get str len

	x = 320.0f - ((float)n / 2.0f) * MYSCORE_DIGIT_SPACING - (MYSCORE_DIGIT_SPACING/2);	// calc starting x

	
	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 0;
		
	for (i = 1; i <= n; i++)
	{
		texNum = CharToSprite(s[i]);				// get texture #

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
AGLContext agl_ctx = gAGLContext;
float	y,cursorY,cursorX;
int		i,j,n;
Str32	s;
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
	for (i = 0; i < NUM_SCORES; i++)
	{
		if (i == gNewScoreSlot)								// see if cursor will go on this line
		{
			cursorY = y;
			cursorX = 150.0f;
			
			n = gHighScores[i].name[0];						// get str len
			for (j = 1; j <= gCursorIndex; j++)
				cursorX += GetCharSpacing(gHighScores[i].name[j], SCORE_TEXT_SPACING);	// calc cursor position
			
		}
		
				/* DRAW NAME */
				
		DrawScoreText(gHighScores[i].name, 150,y, SCORE_TEXT_SPACING);
	
				/* DRAW SCORE */
				
		NumToString(gHighScores[i].score, s);	// convert score to a text string
		if (s[0] < SCORE_NUM_DIGITS)				// pad 0's
		{
			n = SCORE_NUM_DIGITS-s[0];
			BlockMove(&s[1],&s[1+n], 20);		// shift existing data over
			
			for (j = 0; j < n; j++)				// pad with 0's
				s[1+j] = '0';
				
			s[0] = SCORE_NUM_DIGITS;
		}
		DrawScoreText(s, 400, y, SCORE_TEXT_SPACING);
		
		y += SCORE_TEXT_SPACING * 1.05f;
	}	
	
	
		/*****************************/
		/* DRAW CURRENT SCORE AT TOP */
		/*****************************/
		
	if (!gJustShowScores)
	{
		NumToString(gScore, s);						// convert score to a text string
		if (s[0] < SCORE_NUM_DIGITS)				// pad 0's
		{
			n = SCORE_NUM_DIGITS-s[0];
			BlockMove(&s[1],&s[1+n], 20);			// shift existing data over
			
			for (j = 0; j < n; j++)				// pad with 0's
				s[1+j] = '0';
				
			s[0] = SCORE_NUM_DIGITS;
		}
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
		DoFatalAlert("\pLoadHighScores: Error opening High Scores file!");
	else
	{
		count = sizeof(HighScoreType) * NUM_SCORES;
		iErr = FSRead(refNum, &count,  &gHighScores[0]);								// read data from file
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
		DoAlert("\pUnable to Save High Scores file!");
		return;
	}

				/* WRITE DATA */
				
	count = sizeof(HighScoreType) * NUM_SCORES;
	FSWrite(refNum, &count, &gHighScores[0]);	
	FSClose(refNum);			

}


/**************** CLEAR HIGH SCORES **********************/

void ClearHighScores(void)
{
short				i,j;
char				blank[MAX_NAME_LENGTH] = "               ";


			/* INIT SCORES */
			
	for (i=0; i < NUM_SCORES; i++)
	{
		gHighScores[i].name[0] = MAX_NAME_LENGTH;
		for (j=0; j < MAX_NAME_LENGTH; j++)
			gHighScores[i].name[j+1] = blank[j];
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
	gHighScores[slot].name[0] = MAX_NAME_LENGTH;				// clear name
	for (i = 1; i <= MAX_NAME_LENGTH; i++)
		gHighScores[slot].name[i] = ' ';						// clear to all spaces
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

static void DrawScoreText(unsigned char *s, float x, float y, float scale)
{
int	n,i,texNum;

	n = s[0];										// get str len
	
	for (i = 1; i <= n; i++)
	{
		texNum = CharToSprite(s[i]);				// get texture #
		if (texNum != -1)
			DrawInfobarSprite2(x, y, scale, SPRITE_GROUP_FONT, texNum);
			
		x += GetCharSpacing(s[i], scale);
	}



}

