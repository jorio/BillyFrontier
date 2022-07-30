/****************************/
/*    	STAMPEDE.C	 		*/
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

static void MoveEverything_Stampede(void);
static void CleanupStampede(void);
static void InitStampedeArea(void);
static void UpdatePlayerStampede(ObjNode *player);

static void MoveStampedeKangaOnSpline(ObjNode *theNode);
static void MoveStampedeKangaRexOnSpline(ObjNode *theNode);

static Boolean DoTrig_Boost(ObjNode *chipmunk, ObjNode *who, Byte sideBits);
static void MoveBoost(ObjNode *theNode);

static void MovePlayer_Stampede_Start(ObjNode *player);
static void MovePlayer_Stampede_Run(ObjNode *player);
static void MovePlayer_Stampede_Jump(ObjNode *player);
static void MovePlayer_Stampede_Trampled(ObjNode *player);
static void MovePlayer_Stampede_Tripped(ObjNode *player);
static void MovePlayer_Stampede_Stand(ObjNode *player);

static void BillyTripped(ObjNode *player);
static void BillyGotTrampled(ObjNode *player);

static Boolean DoBillyCollisionDetect_Stampede(ObjNode *theNode, Boolean useBBoxForTerrain);

static void MakeDust(OGLPoint3D *pt);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ALLOW_WALK	0


/****************************/
/*    VARIABLES             */
/****************************/

#define	StompDirt		Flag[0]
#define	MooTimer		SpecialF[0]


int			gDustMagicNum, gDustParticleGroup;


/************************* PLAY STAMPEDE *******************************/

void PlayStampede(void)
{
		/*******************************/
        /* LOAD ALL OF THE ART & STUFF */
		/*******************************/

	InitStampedeArea();


			/* PREP STUFF */

	ReadKeyboard();
	CalcFramesPerSecond();
	CalcFramesPerSecond();


		/******************/
		/* MAIN GAME LOOP */
		/******************/

	while(true)
	{
				/* MOVE, UPDATE, & DRAW */
				
		ReadKeyboard();								
		MoveEverything_Stampede();
		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);
		OGL_DrawScene(gGameViewInfoPtr,DefaultDrawCallback);

								
				/* MISC STUFF */
		
		if (GetNewKeyState_Real(KEY_ESC))								// see if paused
			DoPaused();
			
		CalcFramesPerSecond();		
		
		if (gGameFrameNum == 0)											// if that was 1st frame, then create a fade event
			MakeFadeEvent(true);
				
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
					
	MyFlushEvents();
	GammaFadeOut();
	CleanupStampede();
	GameScreenToBlack();	



}



/***************** INIT STAMPEDE AREA ************************/

static void InitStampedeArea(void)
{
OGLSetupInputType	viewDef;

	
		/*********************/
		/* INIT COMMON STUFF */
		/*********************/

	gGameFrameNum 		= 0;
	gGameOver 			= false;
	gPlayerIsDead 		= false;
	gLevelCompleted 	= false;
	gDustParticleGroup = -1;
	
	gGravity = NORMAL_GRAVITY;					// assume normal gravity

	gPlayerInfo.objNode = nil;
	
	gCurrentMaxSpeed = 1000.0f;
	
	

			/*************/
			/* MAKE VIEW */
			/*************/

	SetTerrainScale(DEFAULT_TERRAIN_SCALE);								// set scale to some default for now

	gSuperTileActiveRange = MAX_SUPERTILE_ACTIVE_RANGE-1;
		
		
	if (gGamePrefs.anaglyph)
	{
		gAnaglyphFocallength	= 100.0f;	// set camera info
		gAnaglyphEyeSeparation 	= 13.0f;
	}
				
			/* SETUP VIEW DEF */
			
	OGL_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= 5;
	viewDef.camera.fov 				= .9;	
	viewDef.styles.useFog			= true;
	viewDef.view.clearColor.r 		= .5;
	viewDef.view.clearColor.g 		= .5;
	viewDef.view.clearColor.b		= .35;	
	viewDef.view.clearBackBuffer	= true;
	viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .95f;
	viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * .9f;
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
			
	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


			/**********************/
			/* SET AUTO-FADE INFO */
			/**********************/
			
	gAutoFadeStartDist	= 0; //gGameViewInfoPtr->yon * .85;
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
	
	LoadStampedeArt(gGameViewInfoPtr);			
	InitInfobar();

			/* INIT OTHER MANAGERS */

	InitEnemyManager();
	InitEffects(gGameViewInfoPtr);
	InitSparkles();
	InitItemsManager();

			
		/* INIT THE PLAYER & RELATED STUFF */
	
	PrimeTerrainWater();							// NOTE:  must do this before items get added since some items may be on the water
#if ALLOW_WALK	
	InitPlayerAtHub();
#else
	InitPlayerForStampede();							// NOTE:  this will also cause the initial items in the start area to be created
#endif	
			
	PrimeSplines();
	PrimeFences();
	
	
						
			/* INIT CAMERAS */
			
#if ALLOW_WALK	
	InitCamera_Terrain();
#else
//	InitCamera_Stampede();
#endif
			
	HideCursor();								// do this again to be sure!	
	GammaFadeOut();


		/* START MUSIC */
				
	PlaySong(SONG_STAMPEDE, true);	
}





/**************** CLEANUP LEVEL **********************/

static void CleanupStampede(void)
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
	
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);	// do this last!			
}


/******************** MOVE EVERYTHING ************************/

static void MoveEverything_Stampede(void)
{
	MoveObjects();
	MoveSplineObjects();
	
#if ALLOW_WALK	
	UpdateCamera_Terrain();
#else
//	UpdateCamera_Stampede();								// update camera
#endif	
	
}


#pragma mark -


/************************ MOVE PLAYER: STAMPEDE ***************************/

void MovePlayer_Stampede(ObjNode *player)
{

	GetObjectInfo(player);

	switch(player->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_STAND:
				MovePlayer_Stampede_Stand(player);
				break;

		case	PLAYER_ANIM_STAMPEDESTART:
				MovePlayer_Stampede_Start(player);
				break;
	
		case	PLAYER_ANIM_RUN:
				MovePlayer_Stampede_Run(player);
				break;
	
		case	PLAYER_ANIM_STAMPEDEJUMP:
				MovePlayer_Stampede_Jump(player);
				break;
				
		case	PLAYER_ANIM_STAMPEDETRAMPLED:
				MovePlayer_Stampede_Trampled(player);
				break;

		case	PLAYER_ANIM_TRIPPED:
				MovePlayer_Stampede_Tripped(player);
				break;
		
	}
	

	UpdatePlayerStampede(player);
}

/***************** MOVE PLAYER STAMPEDE: START ***********************/

static void MovePlayer_Stampede_Start(ObjNode *player)
{
	player->Timer -= gFramesPerSecondFrac;				// see if ready to run yet
	if (player->Timer <= 0.0f)
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_RUN, 5);

}


/***************** MOVE PLAYER STAMPEDE: TRAMPLED ***********************/

static void MovePlayer_Stampede_Trampled(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
float	y;

		/* MOVE IT */
		
	gDelta.y -= gGravity * fps;
	gDelta.x = 0;
	gDelta.z = 0;
	
	gCoord.y += gDelta.y * fps;
	
	
	
	y =  GetTerrainY(gCoord.x, gCoord.z) - player->BBox.min.y;					// get terrain Y
	if (gCoord.y < y)
		gCoord.y = y;
				
}



/***************** MOVE PLAYER STAMPEDE: TRIPPED ***********************/

static void MovePlayer_Stampede_Tripped(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
float	r;

			
		/* MOVE IT */
		
	gDelta.y -= gGravity * fps;
	r = player->Rot.y;
	gDelta.x = -sin(r) * player->Speed2D;
	gDelta.z = -cos(r) * player->Speed2D;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/* COLLISION */
		
	DoBillyCollisionDetect_Stampede(player, false);


	if (player->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_RUN, 6);

}




/***************** MOVE PLAYER STAMPEDE: RUN ***********************/

static void MovePlayer_Stampede_Run(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
float	r;

			/* USER TURNING */
			
	if (GetKeyState_Real(KEY_LEFT) || (gMouseDeltaX < -10))
		player->Rot.y -= 3.0f * fps;
	else
	if (GetKeyState_Real(KEY_RIGHT) || (gMouseDeltaX > 10))
		player->Rot.y += 3.0f * fps;

	r = player->Rot.y;
	

		/* ACCEL TO TOP SPEED */
		
	player->Speed2D += fps * 1000.0f;
	if (player->Speed2D > gCurrentMaxSpeed)
		player->Speed2D = gCurrentMaxSpeed;
		
	
		/* MOVE IT */
		
	gDelta.y -= gGravity * fps;
	gDelta.x = -sin(r) * player->Speed2D;
	gDelta.z = -cos(r) * player->Speed2D;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


	r = player->Speed2D;
	if (r < 450.0f)
		r = 450.0f;
	player->Skeleton->AnimSpeed = r * .0016f;


		/* COLLISION */
		
	DoBillyCollisionDetect_Stampede(player, false);



		/* SEE IF DO JUMP */
		
	if (GetNewKeyState_Real(KEY_SPACE) || GetNewKeyState_Real(KEY_APPLE) || gMouseNewButtonState)
	{
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAMPEDEJUMP, 6);
		player->Skeleton->AnimSpeed = 1.6f;
		gDelta.y = 1400.0f;
		gDelta.x *= .9f;
		gDelta.z *= .9f;
	}
	
	
			/* BOOT TO DIRT? */
			
	if (player->StompDirt)
	{
		OGLPoint3D	p;
		
		player->StompDirt = false;
	
		p.x = gCoord.x;
		p.z = gCoord.z;
		p.y = GetTerrainY(p.x, p.z);

		PlayEffect_Parms3D(EFFECT_SPURS2, &p, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x3fff), .8);
		MakeDust(&p);
			
	
	}
	
}


/***************** MOVE PLAYER STAMPEDE: STAND ***********************/

static void MovePlayer_Stampede_Stand(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
float	r;

			/* USER TURNING */
			
//	r = player->Rot.y -= gPlayerInfo.analogControlX * 3.0f * fps;

	if (GetKeyState_Real(KEY_LEFT))
		player->Rot.y -= 3.0f * fps;
	else
	if (GetKeyState_Real(KEY_RIGHT))
		player->Rot.y += 3.0f * fps;

	r = player->Rot.y;


	gDelta.x = gDelta.z = 0;
		
	
		/* MOVE IT */
		
	gDelta.y -= gGravity * fps;


		/* COLLISION */
		
	DoBillyCollisionDetect_Stampede(player, false);



		/* SEE IF DO JUMP */
		
	if (GetNewKeyState_Real(KEY_SPACE) || GetNewKeyState_Real(KEY_APPLE) || gMouseNewButtonState)
	{
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAMPEDEJUMP, 6);
		player->Skeleton->AnimSpeed = 1.6f;
		gDelta.y = 1400.0f;
		gDelta.x *= .9f;
		gDelta.z *= .9f;
	}
}


/***************** MOVE PLAYER STAMPEDE: JUMP ***********************/

static void MovePlayer_Stampede_Jump(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
	
		/* MOVE IT */
	
		
	gDelta.y -= gGravity * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/* COLLISION */
		
	DoBillyCollisionDetect_Stampede(player, false);


		/* SEE IF LAND */
		
	if (player->StatusBits & STATUS_BIT_ONGROUND)
	{
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_RUN, 6);
	}


}


/************** UPDATE PLAYER STAMPEDE ********************/

static void UpdatePlayerStampede(ObjNode *player)
{	
float	fps = gFramesPerSecondFrac;
int		markerNum;

			/* UPDATE CURRENT MAX SPEED */
		
		
	gCurrentMaxSpeed -= fps * 120.0f;
	if (gCurrentMaxSpeed < PLAYER_STAMPEDE_SPEED)
		gCurrentMaxSpeed = PLAYER_STAMPEDE_SPEED;
		

	UpdateObject(player);
	UpdateBillyAttachments(player);	
	UpdatePlayerShield();
	
	gPlayerInfo.coord = gCoord;
	
	VectorLength2D(player->Speed2D, gDelta.x, gDelta.z);
	

			/* SEE IF DONE */
				
	if (SeeIfCrossedLineMarker(&player->OldCoord, &gCoord, &markerNum))
	{
		if (!gLevelCompleted)							// only win if we were the first to cross the line
		{
			StartLevelCompletion(.5);
			gLevelWon[gCurrentArea/2] = true;
		}
	}
}

#pragma mark -

/******************** DO BILLY COLLISION DETECT **************************/
//
// OUTPUT: true = disabled/killed
//

static Boolean DoBillyCollisionDetect_Stampede(ObjNode *theNode, Boolean useBBoxForTerrain)
{
short		i;
ObjNode		*hitObj;
u_char		sides;
float		distToFloor, terrainY;
float		bottomOff;
Boolean		killed = false;		
	
			/***************************************/
			/* AUTOMATICALLY HANDLE THE GOOD STUFF */
			/***************************************/
			//
			// this also sets the ONGROUND status bit if on a solid object.
			//

	if (useBBoxForTerrain)
		theNode->BottomOff = theNode->BBox.min.y;
	else
		theNode->BottomOff = gPlayerBottomOff;

	sides = HandleCollisions(theNode, PLAYER_COLLISION_CTYPE, -.3);

			/* SCAN FOR INTERESTING STUFF */
						
	for (i=0; i < gNumCollisions; i++)						
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision
			
			if (hitObj->CType == INVALID_NODE_FLAG)				// see if has since become invalid
				continue;
		

			/* HIT A KANGA COW? */
			
			if (hitObj->CType & CTYPE_ENEMY)
			{
				BillyGotTrampled(theNode);
			}
			
				/* HIT ANYTHING ELSE TO TRIP? */
				
			else
			if (hitObj->CType & CTYPE_MISC)
			{
				BillyTripped(theNode);
			}
				
		}
	}

		/*************************************/
		/* CHECK & HANDLE TERRAIN  COLLISION */
		/*************************************/

	if (useBBoxForTerrain)
		bottomOff = theNode->BBox.min.y;							// use bbox for bottom
	else
		bottomOff = theNode->BottomOff;								// use collision box for bottom

	terrainY =  GetTerrainY(gCoord.x, gCoord.z);					// get terrain Y
				
	distToFloor = (gCoord.y + bottomOff) - terrainY;				// calc amount I'm above or under
	
	if (distToFloor <= 0.0f)										// see if on or under floor
	{
		gCoord.y = terrainY - bottomOff;
		gDelta.y = -200.0f;											// keep some downward momentum
		theNode->StatusBits |= STATUS_BIT_ONGROUND;	

	}
		
			/**************************/
			/* SEE IF IN WATER VOLUME */
			/**************************/
			
	if (!killed && (gDelta.y <= 0.0f))					// only check water if moving down and not killed yet
	{
		int		patchNum;
		Boolean	wasInWater;
		
					/* REMEMBER IF ALREADY IN WATER */
					
		if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
			wasInWater = true;
		else
			wasInWater = false;
		
					/* CHECK IF IN WATER NOW */
					
		if (DoWaterCollisionDetect(theNode, gCoord.x, gCoord.y+theNode->BottomOff, gCoord.z, &patchNum))
		{
			gPlayerInfo.waterPatch = patchNum;
			
			gCoord.y = gWaterBBox[patchNum].max.y;

//			if (!wasInWater || (!IsPlayerDoingSwimAnim(theNode)))
//				PlayerEntersWater(theNode, patchNum);

			gDelta.y = 0;
		}
	}

	return(killed);
}


/***************** BILLY GOT TRAMPLED ************************/

static void BillyGotTrampled(ObjNode *player)
{
	if (player->Skeleton->AnimNum != PLAYER_ANIM_STAMPEDETRAMPLED)
	{
		KillPlayer(PLAYER_DEATH_TYPE_TRAMPLED);
		PlayEffect3D(EFFECT_TRAMPLED, &player->Coord);
		PlayEffect3D(EFFECT_YELP, &player->Coord);
		StartLevelCompletion(3.0);
		gPlayerIsDead = true;
	}
}


/***************** BILLY TRIPPED ************************/

static void BillyTripped(ObjNode *player)
{
	if (player->Skeleton->AnimNum == PLAYER_ANIM_TRIPPED)			// already tripped?
		return;

	if (player->Speed2D > 300.0f)
	{
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_TRIPPED, 8);
		gDelta.y = 1100.0f;
		gCoord.y += 20.0f;		
	}
	
}





#pragma mark -


/************************ PRIME STAMPEDE KANGACOW *************************/

Boolean PrimeStampedeKangaCow(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE KANGA SKELETON */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_KANGACOW, KANGA_ANIM_STAMPEDE, x,z, 2.1, 0, nil, 0);
				
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;			
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveStampedeKangaOnSpline;					// set move call
			

	newObj->CType 			= CTYPE_ENEMY;
	newObj->CBits			= CBITS_ALLSOLID;
	
//	newObj->StatusBits |= STATUS_BIT_NOFOG;

	CreateCollisionBoxFromBoundingBox(newObj,1,1);
	newObj->LeftOff 	= -80;
	newObj->RightOff 	= 80;
	newObj->FrontOff 	= 80;
	newObj->BackOff 	= -80;
	CalcObjectBoxFromNode(newObj);
	KeepOldCollisionBoxes(newObj);	


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 5, 12,false);


	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->Skeleton->AnimSpeed = 1.7;
	
	newObj->MooTimer = RandomFloat() * 2.0f;

	return(true);
}


/******************** MOVE KANGA ON SPLINE ***************************/

static void MoveStampedeKangaOnSpline(ObjNode *theNode)
{
Boolean 	isInRange; 
float	speedToMove;
int		markerNum;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

#if 1
	speedToMove = 490.0f;
#else
	distToPlayer = CalcDistance(theNode->Coord.x, theNode->Coord.z, player->Coord.x, player->Coord.z);

	speedToMove = distToPlayer * .4f;
	if (speedToMove < 400.0f)
		speedToMove = 400.0f;
	else
	if (speedToMove > 600.0f)
		speedToMove = 600.0f;
#endif

	IncreaseSplineIndex(theNode, speedToMove, false);
	GetObjectCoordOnSpline(theNode);

			/* DID KANGA WIN BEFORE US? */
			
	if (SeeIfCrossedLineMarker(&theNode->OldCoord, &theNode->Coord, &markerNum))
	{
		StartLevelCompletion(.1);
	}			


			/* UPDATE STUFF IF IN RANGE */
			
//	if (isInRange)
	{		
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord


				/* DO TRIGGER COLLISION */
				
		gCoord = theNode->Coord;
		gDelta.x = gCoord.x - theNode->OldCoord.x;
		gDelta.y = gCoord.y - theNode->OldCoord.y;
		gDelta.z = gCoord.z - theNode->OldCoord.z;
		
		HandleCollisions(theNode, CTYPE_TRIGGERENEMYONLY, 0);


				/* UPDATE */
			
		UpdateObject(theNode);


				/* MOO? */
				
		theNode->MooTimer -= gFramesPerSecondFrac;
		if (theNode->MooTimer <= 0.0f)
		{
			theNode->MooTimer = 2.0f + RandomFloat() * 3.0f;
			
			if (MyRandomLong() & 1)
				PlayEffect_Parms3D(EFFECT_MOO1, &theNode->Coord, NORMAL_CHANNEL_RATE - (MyRandomLong() & 0x3fff), 1.0f);
			else
				PlayEffect_Parms3D(EFFECT_MOO2, &theNode->Coord, NORMAL_CHANNEL_RATE - (MyRandomLong() & 0x3fff), 1.0f);
		}
		
				/* HOOF TO DIRT? */
				
		if (theNode->StompDirt)
		{
			OGLPoint3D	p;
			
			theNode->StompDirt = false;
		
			p.x = gCoord.x;
			p.z = gCoord.z;
			p.y = GetTerrainY(p.x, p.z);

			PlayEffect_Parms3D(EFFECT_HOOF, &p, NORMAL_CHANNEL_RATE - (MyRandomLong() & 0x7fff), .5f + RandomFloat() * .5f);
			MakeDust(&p);
				
		
		}
	}	
}


/************************* MAKE DUST *****************************/

static void MakeDust(OGLPoint3D *pt)
{
long					i;
OGLVector3D				delta;
NewParticleDefType		newParticleDef;

	if (gCurrentArea == AREA_SWAMP_STAMPEDE)		// no dust in the swamp
		return;
	
	if ((gDustParticleGroup == -1) || (!VerifyParticleGroupMagicNum(gDustParticleGroup, gDustMagicNum)))
	{
		
		gDustMagicNum = MyRandomLong();			// generate a random magic num
								
		gNewParticleGroupDef.magicNum				= gDustMagicNum;
		gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
		gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|PARTICLE_FLAGS_ALLAIM;
		gNewParticleGroupDef.gravity				= -100;
		gNewParticleGroupDef.magnetism				= 0;
		gNewParticleGroupDef.baseScale				= 50.0f;
		gNewParticleGroupDef.decayRate				= -1.2;
		gNewParticleGroupDef.fadeRate				= .5;
		gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Dust;
		gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
		gNewParticleGroupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
	}

	gDustParticleGroup = NewParticleGroup(&gNewParticleGroupDef);
	if (gDustParticleGroup != -1)
	{	
		for (i = 0; i < 3; i++)
		{
			delta.x = RandomFloat2() * 30.0f;
			delta.y = RandomFloat() * 20.0f;
			delta.z = RandomFloat2() * 30.0f;
			
			
			newParticleDef.groupNum		= gDustParticleGroup;
			newParticleDef.where		= pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat() * .2f;
			newParticleDef.rotZ			= RandomFloat() * PI2;
			newParticleDef.rotDZ		= RandomFloat2() * 4.0f;
			newParticleDef.alpha		= FULL_ALPHA;		
			if (AddParticleToGroup(&newParticleDef))
				gDustParticleGroup = -1;
		}
	}
}





/************************ PRIME STAMPEDE KANGAREX *************************/

Boolean PrimeStampedeKangaRex(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE KANGA SKELETON */
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_KANGAREX, KANGA_ANIM_STAMPEDE, x,z, 2.1, 0, nil, 0);
				
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;			
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveStampedeKangaRexOnSpline;					// set move call
			

	newObj->CType 			= CTYPE_ENEMY;
	newObj->CBits			= CBITS_ALLSOLID;

	CreateCollisionBoxFromBoundingBox(newObj,1,1);
	newObj->LeftOff 	= -80;
	newObj->RightOff 	= 80;
	newObj->FrontOff 	= 80;
	newObj->BackOff 	= -80;
	CalcObjectBoxFromNode(newObj);
	KeepOldCollisionBoxes(newObj);	


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 5, 12,false);


	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->Skeleton->AnimSpeed = 1.7;
	
	newObj->MooTimer = RandomFloat() * 2.0f;

	return(true);
}


/******************** MOVE KANGA REX ON SPLINE ***************************/

static void MoveStampedeKangaRexOnSpline(ObjNode *theNode)
{
Boolean 	isInRange; 
float		speedToMove;
int			markerNum;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

#if 1
	speedToMove = 450.0f;
#else
	distToPlayer = CalcDistance(theNode->Coord.x, theNode->Coord.z, player->Coord.x, player->Coord.z);

	speedToMove = distToPlayer * .4f;
	if (speedToMove < 400.0f)
		speedToMove = 400.0f;
	else
	if (speedToMove > 600.0f)
		speedToMove = 600.0f;
#endif

	IncreaseSplineIndex(theNode, speedToMove, false);
	GetObjectCoordOnSpline(theNode);


			/* DID KANGA WIN BEFORE US? */
			
	if (SeeIfCrossedLineMarker(&theNode->OldCoord, &theNode->Coord, &markerNum))
	{
		StartLevelCompletion(.1);
	}			


			/* UPDATE STUFF IF IN RANGE */
			
//	if (isInRange)
	{		
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord


				/* DO TRIGGER COLLISION */
				
		gCoord = theNode->Coord;
		gDelta.x = gCoord.x - theNode->OldCoord.x;
		gDelta.y = gCoord.y - theNode->OldCoord.y;
		gDelta.z = gCoord.z - theNode->OldCoord.z;
		
		HandleCollisions(theNode, CTYPE_TRIGGERENEMYONLY, 0);


				/* UPDATE */
			
		UpdateObject(theNode);
		

				/* MOO? */
				
		theNode->MooTimer -= gFramesPerSecondFrac;
		if (theNode->MooTimer <= 0.0f)
		{
			theNode->MooTimer = 2.0f + RandomFloat() * 4.0f;
			
			if (MyRandomLong() & 1)
				PlayEffect_Parms3D(EFFECT_MOO1, &theNode->Coord, NORMAL_CHANNEL_RATE - 0x4000 - (MyRandomLong() & 0x3fff), 1.5f);
			else
				PlayEffect_Parms3D(EFFECT_MOO2, &theNode->Coord, NORMAL_CHANNEL_RATE - 0x4000 - (MyRandomLong() & 0x3fff), 1.5f);
		}
	}	
}




#pragma mark -

/************************* ADD BOOST *********************************/

Boolean AddBoost(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Boost;
	gNewObjectDefinition.scale 		= 1.5f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 40.0f;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 31;
	gNewObjectDefinition.moveCall 	= MoveBoost;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


	newObj->CType 			= CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID | CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox(newObj,2,1);

	newObj->TriggerCallback = DoTrig_Boost;

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 1, 2, false);


	return(true);													// item was added
}

/************** DO TRIGGER - BOOST ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Boost(ObjNode *pow, ObjNode *who, Byte sideBits)
{

	sideBits; who;
	
	pow->ColorFilter.a = 0;					// make invisible so it will delete itself in the move call.

	gCurrentMaxSpeed = 1600.0f;

	PlayEffect_Parms3D(EFFECT_TIMERCHIME, &pow->Coord, NORMAL_CHANNEL_RATE, 1.0f);			

	return(false);
	
	
}


/****************** MOVE BOOST ******************/

static void MoveBoost(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode) || (theNode->ColorFilter.a == 0.0f))			// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
	
	theNode->Rot.y += gFramesPerSecondFrac * 6.0f;
	
	UpdateObjectTransforms(theNode);
	UpdateShadow(theNode);		
}












