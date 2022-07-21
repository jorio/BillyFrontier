/****************************/
/*   	MAINMENU SCREEN.C	*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "3dmath.h"
#include "infobar.h"
#include <aglmacro.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	FSSpec		gDataSpec;
extern	Boolean		gGameOver, gGameIsRegistered;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gOSX,gDisableAnimSounds, gPlayingFromSavedGame, gMouseNewButtonState;
extern	PrefsType	gGamePrefs;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	OGLPoint3D	gCoord;
extern	OGLVector3D	gDelta;
extern	u_long			gScore,gGlobalMaterialFlags;
extern	float			gGammaFadePercent;
extern	OGLColorRGB			gGlobalColorFilter;
extern	MOPictureObject 	*gBackgoundPicture;
extern	int					gGameWindowWidth, gGameWindowHeight, gPlayMode, gCurrentArea;
extern	short				gCurrentSong;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupMainMenuScreen(void);
static void FreeMainMenuScreen(void);
static void BuildMainMenu(int menuLevel);
static void DrawMainMenuCallback(OGLSetupOutputType *info);
static void ProcessMainMenu(void);
static void DoMenuControls(void);
static void DrawDarkenPane(ObjNode *theNode, const OGLSetupOutputType *setupInfo);
static void MoveDarkenPane(ObjNode *theNode);
static void MoveCursor(ObjNode *theNode);
static void MoveMenuItem(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	MAINMENU_SObjType_Cursor,
	MAINMENU_SObjType_BulletHole
};




#define	MAX_MENU_ITEMS	20

enum
{
	MENU_PAGE_MAIN
};


#define	MENU_FONT_SCALE		30.0f

/*********************/
/*    VARIABLES      */
/*********************/

Boolean	gPlayNow, gShowCredits, gShowHighScores;

static	float	gInactivityTimer;

ObjNode	*gCursor = nil;

static	int		gMenuMode ;
int		gCurrentMenuItem = -1;

static	ObjNode	*gMenuItems[MAX_MENU_ITEMS];
static	float	gMenuItemMinX[MAX_MENU_ITEMS];
static	float	gMenuItemMaxX[MAX_MENU_ITEMS];


/********************** DO MAINMENU SCREEN **************************/

void DoMainMenuScreen(void)
{
	
	GammaFadeOut();
	
do_again:	
			/* SETUP */
		
	SetupMainMenuScreen();

	ProcessMainMenu();
	
			/* CLEANUP */

	GammaFadeOut();
	Wait(30);
	FreeMainMenuScreen();


			/* SEE IF JUST SHOW CREDITS */
		
	if (gShowCredits)
	{
		FSSpec	spec;
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:Credits", &spec);
		DisplayPicture(&spec);							
		goto do_again;
	}		
	
			/* SEE IF JUST SHOW HIGH SCORES */
			
	else
	if (gShowHighScores)
	{
		NewScore(true);
		goto do_again;
	}
}



/********************* SETUP MAINMENU SCREEN **********************/

static void SetupMainMenuScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
const static OGLVector3D	fillDirection1 = { -.7, .9, -1.0 };
const static OGLVector3D	fillDirection2 = { .3, .8, 1.0 };
int					i;

	if (gCurrentSong != SONG_THEME)
		PlaySong(SONG_THEME, true);



	gPlayNow 		= false;
	gShowCredits 	= false;
	gShowHighScores = false;
	gPlayingFromSavedGame = false;
	gInactivityTimer = 0;
	gCurrentMenuItem = -1;
	gBackgoundPicture = nil;
	
	for (i = 0; i < MAX_MENU_ITEMS; i++)
		gMenuItems[i] = nil;
	
	
			/**************/
			/* SETUP VIEW */
			/**************/
			
	OGL_NewViewDef(&viewDef);	

	viewDef.camera.fov 			= 1.0;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 2500;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * .9f;

	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;	

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 50;
	viewDef.camera.from.z		= 500;

	viewDef.camera.to.y 		= 100.0f;
	
	viewDef.lights.ambientColor.r = .2;
	viewDef.lights.ambientColor.g = .2;
	viewDef.lights.ambientColor.b = .2;

	viewDef.lights.numFillLights 	= 2;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= .8;
	viewDef.lights.fillColor[0].g 	= .8;
	viewDef.lights.fillColor[0].b 	= .6;

	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[1].r 	= .5;
	viewDef.lights.fillColor[1].g 	= .5;
	viewDef.lights.fillColor[1].b 	= .0;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();


			/* LOAD SPRITES */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:font.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:mainmenu.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_MAINMENU, gGameViewInfoPtr);



			/*****************/
			/* BUILD OBJECTS */
			/*****************/

			/* CURSOR */
			
	gNewObjectDefinition.group 		= SPRITE_GROUP_MAINMENU;	
	gNewObjectDefinition.type 		= MAINMENU_SObjType_Cursor;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT+5;			// make sure this is the last sprite drawn
	gNewObjectDefinition.moveCall 	= MoveCursor;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 40;
	gCursor = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

	gCursor->AnaglyphZ = 2.0f;

	BuildMainMenu(0);


	MakeFadeEvent(true);
}


/********************* BUILD MAIN MENU ***************************/

static void BuildMainMenu(int menuLevel)
{
ObjNode				*newObj;
int					i;
float				y,w;
FSSpec				spec;


	gMenuMode = menuLevel;

			/* DELETE EXISTING MENU DATA */
			
	if (gBackgoundPicture)
		MO_DisposeObjectReference(gBackgoundPicture);	
	
	for (i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (gMenuItems[i])
		{
			DeleteObject(gMenuItems[i]);
			gMenuItems[i] = nil;
		}
	}
	
	
	switch(menuLevel)
	{
				/*************/
				/* MAIN MENU */
				/*************/
				
		case	MENU_PAGE_MAIN:


						/* MAKE BACKGROUND PICTURE OBJECT */

				if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:images:MainMenu", &spec))
					DoFatalAlert("\pSetupMainMenuScreen: background pict not found.");
				gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, &spec);


						/* MAKE MENU ITEMS */
						
				y = 140.0f;
				for (i = 0; i < 6; i++)
				{	
					static const Str31 names[] =
					{
						"\pPLAY NEW GAME",
						"\pPLAY SAVED GAME",
						"\pSETTINGS",
						"\pHIGH SCORES",
						"\pCREDITS",
						"\pEXIT",
					};
				
					gNewObjectDefinition.coord.x 	= 130;
					gNewObjectDefinition.coord.y 	= y;
					gNewObjectDefinition.coord.z 	= 0;
					gNewObjectDefinition.flags 		= 0;
					gNewObjectDefinition.moveCall 	= MoveMenuItem;
					gNewObjectDefinition.rot 		= 0;
					gNewObjectDefinition.scale 	    = MENU_FONT_SCALE;
					gNewObjectDefinition.slot 		= SPRITE_SLOT;
					gMenuItems[i] = newObj = MakeFontStringObject(names[i], &gNewObjectDefinition, gGameViewInfoPtr, false);		
					
					w = GetStringWidth(names[i], gNewObjectDefinition.scale);
					gMenuItemMinX[i] = gNewObjectDefinition.coord.x;
					gMenuItemMaxX[i] = gMenuItemMinX[i] + w;
					
									
					newObj->Kind = i;
										
					newObj->AnaglyphZ = 1.0f;
										
					y += gNewObjectDefinition.scale * 1.0f;
					
				}
				break;				
	}
		

}


/********************** FREE MAINMENU SCREEN **********************/

static void FreeMainMenuScreen(void)
{				
	MyFlushEvents();
	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);	
	gBackgoundPicture = nil;
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();	
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_MAINMENU);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
	
}

#pragma mark -


/**************** PROCESS MAIN MENU ********************/

static void ProcessMainMenu(void)
{
float	charTimer = 2.0f;

	CalcFramesPerSecond();
	ReadKeyboard();		

	while(!gPlayNow)
	{
		const float fps = gFramesPerSecondFrac;
		
		CalcFramesPerSecond();
		ReadKeyboard();		
		
				/* MOVE */
				
		gCurrentMenuItem = -1;					// assume nothing selected
		MoveObjects();	
		
		
				/* DRAW */
				
		OGL_DrawScene(gGameViewInfoPtr, DrawMainMenuCallback);			

				/* DO USER INPUT */
				
		DoMenuControls();
	}	
	
	
		
		
}


/*********************** DO MENU CONTROLS ***********************/

static void DoMenuControls(void)
{
ObjNode	*newObj;

	if (gMouseNewButtonState)
	{
		PlayEffect_Parms(EFFECT_GUNSHOT2,FULL_CHANNEL_VOLUME*2/3,FULL_CHANNEL_VOLUME/3,NORMAL_CHANNEL_RATE);
	

				/* MAKE BULLET HOLE */
					
		gNewObjectDefinition.group 		= SPRITE_GROUP_MAINMENU;	
		gNewObjectDefinition.type 		= MAINMENU_SObjType_BulletHole;
		gNewObjectDefinition.coord.x 	= gCursor->Coord.x;
		gNewObjectDefinition.coord.y 	= gCursor->Coord.y;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SPRITE_SLOT+1;
		gNewObjectDefinition.moveCall 	= MoveBulletHole;
		gNewObjectDefinition.rot 		= RandomFloat2() * PI2;
		gNewObjectDefinition.scale 	    = 50;
		newObj = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);
	
		newObj->Kind = gMenuMode;
		newObj->AnaglyphZ = -5;

		OGL_DrawScene(gGameViewInfoPtr, DrawMainMenuCallback);			
	
	
				/* SEE WHAT WAS SELECTED */
	
		if (gCurrentMenuItem != -1)
		{
			PlayEffect_Parms(EFFECT_RICOCHET,FULL_CHANNEL_VOLUME/2,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE);

			switch(gMenuMode)
			{
				case	MENU_PAGE_MAIN:
						switch(gCurrentMenuItem)
						{
							case	0:							// PLAY NEW GAME
									gPlayNow = true;
									break;

							case	1:							// PLAY SAVED GAME
									if (gGameIsRegistered)
									{
										if (LoadSavedGame())
										{
											gPlayNow = true;
											gPlayingFromSavedGame = true;									
										}
									}
									else
										DoAlert("\pYou cannot play saved games in Demo mode.");
									break;

							case	2:							// SETTINGS
									DoGameOptionsDialog();
									break;
						
							case	3:							// HIGH SCORES
									gShowHighScores = true;
									gPlayNow = true;
									break;

							case	4:							// CREDITS
									gShowCredits = true;
									gPlayNow = true;
									break;
						
							case	5:							// EXIT
									CleanQuit();
									break;
						
						
						}
						break;
			
			
			}
		}
	}
}



/***************** DRAW MAINMENU CALLBACK *******************/

static void DrawMainMenuCallback(OGLSetupOutputType *info)
{
	MO_DrawObject(gBackgoundPicture, info);

	DrawObjects(info);

}


#pragma mark -

/********************* MOVE CURSOR *********************/

static void MoveCursor(ObjNode *theNode)
{
Point	pt;

	GetMouseCoord(&pt);

	theNode->Coord.x = (pt.h / (float)gGameWindowWidth) * 640.0f;
	theNode->Coord.y = (pt.v / (float)gGameWindowHeight) * 480.0f;

	UpdateObjectTransforms(theNode);

}


/*************** MOVE MENU ITEM ***********************/

static void MoveMenuItem(ObjNode *theNode)
{
float	d;
int		i = theNode->Kind;

			/* IS THE CURSOR OVER THIS ITEM? */
			
	if ((gCursor->Coord.x >= gMenuItemMinX[i]) &&
		(gCursor->Coord.x < gMenuItemMaxX[i]))
	{
				
		d = fabs(gCursor->Coord.y - theNode->Coord.y);
		if (d < (MENU_FONT_SCALE/2))
		{
			theNode->ColorFilter.r = 1.0;
			theNode->ColorFilter.g = .5;
			theNode->ColorFilter.b = 0;
			gCurrentMenuItem = theNode->Kind;		
			return;
		}
	}

			/* CURSOR NOT ON THIS */
			
	theNode->ColorFilter.r = theNode->ColorFilter.g = theNode->ColorFilter.b = 1;
//	gCurrentMenuItem = -1;
}


/******************** MOVE BULLET HOLE ***************************/

void MoveBulletHole(ObjNode *theNode)
{
	if (gMenuMode != theNode->Kind)	
		DeleteObject(theNode);



}

