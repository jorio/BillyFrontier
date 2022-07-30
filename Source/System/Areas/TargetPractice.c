/****************************/
/*    	TARGET PRACTICE.C	*/
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

static void MoveEverything_TargetPractice(void);
static void CleanupTargetPractice(void);
static void InitTargetPracticeArea(void);

static ObjNode *CreateBullet(OGLPoint3D *muzzleCoord, OGLVector3D *aim, OGLPoint3D *bulletTargetCoord, ObjNode *bulletTargetObj, Boolean isPlayerBullet);
static void MovePlayersBullet(ObjNode *theNode);

static void UpdateTargetPracticeCrosshairs(void);
static Boolean ShootPracticeBullet(void);
static void MoveCrosshairsBullet(ObjNode *bullet);

static void UpdateOrbLauncher(void);
static void LaunchOrb(void);
static void MoveOrb(ObjNode *theNode);
static void OrbHitByBulletCallback(ObjNode *bullet, ObjNode *orb, const OGLPoint3D *impactPt);
static void MoveBonusCoin(ObjNode *theNode);

static void StartDrunkeness(void);
static void UpdateDrunkeness(void);


/****************************/
/*    CONSTANTS             */
/****************************/



/****************************/
/*    VARIABLES             */
/****************************/

float	gTargetPracticeTimer = 0;

static float	gOrbLaunchTimer;

#define	SpewBlood	Flag[3]
#define BloodTimer	SpecialF[0]

static float		gDrunkTimer, gDrunkRadius;
static OGLPoint3D	gDrunkLookOrigin;
static OGLVector2D	gDrunkWobble;
static	float		gRapidFireTimer, gRapidFireDelay;
int			gPepperCount;

/************************* PLAY TARGETPRACTICE *******************************/

void PlayTargetPractice(void)
{
		/*******************************/
        /* LOAD ALL OF THE ART & STUFF */
		/*******************************/

	InitTargetPracticeArea();


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
		MoveEverything_TargetPractice();
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
	CleanupTargetPractice();
	GameScreenToBlack();	
	
	
			/* DID WE WIN? */
			
	if ((gPepperCount <= 0) && (!gPlayerIsDead))
		gLevelWon[gCurrentArea/2] = true;
}



/***************** INIT TARGETPRACTICE AREA ************************/

static void InitTargetPracticeArea(void)
{
OGLSetupInputType	viewDef;
const OGLPoint3D	cameraFrom = { 0, 700, 2500 };
const OGLPoint3D	cameraTo = { 0, 1200, 0 };
	
		/*********************/
		/* INIT COMMON STUFF */
		/*********************/

	gGameFrameNum 		= 0;
	gGameOver 			= false;
	gLevelCompleted 	= false;
	gPlayerIsDead		= false;
	
	gGravity = NORMAL_GRAVITY;					// assume normal gravity

	gPlayerInfo.objNode = nil;
	
	gCrosshairsCoord.x = 640/2;
	gCrosshairsCoord.y = 480/2;

	gOrbLaunchTimer = .5f;

	gTargetPracticeTimer = 60.0f;			// # seconds
	gDrunkTimer	= 0;
	gRapidFireTimer = 0;

	if (gCurrentArea == AREA_TARGETPRACTICE1)
		gPepperCount = 5;
	else
		gPepperCount = 8;


			/*************/
			/* MAKE VIEW */
			/*************/
				
	if (gGamePrefs.anaglyph)
	{
		gAnaglyphFocallength	= 110.0f;	// set camera info
		gAnaglyphEyeSeparation 	= 9.0f;
	}
				
			/* SETUP VIEW DEF */
			
	OGL_NewViewDef(&viewDef);

	viewDef.camera.from			= cameraFrom;
	viewDef.camera.to 				= cameraTo;
	
	viewDef.camera.hither 			= 15;
	viewDef.camera.fov 				= 1.1;	
	viewDef.styles.useFog			= true;
	viewDef.view.clearColor.r 		= .6;
	viewDef.view.clearColor.g 		= .6;
	viewDef.view.clearColor.b		= .4;	
	viewDef.view.clearBackBuffer	= true;
	viewDef.camera.yon 				= 9000.0f;
	viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
	gDrawLensFlare = false;
	
		
			/* SET LIGHTS */


	viewDef.lights.numFillLights 		= 2;	
	viewDef.lights.ambientColor.r 		= .3;
	viewDef.lights.ambientColor.g 		= .25;
	viewDef.lights.ambientColor.b 		= .25;

	gWorldSunDirection.x = -.7;
	gWorldSunDirection.y = -.8;
	gWorldSunDirection.z = .9;
	OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);
	viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
	viewDef.lights.fillColor[0] 		= gFillColor1;

	viewDef.lights.fillDirection[1].x 	= 1.0;
	viewDef.lights.fillDirection[1].y 	= -0.4;
	viewDef.lights.fillDirection[1].z 	= -1.0;
	viewDef.lights.fillColor[1].r		= .7f;
	viewDef.lights.fillColor[1].g		= .7f;
	viewDef.lights.fillColor[1].b		= .6f;


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
		

	
			/*************/
			/* LOAD ART  */
			/*************/
			//
			// NOTE: only call this *after* draw context is created!
			//
	
	LoadTargetPracticeArt(gGameViewInfoPtr);			
	InitInfobar();

			/* INIT OTHER MANAGERS */

	InitPlayerGlobals();
	
	InitEnemyManager();
	InitEffects(gGameViewInfoPtr);
	InitSparkles();
	InitItemsManager();	
	
						
			/* INIT CAMERAS */
			
	InitCamera_TargetPractice();
			
	HideCursor();								// do this again to be sure!	
	GammaFadeOut();


		/* START MUSIC */
				
	PlaySong(SONG_STAMPEDE, true);	
	

			/*******************/	
			/* MAKE BACKGROUND */
			/*******************/	
	
	gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;	
	if (gCurrentArea == AREA_TARGETPRACTICE1)
		gNewObjectDefinition.type 	= PRACTICE_ObjType_CycTown;								
	else
		gNewObjectDefinition.type 	= PRACTICE_ObjType_CycSwamp;								
	
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags 	= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 	= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale 	= gGameViewInfoPtr->yon * .995f / 10.0f;
	gCyc = MakeNewDisplayGroupObject(&gNewObjectDefinition);

//	gCyc->Rot.x  = -.9f;
	gCyc->CustomDrawFunction = DrawCyclorama;		
	
}





/**************** CLEANUP LEVEL **********************/

static void CleanupTargetPractice(void)
{

	StopAllEffectChannels();
 	EmptySplineObjectList();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DeleteAllParticleGroups();
	DeleteAllConfettiGroups();
	DisposeParticleSystem();
	DisposeAllSpriteGroups();
	
	DisposeAllBG3DContainers();
		
	DisposeSoundBank(SOUND_BANK_LEVELSPECIFIC);
	
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);	// do this last!			
}

/******************** MOVE EVERYTHING ************************/

static void MoveEverything_TargetPractice(void)
{
float	fps = gFramesPerSecondFrac;
int		oldTime, newTime;

	MoveObjects();
	
	gCyc->Rot.y += gFramesPerSecondFrac * .04f;
	UpdateObjectTransforms(gCyc);
		
	UpdateOrbLauncher();
	
	UpdateTargetPracticeCrosshairs();	

	
			/* UPDATE TIMER */

	oldTime = gTargetPracticeTimer + .5f;
	
	gTargetPracticeTimer -= fps;
	if (gTargetPracticeTimer < 0.0f)
	{
		gTargetPracticeTimer = 0;
		StartLevelCompletion(1.0);
	}
	
	newTime = gTargetPracticeTimer + .5f;
	
	if (oldTime != newTime)
		if (newTime <= 10)
			PlayEffect(EFFECT_TIMERCHIME);
	
	
	
		/****************/
		/* UPDATE DRUNK */
		/****************/

	UpdateDrunkeness();
	
	
		/* UPDATE RAPID FIRE */
		
	gRapidFireTimer -= fps;
	if (gRapidFireTimer < 0.0f)
		gRapidFireTimer= 0.0f;
}



#pragma mark -

/******************** UPDATE CROSSHAIRS ************************/


static void UpdateTargetPracticeCrosshairs(void)
{
float	fps = gFramesPerSecondFrac;
Point	mousePt;
OGLPoint2D	windowPt;
OGLRay		ray;
ObjNode		*pickedObj;
OGLPoint3D	worldHitCoord;


			/*****************************/
			/* UPDATE CROSSHAIR POSITION */
			/*****************************/

	GetMouseCoord(&mousePt);
	gCrosshairsCoord.x = ((float)mousePt.h / ((float)gGameWindowWidth * .5f)) * 640.0f;
	gCrosshairsCoord.y = ((float)mousePt.v / ((float)gGameWindowHeight * .5f)) * 480.0f;


		
	if (gCrosshairsCoord.y < 0.0f)							// check y coord
		gCrosshairsCoord.y = 0;
	else
	if (gCrosshairsCoord.y >= 480.0f)
		gCrosshairsCoord.y = 479;

	if (gCrosshairsCoord.x < 0.0f)							// check y coord
		gCrosshairsCoord.x = 0;
	else
	if (gCrosshairsCoord.x >= 640.0f)
		gCrosshairsCoord.x = 639;


			/****************/
			/* SEE IF SHOOT */
			/****************/

		/* SEE IF DO RAPID FIRE */
		
	if (gRapidFireTimer > 0.0f)
	{
		if (GetKeyState_Real(kKey_Shoot) || gMouseButtonState)		// trying to rapid fire?
		{
			gRapidFireDelay -= fps;
			if (gRapidFireDelay < 0.0f)
			{
				gRapidFireDelay += .15f;
				goto shoot;
			}
		}
		else
			gRapidFireDelay = 0.0f;								// not trying, so keep delay ready to go instantly
	}
	else
	if (GetNewKeyState_Real(kKey_Shoot) || gMouseNewButtonState)
	{
shoot:	
		
				/* SHOOT BULLET */
				
		if (ShootPracticeBullet())
		{		
			
				/* SEE IF HIT ANYTHING */
						
			windowPt.x = gCrosshairsCoord.x * (gGameWindowWidth/640.0f);			// convert cursor coords to grafport window coords
			windowPt.y = gCrosshairsCoord.y * (gGameWindowHeight/480.0f);
			OGL_GetWorldRayAtScreenPoint(&windowPt, &ray, gGameViewInfoPtr);	// get a world-space ray for picking on the screen

			pickedObj = OGL_DoRayCollision(&ray, &worldHitCoord, STATUS_BIT_HIDDEN|STATUS_BIT_ISCULLED, CTYPE_PICKABLE);
			if (pickedObj)
			{
				if (pickedObj->HitByBulletCallback)
				{
					pickedObj->HitByBulletCallback(nil, pickedObj, &worldHitCoord);
				}
			}	
		}
	
	
			/* IF OUT OF AMMO END LEVEL */
			
		if (gPlayerInfo.ammoCount <= 0)
		{
			PlayEffect(EFFECT_EMPTY);
			StartLevelCompletion(3.0);
		}

	
	}
}


/************************* SHOOT BULLET THRU CROSSHAIRS ***************************/
//
// returns true if shot bullet
//

static Boolean ShootPracticeBullet(void)
{
OGLPoint2D	screenPt;
OGLRay		ray;
OGLPoint3D	bulletStart;
ObjNode	*newObj;
float	speed;


			/* SEE IF HAVE AMMO */
			
	if ((gPlayerInfo.ammoCount <= 0) || gLevelCompleted)
	{
		PlayEffect(EFFECT_EMPTY);
		return(false);
	}
	else
	{
		gPlayerInfo.ammoCount--;
		if (gPlayerInfo.ammoCount == AMMO_CLIP_SIZE)		// sound warning if this is the last clip
			PlayEffect(EFFECT_ALARM);
	}


					/* GET BULLET INIT INFO */

	screenPt.x = (gCrosshairsCoord.x / 639.0f) * gGameWindowWidth;				// calc screen coords of crosshairs
	screenPt.y = (gCrosshairsCoord.y / 479.0f) * gGameWindowHeight;
					
	OGL_GetWorldRayAtScreenPoint(&screenPt, &ray, gGameViewInfoPtr);					// get bullet direction vector
	bulletStart.x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x + (ray.direction.x * 300.0f);	// get bullet start coord
	bulletStart.y = gGameViewInfoPtr->cameraPlacement.cameraLocation.y + (ray.direction.y * 300.0f);
	bulletStart.z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z + (ray.direction.z * 300.0f);


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

	newObj->Health = 1.5f;										// time that bullet can live

	newObj->What = WHAT_PLAYERBULLET;

				/* SET SPEED OF BULLET */
					
	speed = 4000.0f;
	newObj->Delta.x = ray.direction.x * speed;
	newObj->Delta.y = ray.direction.y * speed;
	newObj->Delta.z = ray.direction.z * speed;


			/* GIVE IT A SHADOW */
			
	AttachShadowToObject(newObj, 0, 1, 1, false);


	PlayEffect(EFFECT_GUNSHOT);	
	
	return(true);		
}



/********************* MOVE CROSSHAIRS BULLET ******************************/

static void MoveCrosshairsBullet(ObjNode *bullet)
{
float		fps = gFramesPerSecondFrac;


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


#if 0

ObjNode		*target;
OGLPoint3D	impactCoord;
OGLVector3D	impactNormal;

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
			MakePuff(&impactCoord, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);			
									
		DeleteObject(bullet);
		return;
	}
#endif

	UpdateObject(bullet);

}




#pragma mark -


/******************** UPDATE ORB LAUNCHER ************************/

static void UpdateOrbLauncher(void)
{
	if (gTargetPracticeTimer < 2.0f)			// stop launching when we're about done
		return;

	if (gLevelCompleted)
		return;
		

			/* SEE IF LAUNCH NOW */
			
	gOrbLaunchTimer -= gFramesPerSecondFrac;
	if (gOrbLaunchTimer <= 0.0f)
	{
		if (gCurrentArea == AREA_TARGETPRACTICE1)
			gOrbLaunchTimer = RandomFloat() * 1.0f;
		else
			gOrbLaunchTimer = RandomFloat() * .9f;
	}
	else
		return;


		/* PICK A RANDOM LAUNCH PORT */
		
	LaunchOrb();
}


/******************** LAUNCH ORB ***********************/

static void LaunchOrb(void)
{
ObjNode	*newObj;
int		item;

const Byte	townTable[32] =
{
	WHAT_TNT,
	WHAT_TNT,
	WHAT_SKULL,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_RAPIDFIRE,
	WHAT_RAPIDFIRE,
	WHAT_TIMEORB,
	WHAT_KANGACOW,
	WHAT_KANGACOW,
	WHAT_KANGACOW,
	WHAT_KANGACOW,
	WHAT_KANGACOW,
	WHAT_KANGACOW,
	WHAT_PEPPER,
	WHAT_SHORTY,
	WHAT_SHORTY,
	WHAT_SHORTY,
	WHAT_SHORTY,
	WHAT_SHORTY,
	WHAT_SHORTY,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_AMMOBOX,
	WHAT_AMMOBOX,
};


const Byte	swampTable[32] =
{
	WHAT_TNT,
	WHAT_TNT,
	WHAT_SKULL,
	WHAT_RAPIDFIRE,
	WHAT_RAPIDFIRE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_BOTTLE,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_COINORB,
	WHAT_AMMOBOX,
	WHAT_AMMOBOX,
	WHAT_FROGMAN,
	WHAT_FROGMAN,
	WHAT_FROGMAN,
	WHAT_FROGMAN,
	WHAT_FROGMAN,
	WHAT_FROGMAN,
	WHAT_TREMORGHOST,
	WHAT_TREMORGHOST,
	WHAT_TREMORGHOST,
	WHAT_TREMORGHOST,
	WHAT_TIMEORB,
	WHAT_PEPPER,
};



			/* PICK RANDOM ITEM FROM TABLE */
			
	item = MyRandomLong() & 0x1f;
	if (gCurrentArea == AREA_TARGETPRACTICE1)
		item = townTable[item];
	else
		item = swampTable[item];
	

	gNewObjectDefinition.moveCall 	= MoveOrb;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	gNewObjectDefinition.slot 		= FENCE_SLOT-1;
	gNewObjectDefinition.coord.x	= RandomFloat2() * 900.0f;
	gNewObjectDefinition.coord.y	= -100;
	gNewObjectDefinition.coord.z	= RandomFloat2() * 500.0f;
	gNewObjectDefinition.flags 		= 0;

	switch(item)
	{
				/* COIN ORB */
				
		case	WHAT_COINORB:
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
				gNewObjectDefinition.type 		= PRACTICE_ObjType_Orb_Points;
				gNewObjectDefinition.scale 		= 6.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
				newObj->What = WHAT_COINORB;
				break;

				/* TIME ORB */
				
		case	WHAT_TIMEORB:
//				if (gTargetPracticeTimer > 90.0f)					// don't give more time orbs if we're already loaded
//					return;
		
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
				gNewObjectDefinition.type 		= PRACTICE_ObjType_Orb_Time;
				gNewObjectDefinition.scale 		= 6.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
				newObj->What = WHAT_TIMEORB;
				break;

				/* TNT */
				
		case	WHAT_TNT:
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
				gNewObjectDefinition.type 		= PRACTICE_ObjType_Orb_TNT;
				gNewObjectDefinition.scale 		= 6.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
				newObj->What = WHAT_TNT;
				break;


				/* DEATH SKULL */
				
		case	WHAT_SKULL:
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
				gNewObjectDefinition.type 		= PRACTICE_ObjType_DeathSkull;
				gNewObjectDefinition.scale 		= 3.5f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
				newObj->What = WHAT_SKULL;
				break;


				/* RAPID FIRE */
				
		case	WHAT_RAPIDFIRE:
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
				gNewObjectDefinition.type 		= PRACTICE_ObjType_Orb_RapidFire;
				gNewObjectDefinition.scale 		= 6.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
				newObj->What = WHAT_RAPIDFIRE;
				break;


				/* BOTTLE */
				
		case	WHAT_BOTTLE:
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
				gNewObjectDefinition.type 		= PRACTICE_ObjType_Bottle;
				gNewObjectDefinition.scale 		= 4.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
				newObj->What = WHAT_BOTTLE;
				break;


				/* PEPPER */
				
		case	WHAT_PEPPER:
				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
				gNewObjectDefinition.type 		= GLOBAL_ObjType_Boost;
				gNewObjectDefinition.scale 		= 8.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
				newObj->What = WHAT_PEPPER;
				break;


				/* KANGA COW */

		case	WHAT_KANGACOW:
				gNewObjectDefinition.type 		= SKELETON_TYPE_KANGACOW;
				gNewObjectDefinition.animNum 	= KANGA_ANIM_FLAIL;
				gNewObjectDefinition.scale 		= 6.0;
				newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
				newObj->What = WHAT_KANGACOW;

				if (MyRandomLong() & 1)
					PlayEffect_Parms3D(EFFECT_MOO1, &newObj->Coord, NORMAL_CHANNEL_RATE - (MyRandomLong() & 0x3fff), 4.0f);
				else
					PlayEffect_Parms3D(EFFECT_MOO2, &newObj->Coord, NORMAL_CHANNEL_RATE - (MyRandomLong() & 0x3fff), 4.0f);
				break;

				/* SHORTY */

		case	WHAT_SHORTY:
				gNewObjectDefinition.type 		= SKELETON_TYPE_SHORTY;
				gNewObjectDefinition.animNum 	= SHORTY_ANIM_TOSSED;		
				gNewObjectDefinition.scale 		= 5.0;
				newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
				newObj->What = WHAT_SHORTY;
				break;


				/* AMMO BOX */

		case	WHAT_AMMOBOX:
				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
				gNewObjectDefinition.type 		= GLOBAL_ObjType_AmmoBoxPOW;
				gNewObjectDefinition.scale 		= 3.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
				newObj->What = WHAT_AMMOBOX;
				break;
				
				
				/* FROG MAN */
				
		case	WHAT_FROGMAN:
				gNewObjectDefinition.type 		= SKELETON_TYPE_FROGMAN;
				gNewObjectDefinition.animNum 	= FROGMAN_ANIM_TARGETPRACTICE;			
				gNewObjectDefinition.scale 		= 4.0;
				newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
				newObj->What = WHAT_FROGMAN;
				break;

				/* TREMOR GHOST */
				
		case	WHAT_TREMORGHOST:
				gNewObjectDefinition.type 		= SKELETON_TYPE_TREMORGHOST;
				gNewObjectDefinition.animNum 	= TREMORGHOST_ANIM_FLAIL;			
				gNewObjectDefinition.scale 		= 4.0;
				newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
				newObj->What = WHAT_TREMORGHOST;
				newObj->ColorFilter.a = .5f;
				newObj->StatusBits |= STATUS_BIT_GLOW | STATUS_BIT_NOZWRITES | STATUS_BIT_DOUBLESIDED;
				break;

		default:
				GAME_ASSERT_MESSAGE(false, "Unsupported item");
	}


	newObj->Delta.y = 3500.0f + RandomFloat() * 1600.0f;
	newObj->Delta.x = RandomFloat2() * 1300.0f;
	newObj->Delta.z = RandomFloat2() * 800.0f;

	newObj->DeltaRot.x = RandomFloat2() * PI2;
	newObj->DeltaRot.y = RandomFloat2() * PI;

	newObj->CType = CTYPE_PICKABLE;
	newObj->HitByBulletCallback = OrbHitByBulletCallback;

}


/******************** MOVE ORB **************************/

static void MoveOrb(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if ((theNode->StatusBits & STATUS_BIT_ISCULLED) && (theNode->Coord.y < 0.0f))
	{
		DeleteObject(theNode);
		return;	
	}

	GetObjectInfo(theNode);
	
	gDelta.y -= 4000.0f * fps;
	
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;


	UpdateObject(theNode);


			/* SEE IF SPEWING BLOOD */
			
	if (!theNode->SpewBlood)
		return;
		
	theNode->BloodTimer -= fps;													// see if add fire
	if (theNode->BloodTimer <= 0.0f)
	{
		int		particleGroup,magicNum, i;
		NewParticleGroupDefType	groupDef;
		NewParticleDefType	newParticleDef;
		OGLPoint3D	p;
		OGLVector3D	d;
	
		theNode->BloodTimer += .02f;											// reset timer
		
		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;
		
		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num
			
			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 600;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 25.0f;
			groupDef.decayRate				=  -1.0f;
			groupDef.fadeRate				= .5;
			groupDef.particleTextureNum		= PARTICLE_SObjType_BloodSpat;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			OGLPoint3D_Transform(&theNode->SpecialPt[0], &theNode->BaseTransformMatrix, &p);
		
			for (i = 0; i < 3; i++)
			{
				d.x = RandomFloat2() * 30.0f;
				d.y = RandomFloat2() * 50.0f;
				d.z = RandomFloat2() * 30.0f;
			
				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2() * 5.0f;
				newParticleDef.alpha		= .8;		
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
	


}


/******************* ORB HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void OrbHitByBulletCallback(ObjNode *bullet, ObjNode *orb, const OGLPoint3D *impactPt)
{
OGLVector3D		splatVec;
OGLMatrix4x4	m;
int				i;

	bullet;
	
	switch(orb->What)
	{
		case	WHAT_COINORB:
				PlayEffect_Parms3D(EFFECT_CRATEEXPLODE, &orb->Coord, NORMAL_CHANNEL_RATE, 3.0f);			
				PlayEffect3D(EFFECT_COINSMASH, &orb->Coord);
				MakeFireExplosion(&orb->Coord);
				
				ExplodeGeometry(orb, 300.0, 0, 1, 1.0);
				
						/* MAKE COIN EXPLOSION */
						
				for (i = 0; i < 10; i++)
				{
					ObjNode *newObj;
				
					gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
					gNewObjectDefinition.type 		= GLOBAL_ObjType_PesoPOW;
					gNewObjectDefinition.slot 		= orb->Slot;
					gNewObjectDefinition.coord.x	= orb->Coord.x + RandomFloat2() * 30.0f;
					gNewObjectDefinition.coord.y	= orb->Coord.y + RandomFloat2() * 30.0f;
					gNewObjectDefinition.coord.z	= orb->Coord.z + RandomFloat2() * 30.0f;
					gNewObjectDefinition.flags 		= 0;
					gNewObjectDefinition.scale 		= .8f;
					gNewObjectDefinition.rot 		= RandomFloat() * PI2;
					gNewObjectDefinition.moveCall 	= MoveBonusCoin;
					newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				
					newObj->Rot.x = RandomFloat() * PI2;
					newObj->Rot.z = RandomFloat() * PI2;
				
					newObj->DeltaRot.x = RandomFloat2() * 11.0f;
					newObj->DeltaRot.y = RandomFloat2() * 11.0f;
					newObj->DeltaRot.z = RandomFloat2() * 11.0f;
					
					newObj->Delta.x = orb->Delta.x + RandomFloat2() * 800.0f;
					newObj->Delta.y = orb->Delta.y + RandomFloat2() * 800.0f;
					newObj->Delta.z = orb->Delta.z + RandomFloat2() * 800.0f;
				
				}
				
				DeleteObject(orb);					
				gScore += POINTS_ORB;				
				break;
				
		case	WHAT_TIMEORB:
				PlayEffect_Parms3D(EFFECT_CRATEEXPLODE, &orb->Coord, NORMAL_CHANNEL_RATE, 3.0f);			
				MakeFireExplosion(&orb->Coord);
				ExplodeGeometry(orb, 400.0, SHARD_MODE_FROMORIGIN, 1, .7);
				DeleteObject(orb);					
				gTargetPracticeTimer += 15.0f;
				break;
				

		case	WHAT_RAPIDFIRE:
				gRapidFireTimer += 8.0f;
				gRapidFireDelay = .1;
				PlayEffect_Parms3D(EFFECT_CRATEEXPLODE, &orb->Coord, NORMAL_CHANNEL_RATE, 3.0f);			
				MakeFireExplosion(&orb->Coord);
				ExplodeGeometry(orb, 400.0, SHARD_MODE_FROMORIGIN, 1, .7);
				DeleteObject(orb);					
				break;

		case	WHAT_SKULL:
				PlayEffect_Parms3D(EFFECT_CRATEEXPLODE, &orb->Coord, NORMAL_CHANNEL_RATE, 4.0f);			
				PlayEffect_Parms3D(EFFECT_DEATHSKULL, &orb->Coord, NORMAL_CHANNEL_RATE, 8.0f);			
				PlayEffect(EFFECT_YELP);			
				MakeFireExplosion(&orb->Coord);
				ExplodeGeometry(orb, 500.0, SHARD_MODE_FROMORIGIN, 1, .7);
				DeleteObject(orb);
				gPlayerInfo.shieldPower = 0;
				gPlayerIsDead = true;
				StartLevelCompletion(3.0);
				break;

		case	WHAT_TNT:
				PlayEffect_Parms3D(EFFECT_EXPLOSION, &orb->Coord, NORMAL_CHANNEL_RATE, 6.0f);			
				MakeSparkExplosion(orb->Coord.x, orb->Coord.y, orb->Coord.z, 250.0f, 30.0, PARTICLE_SObjType_YellowGlint, 600, .5);
				ExplodeGeometry(orb, 400.0, SHARD_MODE_FROMORIGIN, 1, .5);
				DeleteObject(orb);		
				
				gPlayerInfo.shieldPower -= 7.0f;
				if (gPlayerInfo.shieldPower <= 0.0f)
				{
					gPlayerIsDead = true;
					StartLevelCompletion(3.0);		
					PlayEffect(EFFECT_YELP);			
				}				
				StartDrunkeness();						// make wobble too
				gDrunkTimer *= .33f;						// but not for so long
				break;
				

		case	WHAT_BOTTLE:
				PlayEffect3D(EFFECT_GLASSBREAK, &orb->Coord);
				ExplodeGeometry(orb, 400.0, SHARD_MODE_FROMORIGIN, 1, .7);
				StartDrunkeness();
				DeleteObject(orb);					
				break;

		case	WHAT_PEPPER:
				gPepperCount--;
				if (gPepperCount < 0)
					gPepperCount = 0;
				ExplodeGeometry(orb, 400.0, SHARD_MODE_FROMORIGIN, 1, .7);
				PlayEffect_Parms3D(EFFECT_DUELKEYSDONE, &orb->Coord, NORMAL_CHANNEL_RATE, 4.0f);			
				DeleteObject(orb);					
				break;

		case	WHAT_AMMOBOX:
				PlayEffect_Parms3D(EFFECT_RELOAD, &orb->Coord, NORMAL_CHANNEL_RATE, 3.0f);			
				PlayEffect_Parms3D(EFFECT_CRATEEXPLODE, &orb->Coord, NORMAL_CHANNEL_RATE, 3.0f);			
				MakeFireExplosion(&orb->Coord);			
				ExplodeGeometry(orb, 400.0, SHARD_MODE_FROMORIGIN, 1, .3);
				DeleteObject(orb);	
				
				gPlayerInfo.ammoCount += 6;
				break;
				
				
		case	WHAT_TREMORGHOST:
				PlayEffect_Parms3D(EFFECT_GHOSTVAPORIZE, &orb->Coord, NORMAL_CHANNEL_RATE*3/2, 7.0f);			
				ExplodeGeometry(orb, 300.0, SHARD_MODE_FROMORIGIN, 1, 1.0);
				DeleteObject(orb);
				gScore += POINTS_ANIMAL;
				break;
				
				
		default:
		
						/* MAKE BULLET IMPACT SPLAT */
						
				splatVec.x = orb->Coord.x - gGameViewInfoPtr->cameraPlacement.cameraLocation.x;
				splatVec.y = orb->Coord.y - gGameViewInfoPtr->cameraPlacement.cameraLocation.y;
				splatVec.z = orb->Coord.z - gGameViewInfoPtr->cameraPlacement.cameraLocation.z;
				
				splatVec.x += RandomFloat2() * 1.1f;
				splatVec.y += RandomFloat2() * 1.1f;
				splatVec.z += RandomFloat2() * 1.1f;
				
				OGLVector3D_Normalize(&splatVec, &splatVec);
				DoBulletImpact(impactPt, &splatVec, 10.0);

				PlayEffect_Parms3D(EFFECT_BULLETHIT, impactPt, NORMAL_CHANNEL_RATE, 3.0f);			


						/* WHACK THE ROTS */
				
				orb->DeltaRot.x *= 2.0f;
				orb->DeltaRot.y *= 2.0f;
				orb->DeltaRot.z *= 5.0f;

				orb->Delta.z -= 500.0f;
				
				orb->Delta.x += RandomFloat2() * 3500.0f;
				orb->Delta.y += RandomFloat2() * 1500.0f;

						/***********************/
						/* START BLOOD SPEWING */
						/***********************/
						
						/* CALC OFFSET OF IMPACT PT ON ORB */						

				OGLMatrix4x4_Invert(&orb->BaseTransformMatrix, &m);
				OGLPoint3D_Transform(impactPt, &m, &orb->SpecialPt[0]);

				orb->SpewBlood = true;

				gScore += POINTS_ANIMAL;
	}	
}


/********************** MOVE BONUS COIN ******************************/

static void MoveBonusCoin(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF GONE */
			
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

	gDelta.y -= 2000.0f * fps;
	
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;

	UpdateObject(theNode);
}


#pragma mark -

/****************** START DRUNKENESS ************************/

static void StartDrunkeness(void)
{
	if (gDrunkTimer > 0.0f)			// if already drunk just bump up the timer
	{
		gDrunkTimer += 6.0f;
		return;
	}

	gDrunkTimer = 10.0f;
	gDrunkRadius = 0;

	gDrunkLookOrigin = gGameViewInfoPtr->cameraPlacement.pointOfInterest;

	gDrunkWobble.x = RandomFloat2() * PI2;
	gDrunkWobble.y = RandomFloat2() * PI2;

}


/******************* UPDATE DRUNKENESS ***************************/

static void UpdateDrunkeness(void)
{		
float	fps = gFramesPerSecondFrac;
float	offX,offY;
OGLPoint3D	lookAt;

	if (gDrunkTimer > 0.0f)
	{
		gDrunkTimer -= fps;

			/*************************/
			/* ACCELL INTO/OUT DRUNK */
			/*************************/
			
		if (gDrunkTimer > 1.1f)				// get more drunk
		{
			gDrunkRadius += fps;			
			if (gDrunkRadius > 1.0f)		// maxxed out
				gDrunkRadius = 1.0f;
		}
		else
		{
			gDrunkRadius -= fps;			// get less drunk
			if (gDrunkRadius <= 0.0f)		// maxxed out
			{
				gDrunkRadius = 0.0f;
				gDrunkTimer = 0;
			}
		}	
		
			/*************************/
			/* UPDATE CAMERA OFFSETS */
			/*************************/
		
		gDrunkWobble.x += fps * 4.0f;
		gDrunkWobble.y += fps * 6.0f;
		
		offX = sin(gDrunkWobble.x) * gDrunkRadius * 900.0f;
		offY = cos(gDrunkWobble.y) * gDrunkRadius * 500.0f;
		
		lookAt.x = offX + gDrunkLookOrigin.x;
		lookAt.y = offY + gDrunkLookOrigin.y;
		lookAt.z = gDrunkLookOrigin.z;
		
		OGL_UpdateCameraFromTo(gGameViewInfoPtr, nil, &lookAt);
	}
}



















