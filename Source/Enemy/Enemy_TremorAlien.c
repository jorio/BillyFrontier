/****************************/
/*   ENEMY: TREMORALIEN.C	*/
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

static void MoveShootoutTremorAlien(ObjNode *enemy);
static void TremorAlienThrowTomahawk(ObjNode *enemy);
static void ShootoutTremorAlienHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt);
static void MoveShootoutTremorAlien_Throw(ObjNode *enemy);
static void MoveShootoutTremorAlien(ObjNode *enemy);
static void MoveShootoutTremorAlien_Hover(ObjNode *enemy);
static void MoveShootoutTremorAlien_Shot(ObjNode *enemy);
static void TremorAlienPutTomahawkInHand(ObjNode *enemy);
static void MoveTomahawk(ObjNode *theNode);
static void UpdateJetBlast(ObjNode *enemy);
static void TomahawkHitByBulletCallback(ObjNode *bullet, ObjNode *toma, const OGLPoint3D *impactPt);
static void MoveTremorAlienOnSpline(ObjNode *theNode);
static void MoveShootoutTremorAlien_Death(ObjNode *enemy);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_TREMORALIENS				8

#define	TREMORALIEN_SCALE			1.1f


enum
{
	TREMORALIEN_JOINT_RIGHTHAND	= 8

};

enum
{
	TREMORALIEN_ANIM_STAND = 0,
	TREMORALIEN_ANIM_WALK,
	TREMORALIEN_ANIM_HOVER,
	TREMORALIEN_ANIM_THROW,
	TREMORALIEN_ANIM_DEATH,
	TREMORALIEN_ANIM_HITGROUND,
	TREMORALIEN_ANIM_SHOTINCHEST
};
		


/*********************/
/*    VARIABLES      */
/*********************/

#define	NewTomaNow	Flag[1]
#define	HoverUp		Flag[3]


/************************* MAKE TREMORALIEN ****************************/

ObjNode *MakeTremorAlien(float x, float z, float rot, short animNum, movecall_t moveCall, Boolean gunInHand)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_TREMORALIEN,animNum, x,z, TREMORALIEN_SCALE, rot, moveCall,
								gAutoFadeStatusBits);
	


				/* SET BETTER INFO */

	newObj->HoverUp		= false;							// down
	newObj->NewTomaNow	= false;
	
				/* SET COLLISION INFO */

	newObj->Kind 	= ENEMY_KIND_TREMORALIEN;
	newObj->CType	= CTYPE_PICKABLE;
	
	newObj->Health 	= 2.5f;
	
								
	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 3, 3,false);
				

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_TREMORALIEN]++;

	if (gunInHand)
		TremorAlienPutTomahawkInHand(newObj);
				
	return(newObj);

}








#pragma mark -


/************************* ADD TREMORALIEN: SHOOTOUT *********************************/

Boolean AddTremorAlien_Shootout(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
short	stopPoint 	= itemPtr->parm[0];
float	rot 		= (float)itemPtr->parm[2] * (PI2/8);

			
			/***************************/
			/* MAKE THE ENEMY SKELETON */
			/***************************/
			
	newObj = MakeTremorAlien(x, z, rot, TREMORALIEN_ANIM_HOVER, MoveShootoutTremorAlien, true);			
			
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->StopPoint 		= stopPoint;
	newObj->EnemyIsDead 	= false;

	newObj->HitByBulletCallback = ShootoutTremorAlienHitByBulletCallback;

	newObj->Timer = 2.0f + RandomFloat() * 3.0f;					// set random attack timer delay

	return(true);													// item was added
}


/*********************** MOVE SHOOTOUT TREMORALIEN ***************************/

static void MoveShootoutTremorAlien(ObjNode *enemy)
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
		if (enemy->EnemyIsDead)							// for dead enemies we wait until they're culled and then delete them rather than hide them now
		{
			if (enemy->StatusBits & STATUS_BIT_ISCULLED)	// once it's culled then delete it
				DeleteEnemy(enemy);
			else
				UpdateEnemyAttachments(enemy);				// keep attachments aligned during death
		}
		else
			HideObject(enemy);	

		return;
	}


		/********************/
		/* PROCESS ENEMY AI */
		/********************/
		
	GetObjectInfo(enemy);

		/* HANDLE ACTIONS */

	switch(enemy->Skeleton->AnimNum)
	{
		case	TREMORALIEN_ANIM_HOVER:
				MoveShootoutTremorAlien_Hover(enemy);
				break;
				
		case	TREMORALIEN_ANIM_THROW:
				MoveShootoutTremorAlien_Throw(enemy);
				break;

		case	TREMORALIEN_ANIM_SHOTINCHEST:
				MoveShootoutTremorAlien_Shot(enemy);
				break;


		case	TREMORALIEN_ANIM_DEATH:
				MoveShootoutTremorAlien_Death(enemy);
				break;
	}



			/* UPDATE */
			
	UpdateObject(enemy);
	UpdateTremorAlienAttachments(enemy);
	
	if (enemy->Health >= 0.0f)
		UpdateJetBlast(enemy);
}



/********************* MOVE SHOOTOUT TREMORALIEN: HOVER ***************************/

static void MoveShootoutTremorAlien_Hover(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;
float	y;

		/* KEEP AIMED AT PLAYER */
		
	TurnObjectTowardTarget(enemy, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 4.0, false);		
		

	y = GetTerrainY(gCoord.x, gCoord.z) + 90.0f;				// get lower y coord
	
			/******************************/
			/* ARE WE IN THE UP POSITION? */
			/******************************/
				
	if (enemy->HoverUp)
	{
				/* RISE */
				
		if (gCoord.y < (y + 60.0f))			// under mid-point
		{
			gDelta.y += fps * 500.0f;		// accelerate
			gCoord.y += gDelta.y * fps;		
		}
		else								// above mid-point
		{
			gDelta.y -= fps * 500.0f;		// decelerate
			if (gDelta.y < 0.0f)
				gDelta.y = 0;
			gCoord.y += gDelta.y * fps;
			
			if (gDelta.y <= 0.0f)			// stopped at apex?
			{
				/* THROW NOW */
				
				enemy->ShootNow = false;
				MorphToSkeletonAnim(enemy->Skeleton, TREMORALIEN_ANIM_THROW, 8);
			}
		}
	}
	
			/*****************/
			/* DOWN POSITION */
			/*****************/
			
	else
	{
		
			/* MOVE DOWN */
			
		gDelta.y -= 300.0f * fps;
		gCoord.y += gDelta.y * fps;
		if (gCoord.y < y)
		{
			gCoord.y = y;
			gDelta.y = 0;
		}
			
	
			/* SEE IF RISE NOW */
	
		if (gShootoutMode == SHOOTOUT_MODE_BATTLE)					// only attack once in battle mode (not still walking)
		{
			if (!(enemy->StatusBits & STATUS_BIT_ISCULLED))			// dont attack if out of view
			{
				enemy->Timer -= fps;								// check attack timer
				if (enemy->Timer <= 0.0f)							// is time to do attack?
				{
					enemy->HoverUp = true;
				}
			}
		}
	}
}



/********************* MOVE SHOOTOUT TREMORALIEN: SHOT ***************************/

static void MoveShootoutTremorAlien_Shot(ObjNode *enemy)
{
	if (enemy->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(enemy->Skeleton, TREMORALIEN_ANIM_HOVER, 5);
}


/********************* MOVE SHOOTOUT TREMORALIEN: DEATH ***************************/

static void MoveShootoutTremorAlien_Death(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;
float	y;

	y = GetTerrainY(gCoord.x, gCoord.z) + 100.0f;				// get lower y coord
	
	gDelta.y -= 1000.0f * fps;
	gCoord.y += gDelta.y * fps;
	
	if (gCoord.y <= y)
	{
		gCoord.y = y;	
		SetSkeletonAnim(enemy->Skeleton, TREMORALIEN_ANIM_HITGROUND);
	}
}



/********************* MOVE SHOOTOUT TREMORALIEN: THROW ***************************/

static void MoveShootoutTremorAlien_Throw(ObjNode *enemy)
{

		/* FIRE GUN NOW? */
		
	if (enemy->ShootNow)
	{
		enemy->ShootNow = false;		
		TremorAlienThrowTomahawk(enemy);
	}


		/* GET NEW TOMAHAWK? */

	if (enemy->NewTomaNow)
	{
		enemy->NewTomaNow = false;
		TremorAlienPutTomahawkInHand(enemy);
	}
	

	if (enemy->Skeleton->AnimHasStopped)					// done?  go back to ducked
	{
		MorphToSkeletonAnim(enemy->Skeleton, TREMORALIEN_ANIM_HOVER, 5);
		enemy->Timer = 2.0f + RandomFloat() * 2.0f;			// random delay until he shoots again
		enemy->HoverUp = false;
	}

}



/******************* SHOOTOUT TREMORALIEN HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void ShootoutTremorAlienHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt)
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
		if (enemy->Health > 0.0f)
		{
			MorphToSkeletonAnim(enemy->Skeleton, TREMORALIEN_ANIM_SHOTINCHEST, 3);
		}
		else
		{
			MorphToSkeletonAnim(enemy->Skeleton, TREMORALIEN_ANIM_DEATH, 12);
			enemy->Delta.y = 350.0f;
			enemy->EnemyIsDead = true;
			RemoveFromSplineObjectList(enemy);			// detach from spline (if any)
			enemy->MoveCall = MoveShootoutTremorAlien;
			enemy->CType = 0;
			
				/* DEC COUNTER & SEE IF READY TO PROCEED TO NEXT STOP POINT */
						
			DecEnemiesAtStopPoint();
		}


	}
	
}


#pragma mark -

/************************ PRIME TREMOR ALIEN *************************/

Boolean PrimeTremorAlien(long splineNum, SplineItemType *itemPtr)
{
short		stopPoint 	= itemPtr->parm[0];
ObjNode		*newObj;
float		x,z,placement;


	gNumEnemiesThisStopPoint[stopPoint]++;							// inc sp count

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE KANGA SKELETON */
				
//	newObj = MakeEnemySkeleton(SKELETON_TYPE_TREMORALIEN, TREMORALIEN_ANIM_HOVER, x,z, TREMORALIEN_SCALE, 0, nil, 0);

	newObj = MakeTremorAlien(x, z, 0, TREMORALIEN_ANIM_HOVER, nil, true);

				
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;			
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveTremorAlienOnSpline;					// set move call
			
	newObj->StopPoint 		= stopPoint;
	newObj->EnemyIsDead 	= false;

	newObj->HitByBulletCallback = ShootoutTremorAlienHitByBulletCallback;

	TremorAlienPutTomahawkInHand(newObj);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 4, false);


	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync



	return(true);
}


/******************** MOVE TREMOR ALIEN ON SPLINE ***************************/

static void MoveTremorAlienOnSpline(ObjNode *theNode)
{
Boolean 	isInRange, atEnd = false; 

	isInRange = UpdateSplineItemVisibilityOnActiveTerrain(theNode);					// update its visibility


			/* HIDE WHEN NOT AT THIS STOPPOINT */
			
	if (theNode->StopPoint != gStopPointNum)
		HideObject(theNode);
	else
		ShowObject(theNode);


			/********************/
			/* UPDATE FOR DEATH */
			/********************/
				

	if (theNode->Skeleton->AnimNum == TREMORALIEN_ANIM_DEATH)
	{
	
	
	
	}


			/*******************************/
			/* UPDATE STUFF IF ACTIVE HERE */
			/*******************************/
			
	if ((theNode->StopPoint == gStopPointNum) && (gShootoutMode == SHOOTOUT_MODE_BATTLE))
	{		
			/* MOVE ALONG THE SPLINE */

		if (theNode->Skeleton->AnimNum == TREMORALIEN_ANIM_HOVER)
		{
			atEnd = IncreaseSplineIndex(theNode, 150.0f, true);
			GetObjectCoordOnSpline(theNode);
		}

		GetObjectInfo(theNode);
	
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												gCoord.x, gCoord.z);		

		gCoord.y = GetTerrainY(gCoord.x, gCoord.z) - theNode->BottomOff + 55.0f;	// calc y coord

		UpdateObject(theNode);
		
		
				/* ATTACK? */
				
		if (atEnd)
		{
			if (theNode->Skeleton->AnimNum != TREMORALIEN_ANIM_THROW)
			{
				theNode->Timer -= gFramesPerSecondFrac;
				if (theNode->Timer <= 0.0f)
				{				
					MorphToSkeletonAnim(theNode->Skeleton, TREMORALIEN_ANIM_THROW, 5);
					theNode->NewTomaNow = false;
					theNode->ShootNow = false;				
				}
			}
		}
		
		if (theNode->Skeleton->AnimNum == TREMORALIEN_ANIM_THROW)
		{
			if (theNode->ShootNow)
			{
				theNode->ShootNow = false;				
				TremorAlienThrowTomahawk(theNode);		
			}
				
			if (theNode->Skeleton->AnimHasStopped)
			{
				MorphToSkeletonAnim(theNode->Skeleton, TREMORALIEN_ANIM_HOVER, 5);
				theNode->Timer = 2.0f + RandomFloat() * 3.0f;			// random delay until he shoots again
			}
		}
		
			/* GET NEW TOMAHAWK? */

		if (theNode->NewTomaNow)
		{
			theNode->NewTomaNow = false;
			TremorAlienPutTomahawkInHand(theNode);
		}
		

				/* SEE IF DONE BEING SHOT */
						
		if (theNode->Skeleton->AnimNum == TREMORALIEN_ANIM_SHOTINCHEST)
		{
			if (theNode->Skeleton->AnimHasStopped)
			{
				MorphToSkeletonAnim(theNode->Skeleton, TREMORALIEN_ANIM_HOVER, 5);
			}		
		}
	}	
	
			/* UPDATE STUFF IF IN RANGE */

	if (isInRange && (!(theNode->StatusBits & STATUS_BIT_HIDDEN)))
	{
		UpdateTremorAlienAttachments(theNode);
		UpdateJetBlast(theNode);
	}
}





#pragma mark -

/*********************** TREMORALIEN PUT TOMAHAWK INTO HAND ***********************/

static void TremorAlienPutTomahawkInHand(ObjNode *enemy)
{
ObjNode 		*toma;

				/* MAKE IT */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Tomahawk;
	gNewObjectDefinition.scale 		= enemy->Scale.x;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= enemy->Slot + 1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;	
	toma = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				

			/* IN HANDS */
				
	toma->GunLocation = GUN_LOCATION_HAND;

	toma->Damage = 1.0f;


		/* GUN ALIGNMENT */

	toma->HoldOffset.x = 5;
	toma->HoldOffset.y = -3;
	toma->HoldOffset.z = -16;

	toma->HoldRot.x = .2;
	toma->HoldRot.y = 0;
	toma->HoldRot.z = 0;
	
	enemy->ChainNode = toma;
	
	
	CreateCollisionBoxFromBoundingBox(toma, .7, .7);
}



/************************ UPDATE TREMORALIEN ATTACHMENTS ********************************/

void UpdateTremorAlienAttachments(ObjNode *enemy)
{	
ObjNode 		*toma;
OGLMatrix4x4	m;

			/* GET ATTACHMENTS */
			
	toma = enemy->ChainNode;
	


			/*******************/
			/* UPDATE TOMAHAWK */
			/*******************/
	
	if (toma)
	{	
		FindJointFullMatrix(enemy, TREMORALIEN_JOINT_RIGHTHAND, &m);					
			
		CalcGunMatrixFromJointMatrix(toma, &m, &toma->BaseTransformMatrix);		
		SetObjectTransformMatrix(toma);

		toma->Coord.x = toma->BaseTransformMatrix.value[M03];				// extract coord from matrix
		toma->Coord.y = toma->BaseTransformMatrix.value[M13];
		toma->Coord.z = toma->BaseTransformMatrix.value[M23];
	}
	
}


/********************* TREMOR ALIEN THROW TOMAHAWK **************************/

static void TremorAlienThrowTomahawk(ObjNode *enemy)
{
ObjNode		*toma;			
float		d;	
OGLVector2D	toPlayer;

	toma = enemy->ChainNode;
	if (!toma)
		return;

	enemy->ChainNode = nil;						// detach from enemy's chain

	toma->MoveCall = MoveTomahawk;				// it moves on its own now
	toma->CType    = CTYPE_PICKABLE;			// and can be shot out of the sky
	toma->HitByBulletCallback = TomahawkHitByBulletCallback;
	
	toma->Rot.y = enemy->Rot.y;
	
			/* CALC VECTOR TO PLAYER */
			
	toPlayer.x = gPlayerInfo.coord.x - toma->Coord.x;
	toPlayer.y = gPlayerInfo.coord.z - toma->Coord.z;
	FastNormalizeVector2D(toPlayer.x, toPlayer.y, &toPlayer, false);
	
	
			/* CALC DIST TO PLAYER */
			
	d = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, toma->Coord.x, toma->Coord.z);
	d *= .9f;
	
	toma->Delta.x = toPlayer.x * d;
	toma->Delta.z = toPlayer.y * d;
	toma->Delta.y = 300.0f;
	
	
	PlayEffect3D(EFFECT_SWISH, &toma->Coord);
	
}


/*********************** MOVE TOMAHAWK *************************/

static void MoveTomahawk(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);


			/* MOVE IT */
			
	gDelta.y -= 800.0f * fps;							// gravity
	
	gCoord.x += gDelta.x * fps;							// move
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x -= fps * 11.0f;						// spin
	theNode->Rot.y += fps * .9f;


		/* SEE IF HIT ANYTHING */

	if (HandleCollisions(theNode, CTYPE_PLAYER | CTYPE_TERRAIN, 0))
	{
		if (gCollisionList[0].objectPtr)									// did we hit an objNode?
		{
			if (gCollisionList[0].objectPtr->CType & CTYPE_PLAYER)			// hit player?
			{
				ShootoutPlayerHitByBulletCallback(theNode, gPlayerInfo.objNode, nil);		// hurt player				
				MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_STUNNED, 15);
			}
		}
				
		DeleteObject(theNode);
		return;
	}


			


	UpdateObject(theNode);
}




/******************* TOMAHAWK HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void TomahawkHitByBulletCallback(ObjNode *bullet, ObjNode *toma, const OGLPoint3D *impactPt)
{
	(void) bullet;

	PlayEffect3D(EFFECT_RICOCHET, impactPt);			

	ExplodeGeometry(toma, 100.0, 0, 1, .4);
	DeleteObject(toma);
}


#pragma mark -


/********************* UPDATE JET BLAST ***********************/

static void UpdateJetBlast(ObjNode *theNode)
{
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
int					i;

	theNode->ParticleTimer -= gFramesPerSecondFrac;									// see if add smoke
	if (theNode->ParticleTimer <= 0.0f)
	{
		theNode->ParticleTimer += .05f;												// reset timer
		
		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;
		
		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num
			
			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= 0;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 7.0f;
			groupDef.decayRate				=  -.9f;
			groupDef.fadeRate				= 2.5;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float x,y,z;
			const OGLPoint3D off = {0,-5,30};
			
			FindCoordOnJoint(theNode, 0, &off, &p);
			x = p.x;
			y = p.y;
			z = p.z;
						
			for (i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * 5.0f;
				p.y = y;
				p.z = z + RandomFloat2() * 5.0f;

				d.x = RandomFloat2() * 20.0f;
				d.y = -300.0f - RandomFloat() * 40.0f;
				d.z = RandomFloat2() * 20.0f;
			
				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + .5f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= .7;		
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}

}









