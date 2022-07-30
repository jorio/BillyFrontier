/****************************/
/*   ENEMY: SHORTY.C	*/
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

static void	ShortyPutgunInHolster(ObjNode *gun);
static void	ShortyPutHatOnHead(ObjNode *hat);

static void MoveShootoutShorty(ObjNode *enemy);
static void FireShortyShootoutGun(ObjNode *enemy);
static void ShootoutShortyHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt);
static void MoveShootoutShorty_DuckDown(ObjNode *enemy);
static void MoveShootoutShorty(ObjNode *enemy);
static void MoveShootoutShorty_DuckShoot(ObjNode *enemy);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SHORTYS				8

#define	SHORTY_SCALE			.7f


/*********************/
/*    VARIABLES      */
/*********************/



/************************* MAKE SHORTY ****************************/

ObjNode *MakeShorty(float x, float z, float rot, short animNum, movecall_t moveCall, Boolean gunInHand)
{
ObjNode	*newObj, *gun, *hat;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_SHORTY,animNum, x,z, SHORTY_SCALE, rot, moveCall,
								gAutoFadeStatusBits);
	


				/* SET BETTER INFO */

	newObj->Kind 		= ENEMY_KIND_SHORTY;
	newObj->CType		= CTYPE_PICKABLE;
	
				/* SET COLLISION INFO */
								
	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	




		/********************/
		/* MAKE ATTACHMENTS */
		/********************/

			/* GUN */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_ShortyGun;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gun = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	gun->Side = SIDE_RIGHT;
	ShortyPutgunInHolster(gun);
	

			/* HAT */
			
	gNewObjectDefinition.type 		= GLOBAL_ObjType_ShortyHat;
	hat = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	ShortyPutHatOnHead(hat);
	
			/* CHAIN THEM */
			
	newObj->ChainNode = gun;
	gun->ChainNode = hat;



				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 3, 3,false);
				

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_SHORTY]++;

	if (gunInHand)
		ShortyPutGunInHand(newObj);
				
	return(newObj);

}



/************************* PUT HAT ON HEAD *************************/

static void	ShortyPutHatOnHead(ObjNode *hat)
{
	hat->HoldOffset.x = 0;
	hat->HoldOffset.y = 70;
	hat->HoldOffset.z = 0;

	hat->HoldRot.x = 0;
	hat->HoldRot.y = 0;
	hat->HoldRot.z = 0;
}



/************************* PUT GUN IN HOLSTER *************************/

static void	ShortyPutgunInHolster(ObjNode *gun)
{
	gun->HoldOffset.x = 20;
	gun->HoldOffset.y = 5;
	gun->HoldOffset.z = 0;

	gun->HoldRot.x = -.1;
	gun->HoldRot.y = 0;
	gun->HoldRot.z = -.1;

	gun->GunLocation = GUN_LOCATION_HOLSTER;
}


/*********************** SHORTY PUT GUN INTO HAND ***********************/

void ShortyPutGunInHand(ObjNode *enemy)
{
ObjNode 		*gun;

	gun = enemy->ChainNode;


			/* IN HANDS */
				
	gun->GunLocation = GUN_LOCATION_HAND;


		/* GUN ALIGNMENT */

	gun->HoldOffset.x = 0;
	gun->HoldOffset.y = -24;
	gun->HoldOffset.z = -24;

	gun->HoldRot.x = .2;
	gun->HoldRot.y = 0;
	gun->HoldRot.z = 0;

}



/************************ UPDATE SHORTY ATTACHMENTS ********************************/

void UpdateShortyAttachments(ObjNode *enemy)
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
		FindJointFullMatrix(enemy, SHORTY_JOINT_RIGHTHIP, &m);					
	else
		FindJointFullMatrix(enemy, SHORTY_JOINT_RIGHTHAND, &m);					
		
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
			
	FindJointFullMatrix(enemy, SHORTY_JOINT_HEAD, &m);					
	
	OGLMatrix4x4_Multiply(&m2, &m, &hat->BaseTransformMatrix);
	SetObjectTransformMatrix(hat);

	hat->Coord.x = hat->BaseTransformMatrix.value[M03];					// extract coord from matrix
	hat->Coord.y = hat->BaseTransformMatrix.value[M13];
	hat->Coord.z = hat->BaseTransformMatrix.value[M23];


	
}





#pragma mark -


/************************* ADD SHORTY: SHOOTOUT *********************************/

Boolean AddShorty_Shootout(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
short	stopPoint 	= itemPtr->parm[0];
short	actionType 	= itemPtr->parm[1];
float	rot 		= (float)itemPtr->parm[2] * (PI2/8);
const 	int initialAnim[] =
{
	SHORTY_ANIM_DUCK,			/* DUCK DOWN */
	SHORTY_ANIM_DUCK,			/* DUCK LEFT */
	SHORTY_ANIM_DUCK,			/* DUCK RIGHT */
};

			
			/***************************/
			/* MAKE THE ENEMY SKELETON */
			/***************************/
			
	newObj = MakeShorty(x, z, rot, initialAnim[actionType], MoveShootoutShorty, true);			
			
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->StopPoint 		= stopPoint;
	newObj->ActionType 	= actionType;
	newObj->EnemyIsDead = false;

	newObj->HitByBulletCallback = ShootoutShortyHitByBulletCallback;

	newObj->Timer = 1.0f + RandomFloat();							// set random attack timer delay



	return(true);													// item was added
}


/*********************** MOVE SHOOTOUT SHORTY ***************************/

static void MoveShootoutShorty(ObjNode *enemy)
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

	switch(enemy->Skeleton->AnimNum)
	{
		case	SHORTY_ANIM_DUCK:
				MoveShootoutShorty_DuckDown(enemy);
				break;
				
		case	SHORTY_ANIM_DUCKSHOOTRIGHT:
		case	SHORTY_ANIM_DUCKSHOOTLEFT:
		case	SHORTY_ANIM_STANDSHOOT:
				MoveShootoutShorty_DuckShoot(enemy);
				break;
	}



			/* UPDATE */
			
	UpdateObject(enemy);
	UpdateEnemyAttachments(enemy);
}



/********************* MOVE SHOOTOUT SHORTY: DUCK DOWN ***************************/

static void MoveShootoutShorty_DuckDown(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;

		/* SHOULD ATTACK NOW? */

	if (gShootoutMode == SHOOTOUT_MODE_BATTLE)					// only attack once in battle mode (not still walking)
	{
		if (gTimeSinceLastEnemyShot > 1.0f)
		{
			if ((!(enemy->StatusBits & STATUS_BIT_ISCULLED)) ||	// only attack if we can see it or if player is out of ammo
				(gPlayerInfo.ammoCount == 0))			
			{
				enemy->Timer -= fps;								// check attack timer
				if (enemy->Timer <= 0.0f)							// is time to do attack?
				{
					enemy->ShootNow = false;
					
					switch(enemy->ActionType)
					{
						case	ACTION_TYPE_DUCKDOWN:
								MorphToSkeletonAnim(enemy->Skeleton, SHORTY_ANIM_STANDSHOOT, 8);		// shoot to right
								break;

						case	ACTION_TYPE_DUCKLEFT:
								MorphToSkeletonAnim(enemy->Skeleton, SHORTY_ANIM_DUCKSHOOTLEFT, 8);		
								break;

						case	ACTION_TYPE_DUCKRIGHT:
								MorphToSkeletonAnim(enemy->Skeleton, SHORTY_ANIM_DUCKSHOOTRIGHT, 8);
								break;
					}
					
				}
			}
		}
	}
	
}



/********************* MOVE SHOOTOUT SHORTY: DUCK SHOOT ***************************/

static void MoveShootoutShorty_DuckShoot(ObjNode *enemy)
{

		/* FIRE GUN NOW? */
		
	if (enemy->ShootNow)
	{
		enemy->ShootNow = false;		
		FireShortyShootoutGun(enemy);
	}

	if (enemy->Skeleton->AnimHasStopped)					// done?  go back to ducked
	{
		MorphToSkeletonAnim(enemy->Skeleton, SHORTY_ANIM_DUCK, 5);
		enemy->Timer = 2.0f + RandomFloat() * 2.0f;			// random delay until he shoots again
	}

}



/******************* SHOOTOUT SHORTY HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void ShootoutShortyHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt)
{
OGLVector3D	splatVec;

	bullet;

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
		MorphToSkeletonAnim(enemy->Skeleton, SHORTY_ANIM_SHOTINCHEST, 10);
		enemy->EnemyIsDead = true;


			/* DEC COUNTER & SEE IF READY TO PROCEED TO NEXT STOP POINT */
						
		DecEnemiesAtStopPoint();
	}
	
}


/********************* FIRE SHORTY SHOOTOUT GUN **************************/

static void FireShortyShootoutGun(ObjNode *enemy)
{
const OGLPoint3D	muzzleOff = {0, -9.3, -3.3};
OGLPoint3D			muzzleCoord;
OGLMatrix4x4		jm,m;
ObjNode				*gun;			
OGLVector3D			muzzToPlayer;
OGLPoint3D			targetPt;
const OGLPoint3D	headOff = {0,15,0};
		
	gun = enemy->ChainNode;

	
		
			/* CALC TRUE AIM AND MUZZLE TIP */
								
	FindJointMatrixAtFlagEvent(enemy, SHORTY_JOINT_RIGHTHAND, 0, &jm);					// get hand matrix
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
	PlayEffect3D(EFFECT_GUNSHOT, &muzzleCoord);

	ShootEnemyBullet(&muzzleCoord, &muzzToPlayer);
	
	gTimeSinceLastEnemyShot = 0;

}













