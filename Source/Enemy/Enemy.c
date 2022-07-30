/****************************/
/*   	ENEMY.C  			*/
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

static void MoveEnemyBullet(ObjNode *bullet);

/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

signed char	gNumEnemyOfKind[NUM_ENEMY_KINDS];
int			gNumEnemies;
int			gMaxEnemies;


/*********************  INIT ENEMY MANAGER **********************/

void InitEnemyManager(void)
{
short	i;

	gNumEnemies = 0;

	if (gG4)					// tweak based on horsepower
		gMaxEnemies = 20;
	else
		gMaxEnemies = 16;

	for (i=0; i < NUM_ENEMY_KINDS; i++)
		gNumEnemyOfKind[i] = 0;


}


/********************** DELETE ENEMY **************************/

void DeleteEnemy(ObjNode *theEnemy)
{
	if (!(theEnemy->StatusBits & STATUS_BIT_ONSPLINE))		// spline enemies dont factor into the enemy counts!
	{
		gNumEnemyOfKind[theEnemy->Kind]--;					// dec kind count
		if (gNumEnemyOfKind[theEnemy->Kind] < 0)
		{
			DoAlert("DeleteEnemy: < 0");
			gNumEnemyOfKind[theEnemy->Kind] = 0;
		}

		gNumEnemies--;										// dec global count
	}
	
	
	DeleteObject(theEnemy);								// nuke the obj
}



/*********************** UPDATE ENEMY ******************************/

void UpdateEnemy(ObjNode *theNode)
{
	theNode->Speed3D = CalcVectorLength(&gDelta);	

	UpdateObject(theNode);
}



/******************* MAKE ENEMY SKELETON *********************/
//
// This routine creates a non-character skeleton which is an enemy.
//
// INPUT:	itemPtr->parm[0] = skeleton type 0..n
//
// OUTPUT:	ObjNode or nil if err.
//

ObjNode *MakeEnemySkeleton(Byte skeletonType, short animNum, float x, float z, float scale, float rot, void *moveCall, u_long flags)
{
ObjNode	*newObj;
	
			/****************************/
			/* MAKE NEW SKELETON OBJECT */
			/****************************/

	gNewObjectDefinition.type 		= skeletonType;
	gNewObjectDefinition.animNum 	= animNum;							// assume default anim is #0
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= flags;
	gNewObjectDefinition.slot 		= ENEMY_SLOT + skeletonType;
	gNewObjectDefinition.moveCall 	= moveCall;
	gNewObjectDefinition.rot 		= rot;
	gNewObjectDefinition.scale 		= scale;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	
	
	
				/* SET DEFAULT COLLISION INFO */
				
	newObj->CType = CTYPE_ENEMY;
	newObj->CBits = CBITS_ALLSOLID;
	
	newObj->Coord.y -= newObj->BBox.min.y;						// offset so bottom touches ground
	UpdateObjectTransforms(newObj);
	
	return(newObj);
}




/***************** DETACH ENEMY FROM SPLINE *****************/
//
// OUTPUT: true if was on spline, false if wasnt
//

void DetachEnemyFromSpline(ObjNode *theNode, void *moveCall)
{
	if (!(theNode->StatusBits & STATUS_BIT_ONSPLINE))	// must be on spline
		return;

	DetachObjectFromSpline(theNode, moveCall);
	
	gNumEnemies++;									// count as a normal enemy now
	gNumEnemyOfKind[theNode->Kind]++;
}


#pragma mark -



/********************* FIND CLOSEST ENEMY *****************************/
//
// OUTPUT: nil if no enemies
//

ObjNode *FindClosestEnemy(OGLPoint3D *pt, float *dist)
{
ObjNode		*thisNodePtr,*best = nil;
float	d,minDist = 10000000;

			
	thisNodePtr = gFirstNodePtr;
	
	do
	{
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)					// see if reach end of usable list
			break;
	
		if (thisNodePtr->CType & CTYPE_ENEMY)
		{
			d = CalcQuickDistance(pt->x,pt->z,thisNodePtr->Coord.x, thisNodePtr->Coord.z);
			if (d < minDist)
			{
				minDist = d;
				best = thisNodePtr;
			}
		}	
		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	*dist = minDist;
	return(best);
}



/************************* IS WATER IN FRONT OF ENEMY *****************************/
//
// coord is in gCoord
//

Boolean	IsWaterInFrontOfEnemy(float r)
{
float	x,z;

	x = gCoord.x - sin(r) * 30.0f;
	z = gCoord.z - cos(r) * 30.0f;

	return(IsXZOverWater(x,z));
}




/****************** DO ENEMY COLLISION DETECT ***************************/
//
// For use by non-skeleton enemies.
//
// OUTPUT: true = was deleted
//

Boolean DoEnemyCollisionDetect(ObjNode *theEnemy, u_long ctype, Boolean useBBoxBottom)
{
float	terrainY,distToFloor,bottomOff;
int		i;

			/* AUTOMATICALLY HANDLE THE BORING STUFF */
			
	HandleCollisions(theEnemy, ctype, -.9);


			/******************************/
			/* SCAN FOR INTERESTING STUFF */
			/******************************/
		
	for (i=0; i < gNumCollisions; i++)						
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			ObjNode	*hitObj = gCollisionList[i].objectPtr;		// get ObjNode of this collision
			ctype = hitObj->CType;

			if (ctype == INVALID_NODE_FLAG)						// see if has since become invalid
				continue;
			
					/* HURT */
	
			if (ctype & CTYPE_HURTENEMY)
			{
				if (theEnemy->HurtCallback != nil)							// if has a hurt callback
					if (theEnemy->HurtCallback(theEnemy, hitObj->Damage))	// handle hit (returns true if was deleted)
						return(true);			
			}
			
				/* TOUCHED PLAYER */
				
			else
			if (ctype & CTYPE_PLAYER)
			{
				EnemyTouchedPlayer(theEnemy, hitObj);
			}
		}
	}
		
		
	
				/* CHECK PARTICLE COLLISION */

	if (theEnemy->HurtCallback != nil)							// if has a hurt callback
	{
		if (ParticleHitObject(theEnemy, PARTICLE_FLAGS_HURTENEMY))
		{
		
			if (theEnemy->HurtCallback(theEnemy, .3))			// handle hit (returns true if was deleted)
				return(true);
		}
	}

			/*************************************/
			/* CHECK & HANDLE TERRAIN  COLLISION */
			/*************************************/

	if (useBBoxBottom)
		bottomOff = theEnemy->BBox.min.y;						// use bbox's bottom
	else
		bottomOff = theEnemy->BottomOff;						// use collision box's bottom
	

	terrainY =  GetTerrainY(gCoord.x, gCoord.z);				// get terrain Y
	distToFloor = (gCoord.y + bottomOff) - terrainY;			// calc amount I'm above or under
	
	if (distToFloor <= 0.0f)									// see if on or under floor
	{
		gCoord.y = terrainY - bottomOff;
		gDelta.y = 0;
		theEnemy->StatusBits |= STATUS_BIT_ONGROUND;	
		
				/* DEAL WITH SLOPES */
				//
				// Using the floor normal here, apply some deltas to it.
				// Only apply slopes when on the ground (or really close to it)
				//
						
//		gDelta.x += gRecentTerrainNormal.x * (gFramesPerSecondFrac * ENEMY_SLOPE_ACCEL);
//		gDelta.z += gRecentTerrainNormal.z * (gFramesPerSecondFrac * ENEMY_SLOPE_ACCEL);		
	}
	return(false);
}


/********************** ENEMY TOUCHED PLAYER ***************************/

void EnemyTouchedPlayer(ObjNode *enemy, ObjNode *player)
{
	player;
	
	switch(enemy->Kind)
	{

	}

}



#pragma mark -

/******************* GET ENEMY HEAD COORD **********************/

void GetEnemyHeadCoord(ObjNode *enemy, OGLPoint3D *coord)
{

	switch(enemy->Kind)
	{
		case	ENEMY_KIND_BANDITO:
				FindCoordOfJoint(enemy, BANDITO_JOINT_HEAD, coord);
				break;
	
		case	ENEMY_KIND_RYGAR:
				FindCoordOfJoint(enemy, RYGAR_JOINT_HEAD, coord);
				break;

		case	ENEMY_KIND_SHORTY:
				FindCoordOfJoint(enemy, SHORTY_JOINT_HEAD, coord);
				break;
	
	
	}
}


/******************* GET ENEMY HAND COORD **********************/

void GetEnemyHandCoord(ObjNode *enemy, OGLPoint3D *coord)
{

	switch(enemy->Kind)
	{
		case	ENEMY_KIND_BANDITO:
				FindCoordOfJoint(enemy, BANDITO_JOINT_RIGHTHAND, coord);
				break;
	
		case	ENEMY_KIND_RYGAR:
				FindCoordOfJoint(enemy, RYGAR_JOINT_RIGHTHAND, coord);
				break;

		case	ENEMY_KIND_SHORTY:
				FindCoordOfJoint(enemy, SHORTY_JOINT_RIGHTHAND, coord);
				break;
	
	
	}
}


/********************* UPDATE ENEMY ATTACHMENTS ********************/

void UpdateEnemyAttachments(ObjNode *theNode)
{
	switch(theNode->Kind)
	{
		case	ENEMY_KIND_BANDITO:
				UpdateBanditoAttachments(theNode);
				break;

		case	ENEMY_KIND_RYGAR:
				UpdateRygarAttachments(theNode);
				break;

		case	ENEMY_KIND_SHORTY:
				UpdateShortyAttachments(theNode);
				break;
	}
}


#pragma mark -

/************************* SHOOT ENEMY BULLET ***************************/

void ShootEnemyBullet(OGLPoint3D *muzzlePt, OGLVector3D *aim)
{
ObjNode	*newObj;
float	speed;
int		i;	

			/**********************/
			/* MAKE BULLET OBJECT */
			/**********************/
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Bullet;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord		= *muzzlePt;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_USEALIGNMENTMATRIX;
	gNewObjectDefinition.slot 		= 228;
	gNewObjectDefinition.moveCall 	= MoveEnemyBullet;		
	gNewObjectDefinition.rot 		= 0;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	SetAlignmentMatrix(&newObj->AlignmentMatrix, aim);

	newObj->Health = 3.0f;										// time that bullet can live
	newObj->Damage = .5;

	newObj->What = WHAT_ENEMYBULLET;
	
				/* SET SPEED OF BULLET */
					
	speed = 2500.0f + RandomFloat2() * 100.0f;					// slightly random bullet speeds
	newObj->Delta.x = aim->x * speed;
	newObj->Delta.y = aim->y * speed;
	newObj->Delta.z = aim->z * speed;


			/* GIVE IT A SHADOW */
			
	AttachShadowToObject(newObj, 0, 1, 1, false);


	PlayEffect3D(EFFECT_GUNSHOT, muzzlePt);		
	
	
	
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
		gSparkles[i].scale = 80.0f;
		gSparkles[i].separation = 0.0f;
		gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
		
		
	}	
		
}



/*********************** MOVE ENEMY BULLET ******************************/

static void MoveEnemyBullet(ObjNode *bullet)
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
						
		
		/* SEE IF BULLET RAY WILL HIT ANYTHING */
		
	target = OGL_DoLineSegmentCollision(&bullet->OldCoord, &gCoord, &impactCoord, &impactNormal, CTYPE_HITENEMYBULLET);				
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


#pragma mark-


/****************** DEC ENEMIES AT STOP POINT **********************/

void DecEnemiesAtStopPoint(void)
{
	gScore += POINTS_SHOOTOUTENEMY;

	gNumEnemiesThisStopPoint[gStopPointNum]--;
	if (gNumEnemiesThisStopPoint[gStopPointNum] <= 0)
	{
			/* SEE IF THAT'S THE END OF THE SWAMP */
			
		if (gCurrentArea == AREA_SWAMP_SHOOTOUT)
		{
			if (gStopPointNum == 7)
			{
				StartLevelCompletion(3.0);
				gLevelWon[gCurrentArea/2] = true;
				return;
			}
		}


				/* LET USER PRESS KEY WHEN READY */
					
		gShootoutCanProceedToNextStopPoint = true;
	}
}





