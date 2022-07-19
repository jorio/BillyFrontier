/*******************************/
/*   		BILLY.C			   */
/* (c)2002 Pangea Software     */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <textutils.h>

#include "game.h"
#include "3dmath.h"
#include "bones.h"
#include "infobar.h"

extern	OGLBoundingBox			gWaterBBox[];
extern	ObjNode					*gFirstNodePtr;
extern	float					gFramesPerSecondFrac,gFramesPerSecond,gCameraLookAtYOff,gGlobalTransparency;
extern	OGLPoint3D				gCoord;
extern	OGLVector3D				gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	short					gNumCollisions;
extern	CollisionRec			gCollisionList[];
extern	PrefsType				gGamePrefs;
extern	OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLVector3D				gRecentTerrainNormal;
extern	OGLMatrix4x4			gWorldToWindowMatrix;
extern	PlayerInfoType			gPlayerInfo;
extern	Boolean					gFreezeCameraFromXZ;
extern	int						gScratch;
extern	ParticleGroupType		*gParticleGroups[];
extern	SpriteType			*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	float				gPlayerToCameraAngle,gGravity,gMinHeightOffGround,gCameraDistFromMe;
extern	WaterDefType			*gWaterList;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLColorRGB			gGlobalColorFilter;
extern	float					gGammaFadePercent;
extern	u_long					gAutoFadeStatusBits;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void CheckPlayerActionControls(ObjNode *theNode);
static void DoPlayerFrictionAndGravity_Terrain(ObjNode *theNode, float friction);

static float CalcWalkAnimSpeed(ObjNode *theNode);
static void	BillyPutLeftGunInHolster(ObjNode *leftGun);
static void	BillyPutRightGunInHolster(ObjNode *rightGun);
static void	PutHatOnHead(ObjNode *hat);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	PLAYER_WATER_FRICTION	200.0f
#define	PLAYER_AIR_FRICTION		800.0f	//400.0f
#define	PLAYER_LANDING_FRICTION	600.0f
#define	PLAYER_DEFAULT_FRICTION	4000.0f	//2500.0f
#define	PLAYER_HEAVY_FRICTION	5000.0f


#define	JUMP_DELTA				2000.0f
#define	HOP_DELTA				1300.0f

#define	DELTA_SUBDIV			8.0f				// smaller == more subdivisions per frame

#define	CONTROL_SENSITIVITY		3000.0f //2200.0f
#define	CONTROL_SENSITIVITY_AIR	5000.0f

#define	WALK_STAND_THRESHOLD	0.3f

#define	KEY_THRUST				4000.0f

/*********************/
/*    VARIABLES      */
/*********************/

#define	PickupNow			Flag[0]
#define	KickHitNow			Flag[0]



float			gPlayerBottomOff = 0;

short			gPlayerMultiPassCount = 0;


//
// In order to let the player move faster than the max speed, we use a current and target value.
// the target is what we want the max to normally be, and the current is what it currently is.
// So, when the player needs to go faster, we modify Current and then slowly decay it back to Target.
//

float	gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
float	gCurrentMaxSpeed = PLAYER_NORMAL_MAX_SPEED;


/*************************** CREATE BILLY ****************************/
//
// Creates an ObjNode for the player 
//
// INPUT:	
//			where = floor coord where to init the player.
//			rotY = rotation to assign to player if oldObj is nil.
//

ObjNode *CreateBilly(OGLPoint3D *where, float rotY)
{
ObjNode	*player, *leftGun, *rightGun, *hat;

		/***********************/
		/* MAKE  SKELETON BODY */
		/***********************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_BILLY;	
	gNewObjectDefinition.animNum	= PLAYER_ANIM_STAND;	
	gNewObjectDefinition.coord.x 	= where->x;
	gNewObjectDefinition.coord.z 	= where->z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(where->x,where->z);	
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall	= nil;
	gNewObjectDefinition.rot 		= rotY;
	gNewObjectDefinition.scale 		= PLAYER_DEFAULT_SCALE;

	player = MakeNewSkeletonObject(&gNewObjectDefinition);

	SetPlayerStandAnim(player, 10);						// just make sure we're standing appropriately

	player->Coord.y -= player->BBox.min.y;				// offset y so foot is on ground
	UpdateObjectTransforms(player);


				/* SET COLLISION INFO */

	player->CType = CTYPE_PLAYER | CTYPE_HITENEMYBULLET;
	player->CBits = CBITS_ALLSOLID;

	SetObjectCollisionBounds(player, player->BBox.max.y, gPlayerBottomOff = player->BBox.min.y, -18, 18, 18, -18);
	
	player->Health = 1.0f;


		/********************/
		/* MAKE ATTACHMENTS */
		/********************/

		/* LEFT GUN */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_BillyGun;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	leftGun = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	leftGun->Side = SIDE_LEFT;
	BillyPutLeftGunInHolster(leftGun);
	

		/* RIGHT GUN */
			
	gNewObjectDefinition.type 		= GLOBAL_ObjType_BillyGun;
	rightGun = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	rightGun->Side = SIDE_RIGHT;
	BillyPutRightGunInHolster(rightGun);
	

			/* HAT */
			
	gNewObjectDefinition.type 		= GLOBAL_ObjType_BillyHat;
	hat = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	PutHatOnHead(hat);
	


			/* CHAIN THEM */
			
	player->ChainNode = leftGun;
	leftGun->ChainNode = rightGun;
	rightGun->ChainNode = hat;


		/*******************/
		/* SET OTHER STUFF */
		/*******************/

	gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
	gCurrentMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
						
	gPlayerInfo.objNode 	= player;
	gPlayerInfo.coord		= player->Coord;		
	

	AttachShadowToObject(player, 0, DEFAULT_PLAYER_SHADOW_SCALE,DEFAULT_PLAYER_SHADOW_SCALE, true);


//	BillyPutGunsInHands(player);	//-------

	return(player);			
}



/******************* CALC WALK ANIM SPEED **********************/

static float CalcWalkAnimSpeed(ObjNode *theNode)
{
float	speed = CalcQuickDistance(0,0,gDelta.x, gDelta.z) * .007f;
	theNode;
	
	return(speed);
}




#pragma mark -

/************************ UPDATE BILLY ***************************/

void UpdateBilly(ObjNode *theNode)
{
const float fps = gFramesPerSecondFrac;


			/* UPDATE COLLISION BOX TOP */
			
	theNode->TopOff = theNode->BBox.max.y;
	

			/*****************************/
			/* UPDATE FINAL SPEED VALUES */
			/*****************************/
		
	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);
	

			/* UPDATE CURRENT MAX SPEED */
		
	if (theNode->Speed2D < gTargetMaxSpeed)					// if we're less than the target, then just reset current to target
		gCurrentMaxSpeed = gTargetMaxSpeed;
	else
	if (gCurrentMaxSpeed > gTargetMaxSpeed)					// see if in overdrive, so readjust currnet
	{
		if (theNode->Speed2D < gCurrentMaxSpeed)			// we're slower than Current, so adjust current down to us
		{
			gCurrentMaxSpeed = theNode->Speed2D;
		}
	}	
	
	theNode->Speed3D = CalcVectorLength(&gDelta);	


		/* CALC Y-ROTATION BASED ON DELTA VECTOR */
		//
		// We want the player to continue aiming in the direction of thrust when
		// pushed up against a solid object.  So, if going slow then turn toward accel vector
		// otherwise, turn toward actual delta.  Turning at the accel vector will cause
		// some minor moon-walking.
		//
	
	switch(theNode->Skeleton->AnimNum)
	{
		default:			
				if (theNode->Speed2D > (PLAYER_NORMAL_MAX_SPEED/4))
					TurnObjectTowardTarget(theNode, &gCoord, gCoord.x+gDelta.x, gCoord.z+gDelta.z, theNode->Speed2D*.02f, false);
				else														// if going really slow then aim in acceleration direction instead
					TurnObjectTowardTarget(theNode, &gCoord, gCoord.x+theNode->AccelVector.x, gCoord.z+theNode->AccelVector.y, theNode->Speed2D*.02f, false);
	}


		/* UPDATE OBJECT AS LONG AS NOT BEING MATRIX CONTROLLED */
			
	switch(theNode->Skeleton->AnimNum)
	{
//		case	PLAYER_ANIM_GRABBED:
//		case	PLAYER_ANIM_GRABBED2:
//		case	PLAYER_ANIM_GRABBEDBYSTRONGMAN:
//		case	PLAYER_ANIM_ROCKETSLED:
//				break;

		default:
				UpdateObject(theNode);
	}
	
	gPlayerInfo.coord = gCoord;				// update player coord		
	
	
				
	
		/* CHECK INV TIMER */
		
	gPlayerInfo.invincibilityTimer -= fps;
	
	
	
			/**********************/
			/* UPDATE ATTACHMENTS */
			/**********************/
			
	UpdateBillyAttachments(theNode);
	
		
	
		/* UPDATE SHIELD */
		
	UpdatePlayerShield();	
}



/************************ DO FRICTION & GRAVITY ****************************/
//
// Applies friction to the gDeltas
//

static void DoPlayerFrictionAndGravity_Terrain(ObjNode *theNode, float friction)
{
OGLVector2D	v;
float	x,z,fps;

	fps = gFramesPerSecondFrac;

			/**************/
			/* DO GRAVITY */
			/**************/
			
	gDelta.y -= gGravity*fps;					// add gravity

	if (gDelta.y < 0.0f)							// if falling, keep dy at least -1.0 to avoid collision jitter on platforms
		if (gDelta.y > (-20.0f * fps))
			gDelta.y = (-20.0f * fps);


			/***************/
			/* DO FRICTION */
			/***************/
			//
			// Dont do friction if player is pressing controls
			//

	if (gPlayerInfo.analogControlX || gPlayerInfo.analogControlZ)	// if there is any player control then no friction
		return;

			
	friction *= fps;							// adjust friction

	v.x = gDelta.x;
	v.y = gDelta.z;
	
	OGLVector2D_Normalize(&v, &v);				// get normalized motion vector
	x = -v.x * friction;						// make counter-motion vector
	z = -v.y * friction;
	
	if (gDelta.x < 0.0f)						// decelerate against vector
	{
		gDelta.x += x;
		if (gDelta.x > 0.0f)					// see if sign changed
			gDelta.x = 0;											
	}
	else
	if (gDelta.x > 0.0f)									
	{
		gDelta.x += x;
		if (gDelta.x < 0.0f)								
			gDelta.x = 0;											
	}
	
	if (gDelta.z < 0.0f)								
	{
		gDelta.z += z;
		if (gDelta.z > 0.0f)								
			gDelta.z = 0;											
	}
	else
	if (gDelta.z > 0.0f)									
	{
		gDelta.z += z;
		if (gDelta.z < 0.0f)								
			gDelta.z = 0;											
	}

	if ((gDelta.x == 0.0f) && (gDelta.z == 0.0f))
	{
		theNode->Speed2D = 0;
	}


}



/******************** DO SKIP COLLISION DETECT **************************/
//
// Standard collision handler for player
//
// OUTPUT: true = disabled/killed
//

Boolean DoBillyCollisionDetect(ObjNode *theNode, Boolean useBBoxForTerrain)
{
short		i;
ObjNode		*hitObj;
u_char		sides;
float		distToFloor, terrainY, fps = gFramesPerSecondFrac;
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

//	sides = HandleCollisions(theNode, PLAYER_COLLISION_CTYPE, -.3);
	sides = HandleCollisions(theNode, 0, -.3);

			/* SCAN FOR INTERESTING STUFF */
						
	for (i=0; i < gNumCollisions; i++)						
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision
			
			if (hitObj->CType == INVALID_NODE_FLAG)				// see if has since become invalid
				continue;
		
							
					
			/* CHECK FOR TOTALLY IMPENETRABLE */
			
			if (hitObj->CBits & CBITS_IMPENETRABLE2)
			{
				if (!(gCollisionList[i].sides & SIDE_BITS_BOTTOM))	// dont do this if we landed on top of it
				{
					gCoord.x = theNode->OldCoord.x;					// dont take any chances, just move back to original safe place
					gCoord.z = theNode->OldCoord.z;
				}
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




#pragma mark -


/**************** CHECK PLAYER ACTION CONTROLS ***************/
//
// Checks for special action controls 
//
// INPUT:	theNode = the node of the player
//

static void CheckPlayerActionControls(ObjNode *theNode)
{
	theNode;
			

}





#pragma mark -

/************************ UPDATE BILLY ATTACHMENTS ********************************/

void UpdateBillyAttachments(ObjNode *player)
{	
ObjNode 		*leftGun, *rightGun, *hat;
OGLMatrix4x4	m,mst,rm,m2;

			/* GET ATTACHMENTS */
			
	leftGun = player->ChainNode;
	rightGun = leftGun->ChainNode;
	hat = rightGun->ChainNode;


			/*******************/
			/* UPDATE LEFT GUN */
			/*******************/
	
	if (leftGun->GunLocation == GUN_LOCATION_HOLSTER)							// attached to hip or hand?
		FindJointFullMatrix(player, PLAYER_JOINT_LEFTHIP, &m);					
	else
		FindJointFullMatrix(player, PLAYER_JOINT_LEFTHAND, &m);					
	
	CalcGunMatrixFromJointMatrix(leftGun, &m, &leftGun->BaseTransformMatrix);		
	SetObjectTransformMatrix(leftGun);

	leftGun->Coord.x = leftGun->BaseTransformMatrix.value[M03];					// extract coord from matrix
	leftGun->Coord.y = leftGun->BaseTransformMatrix.value[M13];
	leftGun->Coord.z = leftGun->BaseTransformMatrix.value[M23];


			/********************/
			/* UPDATE RIGHT GUN */
			/********************/
		
	if (rightGun->GunLocation == GUN_LOCATION_HOLSTER)							// attached to hip or hand?
		FindJointFullMatrix(player, PLAYER_JOINT_RIGHTHIP, &m);					
	else
		FindJointFullMatrix(player, PLAYER_JOINT_RIGHTHAND, &m);					
		
	CalcGunMatrixFromJointMatrix(rightGun, &m, &rightGun->BaseTransformMatrix);		
	SetObjectTransformMatrix(rightGun);

	rightGun->Coord.x = rightGun->BaseTransformMatrix.value[M03];				// extract coord from matrix
	rightGun->Coord.y = rightGun->BaseTransformMatrix.value[M13];
	rightGun->Coord.z = rightGun->BaseTransformMatrix.value[M23];


			/**************/
			/* UPDATE HAT */
			/**************/
		
	OGLMatrix4x4_SetTranslate(&mst, hat->HoldOffset.x, hat->HoldOffset.y, hat->HoldOffset.z);
	OGLMatrix4x4_SetRotate_XYZ(&rm, hat->HoldRot.x, hat->HoldRot.y, hat->HoldRot.z);
	OGLMatrix4x4_Multiply(&rm, &mst, &m2);
			
	FindJointFullMatrix(player, PLAYER_JOINT_HEAD, &m);					
	
	OGLMatrix4x4_Multiply(&m2, &m, &hat->BaseTransformMatrix);
	SetObjectTransformMatrix(hat);

	hat->Coord.x = hat->BaseTransformMatrix.value[M03];					// extract coord from matrix
	hat->Coord.y = hat->BaseTransformMatrix.value[M13];
	hat->Coord.z = hat->BaseTransformMatrix.value[M23];


	
}


/************************* PUT LEFT GUN IN HOLSTER *************************/

static void	BillyPutLeftGunInHolster(ObjNode *leftGun)
{
	leftGun->HoldOffset.x = -10;
	leftGun->HoldOffset.y = -9;
	leftGun->HoldOffset.z = 2;

	leftGun->HoldRot.x = .0;
	leftGun->HoldRot.y = 0;
	leftGun->HoldRot.z = .14;
	
	leftGun->GunLocation = GUN_LOCATION_HOLSTER;
	
}


/************************* PUT RIGHT GUN IN HOLSTER *************************/

static void	BillyPutRightGunInHolster(ObjNode *rightGun)
{
	rightGun->HoldOffset.x = 11;
	rightGun->HoldOffset.y = -9;
	rightGun->HoldOffset.z = 3;

	rightGun->HoldRot.x = .0;
	rightGun->HoldRot.y = 0;
	rightGun->HoldRot.z = -.14;

	rightGun->GunLocation = GUN_LOCATION_HOLSTER;
}


/******************** CALC GUN MATRIX FROM JOINT MATRIX **************************/

void CalcGunMatrixFromJointMatrix(ObjNode *gun, OGLMatrix4x4 *jointMatrix, OGLMatrix4x4 *gunMatrix)
{
OGLMatrix4x4	mst,rm,m2;

	OGLMatrix4x4_SetTranslate(&mst, gun->HoldOffset.x, gun->HoldOffset.y, gun->HoldOffset.z);
	OGLMatrix4x4_SetRotate_XYZ(&rm, gun->HoldRot.x, gun->HoldRot.y, gun->HoldRot.z);
	OGLMatrix4x4_Multiply(&rm, &mst, &m2);
				
	OGLMatrix4x4_Multiply(&m2, jointMatrix, gunMatrix);
}


/************************* PUT HAT ON HEAD *************************/

static void	PutHatOnHead(ObjNode *hat)
{
	hat->HoldOffset.x = 0;
	hat->HoldOffset.y = 20;
	hat->HoldOffset.z = -2;

	hat->HoldRot.x = .2;
	hat->HoldRot.y = 0;
	hat->HoldRot.z = 0;
}


/******************* BILLY PUT GUNS IN HANDS **********************/

void BillyPutGunsInHands(ObjNode *player)
{
ObjNode 		*leftGun, *rightGun;

			/* GET ATTACHMENTS */
			
	leftGun = player->ChainNode;
	rightGun = leftGun->ChainNode;


			/* IN HANDS */
				
	rightGun->GunLocation = GUN_LOCATION_HAND;
	leftGun->GunLocation = GUN_LOCATION_HAND;


		/* RIGHT GUN ALIGNMENT */

	rightGun->HoldOffset.x = 8;
	rightGun->HoldOffset.y = -22;
	rightGun->HoldOffset.z = -5;

	rightGun->HoldRot.x = 0;
	rightGun->HoldRot.y = 0;
	rightGun->HoldRot.z = .4;


		/* LEFT GUN ALIGNMENT */

	leftGun->HoldOffset.x = -8;
	leftGun->HoldOffset.y = -22;
	leftGun->HoldOffset.z = -5;

	leftGun->HoldRot.x = 0;
	leftGun->HoldRot.y = 0;
	leftGun->HoldRot.z = -.4;
}












