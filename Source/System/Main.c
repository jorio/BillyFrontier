/****************************/
/*    	BILLY FRONTIER - MAIN 		*/
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "windows.h"
#include "3dmath.h"
#include "mainmenu.h"
#include "infobar.h"
#include "internet.h"
#include <folders.h>

extern	Boolean			gDrawLensFlare,gDisableHiccupTimer, gPlayerIsDead;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gAutoFadeStartDist,gAutoFadeEndDist,gAutoFadeRange_Frac;
extern	float			gTerrainPolygonSize, gGammaFadePercent;
extern	OGLPoint3D	gCoord;
extern	ObjNode				*gFirstNodePtr;
extern	short		gNumSuperTilesDrawn;
extern	float		gGlobalTransparency;
extern	int			gNumObjectNodes,gSuperTileActiveRange;
extern	PlayerInfoType	gPlayerInfo;
extern	PrefsType	gGamePrefs;
extern	short	gNumTerrainDeformations;
extern	DeformationType	gDeformationList[];
extern	long			gTerrainUnitWidth,gTerrainUnitDepth;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	Boolean		gGameIsRegistered,gSlowCPU, gPlayerToWinDuel, gSerialWasVerified;


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

Boolean				gShareware = false;

float				gDemoVersionTimer = 0;

Boolean				gOSX = false, gG4 = false;

float				gGravity = NORMAL_GRAVITY;

Byte				gDebugMode = 0;				// 0 == none, 1 = fps, 2 = all

u_long				gAutoFadeStatusBits;
short				gMainAppRezFile;

OGLSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType			gGamePrefs;

FSSpec				gDataSpec;



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


#include "serialVerify.h"


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
		DoAlert("\pWarning: Cannot locate the Preferences folder.");

	iErr = DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"\pBilly",&createdDirID);		// make folder in there



			/* MAKE FSSPEC FOR DATA FOLDER */
	
	SetDefaultDirectory();							// be sure to get the default directory

	iErr = FSMakeFSSpec(0, 0, "\p:Data:Images", &gDataSpec);
	if (iErr)
	{
		DoAlertNum(133);
		CleanQuit();
	}


			/* DETERMINE IF SHAREWARE OR BOXED VERSION */
			//
			// The boxed version cannot play in demo mode, so it will not
			// have the DemoQuit / DemoExpired image files.
			//
			
	{
		FSSpec	spc;
		
		if (FSMakeFSSpec(0, 0, "\p:Data:Images:DemoQuit", &spc) == noErr)
			gShareware = true;	
		else
			gShareware = false;
	}


			/********************/
			/* INIT PREFERENCES */
			/********************/
	
	InitDefaultPrefs();
	LoadPrefs(&gGamePrefs);	

	AutoSleepControl(false);							// don't let system sleep (mainly for OS X)


		/* FIRST VERIFY SYSTEM BEFORE GOING TOO FAR */
				
	VerifySystem();


			/* INIT ISP OR HID */
			
 	InitInput();


			/* BOOT OGL */
			
	OGL_Boot();


			/* START QUICKTIME */
			
	EnterMovies();
	


			/************************************/
            /* SEE IF GAME IS REGISTERED OR NOT */
			/************************************/

    CheckGameSerialNumber(false);


		/**********************************/
		/* SEE IF SHOULD DO VERSION CHECK */
		/**********************************/

	if (gGameIsRegistered)								// only do HTTP if running full version in registered mode
	{
		DateTimeRec	dateTime;
		u_long		seconds, seconds2;
	
		GetTime(&dateTime);								// get date time
		DateToSeconds(&dateTime, &seconds);			
				
		DateToSeconds(&gGamePrefs.lastVersCheckDate, &seconds2);			
		
		if ((seconds - seconds2) > 259000)				// see if 3 days have passed since last check
		{
			MyFlushEvents();
			gGamePrefs.lastVersCheckDate = dateTime;	// update time
			SavePrefs();
			
			ReadHTTPData_VersionInfo();					// do version check (also checks serial #'s)
		}	
	}
	
			/*********************************/
			/* DO BOOT CHECK FOR SCREEN MODE */
			/*********************************/

			
	DoScreenModeDialog();	
									
	MyFlushEvents();
}


/************************* INIT DEFAULT PREFS **************************/

void InitDefaultPrefs(void)
{
int			i;
long 		keyboardScript, languageCode;

		/* DETERMINE WHAT LANGUAGE IS ON THIS MACHINE */
					
	keyboardScript = GetScriptManagerVariable(smKeyScript);
	languageCode = GetScriptVariable(keyboardScript, smScriptLang);			
		
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
	gGamePrefs.lastVersCheckDate.year = 0;
	gGamePrefs.hasConfiguredISpControls = false;
	gGamePrefs.oldOSWarned 			= false;
	gGamePrefs.anaglyph				=  false;
	gGamePrefs.anaglyphColor		= true;
	gGamePrefs.reserved[0] 			= 0;
	gGamePrefs.reserved[1] 			= 0;				
	gGamePrefs.reserved[2] 			= 0;
	gGamePrefs.reserved[3] 			= 0;				
	gGamePrefs.reserved[4] 			= 0;
	gGamePrefs.reserved[5] 			= 0;				
	gGamePrefs.reserved[6] 			= 0;				
	gGamePrefs.reserved[7] 			= 0;	
	
	for (i = 0; i < MAX_HTTP_NOTES; i++)
		gGamePrefs.didThisNote[i] = false;				
}


#pragma mark -



/******************** PLAY GAME: ARCADE ************************/

static void PlayGame_Arcade(void)
{
int		i;
u_long	oldScore;

	gGameOver = gWonGame = gLostGame = false;

	if (!gSerialWasVerified)							// check if hackers bypassed the reg verify - if so, de-register us
	{
		FSSpec	spec;
		
		gGameIsRegistered = false;

		if (FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gSerialFileName, &spec) == noErr)	// delete the serial # file
			FSpDelete(&spec);
		ExitToShell();
	}


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

		if (!gGameIsRegistered)				// if not registered then bail since that's the only level
			gGameOver = true;

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


void main(void)
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



		/* SEE IF DEMO MODE EXPIRED */
			
	if (!gGameIsRegistered)
		GetDemoTimer();


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




