/****************************/
/*   	BIGBOARD.C			*/
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

static void SetupBigBoardScreen(void);
static void FreeBigBoardScreen(void);
static void BuildBigBoardIcons(void);
static void DrawBigBoardInfo(ObjNode* theNode);
static void ProcessBigBoard(void);
static void DoBigBoardControls(void);
static void MoveCursor(ObjNode *theNode);
static void MoveBigBoardtem(ObjNode *theNode);
void MoveBulletHole(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



#define	NUM_BIGBOARD_ITEMS	8


/*********************/
/*    VARIABLES      */
/*********************/

static	ObjNode	*gBigBoardItems[NUM_BIGBOARD_ITEMS];


/********************** DO BIGBOARD SCREEN **************************/

void DoBigBoardScreen(void)
{
again:
			/* SETUP */

	SetupBigBoardScreen();

	gShowSaveMenu = false;

	ProcessBigBoard();

	
			/* CLEANUP */

	Wait(30);
	FreeBigBoardScreen();


	if (gShowSaveMenu)
	{
		DoMainMenuScreen(3);
		goto again;
	}
}



/********************* SETUP BIGBOARD SCREEN **********************/

static void SetupBigBoardScreen(void)
{
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -.7, .9, -1.0 };
static const OGLVector3D	fillDirection2 = { .3, .8, 1.0 };

	if (gCurrentSong != SONG_THEME)
		PlaySong(SONG_THEME, true);

	gCurrentMenuItem = -1;
	
	gPlayNow = false;
	gGameOver = false;
	
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
			
	LoadSpriteGroup(SPRITE_GROUP_CURSOR);
	LoadSpriteGroup(SPRITE_GROUP_FONT);
	LoadSpriteGroup(SPRITE_GROUP_BIGBOARD);
	LoadSpriteGroup(SPRITE_GROUP_INFOBAR);		// for hearts


			/* MAKE BACKGROUND PICTURE OBJECT */

	MakeBackgroundPictureObject(":images:BigBoard.png");


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

	gCursor->AnaglyphZ = 3.0f;
	

			/* ICONS */
	
	BuildBigBoardIcons();


			/* INFO */

	NewObjectDefinitionType infoDef =
	{
		.genre = CUSTOM_GENRE,
		.scale = 1,
		.slot = SPRITE_SLOT,
	};
	ObjNode* infoObj = MakeNewObject(&infoDef);
	infoObj->CustomDrawFunction = DrawBigBoardInfo;

			/* FADE IN */
	
	MakeFadeEvent(true);
}







/********************** FREE BIGBOARD SCREEN **********************/

static void FreeBigBoardScreen(void)
{				
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSpriteGroup(SPRITE_GROUP_BIGBOARD);
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup();

	memset(gBigBoardItems, 0, sizeof(gBigBoardItems));
}

#pragma mark -


/**************** PROCESS BIGBOARD ********************/

static void ProcessBigBoard(void)
{
	gShowSaveMenu = false;

	CalcFramesPerSecond();
	ReadKeyboard();		

	while((!gPlayNow) && (!gGameOver) && !(gShowSaveMenu))
	{
		CalcFramesPerSecond();
		ReadKeyboard();		
		
				/* MOVE */
				
		gCurrentMenuItem = -1;					// assume nothing selected
		MoveObjects();	
		
		
				/* DRAW */
				
		OGL_DrawScene(DrawObjects);			

				/* DO USER INPUT */
				
		DoBigBoardControls();
		
		
			/* CHECK FOR WIN CHEAT */
			
		if (IsCheatKeyComboDown())
		{
			gWonGame = false;
			gLostGame = true;
			gGameOver = true;
		}		
	}

	OGL_FadeOutScene(DrawObjects, MoveObjects);
}


/*********************** DO BIGBOARD CONTROLS ***********************/

static void DoBigBoardControls(void)
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
		gNewObjectDefinition.moveCall	= nil; //MoveBulletHole;
		gNewObjectDefinition.rot 		= RandomFloat2() * PI2;
		gNewObjectDefinition.scale 	    = 50;
		newObj = MakeSpriteObject(&gNewObjectDefinition);
		
		newObj->AnaglyphZ = -5;
		
		OGL_DrawScene(DrawObjects);
	
	
				/* SEE WHAT WAS SELECTED */
	
		if (gCurrentMenuItem != -1)
		{
	
			PlayEffect_Parms(EFFECT_RICOCHET,FULL_CHANNEL_VOLUME/2,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE);

			switch(gCurrentMenuItem)
			{
				case	0:							// TOWN SHOOTOUT
						gPlayNow = true;
						gCurrentArea = AREA_TOWN_SHOOTOUT;				
						break;

				case	1:							// STAMPEDE
						gPlayNow = true;
						gCurrentArea = AREA_TOWN_STAMPEDE;				
						break;

				case	2:							// TARGET PRACTICE 1
						gPlayNow = true;
						gCurrentArea = AREA_TARGETPRACTICE1;				
						break;

				case	3:							// SWAMP SHOOTOUT
						gPlayNow = true;
						gCurrentArea = AREA_SWAMP_SHOOTOUT;				
						break;

				case	4:							// SWAMP STAMPEDE
						gPlayNow = true;
						gCurrentArea = AREA_SWAMP_STAMPEDE;				
						break;

				case	5:							// TARGET PRACTICE 2
						gPlayNow = true;
						gCurrentArea = AREA_TARGETPRACTICE2;				
						break;
						
						
				case	6:							// SAVE GAME
						gShowSaveMenu = true;
						break;
						
				case	7:							// END GAME
						gGameOver = true;
						break;
			}
		}
	}
}



/***************** DRAW BIGBOARD CALLBACK *******************/

static void DrawBigBoardInfo(ObjNode* theNode)
{
	(void) theNode;

	OGL_PushState();

			/* DRAW SCORE */

	SetInfobarSpriteState(0);
	Infobar_DrawNumber(gScore, 15, 15, 45, SCORE_NUM_DIGITS, true);


			/* DRAW LIVES */			

	for (int i = 0; i < gPlayerInfo.lives; i++)
	{
		DrawInfobarSprite(605-(i * 20), 20, 30, INFOBAR_SObjType_Heart);
	}

	OGL_PopState();
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


#pragma mark -

/******************* BUILD BIGBOARD ICONS **************************/

static void BuildBigBoardIcons(void)
{
const int	spriteNum[NUM_BIGBOARD_ITEMS] =
{
	BIGBOARD_SObjType_TownShootoutIcon,
	BIGBOARD_SObjType_TownStampedeIcon,
	BIGBOARD_SObjType_TownTargetsIcon,

	BIGBOARD_SObjType_SwampShootoutIcon,
	BIGBOARD_SObjType_SwampStampedeIcon,
	BIGBOARD_SObjType_SwampTargetsIcon,
	
	BIGBOARD_SObjType_SaveGameIcon,
	BIGBOARD_SObjType_EndGameIcon,
};

const OGLPoint2D coords[NUM_BIGBOARD_ITEMS] =
{
	{100, 220},				// town shootout
	{270, 182},				// town stampede
	{250, 339},				// town target

	{390, 180},				// swamp shootout
	{520, 222},				// swamp stampede
	{415, 328},				// swamp target

	{ 90, 440},				// end game
	{550, 440},				// save game
};

const float scale[NUM_BIGBOARD_ITEMS] =
{
	180,					// town shootout
	164,					// town stampede
	125,					// town target

	160,					// swamp shootout
	175,					// swamp stampede
	145,					// swamp target

	90,
	90,
};


	for (int i = 0; i < NUM_BIGBOARD_ITEMS; i++)
	{
		gNewObjectDefinition.group 		= SPRITE_GROUP_BIGBOARD;	
		gNewObjectDefinition.type 		= spriteNum[i];
		gNewObjectDefinition.coord.x 	= coords[i].x;
		gNewObjectDefinition.coord.y 	= coords[i].y;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SPRITE_SLOT;
		gNewObjectDefinition.moveCall 	= MoveBigBoardtem;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = scale[i];
		gBigBoardItems[i] = MakeSpriteObject(&gNewObjectDefinition);
		
		gBigBoardItems[i]->Kind = i;
		gBigBoardItems[i]->Special[0] = gNewObjectDefinition.type;
		
		gBigBoardItems[i]->AnaglyphZ = 2.0f;
		
			/* IF LEVEL ICON & COMPLETED THEN DIM */
				
		if (i < NUM_LEVELS && IsLevelWon(i))
		{
			gBigBoardItems[i]->Kind = -1;
			gBigBoardItems[i]->ColorFilter.a = .5f;
			gBigBoardItems[i]->MoveCall = nil;
		}
	}
}



/*************** MOVE BIGBOARD ITEM ***********************/

static void MoveBigBoardtem(ObjNode *theNode)
{
float	x,y,scaleX, scaleY, cx, cy;
int		i = theNode->Kind;
float	top,bottom,left,right;
const Rect crop[NUM_BIGBOARD_ITEMS] =
{
	{0,40,0,-40},				// town shootout
	{0,0,0,-65},				// town stampede
	{50,0,0,0},					// town target

	{0,70,0,-5},				// swamp shootout
	{0,75,0,0},					// swamp stampede
	{50,0,0,0},					// swamp target

	{0,0,0,0},
	{0,0,0,0},
};


	cx = gCursor->Coord.x;
	cy = gCursor->Coord.y;

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	
	scaleX = theNode->SpriteMO->objectData.scaleX * .5f;
	scaleY = theNode->SpriteMO->objectData.scaleY * .5f * theNode->SpriteMO->objectData.aspectRatio;

	top = y - scaleY + crop[i].top;
	bottom = y + scaleY + crop[i].bottom;
	left = x - scaleX + crop[i].left;
	right = x + scaleX + crop[i].right;

		/* IS THE CURSOR OVER THIS ITEM? */

	if ((cx >= left) &&	(cx < right) &&
		(cy >= top) && (cy < bottom))
	{
		gCurrentMenuItem = theNode->Kind;		
		ModifySpriteObjectFrame(theNode, theNode->Special[0]+1);
	}
	
			/* CURSOR NOT ON THIS */

	else
	{
		ModifySpriteObjectFrame(theNode, theNode->Special[0]);			
	}
}



