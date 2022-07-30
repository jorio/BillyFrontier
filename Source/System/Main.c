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

Boolean				gOSX = false, gG4 = false;

float				gGravity = NORMAL_GRAVITY;

Byte				gDebugMode = 0;				// 0 == none, 1 = fps, 2 = all

u_long				gAutoFadeStatusBits;
short				gMainAppRezFile;

OGLSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType			gGamePrefs;

extern FSSpec				gDataSpec;



u_long				gGameFrameNum = 0;

Boolean				gPlayingFromSavedGame = false;
Boolean				gGameOver = false;
Boolean				gWonGame, gLostGame;
Boolean				gLevelCompleted = false;
Boolean				gWonLevel = false;
float				gLevelCompletedCoolDownTimer = 0;
int					gCurrentArea;

int					gScratch = 0;
float				gScratchF = 0;

u_long				gScore,gLoadedScore;


Boolean				gDuelWon[NUM_LEVELS];
Boolean				gLevelWon[NUM_LEVELS];


//======================================================================================
//======================================================================================
//======================================================================================


/****************** TOOLBOX INIT  *****************/

void ToolBoxInit(void)
{
OSErr		iErr;
long		createdDirID;

 	MoreMasters(); 	MoreMasters(); 	MoreMasters(); MoreMasters(); 	MoreMasters(); 	MoreMasters();

	MyFlushEvents();
	gMainAppRezFile = CurResFile();

			/* CHECK PREFERENCES FOLDER */
			
	iErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,			// locate the folder
					&gPrefsFolderVRefNum,&gPrefsFolderDirID);
	if (iErr != noErr)
		DoAlert("Warning: Cannot locate the Preferences folder.");

	iErr = DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"Billy",&createdDirID);		// make folder in there



			/* MAKE FSSPEC FOR DATA FOLDER */
	
	SetDefaultDirectory();							// be sure to get the default directory

	iErr = FSMakeFSSpec(0, 0, ":Data:Images", &gDataSpec);
	if (iErr)
	{
		DoFatalAlert("Can't find Data folder.");
	}


			/********************/
			/* INIT PREFERENCES */
			/********************/
	
	InitDefaultPrefs();
	LoadPrefs(&gGamePrefs);	


		/* FIRST VERIFY SYSTEM BEFORE GOING TOO FAR */
				
	VerifySystem();


			/* INIT ISP OR HID */
			
 	InitInput();


			/* BOOT OGL */
			
	OGL_Boot();


			/*********************************/
			/* DO BOOT CHECK FOR SCREEN MODE */
			/*********************************/

			
	DoScreenModeDialog();	
									
	MyFlushEvents();
}


/************************* INIT DEFAULT PREFS **************************/

void InitDefaultPrefs(void)
{
long 		languageCode;

		/* DETERMINE WHAT LANGUAGE IS ON THIS MACHINE */

	IMPLEMENT_ME_SOFT();
	languageCode = langEnglish;		// TODO: get actual language -- but Billy is not localized anyway

	switch(languageCode)
	{
		case	langFrench:
				gGamePrefs.language 			= LANGUAGE_FRENCH;
				break;

		case	langGerman:
				gGamePrefs.language 			= LANGUAGE_GERMAN;
				break;

		case	langSpanish:
				gGamePrefs.language 			= LANGUAGE_SPANISH;
				break;

		case	langItalian:
				gGamePrefs.language 			= LANGUAGE_ITALIAN;
				break;

		case	langDutch:
				gGamePrefs.language 			= LANGUAGE_DUTCH;
				break;
	
		default:
				gGamePrefs.language 			= LANGUAGE_ENGLISH;
	}
	
			
	gGamePrefs.version				= CURRENT_PREFS_VERS;						
	gGamePrefs.difficulty			= 0;	
	gGamePrefs.showScreenModeDialog = true;
	gGamePrefs.depth				= 32;
	gGamePrefs.screenWidth			= 640;
	gGamePrefs.screenHeight			= 480;
	gGamePrefs.videoHz				= 0;
	gGamePrefs.deepZ				= true;
	gGamePrefs.hasConfiguredISpControls = false;
	gGamePrefs.oldOSWarned 			= false;
	gGamePrefs.anaglyph				=  false;
	gGamePrefs.anaglyphColor		= true;
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

		GammaFadeOut();
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

void DefaultDrawCallback(OGLSetupOutputType *setupInfo)
{
		/* DRAW OBJECTS & TERAIN */	

	DrawObjects(setupInfo);												// draw objNodes which includes fences, terrain, etc.

			/* DRAW MISC */

	DrawSparkles(setupInfo);											// draw light sparkles
	DrawLensFlare(setupInfo);											// draw lens flare
	DrawInfobar();												// draw infobar last		
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




			/* INIT SOME OF MY STUFF */

	InitSpriteManager();
	InitBG3DManager();
	InitWindowStuff();
	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();


			/* INIT MORE MY STUFF */
					
	InitObjectManager();
	
	GetDateTime ((unsigned long *)(&someLong));		// init random seed
	SetMyRandomSeed(someLong);
	HideCursor();


		/* SHOW TITLES */

	PlaySong(SONG_THEME, true);

	DoLegalScreen();


//NewScore(true);	//----------

		/* MAIN LOOP */
			
	while(true)
	{		
		MyFlushEvents();

		DoMainMenuScreen();
						
		PlayGame_Arcade();
	}
	
}




