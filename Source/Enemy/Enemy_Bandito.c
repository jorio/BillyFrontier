/****************************/
/*   ENEMY: BANDITO.C	*/
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

static void	BanditoPutgunInHolster(ObjNode *gun);
static void	BanditoPutHatOnBack(ObjNode *hat);

static void MoveShootoutBandito(ObjNode *enemy);
static void MoveShootoutBandito_DuckDown(ObjNode *enemy);
static void ShootoutBanditoHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt);
static void FireBanditoShootoutGun(ObjNode *enemy);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_BANDITOS				8

#define	BANDITO_SCALE				1.0f


/*********************/
/*    VARIABLES      */
/*********************/



/************************* MAKE BANDITO ****************************/

ObjNode *MakeBandito(float x, float z, float rot, short animNum, movecall_t moveCall, Boolean gunInHand)
{
ObjNode	*newObj, *gun, *hat;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_BANDITO,animNum, x,z, BANDITO_SCALE, rot, moveCall,
								gAutoFadeStatusBits);
	


				/* SET BETTER INFO */

	newObj->Kind 		= ENEMY_KIND_BANDITO;
	newObj->CType		|= CTYPE_PICKABLE;
	
				/* SET COLLISION INFO */
								
	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	




		/********************/
		/* MAKE ATTACHMENTS */
		/********************/

			/* GUN */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_BanditoGun;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gun = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	gun->Side = SIDE_RIGHT;
	BanditoPutgunInHolster(gun);
	

			/* HAT */
			
	gNewObjectDefinition.type 		= GLOBAL_ObjType_BanditoHat;
	hat = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	BanditoPutHatOnBack(hat);
	
			/* CHAIN THEM */
			
	newObj->ChainNode = gun;
	gun->ChainNode = hat;



				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 7, 7,false);
				

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_BANDITO]++;
				
	if (gunInHand)
		BanditoPutGunInHand(newObj);
				
	return(newObj);

}



/************************* PUT HAT ON BACK *************************/

static void	BanditoPutHatOnBack(ObjNode *hat)
{
	hat->HoldOffset.x = 0;
	hat->HoldOffset.y = 10;
	hat->HoldOffset.z = 26;

	hat->HoldRot.x = 1.4;
	hat->HoldRot.y = 0;
	hat->HoldRot.z = 0;
}



/************************* PUT GUN IN HOLSTER *************************/

static void	BanditoPutgunInHolster(ObjNode *gun)
{
	gun->HoldOffset.x = 22;
	gun->HoldOffset.y = 14;
	gun->HoldOffset.z = -7;

	gun->HoldRot.x = -.1;
	gun->HoldRot.y = 0;
	gun->HoldRot.z = -.1;

	gun->GunLocation = GUN_LOCATION_HOLSTER;
}


/*********************** BANDITO PUT GUN INTO HAND ***********************/

void BanditoPutGunInHand(ObjNode *enemy)
{
ObjNode 		*gun;

	gun = enemy->ChainNode;


			/* IN HANDS */
				
	gun->GunLocation = GUN_LOCATION_HAND;


		/* GUN ALIGNMENT */

	gun->HoldOffset.x = 3;
	gun->HoldOffset.y = -16;
	gun->HoldOffset.z = -19;

	gun->HoldRot.x = .5;
	gun->HoldRot.y = 0;
	gun->HoldRot.z = .5;

}



/************************ UPDATE BANDITO ATTACHMENTS ********************************/

void UpdateBanditoAttachments(ObjNode *enemy)
{	
ObjNode 		*gun, *hat;
OGLMatrix4x4	m,mst,rm,m2;

			/* GET ATTACHMENTS */
			
	gun = enemy->ChainNode;
	hat = gun->ChainNode;


			/**************/
			/* UPDATE GUN */
			/**************/
		
	if (gun->GunLocation == GUN_LOCATION_HOLSTER)							// attached to hip or hand?
		FindJointFullMatrix(enemy, BANDITO_JOINT_RIGHTHIP, &m);					
	else
		FindJointFullMatrix(enemy, BANDITO_JOINT_RIGHTHAND, &m);					
		
	CalcGunMatrixFromJointMatrix(gun, &m, &gun->BaseTransformMatrix);		
	SetObjectTransformMatrix(gun);

	gun->Coord.x = gun->BaseTransformMatrix.value[M03];				// extract coord from matrix
	gun->Coord.y = gun->BaseTransformMatrix.value[M13];
	gun->Coord.z = gun->BaseTransformMatrix.value[M23];


			/**************/
			/* UPDATE HAT */
			/**************/
		
	OGLMatrix4x4_SetTranslate(&mst, hat->HoldOffset.x, hat->HoldOffset.y, hat->HoldOffset.z);
	OGLMatrix4x4_SetRotate_XYZ(&rm, hat->HoldRot.x, hat->HoldRot.y, hat->HoldRot.z);
	OGLMatrix4x4_Multiply(&rm, &mst, &m2);
			
	FindJointFullMatrix(enemy, BANDITO_JOINT_UPPERSPINE, &m);					
	
	OGLMatrix4x4_Multiply(&m2, &m, &hat->BaseTransformMatrix);
	SetObjectTransformMatrix(hat);

	hat->Coord.x = hat->BaseTransformMatrix.value[M03];					// extract coord from matrix
	hat->Coord.y = hat->BaseTransformMatrix.value[M13];
	hat->Coord.z = hat->BaseTransformMatrix.value[M23];


	
}

#pragma mark -


/************************* ADD BANDITO: SHOOTOUT *********************************/

Boolean AddBandito_Shootout(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
short	stopPoint 	= itemPtr->parm[0];
short	actionType 	= itemPtr->parm[1];
float	rot 		= (float)itemPtr->parm[2] * (PI2/8);
const 	int initialAnim[] =
{
	BANDITO_ANIM_DUCK,			/* DUCK DOWN */
	BANDITO_ANIM_DUCK,			/* DUCK LEFT */
	BANDITO_ANIM_DUCK,			/* DUCK RIGHT */
};

			
			/***************************/
			/* MAKE THE ENEMY SKELETON */
			/***************************/
			
	newObj = MakeBandito(x, z, rot, initialAnim[actionType], MoveShootoutBandito, true);			
			
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->StopPoint 		= stopPoint;
	newObj->ActionType 	= actionType;
	newObj->EnemyIsDead = false;

	newObj->HitByBulletCallback = ShootoutBanditoHitByBulletCallback;

	newObj->Timer = 1.0f + RandomFloat();							// set random attack timer delay



	return(true);													// item was added
}


/*********************** MOVE SHOOTOUT BANDITO ***************************/

static void MoveShootoutBandito(ObjNode *enemy)
{

		/***********************************/
		/* DETERMINE IF IT'S ACTIVE OR NOT */
		/***********************************/

	if (enemy->StopPoint == gStopPointNum)				// is this thing visible @ this stop point?
	{
		if (enemy->StatusBits & STATUS_BIT_HIDDEN)		// if hidden then show it
			ShowObject(enemy);	
	}
	else												// not active
	{
		if (!(enemy->StatusBits & STATUS_BIT_HIDDEN))	// if visible then hide it
		{
			if (enemy->EnemyIsDead)						// for dead enemies we wait until they're culled and then delete them rather than hide them now
			{
				if (enemy->StatusBits & STATUS_BIT_ISCULLED)	// once it's culled then delete it
					DeleteEnemy(enemy);
				else
					UpdateEnemyAttachments(enemy);				// keep attachments aligned during death
			}
			else
				HideObject(enemy);	
		}
		return;
	}


		/********************/
		/* PROCESS ENEMY AI */
		/********************/
		
	GetObjectInfo(enemy);

		/* KEEP AIMED AT PLAYER */
		
	TurnObjectTowardTarget(enemy, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 4.0, false);		
		
		
		/* HANDLE ACTIONS */

	switch(enemy->ActionType)
	{
		case	ACTION_TYPE_DUCKDOWN:
				MoveShootoutBandito_DuckDown(enemy);
				break;

		case	ACTION_TYPE_DUCKLEFT:
				break;

		case	ACTION_TYPE_DUCKRIGHT:
				break;
	}



			/* UPDATE */
			
	UpdateObject(enemy);
	UpdateEnemyAttachments(enemy);
}



/********************* MOVE SHOOTOUT BANDITO: DUCK DOWN ***************************/

static void MoveShootoutBandito_DuckDown(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;

				/*****************/
				/* IS DUCKED NOW */
				/*****************/
				
	if (enemy->Skeleton->AnimNum == BANDITO_ANIM_DUCK)
	{
			/* SHOULD ATTACK NOW? */
			
		if (gTimeSinceLastEnemyShot > 1.0f)
		{
			if (gShootoutMode == SHOOTOUT_MODE_BATTLE)				// only attach once in battle mode (not still walking)
			{
				if ((!(enemy->StatusBits & STATUS_BIT_ISCULLED)) ||	// only attack if we can see it or if player is out of ammo
					(gPlayerInfo.ammoCount == 0))			
				{
					enemy->Timer -= fps;								// check attack timer
					if (enemy->Timer <= 0.0f)							// is time to do attack?
					{
						enemy->ShootNow = false;
						MorphToSkeletonAnim(enemy->Skeleton, BANDITO_ANIM_STANDSHOOT, 5);		// stand and shoot!
					}
				}
			}
		}
	}
	
			/***************/
			/* IS SHOOTING */
			/***************/
	else
	if (enemy->Skeleton->AnimNum == BANDITO_ANIM_STANDSHOOT)
	{
			/* FIRE GUN NOW? */
			
		if (enemy->ShootNow)
		{
			enemy->ShootNow = false;		
			FireBanditoShootoutGun(enemy);
		}

		if (enemy->Skeleton->AnimHasStopped)					// done?  go back to ducked
		{
			MorphToSkeletonAnim(enemy->Skeleton, BANDITO_ANIM_DUCK, 5);
			enemy->Timer = 2.0f + RandomFloat() * 2.0f;			// random delay until he shoots again
		}
	}
}


/******************* SHOOTOUT BANDITO HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void ShootoutBanditoHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt)
{
OGLVector3D	splatVec;

	(void) bullet;

			/* MAKE BULLET IMPACT SPLAT */
			
	splatVec.x = -gDelta.x;
	splatVec.y = -gDelta.y;
	splatVec.z = -gDelta.z;
	OGLVector3D_Normalize(&splatVec, &splatVec);
	DoBulletImpact(impactPt, &splatVec, 1.0);

	PlayEffect3D(EFFECT_BULLETHIT, impactPt);			


			/* KILL ENEMY IF WASN'T ALREADY DEAD */
			
	if (!enemy->EnemyIsDead)
	{
		MorphToSkeletonAnim(enemy->Skeleton, BANDITO_ANIM_GOTSHOT2, 10);
		enemy->EnemyIsDead = true;


			/* DEC COUNTER & SEE IF READY TO PROCEED TO NEXT STOP POINT */

		DecEnemiesAtStopPoint();
	}
}


/********************* FIRE BANDITO SHOOTOUT GUN **************************/

static void FireBanditoShootoutGun(ObjNode *enemy)
{
const OGLPoint3D	muzzleOff = {-2, -9.3, -3.3};
OGLPoint3D			muzzleCoord;
OGLMatrix4x4		jm,m;
ObjNode				*gun;			
OGLVector3D			muzzToPlayer;
OGLPoint3D			targetPt;
const OGLPoint3D	headOff = {0,15,0};
		
	gun = enemy->ChainNode;

	
		
			/* CALC TRUE AIM AND MUZZLE TIP */
								
	FindJointMatrixAtFlagEvent(enemy, BANDITO_JOINT_RIGHTHAND, 0, &jm);					// get hand matrix
	CalcGunMatrixFromJointMatrix(gun, &jm, &m);								// calc gun's orientation matrix @ this moment		
	OGLPoint3D_Transform(&muzzleOff, &m, &muzzleCoord);						// calc coord of muzzle tip


			/* CALC BULLET VECTOR */
			
	if (gPlayerInfo.shieldPower > 0.0f)					// if has shield shoot high
		FindCoordOnJoint(gPlayerInfo.objNode, PLAYER_JOINT_HEAD, &headOff, &targetPt);
	else
		FindCoordOfJoint(gPlayerInfo.objNode, PLAYER_JOINT_HEAD, &targetPt);
						
	muzzToPlayer.x = targetPt.x - muzzleCoord.x;
	muzzToPlayer.y = targetPt.y - muzzleCoord.y;
	muzzToPlayer.z = targetPt.z - muzzleCoord.z;
	OGLVector3D_Normalize(&muzzToPlayer, &muzzToPlayer);


			/* MAKE BLAST EFFECT & BULLET */
			
	MakeGunBlast(&muzzleCoord, &muzzToPlayer);
	PlayEffect3D(EFFECT_GUNSHOT1, &muzzleCoord);

	ShootEnemyBullet(&muzzleCoord, &muzzToPlayer);
	
	gTimeSinceLastEnemyShot = 0;
}












