/****************************/
/*   ENEMY: TREMORGHOST.C	*/
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

static void MoveTremorGhost(ObjNode *enemy);
static void MoveTremorGrave(ObjNode *theNode);
static void ShootoutTremorGhostHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt);

static ObjNode *MakeTremorGhost(OGLPoint3D *coord, float rot);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_TREMORGHOSTS				8

#define	TREMORGHOST_SCALE				1.1f

#define	MAX_TREMORGHOST_SPEED			200.0f

enum
{
	TREMORGHOST_JOINT_RIGHTHIP = 0,
	TREMORGHOST_JOINT_RIGHTHAND = 0
};






/*********************/
/*    VARIABLES      */
/*********************/

#define	GraveHasGhosted		Flag[0]
#define	TargetTimer			SpecialF[0]

#define	GhostSwatting		Flag[0]


/************************* ADD TREMOR GRAVE *********************************/

Boolean AddTremorGrave(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
short	stopPoint 	= itemPtr->parm[1];

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= SWAMP_ObjType_Grave;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 188;
	gNewObjectDefinition.moveCall 	= MoveTremorGrave;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list
	newObj->StopPoint 		= stopPoint;

	CalcObjectBoxFromNode(newObj);

	newObj->CType = CTYPE_PICKABLE|CTYPE_HITENEMYBULLET;

	newObj->Timer = 1.0f + RandomFloat() * 3.0f;

	newObj->GraveHasGhosted = false;								// no ghost has risen yet from this grave

	return(true);													// item was added
}


/******************* MOVE TREMOR GRAVE *************************/

static void MoveTremorGrave(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (theNode->GraveHasGhosted)							// if already let loose a ghost then bail
		return;

			/* SEE IF MAKE GHOST */

	if (gShootoutMode == SHOOTOUT_MODE_BATTLE)					// only attack once in battle mode (not still walking)
	{			
		if (theNode->StopPoint == gStopPointNum)					// is this thing active @ this stop point?
		{
			if (gNumEnemyOfKind[ENEMY_KIND_TREMORGHOST] < 2)
			{
				theNode->Timer -= gFramesPerSecondFrac;
				if (theNode->Timer <= 0.0f)
				{
					ObjNode *enemy;
					
					enemy = MakeTremorGhost(&theNode->Coord, theNode->Rot.y);
					enemy->StopPoint = theNode->StopPoint;
					
					theNode->GraveHasGhosted = true;
				}
			}
		}
	}
}



/************************* MAKE TREMORGHOST ****************************/

static ObjNode *MakeTremorGhost(OGLPoint3D *coord, float rot)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_TREMORGHOST,TREMORGHOST_ANIM_RISE, coord->x, coord->z,
								TREMORGHOST_SCALE, rot, MoveTremorGhost,
								gAutoFadeStatusBits | STATUS_BIT_GLOW | STATUS_BIT_NOZWRITES | STATUS_BIT_DOUBLESIDED);
	

	newObj->Skeleton->AnimSpeed = .3f;

	newObj->Coord.y = coord->y + 140.0f;
	UpdateObjectTransforms(newObj);


				/* SET BETTER INFO */

	newObj->HitByBulletCallback = ShootoutTremorGhostHitByBulletCallback;

	newObj->Health 		= 2.5f;
	
	newObj->Kind 		= ENEMY_KIND_TREMORGHOST;
	newObj->CType		|= CTYPE_PICKABLE;
	newObj->Damage 		= .2f;
	
	newObj->ColorFilter.a = .01f;
	
	newObj->GhostSwatting = false;
	

				/* SET COLLISION INFO */
								
	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	CalcNewTargetOffsets(newObj, 200.0f);
	


				

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_TREMORGHOST]++;
								
	return(newObj);

}




/*********************** MOVE SHOOTOUT TREMORGHOST ***************************/

static void MoveTremorGhost(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;
float	speed, r, y, dist, angle;

	GetObjectInfo(enemy);

	enemy->ColorFilter.a += fps * .5f;						// fade in
	if (enemy->ColorFilter.a > .5f)
		enemy->ColorFilter.a = .5f;


	switch(enemy->Skeleton->AnimNum)
	{
		case	TREMORGHOST_ANIM_RISE:					
				if (enemy->Skeleton->AnimHasStopped)			// done rising?
					MorphToSkeletonAnim(enemy->Skeleton, TREMORGHOST_ANIM_FLOAT, 3);
			
				gCoord.y += fps * 50.0f;
				break;
				
				
				/************/
				/* FLOATING */
				/************/
				
		case	TREMORGHOST_ANIM_FLOAT:
		case	TREMORGHOST_ANIM_ATTACK:
		case	TREMORGHOST_ANIM_GOTHIT:

					/* SEE IF CHANGE TARGET OFFSET */
					
				enemy->TargetTimer -= fps;
				if (enemy->TargetTimer <= 0.0f)
				{
					enemy->TargetTimer = .5f + RandomFloat();
					CalcNewTargetOffsets(enemy, 175.0f);
				}

		
						/* ACCEL */
						
				enemy->Speed2D += fps * 100.0f;
				if (enemy->Speed2D > MAX_TREMORGHOST_SPEED)
					enemy->Speed2D = MAX_TREMORGHOST_SPEED;
				
						/* TURN TO PLAYER */
						
				angle = TurnObjectTowardTarget(enemy, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 1.5, true);					
				
						/* MOVE */
						
				r = enemy->Rot.y;
				speed = enemy->Speed2D;
				gDelta.x = -sin(r) * speed;
				gDelta.z = -cos(r) * speed;
				
				gCoord.x += gDelta.x * fps;
				gCoord.z += gDelta.z * fps;
				
				y = GetTerrainY(gCoord.x, gCoord.z) + 100.0f;										// drift down to terrain
				if (gCoord.y > y)
					gCoord.y -= fps * 70.0f;
					
					
						/* SEE IF HURT PLAYER NOW */

				dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
											
				if (enemy->Skeleton->AnimNum == TREMORGHOST_ANIM_ATTACK)
				{
					if (enemy->GhostSwatting)														// can ghost hurt us?
					{
						if (dist < 200.0f)
						{
							enemy->GhostSwatting = false;											// don try again until next anim loop						
							ShootoutPlayerHitByBulletCallback(enemy, gPlayerInfo.objNode, nil);	// hurt player				
							MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_STUNNED, 15);
						}
					}
				}
					/* SEE IF START ATTACK */

				else
				{
					if ((dist < 300.0f) && (angle < (PI/2)))
					{
						MorphToSkeletonAnim(enemy->Skeleton, TREMORGHOST_ANIM_ATTACK, 2);				
						enemy->GhostSwatting = false;
						PlayEffect3D(EFFECT_SWISH, &enemy->Coord);
					}
				}
				
				break;	
	}


			/* UPDATE */
			
	UpdateObject(enemy);
}




/******************* SHOOTOUT TREMORGHOST HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void ShootoutTremorGhostHitByBulletCallback(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt)
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
		MorphToSkeletonAnim(enemy->Skeleton, TREMORGHOST_ANIM_GOTHIT, 10);
		
		enemy->Health -= 1.0f;
		if (enemy->Health < 0.0f)
		{		
			enemy->EnemyIsDead = true;

			PlayEffect_Parms3D(EFFECT_GHOSTVAPORIZE, impactPt, NORMAL_CHANNEL_RATE, 6.0f);			
			ExplodeGeometry(enemy, 300.0, SHARD_MODE_FROMORIGIN, 1, 1.0);
			DeleteEnemy(enemy);


				/* DEC COUNTER & SEE IF READY TO PROCEED TO NEXT STOP POINT */
							
			DecEnemiesAtStopPoint();
		}
	}
	
}












