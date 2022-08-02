/****************************/
/*   	MAINMENU SCREEN.C	*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupMainMenuScreen(void);
static void FreeMainMenuScreen(void);
static void BuildMainMenu(int menuLevel);
static void DrawMainMenuCallback(void);
static void ProcessMainMenu(void);
static void DoMenuControls(void);
static void MoveCursor(ObjNode *theNode);
static void MoveMenuItem(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/




#define	MAX_MENU_ITEMS	20

enum
{
	MENU_PAGE_MAIN,
	MENU_PAGE_SETTINGS
};

enum
{
	kMainMenu_PlayNewGame,
	kMainMenu_PlaySavedGame,
	kMainMenu_Settings,
	kMainMenu_HighScores,
	kMainMenu_Credits,
	kMainMenu_Exit,
	kMainMenu_COUNT,
};

enum
{
	kSettingsMenu_Fullscreen,
	kSettingsMenu_Display,
	kSettingsMenu_Antialiasing,
	kSettingsMenu_Spacer,
	kSettingsMenu_Back,
	kSettingsMenu_COUNT,
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
do_again:	
			/* SETUP */
		
	SetupMainMenuScreen();

	ProcessMainMenu();
	
			/* CLEANUP */

	Wait(30);
	FreeMainMenuScreen();


			/* SEE IF JUST SHOW CREDITS */

	if (gShowCredits)
	{
		DisplayPicture(":images:Credits.jpg");
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
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -.7, .9, -1.0 };
static const OGLVector3D	fillDirection2 = { .3, .8, 1.0 };
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

	OGL_SetupWindow(&viewDef);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();


			/* LOAD SPRITES */

	LoadSpriteGroup(SPRITE_GROUP_FONT);
	LoadSpriteGroup(SPRITE_GROUP_CURSOR);



			/*****************/
			/* BUILD OBJECTS */
			/*****************/

			/* CURSOR */
			
	gNewObjectDefinition.group 		= SPRITE_GROUP_CURSOR;	
	gNewObjectDefinition.type 		= CURSOR_SObjType_Crosshairs;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT+5;			// make sure this is the last sprite drawn
	gNewObjectDefinition.moveCall 	= MoveCursor;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 40;
	gCursor = MakeSpriteObject(&gNewObjectDefinition);

	gCursor->AnaglyphZ = 2.0f;

	BuildMainMenu(0);


	MakeFadeEvent(true);
}


/********************* BUILD MAIN MENU ***************************/

static ObjNode* NewMenuItem(int i, const char* name)
{
	ObjNode* newObj;

	gNewObjectDefinition.coord.x = 130;
	gNewObjectDefinition.coord.y = 140.0f + MENU_FONT_SCALE * i; //y;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.moveCall = MoveMenuItem;
	gNewObjectDefinition.rot = 0;
	gNewObjectDefinition.scale = MENU_FONT_SCALE;
	gNewObjectDefinition.slot = SPRITE_SLOT;
	gMenuItems[i] = newObj = MakeFontStringObject(name, &gNewObjectDefinition, false);

	float w = GetStringWidth(name, gNewObjectDefinition.scale);
	gMenuItemMinX[i] = gNewObjectDefinition.coord.x;
	gMenuItemMaxX[i] = gMenuItemMinX[i] + w;


	newObj->Kind = i;

	newObj->AnaglyphZ = 1.0f;

//	y += gNewObjectDefinition.scale * 1.0f;

	return newObj;
}

static void BuildMainMenu(int menuLevel)
{
	gMenuMode = menuLevel;

			/* DELETE EXISTING MENU DATA */
			
	if (gBackgoundPicture)
	{
		MO_DisposeObjectReference(gBackgoundPicture);
		gBackgoundPicture = NULL;
	}
	
	for (int i = 0; i < MAX_MENU_ITEMS; i++)
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
				
		case MENU_PAGE_MAIN:
		{
			gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, 0, ":images:MainMenu.png");

			for (int i = 0; i < kMainMenu_COUNT; i++)
			{	
				static const char* names[kMainMenu_COUNT] =
				{
					[kMainMenu_PlayNewGame]		= "PLAY NEW GAME",
					[kMainMenu_PlaySavedGame]	= "PLAY SAVED GAME",
					[kMainMenu_Settings]		= "SETTINGS",
					[kMainMenu_HighScores]		= "HIGH SCORES",
					[kMainMenu_Credits]			= "CREDITS",
					[kMainMenu_Exit]			= "EXIT",
				};

				NewMenuItem(i, names[i]);
			}

			break;
		}

		case MENU_PAGE_SETTINGS:
		{
			gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, 0, ":images:MainMenu.png");

			for (int i = 0; i < kSettingsMenu_COUNT; i++)
			{
				char buf[64];
				buf[0] = '\0';
				const char* name = buf;

				switch (i)
				{
					case kSettingsMenu_Fullscreen:
						if (gGamePrefs.fullscreen)
							name = "FULLSCREEN . . . . . . . . .YES";
						else
							name = "FULLSCREEN . . . . . . . . . NO";
						break;

					case kSettingsMenu_Display:
						snprintf(buf, sizeof(buf), "DISPLAY . . . . . . . . . . . . . %d", gGamePrefs.monitorNum + 1);
						break;

					case kSettingsMenu_Antialiasing:
						if (!gGamePrefs.antialiasingLevel)
							name =                     "ANTIALIASING . . . . . . . .OFF";
						else
							snprintf(buf, sizeof(buf), "ANTIALIASING . . . . .MSAA %dx", 1 << gGamePrefs.antialiasingLevel);
						break;

					case kSettingsMenu_Back:
						name = "BACK";
				}
				
				NewMenuItem(i, name);
			}

			if (gGamePrefs.antialiasingLevel != gCurrentAntialiasingLevel)
			{
				int i = kSettingsMenu_COUNT;

				gNewObjectDefinition.coord.x = 130;
				gNewObjectDefinition.coord.y = 390; //y;
				gNewObjectDefinition.coord.z = 0;
				gNewObjectDefinition.flags = 0;
				gNewObjectDefinition.moveCall = nil;// MoveMenuItem;
				gNewObjectDefinition.rot = 0;
				gNewObjectDefinition.scale = MENU_FONT_SCALE * 0.6;
				gNewObjectDefinition.slot = SPRITE_SLOT;
				gMenuItems[i++] = MakeFontStringObject("THE NEW ANTIALIASING SETTING WILL", &gNewObjectDefinition, false);
				gNewObjectDefinition.coord.y += 20;
				gMenuItems[i++] = MakeFontStringObject("TAKE EFFECT WHEN YOU RESTART THE GAME.", &gNewObjectDefinition, false);
			}

			break;
		}
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
	OGL_DisposeWindowSetup();
}

#pragma mark -


/**************** PROCESS MAIN MENU ********************/

static void ProcessMainMenu(void)
{
	CalcFramesPerSecond();
	ReadKeyboard();		

	while(!gPlayNow)
	{
		CalcFramesPerSecond();
		ReadKeyboard();		
		
				/* MOVE */
				
		gCurrentMenuItem = -1;					// assume nothing selected
		MoveObjects();	
		
		
				/* DRAW */
				
		OGL_DrawScene(DrawMainMenuCallback);			

				/* DO USER INPUT */
				
		DoMenuControls();
	}	
	
	
	OGL_FadeOutScene(DrawMainMenuCallback, MoveObjects);
		
}


/*********************** DO MENU CONTROLS ***********************/

static void DoMenuControls(void)
{
ObjNode	*newObj;

	if (GetNewClickState(1))
	{
		PlayEffect_Parms(EFFECT_GUNSHOT2,FULL_CHANNEL_VOLUME*2/3,FULL_CHANNEL_VOLUME/3,NORMAL_CHANNEL_RATE);
	

				/* MAKE BULLET HOLE */
					
		gNewObjectDefinition.group 		= SPRITE_GROUP_CURSOR;	
		gNewObjectDefinition.type 		= CURSOR_SObjType_BulletHole;
		gNewObjectDefinition.coord.x 	= gCursor->Coord.x;
		gNewObjectDefinition.coord.y 	= gCursor->Coord.y;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SPRITE_SLOT+1;
		gNewObjectDefinition.moveCall 	= MoveBulletHole;
		gNewObjectDefinition.rot 		= RandomFloat2() * PI2;
		gNewObjectDefinition.scale 	    = 50;
		newObj = MakeSpriteObject(&gNewObjectDefinition);
	
		newObj->Kind = gMenuMode;
		newObj->AnaglyphZ = -5;

		OGL_DrawScene(DrawMainMenuCallback);			
	
	
				/* SEE WHAT WAS SELECTED */
	
		if (gCurrentMenuItem != -1)
		{
			PlayEffect_Parms(EFFECT_RICOCHET,FULL_CHANNEL_VOLUME/2,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE);

			switch(gMenuMode)
			{
				case MENU_PAGE_MAIN:
					switch(gCurrentMenuItem)
					{
						case kMainMenu_PlayNewGame:
								gPlayNow = true;
								break;

						case kMainMenu_PlaySavedGame:
							if (LoadSavedGame())
							{
								gPlayNow = true;
								gPlayingFromSavedGame = true;
							}
							break;

						case kMainMenu_Settings:
							gMenuMode = MENU_PAGE_SETTINGS;
							BuildMainMenu(MENU_PAGE_SETTINGS);
							break;
						
						case kMainMenu_HighScores:
							gShowHighScores = true;
							gPlayNow = true;
							break;

						case kMainMenu_Credits:
							gShowCredits = true;
							gPlayNow = true;
							break;
						
						case kMainMenu_Exit:
							CleanQuit();
							break;
					}
					break;

				case MENU_PAGE_SETTINGS:
					switch (gCurrentMenuItem)
					{
						case kSettingsMenu_Fullscreen:
							gGamePrefs.fullscreen = !gGamePrefs.fullscreen;
							SetFullscreenMode(true);
							BuildMainMenu(MENU_PAGE_SETTINGS);
							break;

						case kSettingsMenu_Display:
							gGamePrefs.monitorNum++;
							gGamePrefs.monitorNum %= SDL_GetNumVideoDisplays();
							SetFullscreenMode(true);
							BuildMainMenu(MENU_PAGE_SETTINGS);
							break;

						case kSettingsMenu_Antialiasing:
							gGamePrefs.antialiasingLevel++;
							gGamePrefs.antialiasingLevel %= 4;
							BuildMainMenu(MENU_PAGE_SETTINGS);
							break;

						case kSettingsMenu_Back:
							gMenuMode = MENU_PAGE_MAIN;
							BuildMainMenu(MENU_PAGE_MAIN);
							break;
					}
			}
		}
	}
}



/***************** DRAW MAINMENU CALLBACK *******************/

static void DrawMainMenuCallback(void)
{
	MO_DrawObject(gBackgoundPicture);
	DrawObjects();
}


#pragma mark -

/********************* MOVE CURSOR *********************/

static void MoveCursor(ObjNode *theNode)
{
	OGLPoint2D mouse = GetLogicalMouseCoord();
	theNode->Coord.x = mouse.x;
	theNode->Coord.y = mouse.y;

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


