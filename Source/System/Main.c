/****************************/
/*    	BILLY FRONTIER - MAIN 		*/
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

static void PlayGame_Arcade(void);


/****************************/
/*    CONSTANTS             */
/****************************/




/****************************/
/*    VARIABLES             */
/****************************/

OGLVector3D			gWorldSunDirection = { .5, -.9, .8};		// also serves as lense flare vector
OGLColorRGBA		gFillColor1 = { 1.0, 1.0, 1.0, 1};


short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

float				gGravity = NORMAL_GRAVITY;

Byte				gDebugMode = 0;				// 0 == none, 1 = fps, 2 = all

uint32_t			gAutoFadeStatusBits;
short				gMainAppRezFile;

OGLSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType			gGamePrefs;

extern FSSpec				gDataSpec;



uint32_t			gGameFrameNum = 0;

Boolean				gPlayingFromSavedGame = false;
Boolean				gGameOver = false;
Boolean				gWonGame, gLostGame;
Boolean				gLevelCompleted = false;
float				gLevelCompletedCoolDownTimer = 0;
int					gCurrentArea;

uint32_t			gScore,gLoadedScore;


Boolean				gDuelWon[NUM_LEVELS];
Boolean				gLevelWon[NUM_LEVELS];


//======================================================================================
//======================================================================================
//======================================================================================



/****************** TOOLBOX INIT  *****************/

void ToolBoxInit(void)
{
OSErr		iErr;

	gMainAppRezFile = CurResFile();

			/* CHECK PREFERENCES FOLDER */

	InitPrefsFolder(true);

			/* DATA FOLDER IS SET IN C++ ENTRY POINT */
}


/************************* INIT DEFAULT PREFS **************************/

void InitDefaultPrefs(void)
{
	memset(&gGamePrefs, 0, sizeof(gGamePrefs));

//	gGamePrefs.version				= CURRENT_PREFS_VERS;						
	gGamePrefs.anaglyph				= false;
	gGamePrefs.anaglyphColor		= true;

	gGamePrefs.antialiasingLevel	= 0;
	gGamePrefs.fullscreen			= true;
}


#pragma mark -



/******************** PLAY GAME: ARCADE ************************/

static void PlayGame_Arcade(void)
{
int		i;
u_long	oldScore;

	gGameOver = gWonGame = gLostGame = false;


			/***********************/
			/* GAME INITIALIZATION */
			/***********************/

	InitPlayerInfo_Game();					// init player info for entire game

	do
	{
		
				/**********************/
				/* SHOW THE BIG-BOARD */
				/**********************/
				
		DoBigBoardScreen();		
		if (gGameOver)						// did user want to bail?
			break;

		MyFlushEvents();

		oldScore = gScore;					// remember score in case we get killed and have to revert it



			/***********************/
			/* FIRST PLAY THE DUEL */
			/***********************/

		gCurrentArea--;							// dec by 1 so that we're on the duel for this mini-game
		if (!gDuelWon[gCurrentArea/2])				// see if already won this previously
		{

			PlayDuel(gCurrentArea / 2);


				/* DID WE LOSE THE DUEL OR ABORT? */
					
			if (gGameOver)
				return;

				/* DID WE LOSE A LIFE? */
				
			if (gPlayerIsDead)
			{
				gScore = oldScore;				// revert the score
				gPlayerInfo.lives--;
				if (gPlayerInfo.lives < 0)
				{
					gGameOver = true;
					gLostGame = true;
				}
				goto next;
			}

			gDuelWon[gCurrentArea/2] = true;

		}
		gCurrentArea++;

			/**************************/
			/* NOW PLAY THE MINI-GAME */
			/**************************/

		oldScore = gScore;					// remember score in case we get killed and have to revert it

		switch(gCurrentArea)
		{
			        /* PLAY SHOOTOUT */
			        
			case	AREA_TOWN_SHOOTOUT:
			case	AREA_SWAMP_SHOOTOUT:
					PlayShootout();
					break;


			        /* PLAY STAMPEDE */
			        
			case	AREA_TOWN_STAMPEDE:
			case	AREA_SWAMP_STAMPEDE:
					PlayStampede();
					break;
					
					
					/* TARGET PRACTICE */
					
			case	AREA_TARGETPRACTICE1:
			case	AREA_TARGETPRACTICE2:
					PlayTargetPractice();
					break;
		}				

			/***********************/
			/* DID WE LOSE A LIFE? */
			/***********************/
					
		if (gPlayerIsDead)
		{
			gScore = oldScore;				// revert the score
		
			gPlayerInfo.lives--;
			if (gPlayerInfo.lives < 0)
			{
				gGameOver = true;
				gLostGame = true;
			}
		}
		
		
			/*****************************/
			/* DID WE JUST WIN THE GAME? */
			/*****************************/

		for (i = 0; i < NUM_LEVELS; i++)
			if (!gLevelWon[i])
				goto next;		
	
		gWonGame = true;
		gGameOver = true;
		
			
next:;
	}while(!gGameOver);
	
	
			/* IF GAME ENDED THEN DO HIGH SCORES */
			
	if (gWonGame || gLostGame)
		NewScore(false);
}


#pragma mark -

 
/****************** DEFAULT DRAW CALLBACK *****************************/

void DefaultDrawCallback(void)
{
		/* DRAW OBJECTS & TERAIN */	

	DrawObjects();											// draw objNodes which includes fences, terrain, etc.

			/* DRAW MISC */

	DrawSparkles();											// draw light sparkles
	DrawLensFlare();										// draw lens flare
}


 
/***************** START LEVEL COMPLETION ****************/

void StartLevelCompletion(float coolDownTimer)
{
	if (!gLevelCompleted)
	{
		gLevelCompleted = true;
		gLevelCompletedCoolDownTimer = coolDownTimer;		
	}
}


#pragma mark -

/************************************************************/
/******************** PROGRAM MAIN ENTRY  *******************/
/************************************************************/


void GameMain(void)
{
unsigned long	someLong;


				/**************/
				/* BOOT STUFF */
				/**************/

	ToolBoxInit();

	InitDefaultPrefs();
	LoadPrefs();

	InitInput();
	OGL_Boot();
	InitSpriteManager();
	InitBG3DManager();
	InitWindowStuff();
	InitObjectManager();

	GetDateTime(&someLong);		// init random seed
	SetMyRandomSeed(someLong);


			/* DO BOOT CHECK FOR SCREEN MODE */

	SetFullscreenMode(true);

	DoWarmUpScreen();



	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();

		/* PRELOAD ASSETS */

	LoadSpriteGroup(SPRITE_GROUP_CURSOR);
	LoadSpriteGroup(SPRITE_GROUP_FONT);
	LoadSpriteGroup(SPRITE_GROUP_GLOBAL);
	LoadSpriteGroup(SPRITE_GROUP_PARTICLES);
	LoadSpriteGroup(SPRITE_GROUP_SPHEREMAPS);
	LoadSpriteGroup(SPRITE_GROUP_INFOBAR);

		/* SHOW TITLES */

	PlaySong(SONG_THEME, true);
	DoLegalScreen();
	SDL_ShowCursor(0);

//gScore = 12345; NewScore(false);	//----------

		/* MAIN LOOP */
			
	while(true)
	{		
		MyFlushEvents();

		DoMainMenuScreen(0);
						
		PlayGame_Arcade();
	}
	
}




