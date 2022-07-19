/****************************/
/*   	PLAYER_WEAPONS.C    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <textutils.h>

#include "game.h"
#include "globals.h"
#include "objects.h"
#include "misc.h"
#include "collision.h"
#include "player.h"
#include "sound2.h"
#include "main.h"
#include "input.h"
#include "mobjtypes.h"
#include "skeletonanim.h"
#include "effects.h"
#include "3dmath.h"
#include "terrain.h"
#include "sparkle.h"
#include "skeletonJoints.h"
#include "triggers.h"
#include "infobar.h"
#include "skeletonobj.h"
#include "sobjtypes.h"
#include "items.h"
#include "sprites.h"
#include "shards.h"
#include "fences.h"
#include "enemy.h"
#include "vaportrails.h"

extern	float			gFramesPerSecondFrac,gFramesPerSecond,gPlayerToCameraAngle,gGlobalTransparency;
extern	OGLPoint3D		gCoord;
extern	OGLVector3D		gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D		gRecentTerrainNormal;
extern	short			gPlayerMultiPassCount;
extern	PlayerInfoType	gPlayerInfo;
extern	u_long			gAutoFadeStatusBits;
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	SparkleType	gSparkles[];
extern	CollisionRec	gCollisionList[];
extern	short			gNumCollisions;
extern	ObjNode			*gFirstNodePtr,*gTargetPickup;
extern	SpriteType	*gSpriteGroupList[];
extern	float			gTimeSinceLastShoot,gGravity,gCameraUserRotY;
extern	Boolean			gForceCameraAlignment;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void StartPunch(ObjNode *player);
static void ShootWeapon(ObjNode *theNode);
static void MoveStunPulse(ObjNode *theNode);
static void ShootStunPulse(ObjNode *theNode, OGLPoint3D *where, OGLVector3D *aim);
static void MoveStunPulseRipple(ObjNode *theNode);
static Boolean DoWeaponCollisionDetect(ObjNode *theNode);
static void	WeaponAutoTarget(OGLPoint3D *where, OGLVector3D *aim);
static void MoveDisposedWeapon(ObjNode *theNode);
static void ChangeWeapons(short startIndex);
static void StartSuperNovaCharge(ObjNode *player);
static void DrawSuperNovaCharge(ObjNode *theNode, const OGLSetupOutputType *setupInfo);
static void ShootFreezeGun(ObjNode *theNode, OGLPoint3D *where, OGLVector3D *aim);
static void MoveFreezeBullet(ObjNode *theNode);
static void ExplodeFreeze(ObjNode *bullet);
static void CalcAntennaDischargePoint(OGLPoint3D *p);


static void TossGrowthVial(void);
static void MoveTossedGrowthVial(ObjNode *vial);


static void StartThrowDart(ObjNode *player);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_AUTO_AIM_DIST	5000.0f

#define	STUN_PULSE_SPEED	3500.0f

#define	FREEZE_BULLET_SPEED	1500.0f
#define FLAME_BULLET_SPEED	1700.0f
#define	FLARE_BULLET_SPEED	1000.0f
#define	DART_SPEED			900.0f

typedef struct
{
	Boolean		active;
	float		dist;
	OGLPoint3D	where;
	ObjNode		*who;
}SuperNovaTargetType;


/*********************/
/*    VARIABLES      */
/*********************/

const OGLPoint3D 	gPlayerMuzzleTipOff = {-1,-70,-22};		//  offset to muzzle tip
const OGLVector3D	gPlayerMuzzleTipAim = {0,-1,-.149};

static MOVertexArrayData	gNovaChargeMesh;

const OGLPoint3D antennaLOff = {-40,45,12};
const OGLPoint3D antennaROff = {40,45,12};

static SuperNovaTargetType	gSuperNovaTargets[MAX_SUPERNOVA_DISCHARGES];
float		gDischargeTimer;


#define	DelayToSeek		SpecialF[0]									// timer for seeking flares

static const Boolean gWeaponIsGun[NUM_WEAPON_TYPES] =
{
	true,			// WEAPON_TYPE_STUNPULSE
	true,			// WEAPON_TYPE_FREEZE
	true,			// WEAPON_TYPE_FLAME
	true,			// WEAPON_TYPE_GROWTH
	true,			// WEAPON_TYPE_FLARE

	false,			// WEAPON_TYPE_FIST
	false,			// WEAPON_TYPE_SUPERNOVA
	false,			// WEAPON_TYPE_DART
};

/*********************** INIT WEAPON INVENTORY *************************/

void Initinventory(void)
{
int	i;

	for (i = 0; i < MAX_INVENTORY_SLOTS; i++)							// init weapon inventory
	{
		gPlayerInfo.inventory[i].type = NO_INVENTORY_HERE;		
		gPlayerInfo.inventory[i].quantity = 0;		
	}

			/* ALWAYS HAS FIST AS WEAPON */
			
	gPlayerInfo.inventory[0].type = WEAPON_TYPE_FIST;
	gPlayerInfo.inventory[0].quantity = 99;
	gPlayerInfo.currentWeaponType = WEAPON_TYPE_FIST;	
	gPlayerInfo.holdingGun = false;
}


#pragma mark -


/******************* CHECK POWERUP CONTROLS ************************/
//
//

void CheckPOWControls(ObjNode *theNode)
{
int	i;

	if (gPlayerMultiPassCount > 0)							// if doing multipass and on secondary passes, then dont check "New" keys.
		return;
		
		
		/* SEE IF SHOOT GUN */
		
	if (GetNewKeyState(kKey_Shoot))							// see if user pressed the key
	{
		ShootWeapon(theNode);	
	}
	

		/* SEE IF CHANGE WEAPON */
		
	else
	if (GetNewKeyState(kKey_NextWeapon))			
	{
		i = FindinventoryIndex(gPlayerInfo.currentWeaponType);	// get current into inventory list
		i++;															// start searching on next slot
		if (i >= MAX_INVENTORY_SLOTS)
			i = 0;
		ChangeWeapons(i);
	}	

}


/******************* START PUNCH *******************/

static void StartPunch(ObjNode *player)
{
	player->PunchCanHurt = false;
//	MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_PUNCH, 8);
	
//	PlayEffect3D(EFFECT_SERVO, &gCoord);
}




/******************** SHOOT WEAPON *************************/

static void ShootWeapon(ObjNode *theNode)
{
OGLPoint3D		muzzleCoord;
OGLVector3D		muzzleVector;
OGLMatrix4x4	m;

	gTimeSinceLastShoot = 0;
	gCameraUserRotY = 0;											// reset user rot see we can see where we're shooting
	gForceCameraAlignment = true;

		/* CALC COORD & VECTOR OF MUZZLE */

	if (gPlayerInfo.holdingGun)
	{
		FindJointFullMatrix(theNode,PLAYER_JOINT_LEFTHAND,&m);
		OGLPoint3D_Transform(&gPlayerMuzzleTipOff, &m, &muzzleCoord);
		OGLVector3D_Transform(&gPlayerMuzzleTipAim, &m, &muzzleVector);
	}

			/* SHOOT APPROPRIATE WEAPON */

	switch(gPlayerInfo.currentWeaponType)
	{
//		case	WEAPON_TYPE_STUNPULSE:
//				ShootStunPulse(theNode, &muzzleCoord, &muzzleVector);
//				DecWeaponQuantity(WEAPON_TYPE_STUNPULSE);
//				break;
				
		
	}
}




#pragma mark -


/******************* FIND WEAPON INVENTORY INDEX **************************/

short FindinventoryIndex(short weaponType)
{
short	i;

	for (i = 0; i < MAX_INVENTORY_SLOTS; i++)
	{
		if (gPlayerInfo.inventory[i].type == weaponType)
			if (gPlayerInfo.inventory[i].quantity > 0)
				return(i);
	}

	return(NO_INVENTORY_HERE);
}



/******************* DECREMENT WEAPON QUANTITY ***********************/

void DecWeaponQuantity(short weaponType)
{
ObjNode	*newObj;
short	i,type;
static const short weaponToModel[] = 
{
	GLOBAL_ObjType_StunPulseGun,			// WEAPON_TYPE_STUNPULSE
	GLOBAL_ObjType_FreezeGun,				// WEAPON_TYPE_FREEZE
	GLOBAL_ObjType_FlameGun,				// WEAPON_TYPE_FLAME
	-1,										// WEAPON_TYPE_GROWTH
	GLOBAL_ObjType_FlareGun,				// WEAPON_TYPE_FLARE
};

	i = FindinventoryIndex(weaponType);					// get index into inventory list
	if (i == NO_INVENTORY_HERE)
		DoFatalAlert("\pDecWeaponQuantity: weapon not in inventory");

	type = gPlayerInfo.currentWeaponType;						// get current weapon type

	gPlayerInfo.inventory[i].quantity--;					// decrement the inventory


			/****************/
			/* TOSS THE GUN */
			/****************/
			
	if (gPlayerInfo.inventory[i].quantity <= 0)			// see if need to dispose of it
	{
		gPlayerInfo.inventory[i].quantity = 0;
		gPlayerInfo.inventory[i].type = NO_INVENTORY_HERE;
	
		if (gPlayerInfo.holdingGun)
		{
			gPlayerInfo.wasHoldingGun = true;
			
					/* MAKE IT FLY AWAY */
					
			if (type == WEAPON_TYPE_GROWTH)			// special case the growth powerup
				TossGrowthVial();			
			else
			{
				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
				gNewObjectDefinition.type 		= weaponToModel[type];
				gNewObjectDefinition.coord		= gPlayerInfo.leftHandObj->Coord;
				gNewObjectDefinition.flags 		= 0;
				gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1;
				gNewObjectDefinition.moveCall 	= MoveDisposedWeapon;
				gNewObjectDefinition.rot 		= gPlayerInfo.objNode->Rot.y;
				gNewObjectDefinition.scale 		= gPlayerInfo.leftHandObj->Scale.x;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);		
				
				newObj->Rot.x = -PI/2;
				newObj->Delta.y = 600.0f;
				newObj->Delta.x = RandomFloat2() * 200.0f;
				newObj->Delta.z = RandomFloat2() * 200.0f;
			}

			gPlayerInfo.holdingGun = false;
		}
		else
			gPlayerInfo.wasHoldingGun = false;
		
			/* CHANGE TO NEXT WEAPON IF ANY */
			
		gPlayerInfo.currentWeaponType = NO_INVENTORY_HERE;			// clear this so change will not try to do any funky anims	
		ChangeWeapons(i);
		
		if ((gPlayerInfo.currentWeaponType == WEAPON_TYPE_SUPERNOVA) &&		// dont allow automatic switch to supernova since that screws up the player
			(gPlayerInfo.wasHoldingGun))
		{
			gPlayerInfo.currentWeaponType = WEAPON_TYPE_FIST;				// just go to punching in these cases		
		}		
	}				
}

/*********************** TOSS GROWTH VIAL ************************/

static void TossGrowthVial(void)
{
float	r;
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= JUNGLE_ObjType_GrowthPOW;
	gNewObjectDefinition.coord		= gPlayerInfo.leftHandObj->Coord;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= MoveTossedGrowthVial;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .25;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);		
	
	r = gPlayerInfo.objNode->Rot.y;
	newObj->Delta.y = 600.0f;
	newObj->Delta.x = -sin(r) * 600.0f;
	newObj->Delta.z = -cos(r) * 600.0f;
	
	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
}


/***************** MOVE TOSSED GROWTH VIAL *********************/

static void MoveTossedGrowthVial(ObjNode *vial)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(vial);

	gDelta.y -= gGravity * .5f * fps;
	gCoord.x += gDelta.x * fps;	
	gCoord.y += gDelta.y * fps;	
	gCoord.z += gDelta.z * fps;	

	if (HandleCollisions(vial, CTYPE_MISC | CTYPE_TERRAIN | CTYPE_FENCE, 0))
	{
		ExplodeGeometry(vial, 200, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .9);
		PlayEffect3D(EFFECT_SHATTER, &vial->Coord);
		DeleteObject(vial);
		return;
	}

	vial->Rot.x += PI2 * fps;
	vial->Rot.z += 9.0f * fps;

	UpdateObject(vial);
}


/******************* INCREMENT WEAPON QUANTITY ***********************/

void IncWeaponQuantity(short weaponType, short amount)
{
short	i;


			/* FIND EXISTING WEAPON IN INVENTORY */
			
	i = FindinventoryIndex(weaponType);					// get index into inventory list
	
	
			/* ADD NEW WEAPON TO INVENTORY */
				
	if (i == NO_INVENTORY_HERE)
	{
		for (i = 0; i < MAX_INVENTORY_SLOTS; i++)
		{
			if (gPlayerInfo.inventory[i].type == NO_INVENTORY_HERE)		// see if found a blank slot
			{
				gPlayerInfo.inventory[i].type = weaponType;
				gPlayerInfo.inventory[i].quantity = amount;
				return;
			}	
		}
	}

	gPlayerInfo.inventory[i].quantity += amount;					// inc current inventory
	if (gPlayerInfo.inventory[i].quantity > 99)					// max @ 99
		gPlayerInfo.inventory[i].quantity = 99;
}



/****************** MOVED DISPOSED WEAPON *******************/
//
// When player runs out of ammo and the gun gets thrown.
//


static void MoveDisposedWeapon(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	theNode->Rot.x += fps * 3.0f;

	gDelta.y -= 2000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		DeleteObject(theNode);
		return;	
	}

	UpdateObject(theNode);
}






















