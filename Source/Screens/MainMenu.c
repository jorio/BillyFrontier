/****************************/
/*   	MAINMENU SCREEN.C	*/
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupMainMenuScreen(Byte startMenu);
static void FreeMainMenuScreen(void);
static void ProcessMainMenu(void);
static void DoMenuControls(void);
static void MoveCursor(ObjNode *theNode);
static void MoveMenuItem(ObjNode *theNode);
static const char* DotConcat(const char* prefix, const char* suffix, float targetWidth);


/****************************/
/*    CONSTANTS             */
/****************************/

#if __APPLE__
	#define EXPOSE_MSAA_SETTING			0
	#define EXPOSE_DISPLAY_SETTING		0
#else
	#define EXPOSE_MSAA_SETTING			1
	#define EXPOSE_DISPLAY_SETTING		1
#endif

#define	MAX_MENU_ITEMS	20

enum
{
	MENU_PAGE_MAIN,
	MENU_PAGE_SETTINGS,
	MENU_PAGE_LOADGAME,
	MENU_PAGE_SAVEGAME,
	NUM_MENU_PAGES
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
#if EXPOSE_DISPLAY_SETTING
	kSettingsMenu_Display,
#endif
#if EXPOSE_MSAA_SETTING
	kSettingsMenu_Antialiasing,
#endif
	kSettingsMenu_MouseWheelScrollSpeed,
	kSettingsMenu_InvertMouseWheel,
	kSettingsMenu_Spacer1,
	kSettingsMenu_ResetHighScores,
	kSettingsMenu_Spacer2,
	kSettingsMenu_Back,

	kSettingsMenu_COUNT,
};


#define	MENU_FONT_SCALE		30.0f

/*********************/
/*    VARIABLES      */
/*********************/

Boolean	gPlayNow, gShowCredits, gShowHighScores, gShowSaveMenu;

static	float	gInactivityTimer;

ObjNode	*gCursor = nil;

static	int		gMenuMode = -1;
int		gCurrentMenuItem = -1;

static	ObjNode	*gMenuItems[MAX_MENU_ITEMS];
static	float	gMenuItemMinX[MAX_MENU_ITEMS];
static	float	gMenuItemMaxX[MAX_MENU_ITEMS];


static	int		gMenuIndex_DeleteFile = -1;
static	int		gMenuIndex_Back = -1;

static	int		gAreYouSure = 0;


/********************** DO MAINMENU SCREEN **************************/

void DoMainMenuScreen(Byte startMenu)
{
do_again:	
			/* SETUP */
		
	SetupMainMenuScreen(startMenu);

	ProcessMainMenu();
	
			/* CLEANUP */

	Wait(30);
	FreeMainMenuScreen();


			/* SEE IF JUST SHOW CREDITS */

	if (gShowCredits)
	{
		NewObjectDefinitionType def =
		{
			.coord = {320, 180, 0},
			.scale = 28,
			.slot = SPRITE_SLOT
		};
		def.coord.y += 33; MakeFontStringObject(DotConcat("programming", "Brian Greenstone", 16), &def, true);
		def.coord.y += 33; MakeFontStringObject(DotConcat("art direction", "Scott Harper", 16), &def, true);
		def.coord.y += 33; MakeFontStringObject(DotConcat("animation", "Peter Greenstone", 16), &def, true);
		def.coord.y += 33; MakeFontStringObject(DotConcat("music", "Aleksander Dimitrijevic", 16), &def, true);
		def.coord.y += 33; MakeFontStringObject(DotConcat("modern version", "Iliyas Jorio", 16), &def, true);
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

static void SetupMainMenuScreen(Byte startMenu)
{
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -.7, .9, -1.0 };
static const OGLVector3D	fillDirection2 = { .3, .8, 1.0 };
int					i;

	GAME_ASSERT(startMenu < NUM_MENU_PAGES);

	if (gCurrentSong != SONG_THEME)
		PlaySong(SONG_THEME, true);



	gPlayNow 		= false;
	gShowCredits 	= false;
	gShowHighScores = false;
	gPlayingFromSavedGame = false;
	gShowSaveMenu	= false;
	gInactivityTimer = 0;
	gCurrentMenuItem = -1;
	
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

			/* BACKGROUND */

	if (startMenu != MENU_PAGE_SAVEGAME)		// blank background when saving game
	{
		MakeBackgroundPictureObject(":images:MainMenu.png");
	}

			/* LAY OUT MENU*/

	BuildMainMenu(startMenu);


	MakeFadeEvent(true);
}


/********************* BUILD MAIN MENU ***************************/

static ObjNode* NewMenuItem(int i, const char* name)
{
	GAME_ASSERT(i < MAX_MENU_ITEMS);

	ObjNode* newObj;

	NewObjectDefinitionType def =
	{
		.coord = {130, 140+MENU_FONT_SCALE*i, 0},
		.moveCall = MoveMenuItem,
		.slot = SPRITE_SLOT,
		.scale = MENU_FONT_SCALE,
	};

	if (gMenuMode == MENU_PAGE_SAVEGAME)
	{
		def.coord.y += 40;
	}

	gMenuItems[i] = newObj = MakeFontStringObject(name, &def, false);

	float w = GetStringWidth(name, def.scale);
	gMenuItemMinX[i] = def.coord.x;
	gMenuItemMaxX[i] = gMenuItemMinX[i] + w;


	newObj->Kind = i;

	newObj->AnaglyphZ = 1.0f;

//	y += gNewObjectDefinition.scale * 1.0f;

	return newObj;
}

static void BuildMainMenu_Root(void)
{
	for (int i = 0; i < kMainMenu_COUNT; i++)
	{
		static const char* names[kMainMenu_COUNT] =
		{
			[kMainMenu_PlayNewGame] = "PLAY NEW GAME",
			[kMainMenu_PlaySavedGame] = "PLAY SAVED GAME",
			[kMainMenu_Settings] = "SETTINGS",
			[kMainMenu_HighScores] = "HIGH SCORES",
			[kMainMenu_Credits] = "CREDITS",
			[kMainMenu_Exit] = "EXIT",
		};

		NewMenuItem(i, names[i]);
	}
}

static const char* DotConcat(const char* prefix, const char* suffix, float targetWidth)
{
	static char buf[64];

	memset(buf, 0, sizeof(buf));		// clear with 0s

	float width = GetStringWidth(prefix, 1) + GetStringWidth(suffix, 1);

	float dotWidth = GetCharSpacing('.', 1);

	int cursor = snprintf(buf, sizeof(buf), "%s", prefix);
	if (cursor < 0)
		goto fail;

	bool flipflop = false;

	while (width < targetWidth
		&& (size_t)cursor < sizeof(buf) - 1)		// -1 for nul terminator
	{
		if (width + dotWidth * 0.4f > targetWidth)		// * 0.4 for some tolerance
			break;

		buf[cursor++] = flipflop ? '.' : '\t';
		width += dotWidth;
		flipflop = !flipflop;
	}

	snprintf(buf + cursor, sizeof(buf) - cursor, "%s", suffix);
fail:
	return buf;
}

static void BuildMainMenu_Settings(void)
{
	const char TW = 13;
	char suffix[32];

	for (int i = 0; i < kSettingsMenu_COUNT; i++)
	{
		suffix[0] = '\0';
		const char* name = "";

		switch (i)
		{
		case kSettingsMenu_Fullscreen:
			name = DotConcat("FULLSCREEN", gGamePrefs.fullscreen ? "YES" : "NO", TW);
			break;

#if EXPOSE_DISPLAY_SETTING
		case kSettingsMenu_Display:
			snprintf(suffix, sizeof(suffix), "%d", gGamePrefs.monitorNum + 1);
			name = DotConcat("DISPLAY", suffix, TW);
			break;
#endif

#if EXPOSE_MSAA_SETTING
		case kSettingsMenu_Antialiasing:
			if (!gGamePrefs.antialiasingLevel)
				snprintf(suffix, sizeof(suffix), "OFF");
			else
				snprintf(suffix, sizeof(suffix), "MSAA %dx", 1 << gGamePrefs.antialiasingLevel);
			name = DotConcat("ANTIALIASING", suffix, TW);
			break;
#endif

		case kSettingsMenu_MouseWheelScrollSpeed:
			snprintf(suffix, sizeof(suffix), "%d", 1 + gGamePrefs.mouseWheelScrollSpeed);
			name = DotConcat("MOUSE WHEEL SPEED", suffix, TW);
			break;

		case kSettingsMenu_InvertMouseWheel:
			name = DotConcat("MOUSE WHEEL DIR.", gGamePrefs.invertMouseWheel? "INVERTED": "NORMAL", TW);
			break;

		case kSettingsMenu_ResetHighScores:
			if (!gAreYouSure)
				name = "RESET HIGH SCORES";
			else
				name = "ARE YOU SURE? confirm reset scores";
			break;

		case kSettingsMenu_Back:
			name = "BACK";
			break;
		}

		NewMenuItem(i, name);
	}

	if (gGamePrefs.antialiasingLevel != gCurrentAntialiasingLevel)
	{
		int i = kSettingsMenu_COUNT;

		NewObjectDefinitionType def =
		{
			.coord = {130, 420, 0},
			.scale = MENU_FONT_SCALE * 0.6f,
			.slot = SPRITE_SLOT,
		};
		gMenuItems[i++] = MakeFontStringObject("THE NEW ANTIALIASING SETTING WILL", &def, false);
		def.coord.y += 20;
		gMenuItems[i++] = MakeFontStringObject("TAKE EFFECT WHEN YOU RESTART THE GAME.", &def, false);
	}
}

static void BuildMainMenu_SavedGames(void)
{
	if (gMenuMode == MENU_PAGE_SAVEGAME)
	{
		char title[32];
		int completionPercent = GetGameCompletionPercent(gDuelWonMask, gLevelWonMask);

		snprintf(title, sizeof(title), "%d pts, %d%% complete", gScore, completionPercent);

		NewObjectDefinitionType def =
		{
			.coord = {640 / 2, 100, 0},
			.scale = MENU_FONT_SCALE * 1.5f,
			.slot = SPRITE_SLOT,
		};
		MakeFontStringObject(title, &def, true);
	}

	int i = 0;

	for (i = 0; i < MAX_SAVE_FILES; i++)
	{
		char prefix[32];
		char suffix[32];

		int completionPercent = 0;
		SaveGameType scratch;
		OSErr iErr = LoadSavedGame(i, &scratch);

		if (iErr == noErr)
		{
			completionPercent = GetGameCompletionPercent(scratch.duelWonMask, scratch.levelWonMask);
		}

		if (gMenuMode == MENU_PAGE_LOADGAME)
		{
			if (gAreYouSure)
			{
				snprintf(prefix, sizeof(prefix), "DELETE %d", i + 1);
			}
			else
			{
				snprintf(prefix, sizeof(prefix), "FILE %d", i + 1);
			}
		}
		else
		{
			snprintf(prefix, sizeof(prefix), "OVERWRITE %d", i + 1);
		}


		if (iErr)
		{
			//snprintf(suffix, sizeof(suffix), "EMPTY");
			suffix[0] = '\0';
		}
		else
		{
			snprintf(suffix, sizeof(suffix), "%5dpts\t.\t.\t.\t%2d%%", scratch.score, completionPercent);
		}

		NewMenuItem(i, DotConcat(prefix, suffix, 13));
	}

	NewMenuItem(i++, "");

	if (gMenuMode == MENU_PAGE_LOADGAME)
	{
		gMenuIndex_DeleteFile = i;
		NewMenuItem(i++, !gAreYouSure ? "DELETE FILE" : "CANCEL DELETE");
	}

	gMenuIndex_Back = i;
	NewMenuItem(i++, gMenuMode == MENU_PAGE_LOADGAME ? "BACK" : "CANCEL SAVE");
}

void BuildMainMenu(int menuLevel)
{
			/* CHECK IF ENTERING DIFFERENT MENU */

	if (gMenuMode != menuLevel)
	{
		gAreYouSure = 0;
	}

			/* SET MENU AS CURRENT */

	gMenuMode = menuLevel;

			/* DELETE EXISTING MENU DATA */
	
	for (int i = 0; i < MAX_MENU_ITEMS; i++)
	{
		if (gMenuItems[i])
		{
			DeleteObject(gMenuItems[i]);
			gMenuItems[i] = nil;
		}
	}
	
			/* LAY OUT MENU ITEMS */
	
	switch (menuLevel)
	{
		case MENU_PAGE_MAIN:
			BuildMainMenu_Root();
			break;

		case MENU_PAGE_SETTINGS:
			BuildMainMenu_Settings();
			break;

		case MENU_PAGE_LOADGAME:
			BuildMainMenu_SavedGames();
			break;

		case MENU_PAGE_SAVEGAME:
			BuildMainMenu_SavedGames();
			break;
	}
}


/********************** FREE MAINMENU SCREEN **********************/

static void FreeMainMenuScreen(void)
{				
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	OGL_DisposeWindowSetup();

	memset(gMenuItems, 0, sizeof(gMenuItems));

	gMenuMode = -1;
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
				
		OGL_DrawScene(DrawObjects);			

				/* DO USER INPUT */

		DoMenuControls();
	}	
	
	
	OGL_FadeOutScene(DrawObjects, MoveObjects);
		
}


/*********************** DO MENU CONTROLS ***********************/

static void PlayRicochet(void)
{
	PlayEffect_Parms(EFFECT_RICOCHET, FULL_CHANNEL_VOLUME / 2, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE);
}

static void DoMenuControls(void)
{
ObjNode	*newObj;

				/* SEE IF USER WANTS TO BACK OUT WITH ESCAPE KEY */

	if (GetNewNeedState(kNeed_UIBack))
	{
				/* IF INITIATED ACTION THAT REQUIRES CONFIRMATION, CANCEL IT */

		if (gAreYouSure)
		{
			gAreYouSure = 0;
			PlayRicochet();
			BuildMainMenu(gMenuMode);
			return;
		}

				/* OTHERWISE JUST BACK OUT TO ROOT MENU */
		switch (gMenuMode)
		{
			case MENU_PAGE_SETTINGS:
			case MENU_PAGE_LOADGAME:
				PlayRicochet();
				BuildMainMenu(MENU_PAGE_MAIN);
				return;

			case MENU_PAGE_SAVEGAME:
				PlayRicochet();
				gPlayNow = true;
				return;
		}
	}

				/* NEED MOUSE CLICK TO PROCEED */

	if (!GetNewClickState(1))
		return;
	
	PlayEffect_Parms(EFFECT_GUNSHOT2,FULL_CHANNEL_VOLUME*2/3,FULL_CHANNEL_VOLUME/3,NORMAL_CHANNEL_RATE);

				/* MAKE BULLET HOLE */

	gNewObjectDefinition.group 		= SPRITE_GROUP_CURSOR;	
	gNewObjectDefinition.type 		= CURSOR_SObjType_BulletHole;
	gNewObjectDefinition.coord.x 	= gCursor->Coord.x;
	gNewObjectDefinition.coord.y 	= gCursor->Coord.y;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveBulletHole;
	gNewObjectDefinition.rot 		= RandomFloat2() * PI2;
	gNewObjectDefinition.scale 	    = 50;
	newObj = MakeSpriteObject(&gNewObjectDefinition);
	
	newObj->Kind = gMenuMode;
	newObj->AnaglyphZ = -5;

	OGL_DrawScene(DrawObjects);			
	
	
				/* SEE WHAT WAS SELECTED */

	if (gCurrentMenuItem == -1)
		return;

	PlayRicochet();

	switch(gMenuMode)
	{
		case MENU_PAGE_MAIN:
			switch(gCurrentMenuItem)
			{
				case kMainMenu_PlayNewGame:
					gPlayNow = true;
					break;

				case kMainMenu_PlaySavedGame:
					BuildMainMenu(MENU_PAGE_LOADGAME);
					break;

				case kMainMenu_Settings:
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

#if EXPOSE_DISPLAY_SETTING
				case kSettingsMenu_Display:
					gGamePrefs.monitorNum++;
					gGamePrefs.monitorNum %= SDL_GetNumVideoDisplays();
					SetFullscreenMode(true);
					BuildMainMenu(MENU_PAGE_SETTINGS);
					break;
#endif

#if EXPOSE_MSAA_SETTING
				case kSettingsMenu_Antialiasing:
					gGamePrefs.antialiasingLevel++;
					gGamePrefs.antialiasingLevel %= 4;
					BuildMainMenu(MENU_PAGE_SETTINGS);
					break;
#endif

				case kSettingsMenu_MouseWheelScrollSpeed:
					gGamePrefs.mouseWheelScrollSpeed++;
					gGamePrefs.mouseWheelScrollSpeed %= 4;
					BuildMainMenu(MENU_PAGE_SETTINGS);
					break;

				case kSettingsMenu_InvertMouseWheel:
					gGamePrefs.invertMouseWheel = !gGamePrefs.invertMouseWheel;
					BuildMainMenu(MENU_PAGE_SETTINGS);
					break;

				case kSettingsMenu_ResetHighScores:
					if (!gAreYouSure)
					{
						gAreYouSure++;
						BuildMainMenu(MENU_PAGE_SETTINGS);
					}
					else
					{
						gAreYouSure = 0;
						ClearHighScores();
						BuildMainMenu(MENU_PAGE_SETTINGS);
					}
					
					break;

				case kSettingsMenu_Back:
					SavePrefs();
					BuildMainMenu(MENU_PAGE_MAIN);
					break;
			}
			break;

		case MENU_PAGE_LOADGAME:
			if (gCurrentMenuItem == gMenuIndex_DeleteFile)
			{
				gAreYouSure = !gAreYouSure;
				BuildMainMenu(MENU_PAGE_LOADGAME);
			}
			else if (gCurrentMenuItem == gMenuIndex_Back)
			{
				BuildMainMenu(MENU_PAGE_MAIN);
			}
			else if (gCurrentMenuItem < MAX_SAVE_FILES)
			{
				int fileSlot = gCurrentMenuItem;

				if (!gAreYouSure)										// not in delete mode; load the save
				{
					SaveGameType saveGame;
					memset(&saveGame, 0, sizeof(saveGame));				// clear it just to be sure

					if (noErr == LoadSavedGame(fileSlot, &saveGame))	// try to load the save
					{
						UseSavedGame(&saveGame);						// commit it to gameplay variables
						gPlayingFromSavedGame = true;
					}
					else
					{
						gPlayingFromSavedGame = false;					// save invalid; initialize blank game
					}
					
					
					gPlayNow = true;
				}
				else
				{
					DeleteSavedGame(fileSlot);							// nuke the save
					gAreYouSure = 0;									// back to non-delete mode
					BuildMainMenu(MENU_PAGE_LOADGAME);					// lay out same menu again
				}
			}
			break;


		case MENU_PAGE_SAVEGAME:
			if (gCurrentMenuItem == gMenuIndex_Back)
			{
				gPlayNow = true;
			}
			else if (gCurrentMenuItem < MAX_SAVE_FILES)
			{
				int fileSlot = gCurrentMenuItem;

				SaveGame(fileSlot);

				gPlayNow = true;
			}
			break;
	}
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
	if (gMenuMode != -1 && gMenuMode != theNode->Kind)
		DeleteObject(theNode);
}


