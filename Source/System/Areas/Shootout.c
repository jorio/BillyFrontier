/****************************/
/*    	SHOOTOUT.C	 		*/
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

static void MoveEverything_Shootout(void);
static void CleanupShootout(void);
static void InitShootoutArea(void);
static void CountShooutoutEnemies(void);

static ObjNode *CreateBullet(OGLPoint3D *muzzleCoord, OGLVector3D *aim, OGLPoint3D *bulletTargetCoord, ObjNode *bulletTargetObj, Boolean isPlayerBullet);
static void MovePlayersBullet(ObjNode *theNode);
static void MoveEnemyBullet(ObjNode *theNode);

static void CalcTargetPointAndAimOnEnemy(ObjNode *enemy, OGLPoint3D *muzzleCoord, OGLVector3D *aim, ObjNode *gun, OGLPoint3D *targetPt);
static void CalcTargetPointAndAimOnPlayer(OGLPoint3D *muzzleCoord, OGLVector3D *aim, ObjNode *gun, OGLPoint3D *targetPt);

static void MovePlayer_Shootout_Walk(ObjNode *player);
static void MovePlayer_Shootout_Battle(ObjNode *player);



static void MoveShootoutSaloon(ObjNode *theNode);

static void UpdateCrosshairs(void);
static void ShootBulletThruCrosshairs(void);
static void MoveCrosshairsBullet(ObjNode *bullet);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	ALLOW_WALK	0

#define	MAX_STOP_POINTS	20


/****************************/
/*    VARIABLES             */
/****************************/


Byte		gShootoutMode;

int					gStopPointNum;

Byte			gNumEnemiesThisStopPoint[MAX_STOP_POINTS];

OGLPoint2D			gCrosshairsCoord;			// screen coords based on 640x480 system
static int			gCursorPinLeft, gCursorPinRight;
static int			gCursorPinTop, gCursorPinBottom;

static float		gStepUpOffset = 0, gOutOfAmmoTimer = 100;
static Boolean		gStepUp = false;

Boolean				gShootoutCanProceedToNextStopPoint;
Boolean				gNeedToReloadNextAmmoClip;

float				gTimeSinceLastEnemyShot;


/************************* PLAY SHOOTOUT *******************************/

void PlayShootout(void)
{

		/*******************************/
        /* LOAD ALL OF THE ART & STUFF */
		/*******************************/

	InitShootoutArea();


			/* PREP STUFF */

	ReadKeyboard();
	CalcFramesPerSecond();
	CalcFramesPerSecond();

			/* FADE IN */

	MakeFadeEvent(true);

		/******************/
		/* MAIN GAME LOOP */
		/******************/

	while(true)
	{
				/* MOVE, UPDATE, & DRAW */
				
		ReadKeyboard();								
		MoveEverything_Shootout();
		KeepTerrainAlive();
		OGL_DrawScene(DefaultDrawCallback);

		gTimeSinceLastEnemyShot += gFramesPerSecondFrac;
		
								
				/* MISC STUFF */
		

		if (IsCheatKeyComboDown())											// see if cheat to next stop-point		
			gShootoutCanProceedToNextStopPoint = true;

		
		if (GetNewNeedState(kNeed_UIPause))									// see if paused
			DoPaused();
			
		CalcFramesPerSecond();		
		
		gGameFrameNum++;
		
				
				/* SEE IF LEVEL IS COMPLETED */

		if (gGameOver)													// if we need immediate abort, then bail now
			break;
				
		if (gLevelCompleted)
		{
			gLevelCompletedCoolDownTimer -= gFramesPerSecondFrac;		// game is done, but wait for cool-down timer before bailing
			if (gLevelCompletedCoolDownTimer <= 0.0f)
				break;
		}
		
		
		gDisableHiccupTimer = false;									// reenable this after the 1st frame
		
	}

		/* CLEANUP LEVEL */
					
	OGL_FadeOutScene(DefaultDrawCallback, KeepTerrainAlive);
	MyFlushEvents();
	CleanupShootout();
}



/***************** INIT SHOOTOUT AREA ************************/

static void InitShootoutArea(void)
{
OGLSetupInputType	viewDef;
ObjNode		*player;
float		x,z;
	
		/*********************/
		/* INIT COMMON STUFF */
		/*********************/

//	gScrollWheelDelta = 0;
	gPlayerIsDead 		= false;
	gTimeSinceLastEnemyShot	 = 0;
	gGameFrameNum 		= 0;
	gGameOver 			= false;
	gLevelCompleted 	= false;
	gShootoutCanProceedToNextStopPoint	= false;
	gNeedToReloadNextAmmoClip = false;
	
	gGravity = NORMAL_GRAVITY;					// assume normal gravity

	gPlayerInfo.objNode = nil;
	
	gShootoutMode = SHOOTOUT_MODE_WALK;
	
	gCrosshairsCoord.x = 640/2;
	gCrosshairsCoord.y = 480/2;
	gCursorPinLeft = 0;
	gCursorPinRight = 640;
	gCursorPinTop = 0;
	gCursorPinBottom = 480;

	gStepUpOffset 		= 0;
	gStepUp 			= false;

			/*************/
			/* MAKE VIEW */
			/*************/

			/* SET ANAGLYPH INFO */
			
	if (gGamePrefs.anaglyph)
	{
		gAnaglyphFocallength	= 40.0f;	// set camera info
		gAnaglyphEyeSeparation 	= 5.0f;
	}


	SetTerrainScale(DEFAULT_TERRAIN_SCALE);								// set scale to some default for now

	gSuperTileActiveRange = MAX_SUPERTILE_ACTIVE_RANGE-5;
				
			/* SETUP VIEW DEF */
			
	OGL_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= 5;
	viewDef.camera.fov 				= 1.4;	
	viewDef.styles.useFog			= true;
	viewDef.view.clearColor.r 		= .6;
	viewDef.view.clearColor.g 		= .6;
	viewDef.view.clearColor.b		= .4;	
	viewDef.view.clearBackBuffer	= true;
	viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .95f;
	viewDef.styles.fogStart			= viewDef.camera.yon * .4f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
	gDrawLensFlare = false;
	
		
			/* SET LIGHTS */


	viewDef.lights.numFillLights 		= 2;	
	viewDef.lights.ambientColor.r 		= .25;
	viewDef.lights.ambientColor.g 		= .2;
	viewDef.lights.ambientColor.b 		= .2;

	gWorldSunDirection.x = -.7;
	gWorldSunDirection.y = -.8;
	gWorldSunDirection.z = .9;
	OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);
	viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
	viewDef.lights.fillColor[0] 		= gFillColor1;

	viewDef.lights.fillDirection[1].x 	= 1.0;
	viewDef.lights.fillDirection[1].y 	= -0.4;
	viewDef.lights.fillDirection[1].z 	= -.7;
	viewDef.lights.fillColor[1].r		= .7f;
	viewDef.lights.fillColor[1].g		= .7f;
	viewDef.lights.fillColor[1].b		= .6f;


			/* MAKE OGL DRAW CONTEXT */
			
	OGL_SetupWindow(&viewDef);


			/**********************/
			/* SET AUTO-FADE INFO */
			/**********************/
			
	gAutoFadeStartDist	= gGameViewInfoPtr->yon * .80;
	gAutoFadeEndDist	= gGameViewInfoPtr->yon * .95f;	
	gAutoFadeRange_Frac	= 1.0f / (gAutoFadeEndDist - gAutoFadeStartDist);

	if (gAutoFadeStartDist != 0.0f)
		gAutoFadeStatusBits = STATUS_BIT_AUTOFADE;
	else
		gAutoFadeStatusBits = 0;
		

	
			/**********************/
			/* LOAD ART & TERRAIN */
			/**********************/
			//
			// NOTE: only call this *after* draw context is created!
			//
	
	LoadShootoutArt();
	InitInfobar();

			/* INIT OTHER MANAGERS */

	InitEnemyManager();
	InitEffects();
	InitSparkles();
	InitItemsManager();

			
		/* INIT THE PLAYER & RELATED STUFF */
	
	PrimeTerrainWater();							// NOTE:  must do this before items get added since some items may be on the water
#if ALLOW_WALK	
	InitPlayerAtHub();
#else
	InitPlayerForShootout();							// NOTE:  this will also cause the initial items in the start area to be created
#endif	

	CountShooutoutEnemies();					// MUST DO THIS BEFORE SPLINES SINCE ENEMIES ON SPLINES WILL ADD TO THE COUNTS
			
	PrimeSplines();
	PrimeFences();
	

	

			/*************************************/
			/* RESET PLAYER INIT POINT ON SPLINE */
			/*************************************/
	
	player = gPlayerInfo.objNode;
	GetObjectCoordOnSpline(player);
	player->InitCoord = gPlayerInfo.coord = player->OldCoord = player->Coord; 

	GetCoordOnSpline2(&(*gSplineList)[player->SplineNum], player->SplinePlacement, 10, &x, &z, false);		// also set aim
	TurnObjectTowardTarget(player, &player->Coord, x, z, 4000.0, false);


			/* INIT CAMERAS */
			
			
#if ALLOW_WALK	
	InitCamera_Terrain();
#endif
			

		/* START MUSIC */
				
	PlaySong(SONG_SHOOTOUT, true);	
}





/**************** CLEANUP LEVEL **********************/

static void CleanupShootout(void)
{

	StopAllEffectChannels();
 	EmptySplineObjectList();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeTerrain();
	DeleteAllParticleGroups();
	DeleteAllConfettiGroups();
	DisposeParticleSystem();
	DisposeAllSpriteGroups();
	
	DisposeAllBG3DContainers();
		
	DisposeSoundBank(SOUND_BANK_LEVELSPECIFIC);
	
	OGL_DisposeWindowSetup();	// do this last!			
}

/******************** MOVE EVERYTHING ************************/

static void MoveEverything_Shootout(void)
{
	MoveObjects();
	MoveSplineObjects();
	
	UpdateCamera_Shootout();								// update camera
	
	UpdateCrosshairs();	
	
	
		/* SEE IF OUT OF AMMO */
		
	if (gPlayerInfo.ammoCount <= 0)
	{
		gOutOfAmmoTimer -= gFramesPerSecondFrac;
		if (gOutOfAmmoTimer <= 0.0f)
		{
			StartLevelCompletion(1.5);
			gShootoutMode = SHOOTOUT_MODE_PLAYERKILLED;
			gPlayerIsDead = true;
		}
	}
	else
		gOutOfAmmoTimer = 3.0f;				// wait 3 seconds after running out of ammo before ending level
}



#pragma mark -

/********************** COUNT SHOOUTOUT ENEMIES *************************/

static void CountShooutoutEnemies(void)
{
long					i, sp;
TerrainItemEntryType	*itemPtr;


	
			/* INIT STUFF */
		
	itemPtr = *gMasterItemList; 												// get pointer to data inside the LOCKED handle
	gStopPointNum = 0;


		/******************************************************/
		/* COUNT NUMBER OF ENEMIES TO KILL AT EACH STOP POINT */
		/******************************************************/

	for (sp = 0; sp < MAX_STOP_POINTS; sp++)								// look for enemies in each area
	{
		gNumEnemiesThisStopPoint[sp] = 0;									// assume none


				/* COUNT ITEMS */
				
		for (i= 0; i < gNumTerrainItems; i++)
		{
			switch(itemPtr[i].type)
			{
				case	MAP_ITEM_SHOOTOUT_BANDITO:							// see if it's an enemy
				case	MAP_ITEM_SHOOTOUT_SHORTY:	
				case	MAP_ITEM_SHOOTOUT_TREMORALIEN:
				case	MAP_ITEM_SHOOTOUT_FROGMAN:
						if (itemPtr[i].parm[0] == sp)								// is it active on this stop point?
							gNumEnemiesThisStopPoint[sp]++;
						break;

				case	MAP_ITEM_SHOOTOUT_SWAMPGRAVE:
						if (itemPtr[i].parm[1] == sp)								// is it active on this stop point?
							gNumEnemiesThisStopPoint[sp]++;
						break;
						
			}
		}
				
		
	}
}


/********************* DID BILLY CROSS STOP MARKER ***********************/
//
// Returns true if we've passed it thus we should stop and set to next stop point.
//

static Boolean DidBillyCrossStopMarker(ObjNode *player)
{
int		markerNum;

	if (SeeIfCrossedLineMarker(&player->OldCoord, &player->Coord, &markerNum))
	{			
		return(true);
	}
	
	return(false);
}

/********************* GO TO NEXT STOP POINT *********************/

void GoToNextStopPoint(void)
{
	MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_WALK, 7);
	gShootoutMode = SHOOTOUT_MODE_WALK;
	gStopPointNum++;
}



#pragma mark -

/******************** UPDATE CROSSHAIRS ************************/


static void UpdateCrosshairs(void)
{
float	minX,maxX;
float	fps = gFramesPerSecondFrac;
Boolean	allowRot;
Point	mousePt;


	if (gShootoutMode == SHOOTOUT_MODE_PLAYERKILLED)
		return;


	allowRot = gShootoutMode == SHOOTOUT_MODE_BATTLE;			// only allow edge rotation when in battle mode

	if (allowRot)
	{
		minX = 50;
		maxX = 640-50;
	}
	else
	{
		minX = 0;
		maxX = 640;	
	}


			/*****************************/
			/* UPDATE CROSSHAIR POSITION */
			/*****************************/

			/* GET MOUSE DELTAS & MOVE CROSSHAIRS */

	GetMouseCoord(&mousePt);


				/* CHECK REAL CURSOR WINDOW PINNING TO OUR LOCAL WINDOW */
				
	if (mousePt.h > gCursorPinRight)
	{
		gCursorPinRight = mousePt.h;
		gCursorPinLeft = gCursorPinRight - 640;
	}
	else
	if (mousePt.h < gCursorPinLeft)
	{
		gCursorPinLeft = mousePt.h;
		gCursorPinRight = gCursorPinLeft + 640;
	}

	if (mousePt.v > gCursorPinBottom)
	{
		gCursorPinBottom = mousePt.v;
		gCursorPinTop = gCursorPinBottom - 480;
	}
	else
	if (mousePt.v < gCursorPinTop)
	{
		gCursorPinTop = mousePt.v;
		gCursorPinBottom = gCursorPinTop + 480;
	}

	gCrosshairsCoord.x = (float)(mousePt.h - gCursorPinLeft);
	gCrosshairsCoord.y = (float)(mousePt.v - gCursorPinTop);

		
	if (gCrosshairsCoord.y < 0.0f)							// check y coord
		gCrosshairsCoord.y = 0;
	else
	if (gCrosshairsCoord.y >= 480.0f)
		gCrosshairsCoord.y = 479;


		/* SEE IF DO EDGE-SCROLL */

	if (gCrosshairsCoord.x < minX)							// see if off left
	{
		if (allowRot && (gMouseDeltaX < 0.0f))
		{
			gPlayerInfo.objNode->Rot.y -= gMouseDeltaX * fps * .014;
			if (gPlayerInfo.objNode->Rot.y >= PI2)				// check for wraparound
				gPlayerInfo.objNode->Rot.y -= PI2;
		}
	}
	else
	if (gCrosshairsCoord.x >= maxX)						// see if off right
	{
		if (allowRot && (gMouseDeltaX > 0.0f))
		{
			gPlayerInfo.objNode->Rot.y -= gMouseDeltaX * fps * .014;
			if (gPlayerInfo.objNode->Rot.y <= 0.0f)				// check for wraparound
				gPlayerInfo.objNode->Rot.y += PI2;
		}
		
	}
			/* SEE IF DO KEY SCROLL */
			
	else
	if (allowRot)
	{
		if (GetNeedState(kNeed_UILeft))
			gPlayerInfo.objNode->Rot.y += fps * 2.0f;
		else
		if (GetNeedState(kNeed_UIRight))
			gPlayerInfo.objNode->Rot.y -= fps * 2.0f;
		//else
		//if (gScrollWheelDelta != 0)
		//{
		//	gPlayerInfo.objNode->Rot.y += fps * (float)gScrollWheelDelta * 1.4f;
		//	gScrollWheelDelta = 0;						// reset to 0 since event won't clear it
		//}
	}


			/****************/
			/* SEE IF SHOOT */
			/****************/

	if (GetNewNeedState(kNeed_Shoot) && (!gNeedToReloadNextAmmoClip))
	{
		ShootBulletThruCrosshairs();
	}
}


/************************* SHOOT BULLET THRU CROSSHAIRS ***************************/

static void ShootBulletThruCrosshairs(void)
{
OGLPoint2D	screenPt;
OGLRay		ray;
OGLPoint3D	bulletStart;
ObjNode	*newObj;
float	speed;
int		i, numBulletsInClip;

			/* SEE IF HAVE AMMO */
			
	if (gPlayerInfo.ammoCount > 0)
	{
		gPlayerInfo.ammoCount--;
		if (gPlayerInfo.ammoCount == AMMO_CLIP_SIZE)		// sound warning if this is the last clip
			PlayEffect(EFFECT_ALARM);

		numBulletsInClip = gPlayerInfo.ammoCount % AMMO_CLIP_SIZE;		// how many bullets remaining in this clip?
		if ((numBulletsInClip == 0)	&& (gPlayerInfo.ammoCount > 0))		// if no more bullets in clip, but we have more ammo then we need to reload the next clip
		{
//			gNeedToReloadNextAmmoClip = true;
		}
	}
	else
	{
		PlayEffect(EFFECT_EMPTY);
		return;
	}


					/* GET BULLET INIT INFO */

	screenPt.x = (gCrosshairsCoord.x / 639.0f) * gGameWindowWidth;				// calc screen coords of crosshairs
	screenPt.y = (gCrosshairsCoord.y / 479.0f) * gGameWindowHeight;
					
	OGL_GetWorldRayAtScreenPoint(&screenPt, &ray);								// get bullet direction vector
	bulletStart.x = gPlayerInfo.camera.cameraLocation.x + (ray.direction.x * 40.0f);	// get bullet start coord
	bulletStart.y = gPlayerInfo.camera.cameraLocation.y + (ray.direction.y * 40.0f);
	bulletStart.z = gPlayerInfo.camera.cameraLocation.z + (ray.direction.z * 40.0f);


			/**********************/
			/* MAKE BULLET OBJECT */
			/**********************/
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Bullet;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord		= bulletStart;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_USEALIGNMENTMATRIX;
	gNewObjectDefinition.slot 		= 228;
	gNewObjectDefinition.moveCall 	= MoveCrosshairsBullet;		
	gNewObjectDefinition.rot 		= 0;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	SetAlignmentMatrix(&newObj->AlignmentMatrix, &ray.direction);

	newObj->Health = 3.0f;										// time that bullet can live

	newObj->What = WHAT_PLAYERBULLET;

				/* SET SPEED OF BULLET */
					
	speed = 3400.0f + RandomFloat2() * 100.0f;					// slightly random bullet speeds
	newObj->Delta.x = ray.direction.x * speed;
	newObj->Delta.y = ray.direction.y * speed;
	newObj->Delta.z = ray.direction.z * speed;


			/* GIVE IT A SHADOW */
			
	AttachShadowToObject(newObj, 0, 1, 1, false);



			/* SPARKLE */
			
	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);										// get sparkle index
	if (i != -1)
	{
		
		gSparkles[i].where.x = 0;
		gSparkles[i].where.y = 0;
		gSparkles[i].where.z = 0;
		gSparkles[i].color.r =
		gSparkles[i].color.g =
		gSparkles[i].color.b = 1.0f;
		gSparkles[i].color.a = .8;
		
		gSparkles[i].flags = SPARKLE_FLAG_TRANSFORMWITHOWNER|SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].scale = 70.0f;
		gSparkles[i].separation = 0.0f;
		gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
		
		
	}	



	PlayEffect(EFFECT_GUNSHOT);			
}



/********************* MOVE CROSSHAIRS BULLET ******************************/

static void MoveCrosshairsBullet(ObjNode *bullet)
{
float		fps = gFramesPerSecondFrac;
ObjNode		*target;
OGLPoint3D	impactCoord;
OGLVector3D	impactNormal;


			/* SEE IF BULLET TIMES OUT */
			
	bullet->Health -= fps;
	if (bullet->Health <= 0.0f)
	{
		DeleteObject(bullet);
		return;
	}

	GetObjectInfo(bullet);

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/*************************/
		/* UPDATE COLLISION INFO */
		/*************************/
						
		


	target = OGL_DoLineSegmentCollision(&bullet->OldCoord, &gCoord, &impactCoord, &impactNormal, CTYPE_PICKABLE);				
	if (target)
	{
				/* THERE WAS AN IMPACT */
				
		if (target->HitByBulletCallback)
		{
			target->HitByBulletCallback(bullet, target, &impactCoord);
		}
		else
		{
			MakePuff(&impactCoord, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);			
			PlayEffect3D(EFFECT_RICOCHET, &impactCoord);
		}
									
		DeleteObject(bullet);
		return;
	}

			/* SEE IF HIT GROUND */

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		PlayEffect3D(EFFECT_RICOCHET, &gCoord);
		MakePuff(&gCoord, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);			
		DeleteObject(bullet);
		return;
	}


	UpdateObject(bullet);

}




#pragma mark -


/************************ MOVE PLAYER: SHOOTOUT ***************************/

void MovePlayer_Shootout(ObjNode *player)
{
	switch(gShootoutMode)
	{
		case	SHOOTOUT_MODE_WALK:
				MovePlayer_Shootout_Walk(player);
				break;	


		case	SHOOTOUT_MODE_BATTLE:
				MovePlayer_Shootout_Battle(player);
				break;	
	
	}
	

	UpdateBillyAttachments(player);	
	UpdatePlayerShield();
	CalcObjectBoxFromNode(player);
	
}


/*********************** MOVE PLAYER: SHOOTOUT WALK ***********************/
//
// Player is walking along spline to the next stop point
//

static void MovePlayer_Shootout_Walk(ObjNode *player)
{
float	x,z, y;
float	fps = gFramesPerSecondFrac;


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(player, 120, false);
	GetObjectCoordOnSpline(player);

			/****************/
			/* CALC Y COORD */
			/****************/
			
			/* UPDATE STEP-UP/DOWN */
			
	if (gStepUp)
	{
		gStepUpOffset += 300.0f * fps;
		if (gStepUpOffset > 40.0f)
			gStepUpOffset = 40.0f;
	}
	else
	{
		gStepUpOffset -= 300.0f * fps;
		if (gStepUpOffset < 0.0f)
			gStepUpOffset = 0.0f;
	}
				
	y = (GetTerrainY(player->Coord.x, player->Coord.z) + gStepUpOffset) - player->BottomOff;
	player->Coord.y = y;

			/* DO ROTATION */
			//
			// Since we might be aiming the wrong way when we start walking
			// we'll make the player turn towards the desired rot instead of just setting it.
			// that will be a smoother thing to do.
			//
			
	GetCoordOnSpline2(&(*gSplineList)[player->SplineNum], player->SplinePlacement, 10, &x, &z, false);		// get coord of next point on spline
	TurnObjectTowardTarget(player, &player->Coord, x, z, 4.0, false);

	gPlayerInfo.coord.x = x;
	gPlayerInfo.coord.z = z;
	
	UpdateObjectTransforms(player);											// update transforms
	UpdateShadow(player);	
	
	
			/* SEE IF REACHED STOP POINT */
			
	if (DidBillyCrossStopMarker(player))
	{
		gShootoutMode = SHOOTOUT_MODE_BATTLE;								// do battle here now	
		
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAND, 5);
	}		
	
	
		/* SEE IF CROSSED LINE MARKER */
		
#if 0			
	if (SeeIfCrossedLineMarker(&player->OldCoord, &player->Coord, &markerNum))
	{			
		switch(markerNum)
		{
				/* STEP UP */
				
			case	0:
			case	2:
					gStepUp = true;
					break;
		
				/* STEP DOWN */
				
			case	1:
			case	3:
					gStepUp = false;
					break;
		
		
		
		}
	}
#endif	
}



/*********************** MOVE PLAYER: BATTLE ***********************/

static void MovePlayer_Shootout_Battle(ObjNode *player)
{
	GetObjectInfo(player);
	
	switch(player->Skeleton->AnimNum)
	{
				/* STANDING */
				
		case	PLAYER_ANIM_STAND:
				if (GetNewNeedState(kNeed_Duck))							// duck now?
				{	
					MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_DUCK, 7);
					break;
				}
				break;
				
		case	PLAYER_ANIM_DUCK:
				if (!GetNeedState(kNeed_Duck))								// un-duck
				{	
					MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAND, 4);
					break;
				}			
				break;
				
		case	PLAYER_ANIM_STUNNED:
				if (player->Skeleton->AnimHasStopped)
					MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAND, 10);					
				break;

	
	}
	
	if (gShootoutCanProceedToNextStopPoint)				// are we just waiting for the player to press TAB?
	{
		if (GetNewNeedState(kNeed_Continue))
		{
			GoToNextStopPoint();
			gShootoutCanProceedToNextStopPoint = false;
		}	
	}
	
	UpdateObject(player);
	
}


/******************* SHOOTOUT PLAYER HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

void ShootoutPlayerHitByBulletCallback(ObjNode *bullet, ObjNode *player, const OGLPoint3D *impactPt)
{
OGLVector3D	splatVec;

	

			/* SEE IF MUST HAVE HIT SHIELD FIRST */
			//
			// check this just in case the bullet somehow made it thru the shield
			//
			
	if (gPlayerInfo.shieldPower > 0.0f)
	{
		PingShield(bullet->Damage);
		return;
	}


	if (impactPt)
	{
				/* MAKE BULLET IMPACT SPLAT */
				
		splatVec.x = -gDelta.x;
		splatVec.y = -gDelta.y;
		splatVec.z = -gDelta.z;
		OGLVector3D_Normalize(&splatVec, &splatVec);
		DoBulletImpact(impactPt, &splatVec, 1.0);

		PlayEffect3D(EFFECT_BULLETHIT, impactPt);			
	}

	if (player->Health > 0.0f)									// if not already dead
	{
		player->Health -= bullet->Damage;		
	}

				/*****************************/
				/* PLAYER HAS BEEN SHOT DEAD */
				/*****************************/
	else
	if (gShootoutMode != SHOOTOUT_MODE_PLAYERKILLED)
	{
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_SHOTINCHEST, 2);		
		StartLevelCompletion(5.0);
		gShootoutMode = SHOOTOUT_MODE_PLAYERKILLED;
		gPlayerIsDead = true;
	}

}



#pragma mark -



/************************* ADD SHOOTOUT SALOON *********************************/
//
// This is the special saloon that swaps low-rez / hi-rez versions for the shootout level.
//

Boolean AddShootoutSaloon(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_BUILDINGS;	
	gNewObjectDefinition.type 		= BUILDING_ObjType_Saloon;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 55;
	gNewObjectDefinition.moveCall 	= MoveShootoutSaloon;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE|CTYPE_HITENEMYBULLET;

	return(true);													// item was added
}

/********************* MOVE SHOOTOUT SALOON **********************/

static void MoveShootoutSaloon(ObjNode *theNode)
{		

	if (TrackTerrainItem(theNode))									// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* SEE IF SHOULD BE INTERIOR OR EXTERIOR */

	if ((gStopPointNum == 2) || (gStopPointNum == 3)) // && (gShootoutMode == SHOOTOUT_MODE_WALK)))		// are we doing saloon battle?
	{
		if (theNode->Type != BUILDING_ObjType_SaloonInside)			// do we need to swap to interior?
		{
			theNode->Type = BUILDING_ObjType_SaloonInside;
			ResetDisplayGroupObject(theNode);
		}	
	}
	else															// not saloon battle stop point
	{
		if (theNode->Type != BUILDING_ObjType_Saloon)				// do we need to swap to exterior?
		{
			theNode->Type = BUILDING_ObjType_Saloon;
			ResetDisplayGroupObject(theNode);
		}	
	}

}





/************************* ADD SHOOTOUT ALLEY *********************************/
//
// This is the special saloon that swaps low-rez / hi-rez versions for the shootout level.
//

Boolean AddShootoutAlley(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_Alley;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 88;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE;

	return(true);													// item was added
}



