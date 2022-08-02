/****************************/
/*   ENEMY: FROGMAN.C	*/
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

static ObjNode *MakeFrogMan(float x, float z, float rot, short animNum, movecall_t moveCall);
static void MoveShootoutFrogMan(ObjNode *enemy);
static void FrogManThrowBarrel(ObjNode *enemy);
static void ShootoutFrogManHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt);
static void MoveShootoutFrogMan_Throw(ObjNode *enemy);
static void MoveShootoutFrogMan(ObjNode *enemy);
static void FrogManPutBarrelInHand(ObjNode *enemy);
static void MoveShootoutFrogMan_Stand(ObjNode *enemy);
static void UpdateFrogManAttachments(ObjNode *enemy);
static void MoveBarrel(ObjNode *theNode);
static void BarrelHitByBulletCallback(ObjNode *bullet, ObjNode *barrel, const OGLPoint3D *impactPt);



/****************************/
/*    CONSTANTS             */
/****************************/

#define FROGMAN_HEALTH		4.0f

#define	FROGMAN_SCALE			1.2f

enum
{
	FROGMAN_JOINT_RIGHTHAND	= 6,
	FROGMAN_JOINT_LEFTHAND	= 9

};


/*********************/
/*    VARIABLES      */
/*********************/

#define	GrabBarrelNow		Flag[0]
#define	ThrowBarrelNow		Flag[1]



/************************* ADD FROGMAN: SHOOTOUT *********************************/

Boolean AddFrogMan_Shootout(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
short	stopPoint 	= itemPtr->parm[0];
float	rot 		= (float)itemPtr->parm[1] * (PI2/8);

			
			/***************************/
			/* MAKE THE ENEMY SKELETON */
			/***************************/
			
	newObj = MakeFrogMan(x, z, rot, FROGMAN_ANIM_STAND, MoveShootoutFrogMan);			
			
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->StopPoint 		= stopPoint;
	newObj->EnemyIsDead 	= false;

	newObj->HitByBulletCallback = ShootoutFrogManHitByBulletCallback;

	newObj->Timer = 1.0f + RandomFloat();							// set random attack timer delay

	return(true);													// item was added
}


/************************* MAKE FROGMAN ****************************/

static ObjNode *MakeFrogMan(float x, float z, float rot, short animNum, movecall_t moveCall)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_FROGMAN,animNum, x,z, FROGMAN_SCALE, rot, moveCall,
								gAutoFadeStatusBits);
	


				/* SET BETTER INFO */
	
	newObj->Kind 		= ENEMY_KIND_FROGMAN;
	newObj->CType		= CTYPE_PICKABLE;
	newObj->Health		= FROGMAN_HEALTH;
	
				/* SET COLLISION INFO */
								
	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 3, 3,false);
				

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_FROGMAN]++;
				
	return(newObj);

}

/*********************** MOVE SHOOTOUT FROGMAN ***************************/

static void MoveShootoutFrogMan(ObjNode *enemy)
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

		/* HANDLE ACTIONS */

	switch(enemy->Skeleton->AnimNum)
	{
		case	FROGMAN_ANIM_STAND:
				MoveShootoutFrogMan_Stand(enemy);
				break;
				
		case	FROGMAN_ANIM_THROW:
				MoveShootoutFrogMan_Throw(enemy);
				break;
				
		case	FROGMAN_ANIM_STUNNED:
				if (enemy->Skeleton->AnimHasStopped)
				{
					MorphToSkeletonAnim(enemy->Skeleton, FROGMAN_ANIM_STAND, 5);				
				}
				break;
	}



			/* UPDATE */
			
	UpdateObject(enemy);
	UpdateFrogManAttachments(enemy);
}



/********************* MOVE SHOOTOUT FROGMAN: STAND ***************************/

static void MoveShootoutFrogMan_Stand(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;
float	y;

		/* KEEP AIMED AT PLAYER */
		
	TurnObjectTowardTarget(enemy, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 4.0, false);		
		

	y = GetTerrainY(gCoord.x, gCoord.z) + 100.0f;				// get lower y coord
	
	
		/* SEE IF ATTACK NOW */

	if (gShootoutMode == SHOOTOUT_MODE_BATTLE)					// only attack once in battle mode (not still walking)
	{
		if (!(enemy->StatusBits & STATUS_BIT_ISCULLED))
		{
			enemy->Timer -= fps;								// check attack timer
			if (enemy->Timer <= 0.0f)							// is time to do attack?
			{
				MorphToSkeletonAnim(enemy->Skeleton, FROGMAN_ANIM_THROW, 3);
				enemy->GrabBarrelNow = enemy->ThrowBarrelNow = false;
				
			}
		}
	}
}



/********************* MOVE SHOOTOUT FROGMAN: THROW ***************************/

static void MoveShootoutFrogMan_Throw(ObjNode *enemy)
{

		/* GET BARREL NOW */
		
	if (enemy->GrabBarrelNow)
	{
		enemy->GrabBarrelNow = false;
		FrogManPutBarrelInHand(enemy);
	}


		/* THROW BARREL NOW */
		
	if (enemy->ThrowBarrelNow)
	{
		enemy->ThrowBarrelNow = false;		
		FrogManThrowBarrel(enemy);
	}


			/* RETURN TO STAND */
			
	if (enemy->Skeleton->AnimHasStopped)					// done?  go back to ducked
	{
		MorphToSkeletonAnim(enemy->Skeleton, FROGMAN_ANIM_STAND, 5);
		enemy->Timer = 2.0f + RandomFloat() * 2.0f;			// random delay until he shoots again
	}

}



/******************* SHOOTOUT FROGMAN HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void ShootoutFrogManHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt)
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
		enemy->Health -= 1.0f;
		if (enemy->Health < 0.0f)
		{
			MorphToSkeletonAnim(enemy->Skeleton, FROGMAN_ANIM_SHOTDEAD, 3);
			enemy->Skeleton->AnimSpeed = .5f;
			enemy->EnemyIsDead = true;


			/* DEC COUNTER & SEE IF READY TO PROCEED TO NEXT STOP POINT */
						
			DecEnemiesAtStopPoint();
		}
		else
		{
			MorphToSkeletonAnim(enemy->Skeleton, FROGMAN_ANIM_STUNNED, 5);
		}
	}
	
}


#pragma mark -


/*********************** FROGMAN PUT BARREL INTO HAND ***********************/

static void FrogManPutBarrelInHand(ObjNode *enemy)
{
ObjNode 		*barrel;

				/* MAKE IT */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_FrogBarrel;
	gNewObjectDefinition.scale 		= 1.0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= enemy->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;	
	barrel = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				

	CreateCollisionBoxFromBoundingBox(barrel, .7, .7);

	barrel->Damage = 2.0f;
	

			/* IN HANDS */
				
	barrel->GunLocation = GUN_LOCATION_HAND;


		/* ALIGNMENT ALIGNMENT */

	barrel->HoldOffset.x = 50;
	barrel->HoldOffset.y = 0;
	barrel->HoldOffset.z = 0;

	barrel->HoldRot.x = 0;
	barrel->HoldRot.y = 0;
	barrel->HoldRot.z = 0;
	
	enemy->ChainNode = barrel;
}



/************************ UPDATE FROGMAN ATTACHMENTS ********************************/

static void UpdateFrogManAttachments(ObjNode *enemy)
{	
ObjNode 		*barrel;
OGLPoint3D		p1,p2;

			/* GET ATTACHMENTS */
			
	barrel = enemy->ChainNode;
	


			/*******************/
			/* UPDATE BARREL */
			/*******************/
	
	if (barrel)
	{	
		FindCoordOfJoint(enemy, FROGMAN_JOINT_LEFTHAND, &p1);		// get left hand coord
		FindCoordOfJoint(enemy, FROGMAN_JOINT_RIGHTHAND, &p2);		// get rt hand coord
		
		barrel->Coord.x = (p1.x + p2.x) * .5f;						// calc average center pt.
		barrel->Coord.y = (p1.y + p2.y) * .5f;
		barrel->Coord.z = (p1.z + p2.z) * .5f;
	
		barrel->Rot.y = enemy->Rot.y;
	
		UpdateObjectTransforms(barrel);

	}
	
}


/********************* FROGMAN THROW BARREL **************************/

static void FrogManThrowBarrel(ObjNode *enemy)
{
ObjNode		*barrel;			
float		r, d;	


	barrel = enemy->ChainNode;
	if (!barrel)
		return;

	enemy->ChainNode = nil;						// detach from enemy's chain

	barrel->MoveCall = MoveBarrel;				// it moves on its own now
	barrel->CType    = CTYPE_PICKABLE;			// and can be shot out of the sky
	barrel->HitByBulletCallback = BarrelHitByBulletCallback;
	
	r = barrel->Rot.y = enemy->Rot.y;
	
			/* CALC DIST TO PLAYER */
			
	d = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, enemy->Coord.x, enemy->Coord.z);
	d *= .9f;
	
	barrel->Delta.x = -sin(r) * d;
	barrel->Delta.z = -cos(r) * d;
	barrel->Delta.y = 400.0f;
	
	PlayEffect3D(EFFECT_SWISH, &barrel->Coord);	
}


/*********************** MOVE BARREL *************************/

static void MoveBarrel(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

			/* MOVE IT */

	gDelta.y -= 1000.0f * fps;							// gravity
	
	gCoord.x += gDelta.x * fps;							// move
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += fps * 9.0f;						// spin


		/* SEE IF HIT ANYTHING */

	if (HandleCollisions(theNode, CTYPE_PLAYER | CTYPE_MISC | CTYPE_TERRAIN, 0))
	{
		if (gCollisionList[0].objectPtr)									// did we hit an objNode?
		{
			if (gCollisionList[0].objectPtr->CType & CTYPE_PLAYER)			// hit player?
			{
				ShootoutPlayerHitByBulletCallback(theNode, gPlayerInfo.objNode, nil);		// hurt player				
				MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_STUNNED, 15);
			}
		}
				
		BarrelHitByBulletCallback(nil, theNode, nil);		// blow up barrell
		return;
	}



	UpdateObject(theNode);
}




/******************* BARREL HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void BarrelHitByBulletCallback(ObjNode *bullet, ObjNode *barrel, const OGLPoint3D *impactPt)
{
	(void) bullet;
	(void) impactPt;

	PlayEffect3D(EFFECT_EXPLOSION, &barrel->Coord);			

	ExplodeGeometry(barrel, 500.0, SHARD_MODE_FROMORIGIN, 1, .3);

	MakeSparkExplosion(barrel->Coord.x, barrel->Coord.y, barrel->Coord.z, 250.0f, 30.0, PARTICLE_SObjType_YellowGlint, 500, .5);

	DeleteObject(barrel);
}









