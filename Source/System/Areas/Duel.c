/****************************/
/*    		DUEL.C	 		*/
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

static void MoveEverything_Duel(void);
static void CleanupDuel(void);
static void InitDualArea(void);
static void MoveDueler(ObjNode *theNode);
static void MovePlayer_Duel_WalkOn(ObjNode *player);
static void MovePlayer_Duel_DrawShoot(ObjNode *player);
static void StartDuelDrawShootAction(ObjNode *player);
static void FirePlayerDuelGuns(ObjNode *player, Byte side);
static void FireEnemyDuelGun(ObjNode *enemy);

static ObjNode *CreateBullet(OGLPoint3D *muzzleCoord, OGLVector3D *aim, OGLPoint3D *bulletTargetCoord, ObjNode *bulletTargetObj, Boolean isPlayerBullet);
static void MovePlayersBullet(ObjNode *theNode);
static void MoveEnemyBullet(ObjNode *theNode);

static void InitDuelKeySequence(void);
static void UpdateDuelKeySequence(void);
static void ClearDuelKeyBuffer(void);
static void NewRandomDuelKeySequence(void);

static void DetermineDuelShootOutcome(void);
static void CalcTargetPointAndAimOnEnemy(ObjNode *enemy, OGLPoint3D *muzzleCoord, OGLVector3D *aim, ObjNode *gun, OGLPoint3D *targetPt);
static void CalcTargetPointAndAimOnPlayer(OGLPoint3D *muzzleCoord, OGLVector3D *aim, ObjNode *gun, OGLPoint3D *targetPt);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	TIME_DILATION		.14f

#define	ALLOW_WALK	0

enum
{
	DUEL_STAGE_WALKON = 0,
	DUEL_STAGE_FACEOFF,
	DUEL_STAGE_SHOOT
};

#define	MAX_DUELERS		3

#define	BULLET_SPEED	1100.0f //1500.0f



#define	MAX_DUEL_DIFFICULTY		6


/****************************/
/*    VARIABLES             */
/****************************/

static	int	gDuelStage;

ObjNode	*gDuelers[MAX_DUELERS];

Boolean	gDoneFaceOff;

#define	DrawGunNow		Flag[0]
#define	ShootGunForward	Flag[1]
#define	ShootGunToLeft 	Flag[2]
#define	ShootGunToRight	Flag[3]

#define	TargetCoord	SpecialPt[0]
#define TargetObj	SpecialObjPtr[0]

int		gDuelKeySequenceLength, gDuelKeyBufferIndex;
Byte	gDuelKeySequence[MAX_DUEL_KEY_SEQUENCE_LENGTH];
Byte	gDuelKeySequenceMode;
static float	gDuelKeySequenceDelay, gDuelKeySequenceTimeLimit;
int		gDuelReflex;
Boolean	gPlayerToWinDuel;


static	Byte		gDuelDifficulty = 0;
static 	Byte		gNumDuelers[MAX_DUEL_DIFFICULTY] = {1,2,3,3,3,3};
static	Byte		gDuelerType[MAX_DUEL_DIFFICULTY][3] =
					{
						0,0,0,
						2,0,0,
						1,2,0,
						0,1,2,
						2,0,1,
						1,2,0,
					};


/************************* PLAY DUEL *******************************/
//
// INPUT:  	difficulty = 0..2
//			whichPlanet = 0 = town, 1 = alien
//

void PlayDuel(Byte difficulty)
{
	gDuelDifficulty = difficulty;

		/*******************************/
        /* LOAD ALL OF THE ART & STUFF */
		/*******************************/

	InitDualArea();


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
		MoveEverything_Duel();
		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);
		OGL_DrawScene(gGameViewInfoPtr,DefaultDrawCallback);

								
				/* MISC STUFF */
		
		if (GetKeyState_Real(KEY_APPLE) && GetKeyState_Real(KEY_F10))	// cheat to bail out
		{
			gPlayerToWinDuel = true;
			gPlayerIsDead = false;
			break;
		}
		
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

	OGL_FadeOutScene(DefaultDrawCallback, NULL);

		/* CLEANUP LEVEL */
					
	MyFlushEvents();
	CleanupDuel();
	GameScreenToBlack();	
}



/***************** INIT DUEL AREA ************************/

static void InitDualArea(void)
{
OGLSetupInputType	viewDef;
int					i;

	gDuelStage = DUEL_STAGE_WALKON;
	InitDuelKeySequence();
	
		/*********************/
		/* INIT COMMON STUFF */
		/*********************/

	gGameFrameNum 		= 0;
	gGameOver 			= false;
	gLevelCompleted 	= false;
	gDoneFaceOff		= false;
	gPlayerIsDead		= false;
	
	gGravity = NORMAL_GRAVITY;					// assume normal gravity

	gPlayerInfo.objNode = nil;

	for (i = 0; i < MAX_DUELERS; i++)			// assume no duelers
		gDuelers[i] = nil;


			/*************/
			/* MAKE VIEW */
			/*************/

	SetTerrainScale(DEFAULT_TERRAIN_SCALE);								// set scale to some default for now

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphFocallength	= 40.0f;	// set camera info
		gAnaglyphEyeSeparation 	= 2.5f;
	}

				
			/* SETUP VIEW DEF */
			
	OGL_NewViewDef(&viewDef);
	
	viewDef.camera.hither 			= 10;
	viewDef.camera.fov 				= GAME_FOV;	
	viewDef.styles.useFog			= true;
	viewDef.view.clearColor.r 		= .6;
	viewDef.view.clearColor.g 		= .6;
	viewDef.view.clearColor.b		= .4;	
	viewDef.view.clearBackBuffer	= true;
	viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .95f;
	viewDef.styles.fogStart			= viewDef.camera.yon * .4f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
	gDrawLensFlare = true;
	
		
			/* SET LIGHTS */


	viewDef.lights.numFillLights 		= 2;	
	viewDef.lights.ambientColor.r 		= .35;
	viewDef.lights.ambientColor.g 		= .3;
	viewDef.lights.ambientColor.b 		= .3;

	gWorldSunDirection.x = -1.0;
	gWorldSunDirection.y = -.6;
	gWorldSunDirection.z = 1.0;
	OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);
	viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
	viewDef.lights.fillColor[0] 		= gFillColor1;

	viewDef.lights.fillDirection[1].x 	= 1.0;
	viewDef.lights.fillDirection[1].y 	= -0.4;
	viewDef.lights.fillDirection[1].z 	= -.7;
	viewDef.lights.fillColor[1].r		= .8f;
	viewDef.lights.fillColor[1].g		= .8f;
	viewDef.lights.fillColor[1].b		= .7f;


			/* MAKE OGL DRAW CONTEXT */
			
	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


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
	
	
	PlayEffect_Parms(EFFECT_WIND,FULL_CHANNEL_VOLUME/2,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE);
	LoadDuelArt(gGameViewInfoPtr);			
	InitInfobar();

			/* INIT OTHER MANAGERS */

	InitEnemyManager();
	InitEffects(gGameViewInfoPtr);
	InitSparkles();
	InitItemsManager();

			
		/* INIT THE PLAYER & RELATED STUFF */
	
	PrimeTerrainWater();							// NOTE:  must do this before items get added since some items may be on the water
	PrimeSplines();
	PrimeFences();
#if ALLOW_WALK	
	InitPlayerAtHub();
#else
	InitPlayerForDuel();							// NOTE:  this will also cause the initial items in the start area to be created
#endif	
			
						
			/* INIT CAMERAS */
			
#if ALLOW_WALK	
	InitCamera_Terrain();
#else
	InitCamera_Duel();
#endif
			
	HideCursor();								// do this again to be sure!	


		/* START MUSIC */
				
	PlaySong(SONG_DUEL, false);	
}





/**************** CLEANUP LEVEL **********************/

static void CleanupDuel(void)
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

static void MoveEverything_Duel(void)
{
	MoveObjects();
	MoveSplineObjects();
	
#if ALLOW_WALK	
	UpdateCamera_Terrain();
#else
	UpdateCamera_Duel();								// update camera
#endif	

	UpdateDuelKeySequence();
	
}

 

#pragma mark -


/************************* ADD DUELER *********************************/

Boolean AddDueler(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		duelerNum = itemPtr->parm[0];
float	rot = (float)itemPtr->parm[1] * (PI2/8);
int		duelerType;
			
	if (duelerNum >= gNumDuelers[gDuelDifficulty])						// see if we want this guy
		return(false);

	duelerType = gDuelerType[gDuelDifficulty][duelerNum];

	switch(duelerType)
	{
				/* BANDITO */

		case	0:
				newObj = MakeBandito(x, z, rot, 0, MoveDueler, false);			
				break;

				/* RYGAR */

		case	1:
				newObj = MakeRygar(x, z, rot, 0, MoveDueler, false);			
				break;

				/* SHORTY */

		case	2:
				newObj = MakeShorty(x, z, rot, 0, MoveDueler, false);			
				break;
		
		default:
				GAME_ASSERT_MESSAGE(false, "Unsupported dueler type");
	}
			
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list
	newObj->DuelerNum = duelerNum;

	gDuelers[duelerNum] = newObj;

	return(true);													// item was added
}


/********************** MOVE DUELER *****************************/

static void MoveDueler(ObjNode *theNode)
{
	GetObjectInfo(theNode);

			/* SEE IF DRAW GUN NOW */
			
	if (theNode->DrawGunNow)
	{
		theNode->DrawGunNow = false;
		BanditoPutGunInHand(theNode);
	}


		/* SEE IF FIRE NOW */
		
	if (theNode->ShootGunForward)
	{
		theNode->ShootGunForward = false;
	
		FireEnemyDuelGun(theNode);
	}


			/* UPDATE */
			
	UpdateEnemyAttachments(theNode);
			
				
	UpdateObject(theNode);
}


/******************* GET RANDOM DUELER ********************/

ObjNode *GetRandomDueler(void)
{
int		i;

			/* SEE IF THERE ARE ANY */
			
	for (i = 0; i < MAX_DUELERS; i++)
		if (gDuelers[i])
			goto ok;
	return(nil);
	

		/* RANDOMLY CHOOSE ONE */
ok:
	do													// keep picking a random slot until we find one
	{
		i = RandomRange(0, MAX_DUELERS-1);
	}while(!gDuelers[i]);

	return(gDuelers[i]);
}


#pragma mark -


/************************ MOVE PLAYER: DUEL ***************************/

void MovePlayer_Duel(ObjNode *player)
{

	GetObjectInfo(player);

	switch(gDuelStage)
	{
		case	DUEL_STAGE_WALKON:
				MovePlayer_Duel_WalkOn(player);
				break;
				
		case	DUEL_STAGE_FACEOFF:
				if (gDoneFaceOff)
				{
					DetermineDuelShootOutcome();
					StartDuelDrawShootAction(player);				
				}
				break;
				
		case	DUEL_STAGE_SHOOT:
				MovePlayer_Duel_DrawShoot(player);
				break;
	}


	UpdateBilly(player);

}


/******************** MOVE PLAYER: WALK ON **********************/

static void MovePlayer_Duel_WalkOn(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
int		markerNum;

			/* MOVE PLAYER */
	
	gDelta.y -= gGravity*fps;					// add gravity
	gCoord.x += fps * 90.0f;
	DoBillyCollisionDetect(player, false);


		/* SEE IF CROSSED LINE MARKER */
		//
		// this indicates that we're done walking on since we're at the battle position
			
	if (SeeIfCrossedLineMarker(&player->OldCoord, &gCoord, &markerNum))
	{			
		gDuelStage = DUEL_STAGE_FACEOFF;
		gDoneFaceOff = false;
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAND, 2);

	}
}


/********************** MOVE PLAYER: DRAW & SHOOT ************************/

static void MovePlayer_Duel_DrawShoot(ObjNode *player)
{
			/* SEE IF DRAW GUN NOW */
			
	if (player->DrawGunNow)
	{
		player->DrawGunNow = false;
		BillyPutGunsInHands(player);
	}


		/* SEE IF FIRE NOW */
		
	if (player->ShootGunForward)						// shoot forward (to dueler #0)
	{
		player->ShootGunForward = false;	
		FirePlayerDuelGuns(player, 0);
	}

	if (player->ShootGunToLeft)							// shoot to left side
	{
		player->ShootGunToLeft = false;	
		FirePlayerDuelGuns(player, 1);
	}

	if (player->ShootGunToRight)						// shoot to right side
	{
		player->ShootGunToRight = false;	
		FirePlayerDuelGuns(player, 2);
	}
	


}


#pragma mark -


/********************* FIRE PLAYER DUEL GUNS **************************/
//
// INPUT: side:  0 = forward
//				 1= left
//				 2 = right
//

static void FirePlayerDuelGuns(ObjNode *player, Byte side)
{
const OGLPoint3D	muzzleOff = {0, -9.3, -3.3};
OGLVector3D			gunAim;
OGLPoint3D			muzzleCoord, bulletTargetCoord;
OGLMatrix4x4		jm,m;
ObjNode				*rightGun = nil;
ObjNode				*leftGun = nil;
ObjNode				*enemy = nil;
		
	leftGun = player->ChainNode;
	rightGun = leftGun->ChainNode;
	
		
				/***************/
				/* DO LEFT GUN */
				/***************/
								
			/* CALC TRUE AIM AND MUZZLE TIP */
				
	switch(side)
	{
		case	0:							// shoot forward?
				FindJointMatrixAtFlagEvent(player, PLAYER_JOINT_LEFTHAND, 1, &jm);		// get hand matrix @ forward shot event
				break;
				
		case	1:							// shoot left
				FindJointMatrixAtFlagEvent(player, PLAYER_JOINT_LEFTHAND, 2, &jm);		// get hand matrix @ left shot event
				break;

		case	2:							// shoot right
				FindJointMatrixAtFlagEvent(player, PLAYER_JOINT_LEFTHAND, 3, &jm);		// get hand matrix @ right shot event
				break;

		default:
				GAME_ASSERT_MESSAGE(false, "Unsupported shoot side for left gun");
	}

	CalcGunMatrixFromJointMatrix(leftGun, &jm, &m);							// calc gun's orientation matrix @ this moment		
	OGLPoint3D_Transform(&muzzleOff, &m, &muzzleCoord);						// calc coord of muzzle tip

	switch(player->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_DRAWSHOOT:
				enemy = gDuelers[0];										// shoot center dueler only
				break;
				
		case	PLAYER_ANIM_DRAWSHOOT2:										// shoot center dueler?
				if (side == 0)							
					goto do_right_gun;										// left gun does not shoot at center enemy in this anim
				else
				if (side == 1)												// shoot left dueler?
					goto do_right_gun;										// left gun does not shoot at left enemy in this anim
				else
				if (side == 2)												// shoot right dueler
					enemy = gDuelers[1];								
				break;


		case	PLAYER_ANIM_DRAWSHOOT3:										
				if (side == 0)												// shoot center dueler?		
					enemy = gDuelers[0];									
				else
				if (side == 1)												// shoot left dueler?
					enemy = gDuelers[2];									
				else
				if (side == 2)												// shoot right dueler
					enemy = gDuelers[1];								
				break;
				
		case	PLAYER_ANIM_DRAWSHOOT4:										
				if (side == 0)												// shoot center dueler?		
					goto do_right_gun;										// left gun does not shoot at center enemy in this anim
				else
				if (side == 2)												// shoot right dueler
					enemy = gDuelers[1];								
				break;

		default:
				GAME_ASSERT_MESSAGE(false, "Unsupported anim #");
	}

	GAME_ASSERT(enemy);
	CalcTargetPointAndAimOnEnemy(enemy, &muzzleCoord, &gunAim, leftGun, &bulletTargetCoord);


			/* MAKE BLAST EFFECT & BULLET */
			
	MakeGunBlast(&muzzleCoord, &gunAim);
	PlayEffect_Parms3D(EFFECT_GUNSHOT, &muzzleCoord, NORMAL_CHANNEL_RATE*2/3, 1.0f);
	CreateBullet(&muzzleCoord, &gunAim, &bulletTargetCoord, enemy, true);


				/****************/
				/* DO RIGHT GUN */
				/****************/
do_right_gun:				
			/* CALC TRUE AIM AND MUZZLE TIP */
				
	switch(side)
	{
		case	0:							// shoot forward?
				FindJointMatrixAtFlagEvent(player, PLAYER_JOINT_RIGHTHAND, 1, &jm);		// get hand matrix @ forward shot event
				break;
				
		case	1:							// shoot left
				FindJointMatrixAtFlagEvent(player, PLAYER_JOINT_RIGHTHAND, 2, &jm);		// get hand matrix @ left shot event
				break;

		case	2:							// shoot right
				FindJointMatrixAtFlagEvent(player, PLAYER_JOINT_RIGHTHAND, 3, &jm);		// get hand matrix @ right shot event
				break;

		default:
				GAME_ASSERT_MESSAGE(false, "Unsupported shoot side for right gun");
	}
																
	CalcGunMatrixFromJointMatrix(rightGun, &jm, &m);				
	OGLPoint3D_Transform(&muzzleOff, &m, &muzzleCoord);

	switch(player->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_DRAWSHOOT:
				enemy = gDuelers[0];										// shoot center dueler
				break;
				
		case	PLAYER_ANIM_DRAWSHOOT2:
				if (side == 0)							
					enemy = gDuelers[0];									
				else
				if (side == 1)												// shoot left dueler?
					enemy = gDuelers[2];	
				else
				if (side == 2)												// shoot right dueler?
					return;													// right gun does not shoot right enemy in this anim		
				break;
				
		case	PLAYER_ANIM_DRAWSHOOT3:										// shoot center dueler?
				if (side == 0)							
					enemy = gDuelers[0];									
				else
				if (side == 1)												// shoot left dueler?
					enemy = gDuelers[2];									
				else
				if (side == 2)												// shoot right dueler
					enemy = gDuelers[1];								
				break;

		case	PLAYER_ANIM_DRAWSHOOT4:										// shoot center dueler?
				if (side == 0)							
					enemy = gDuelers[0];									
				else
					return;													// right gun does not shoot right enemy in this anim		
				break;
				
		default:
				GAME_ASSERT_MESSAGE(false, "Unsupported anim #");
	}

	GAME_ASSERT(enemy);
	CalcTargetPointAndAimOnEnemy(enemy, &muzzleCoord, &gunAim, rightGun, &bulletTargetCoord);


			/* MAKE BLAST EFFECT & BULLET */
			
	MakeGunBlast(&muzzleCoord, &gunAim);
	PlayEffect_Parms3D(EFFECT_GUNSHOT, &muzzleCoord, NORMAL_CHANNEL_RATE*2/3, 1.0f);
	CreateBullet(&muzzleCoord, &gunAim, &bulletTargetCoord, enemy, true);

}


/********************* FIRE ENEMY DUEL GUN **************************/

static void FireEnemyDuelGun(ObjNode *enemy)
{
const OGLPoint3D	muzzleOff = {0, -9.3, -3.3};
OGLVector3D			gunAim;
OGLPoint3D			muzzleCoord, bulletTargetCoord;
OGLMatrix4x4		jm,m;
ObjNode				*gun;			
int					joint;
		
	gun = enemy->ChainNode;

			/* WHICH JOINT? */

	switch(enemy->Kind)
	{
		case	ENEMY_KIND_BANDITO:
				joint = BANDITO_JOINT_RIGHTHAND;
				break;

		case	ENEMY_KIND_RYGAR:
				joint = RYGAR_JOINT_RIGHTHAND;
				break;

		case	ENEMY_KIND_SHORTY:
				joint = SHORTY_JOINT_RIGHTHAND;
				break;

		default:
				GAME_ASSERT_MESSAGE(false, "Unsupported enemy kind");
	}
	
		
			/* CALC TRUE AIM AND MUZZLE TIP */
								
	FindJointMatrixAtFlagEvent(enemy, joint, 1, &jm);	// get hand matrix
	CalcGunMatrixFromJointMatrix(gun, &jm, &m);								// calc gun's orientation matrix @ this moment		
	OGLPoint3D_Transform(&muzzleOff, &m, &muzzleCoord);						// calc coord of muzzle tip

	CalcTargetPointAndAimOnPlayer(&muzzleCoord, &gunAim, gun, &bulletTargetCoord);


			/* MAKE BLAST EFFECT & BULLET */
			
	MakeGunBlast(&muzzleCoord, &gunAim);
	PlayEffect_Parms3D(EFFECT_GUNSHOT, &muzzleCoord, NORMAL_CHANNEL_RATE*2/3, 1.0f);


	CreateBullet(&muzzleCoord, &gunAim, &bulletTargetCoord, gPlayerInfo.objNode, false);
}


/******************* CREATE BULLET **********************/

static ObjNode *CreateBullet(OGLPoint3D *muzzleCoord, OGLVector3D *aim, OGLPoint3D *bulletTargetCoord, ObjNode *bulletTargetObj, Boolean isPlayerBullet)
{
ObjNode	*newObj;
float	speed;

			/**********************/
			/* MAKE BULLET OBJECT */
			/**********************/
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Bullet;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord		= *muzzleCoord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_USEALIGNMENTMATRIX;
	gNewObjectDefinition.slot 		= 190;
	if (isPlayerBullet)
		gNewObjectDefinition.moveCall 	= MovePlayersBullet;
	else
		gNewObjectDefinition.moveCall 	= MoveEnemyBullet;
	
		
	gNewObjectDefinition.rot 		= 0;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	SetAlignmentMatrix(&newObj->AlignmentMatrix, aim);

	newObj->Health = 4.0f;

				/* SET SPEED OF BULLET */
					
	speed = BULLET_SPEED + RandomFloat2() * 100.0f;				// slightly random bullet speeds
	newObj->Delta.x = aim->x * speed;
	newObj->Delta.y = aim->y * speed;
	newObj->Delta.z = aim->z * speed;

	newObj->TargetCoord = *bulletTargetCoord;					// remember where it impacts (if applicable)
	newObj->TargetObj = bulletTargetObj;				// remember who we're shooting

			/* GIVE IT A SHADOW */
			
	AttachShadowToObject(newObj, 0, 1, 1, false);

	return(newObj);	
}


/******************* MOVE PLAYERS BULLET ***********************/

static void MovePlayersBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector3D	toImpact, bulletAim, splatVec;
float	dot;
OGLPoint3D	exitWoundPt;
ObjNode	*enemy;
int			anim;

			/* SEE IF GONE */
			
	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;	
	}

	GetObjectInfo(theNode);

			/* MOVE IT */
				
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/*********************/
		/* CHECK FOR IMPACTS */
		/*********************/

		/* SEE IF HIT DIRT */
		
	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		DeleteObject(theNode);
		return;
	}
	
		/* SEE IF HIT ENEMY */
			
	else
	if (gPlayerToWinDuel)												// only check if we know we're going to win
	{
		toImpact.x = theNode->TargetCoord.x - gCoord.x;					// calc vector to impact target
		toImpact.y = theNode->TargetCoord.y - gCoord.y;
		toImpact.z = theNode->TargetCoord.z - gCoord.z;
		FastNormalizeVector(toImpact.x, toImpact.y, toImpact.z, &toImpact);	
		
		FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &bulletAim);	// calc bullet aim vector
	
		dot = OGLVector3D_Dot(&toImpact, &bulletAim);					// calc angle between
		if (dot < 0.0f)													// see if on other side of the target pt now
		{
					/* DO ENTRY WOUND */
					
			splatVec.x = -gDelta.x;
			splatVec.y = -gDelta.y;
			splatVec.z = -gDelta.z;
			OGLVector3D_Normalize(&splatVec, &splatVec);
			DoBulletImpact(&theNode->TargetCoord, &splatVec, 1.0);
		
					/* DO EXIT WOUND */
		
			splatVec = gDelta;
			OGLVector3D_Normalize(&splatVec, &splatVec);
			exitWoundPt.x = theNode->TargetCoord.x + (splatVec.x * 60.0f);
			exitWoundPt.y = theNode->TargetCoord.y + (splatVec.y * 60.0f);
			exitWoundPt.z = theNode->TargetCoord.z + (splatVec.z * 60.0f);
			DoBulletImpact(&theNode->TargetCoord, &splatVec, 1.0);
		
		
			enemy = (ObjNode *)theNode->TargetObj;				// who are we shooting?
			
			
					/* WHICH DEATH ANIM TO DO */
					
			switch(enemy->Kind)
			{
				case	ENEMY_KIND_BANDITO:
						anim = BANDITO_ANIM_GOTSHOT2;
						break;

				case	ENEMY_KIND_RYGAR:
						anim = RYGAR_ANIM_SHOTINCHEST;
						break;

				case	ENEMY_KIND_SHORTY:
						anim = RYGAR_ANIM_SHOTINCHEST;
						break;

				default:
						GAME_ASSERT_MESSAGE(false, "Unsupported enemy kind");
			}
			
			MorphToSkeletonAnim(enemy->Skeleton, anim, 1);
			enemy->Skeleton->AnimSpeed = TIME_DILATION;

			PlayEffect3D(EFFECT_BULLETHIT, &exitWoundPt);

			gScore += POINTS_DUEL_BANDIT;
		
		
			DeleteObject(theNode);
			return;	
		}
	}



	UpdateObject(theNode);
}


/*********************** MOVE ENEMY BULLET ******************************/

static void MoveEnemyBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector3D	toImpact, bulletAim, splatVec;
float	dot;
OGLPoint3D	exitWoundPt;
ObjNode	*player = gPlayerInfo.objNode;

			/* SEE IF GONE */
			
	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;	
	}

	GetObjectInfo(theNode);

			/* MOVE IT */
				
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/*********************/
		/* CHECK FOR IMPACTS */
		/*********************/

		/* SEE IF HIT DIRT */
		
	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		DeleteObject(theNode);
		return;
	}
	
		/* SEE IF HIT PLAYER */
			
	else
	if (!gPlayerToWinDuel)												// only check if we know player lose
	{
		toImpact.x = theNode->TargetCoord.x - gCoord.x;					// calc vector to impact target
		toImpact.y = theNode->TargetCoord.y - gCoord.y;
		toImpact.z = theNode->TargetCoord.z - gCoord.z;
		FastNormalizeVector(toImpact.x, toImpact.y, toImpact.z, &toImpact);	
		
		FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &bulletAim);	// calc bullet aim vector
	
		dot = OGLVector3D_Dot(&toImpact, &bulletAim);					// calc angle between
		if (dot < 0.0f)													// see if on other side of the target pt now
		{
					/* DO ENTRY WOUND */
					
			splatVec.x = -gDelta.x;
			splatVec.y = -gDelta.y;
			splatVec.z = -gDelta.z;
			OGLVector3D_Normalize(&splatVec, &splatVec);
			DoBulletImpact(&theNode->TargetCoord, &splatVec, 1.0);
		
					/* DO EXIT WOUND */
		
			splatVec = gDelta;
			OGLVector3D_Normalize(&splatVec, &splatVec);
			exitWoundPt.x = theNode->TargetCoord.x + (splatVec.x * 30.0f);
			exitWoundPt.y = theNode->TargetCoord.y + (splatVec.y * 30.0f);
			exitWoundPt.z = theNode->TargetCoord.z + (splatVec.z * 30.0f);
			DoBulletImpact(&theNode->TargetCoord, &splatVec, 1.0);
		
		
			MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_SHOTINCHEST, 12);
			player->Skeleton->AnimSpeed = TIME_DILATION;

			PlayEffect3D(EFFECT_BULLETHIT, &exitWoundPt);
		
		
			DeleteObject(theNode);
			return;	
		}
	}



	UpdateObject(theNode);
}


#pragma mark -


/***************** INIT DUEL KEY SEQUENCE ********************/

static void InitDuelKeySequence(void)
{
	gDuelKeySequenceDelay = 2.0f;						// set n second delay before it starts
	gDuelKeySequenceMode = DUEL_KEY_SEQUENCE_MODE_NONE;	// nothing to do yet
	gDuelReflex = 0;									// no reflex rating yet
}


/***************** UPDATE DUEL KEY SEQUENCE ********************/

static void UpdateDuelKeySequence(void)
{
Byte	key;
float	fps = gFramesPerSecondFrac;

			/* SEE IF DEACTIVATE @ END OF FACEOFF */
			
	if (gDoneFaceOff || (gDuelReflex == MAX_REFLEX_DOTS))				// see if done all the dots or firing now
	{
		gDuelKeySequenceMode = DUEL_KEY_SEQUENCE_MODE_SHRINK;
		return;
	}
	
		/* SEE IF STILL IN DELAY MODE */
		
	if (gDuelKeySequenceDelay > 0.0f)
	{
		gDuelKeySequenceDelay -= fps;	
		if (gDuelKeySequenceDelay <= 0.0f)					// see if time for a new sequence
		{
			NewRandomDuelKeySequence();						// make new random sequence
		}
		else
			return;											// not ready for sequence yet, so bail
	}


			/* SEE IF TIME UP ON THIS SEQ */
			
	gDuelKeySequenceTimeLimit -= fps;
	if (gDuelKeySequenceTimeLimit <= 0.0f)
		goto failed;


		/* SEE WHICH ARROW KEY IS PRESSED */
		
	if (GetNewKeyState_Real(KEY_UP))
		key = 0;
	else
	if (GetNewKeyState_Real(KEY_RIGHT))
		key = 1;
	else
	if (GetNewKeyState_Real(KEY_DOWN))
		key = 2;
	else
	if (GetNewKeyState_Real(KEY_LEFT))
		key = 3;
	else
		return;												// no new key is pressed


		/* IS THIS A CORRECT MATCH IN THE SEQUENCE */
		
	if (key == gDuelKeySequence[gDuelKeyBufferIndex])
	{
		PlayEffect_Parms(EFFECT_DUELKEY,FULL_CHANNEL_VOLUME/4,FULL_CHANNEL_VOLUME/6,NORMAL_CHANNEL_RATE - 0x5000 + (gDuelKeyBufferIndex * 0xC00));
		
		gDuelKeyBufferIndex++;								// it matches, so go to next slot
		if (gDuelKeyBufferIndex == gDuelKeySequenceLength)	// is that all?
		{
			gDuelKeySequenceDelay = .5f;					// set delay before next sequence
			gDuelReflex++;
			if (gDuelReflex > MAX_REFLEX_DOTS)				// pin @ max value
				gDuelReflex = MAX_REFLEX_DOTS;
			if (gDuelReflex == MAX_REFLEX_DOTS)
				PlayEffect_Parms(EFFECT_DUELKEYSDONE,FULL_CHANNEL_VOLUME,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE * 3/4);
		}
	}
	else
	{
failed:	
		PlayEffect_Parms(EFFECT_DUELFAIL,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE * 3/4);
		NewRandomDuelKeySequence();							// failure, so start new sequence
		gDuelReflex--;
		if (gDuelReflex < 0)
			gDuelReflex = 0;
	}
}


/******************* CLEAR DUAL KEY BUFFER **************************/

static void ClearDuelKeyBuffer(void)
{
	gDuelKeyBufferIndex = 0;
	
}


/****************** NEW RANDOM DUEL KEY SEQUENCE ******************/

static void NewRandomDuelKeySequence(void)
{
int		i;

	gDuelKeySequenceLength = gDuelDifficulty + 3 + (gDuelReflex / 4);				// increase sequence length as we get more reflex
	if (gDuelKeySequenceLength > MAX_DUEL_KEY_SEQUENCE_LENGTH)
		gDuelKeySequenceLength = MAX_DUEL_KEY_SEQUENCE_LENGTH;
		

	gDuelKeySequenceMode = DUEL_KEY_SEQUENCE_MODE_PROCESS;
	gDuelKeySequenceTimeLimit = 6.0f;

	ClearDuelKeyBuffer();										// clear the current key buffer

	for (i = 0; i < gDuelKeySequenceLength; i++)				// create new sequence
	{
		gDuelKeySequence[i] = MyRandomLong() & 0x3;				// random arrow key
	}
}


#pragma mark -

/********************** DETERMINE DUEL SHOOT OUTCOME ***************************/

static void DetermineDuelShootOutcome(void)
{

			/*********************/
			/* PLAYER SHOULD WIN */
			/*********************/
			
	if (gDuelReflex >= MAX_REFLEX_DOTS)
	{
		gPlayerToWinDuel = true;
		gPlayerIsDead = false;
	}
	
		/**********************/
		/* PLAYER SHOULD LOSE */
		/**********************/

	else
	{
		gPlayerToWinDuel = false;
		gPlayerIsDead = true;
	}

//	gPlayerToWinDuel = true;	//-------

}


/*********************** CALC TARGET POINT AND AIM ON ENEMY *********************/

static void CalcTargetPointAndAimOnEnemy(ObjNode *enemy, OGLPoint3D *muzzleCoord, OGLVector3D *aim, ObjNode *gun, OGLPoint3D *targetPt)
{
OGLPoint3D		offset;
int				joint;

			/***************************/
			/* THE BULLET NEEDS TO HIT */
			/***************************/
			
	if (gPlayerToWinDuel)
	{		
				/* GET CHEST JOINT */
				
		switch(enemy->Kind)
		{
			case	ENEMY_KIND_BANDITO:
					joint = BANDITO_JOINT_CHEST;
					break;

			case	ENEMY_KIND_RYGAR:
					joint = RYGAR_JOINT_CHEST;
					break;

			case	ENEMY_KIND_SHORTY:
					joint = SHORTY_JOINT_CHEST;
					break;

			default:
					GAME_ASSERT_MESSAGE(false, "Unsupported enemy kind");
		}
		
	
	
				/* CALC COORD OF DESIRED IMPACT */
					
		offset.x = RandomFloat2() * 15.0f;									// set a offset from the exact joint
		offset.y = RandomFloat2() * 15.0f;
		offset.z = -25.0f;		
		FindCoordOnJoint(enemy, joint, &offset, targetPt);					// calc the coord of the impact
		
		
			/* CALC AIM VECTOR FROM MUZZLE TO IMPACT COORD */
			
		aim->x = targetPt->x - muzzleCoord->x;	
		aim->y = targetPt->y - muzzleCoord->y;	
		aim->z = targetPt->z - muzzleCoord->z;	
		OGLVector3D_Normalize(aim, aim);
	}
	
		/****************************/	
		/* THE BULLET NEEDS TO MISS */
		/****************************/	

	else
	{
				/* CALC COORD OF MISS */
					
		offset.y = 50.0f;
		offset.z = 0;		
		
		if (gun->Side == SIDE_LEFT)														// see if miss on left or right side of enemy
		{
			offset.x = -50.0f;															// offset from shoulder to miss
			
				/* GET LEFT SHOULDER JOINT */
				
			switch(enemy->Kind)
			{
				case	ENEMY_KIND_BANDITO:
						joint = BANDITO_JOINT_LEFTSHOULDER;
						break;

				case	ENEMY_KIND_RYGAR:
						joint = RYGAR_JOINT_LEFTSHOULDER;
						break;

				case	ENEMY_KIND_SHORTY:
						joint = SHORTY_JOINT_LEFTSHOULDER;
						break;

				default:
						GAME_ASSERT_MESSAGE(false, "Unsupported enemy kind");
			}
					
			FindCoordOnJoint(enemy, joint, &offset, targetPt);	// calc the coord of the miss
		}
		else
		{
			offset.x = 50.0f;															// offset from shoulder to miss

				/* GET RIGHT SHOULDER JOINT */
				
			switch(enemy->Kind)
			{
				case	ENEMY_KIND_BANDITO:
						joint = BANDITO_JOINT_RIGHTSHOULDER;
						break;

				case	ENEMY_KIND_RYGAR:
						joint = RYGAR_JOINT_RIGHTSHOULDER;
						break;

				case	ENEMY_KIND_SHORTY:
						joint = SHORTY_JOINT_RIGHTSHOULDER;
						break;

				default:
						GAME_ASSERT_MESSAGE(false, "Unsupported enemy kind");
			}
			FindCoordOnJoint(enemy, joint, &offset, targetPt);
		}			
	
			/* CALC AIM VECTOR FROM MUZZLE TO MISS COORD */
			
		aim->x = targetPt->x - muzzleCoord->x;	
		aim->y = targetPt->y - muzzleCoord->y;	
		aim->z = targetPt->z - muzzleCoord->z;	
		OGLVector3D_Normalize(aim, aim);
	}

}


/*********************** CALC TARGET POINT AND AIM ON PLAYER *********************/

static void CalcTargetPointAndAimOnPlayer(OGLPoint3D *muzzleCoord, OGLVector3D *aim, ObjNode *gun, OGLPoint3D *targetPt)
{
ObjNode			*player;
OGLPoint3D		offset;

	player = gPlayerInfo.objNode;		

			/***************************/
			/* THE BULLET NEEDS TO HIT */
			/***************************/
			
	if (!gPlayerToWinDuel)
	{		
				/* CALC COORD OF DESIRED IMPACT */
					
		offset.x = RandomFloat2() * 12.0f;									// set a offset from the exact joint
		offset.y = RandomFloat2() * 12.0f;
		offset.z = -10.0f;		
		FindCoordOnJoint(player, PLAYER_JOINT_UPPERBACK, &offset, targetPt);	// calc the coord of the impact
		
		
			/* CALC AIM VECTOR FROM MUZZLE TO IMPACT COORD */
			
		aim->x = targetPt->x - muzzleCoord->x;	
		aim->y = targetPt->y - muzzleCoord->y;	
		aim->z = targetPt->z - muzzleCoord->z;	
		OGLVector3D_Normalize(aim, aim);
	}
	
		/****************************/	
		/* THE BULLET NEEDS TO MISS */
		/****************************/	

	else
	{
				/* CALC COORD OF MISS */
					
		offset.y = 50.0f;
		offset.z = 0;		
		
		if (gun->Side == SIDE_LEFT)														// see if miss on left or right side of enemy
		{
			offset.x = -50.0f;															// offset from shoulder to miss
			FindCoordOnJoint(player, PLAYER_JOINT_LEFTHIP, &offset, targetPt);			// calc the coord of the miss
		}
		else
		{
			offset.x = 50.0f;															// offset from shoulder to miss
			FindCoordOnJoint(player, PLAYER_JOINT_RIGHTHIP, &offset, targetPt);
		}			
	
			/* CALC AIM VECTOR FROM MUZZLE TO MISS COORD */
			
		aim->x = targetPt->x - muzzleCoord->x;	
		aim->y = targetPt->y - muzzleCoord->y;	
		aim->z = targetPt->z - muzzleCoord->z;	
		OGLVector3D_Normalize(aim, aim);
	}

}


/******************** START DUEL DRAW-SHOOT ACTION **************************/

static void StartDuelDrawShootAction(ObjNode *player)
{
ObjNode	*enemy;
int		anim, i;


	StartLevelCompletion(18);		// level will be done in a few seconds


	gDuelStage = DUEL_STAGE_SHOOT;
		
				/**************************/
				/* GET THE PLAYER STARTED */
				/**************************/
				
	player->DrawGunNow 		= false;
	player->ShootGunForward = false;
	player->ShootGunToLeft  = false;
	player->ShootGunToRight  = false;
	
	
			/* WHICH PLAYER ANIM? */
			
	switch(gNumDuelers[gDuelDifficulty])
	{
		case	1:
				anim = PLAYER_ANIM_DRAWSHOOT;			// shoot forward only
				break;

		case	2:										// shoot to sides and forward
				anim = PLAYER_ANIM_DRAWSHOOT4;			
				break;

		case	3:										// shoot to sides and forward
				anim = PLAYER_ANIM_DRAWSHOOT2;			
				break;

		default:
				GAME_ASSERT_MESSAGE(false, "Unsupported num duelers");
	}
	
	
			/* FAST DRAW FOR WINNING PLAYER */
			
	if (gPlayerToWinDuel)
	{
		MorphToSkeletonAnim(player->Skeleton, anim, 3);
		player->Skeleton->AnimSpeed = TIME_DILATION;						//-----------------
	}	
			/* SLOW DRAW FOR LOSER PLAYER */
	else
	{
		if (gDuelReflex < 6)							// see if really sucky
		{
			MorphToSkeletonAnim(player->Skeleton, anim, 1.5);
			player->Skeleton->AnimSpeed = TIME_DILATION;						//-----------------
		}
		else
		{
			MorphToSkeletonAnim(player->Skeleton, anim, 2);	
			player->Skeleton->AnimSpeed = TIME_DILATION;						//-----------------
		}
	}
	
	
				/***************************/
				/* GET THE ENEMIES STARTED */
				/***************************/
			
	for (i = 0; i < gNumDuelers[gDuelDifficulty]; i++)
	{			
		enemy = gDuelers[i];						// get the dueler objnode
		
		enemy->DrawGunNow 		= false;
		enemy->ShootGunForward 	= false;
		enemy->ShootGunToLeft  = false;
		enemy->ShootGunToRight  = false;
		

				/* WHICH ANIM? */

		switch(enemy->Kind)
		{
			case	ENEMY_KIND_BANDITO:
					anim = BANDITO_ANIM_DRAWANDSHOOT;
					break;

			case	ENEMY_KIND_RYGAR:
					anim = RYGAR_ANIM_DRAWANDSHOOT;
					break;

			case	ENEMY_KIND_SHORTY:
					anim = SHORTY_ANIM_DRAWANDSHOOT;
					break;
		}


				/* FAST DRAW FOR LOSING PLAYER */
				
		if (!gPlayerToWinDuel)
			MorphToSkeletonAnim(enemy->Skeleton, anim, 5);
			
				/* SLOW DRAW FOR WINNING PLAYER */
		else
			MorphToSkeletonAnim(enemy->Skeleton, anim, 1);


		enemy->Skeleton->AnimSpeed = TIME_DILATION;
		
	}	
}













