/****************************/
/*   ENEMY: WALKER.C		*/
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

static void AlignPods(ObjNode *walker);
static void MoveWalkerOnSpline(ObjNode *theNode);
static void WalkerShoots(ObjNode *walker);
static void StartWalkerShootAnim(ObjNode *walker);
static void ShootWalkerMissile(OGLPoint3D *coord, OGLVector3D *aim);
static void MoveWalkerMissile(ObjNode *theNode);
static void WalkerHitByBullet(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt);
static void WalkerMissileHitByBullet(ObjNode *bullet, ObjNode *missile, const OGLPoint3D *impactPt);
static void ExplodeMissile(ObjNode *missile);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	WALKER_SCALE	2.0f

enum
{
	WALKER_JOINT_BODY = 7

};


enum
{
	WALKER_ANIM_STAND = 0,
	WALKER_ANIM_WALK,
	WALKER_ANIM_TURN180,
	WALKER_ANIM_SHOOTLEFT,
	WALKER_ANIM_SHOOTRIGHT,
	WALKER_ANIM_SHOOTSTRAIGHT,
	WALKER_ANIM_DEATH
};


#define	MISSILE_SPEED	500.0f

/*********************/
/*    VARIABLES      */
/*********************/


#define	ShootTimer		SpecialF[0]
#define	Turn180Timer	SpecialF[1]
#define	ReverseSpline	Flag[3]
#define	ShootNow		Flag[0]


#define	MissileTurnSpeed	SpecialF[0]

const OGLVector3D gForward = {0,0,-1};



/************************ PRIME WALKER *************************/

Boolean PrimeWalker(long splineNum, SplineItemType *itemPtr)
{
ObjNode	*newObj, *leftPod, *rtPod;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/************************/
				/* MAKE WALKER SKELETON */
				/************************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_WALKER, WALKER_ANIM_WALK, x,z, WALKER_SCALE, 0, nil, 0);
				
			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;			
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveWalkerOnSpline;					// set move call			
	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);


	newObj->CType = CTYPE_PICKABLE;
	newObj->HitByBulletCallback = WalkerHitByBullet;


	newObj->ShootTimer = 2.0f;
	newObj->Turn180Timer = 10.0f;
	newObj->ReverseSpline = false;
	newObj->ShootNow = false;

	newObj->StopPoint = itemPtr->parm[1];				// remember stop point #
	
	newObj->Health = 20.0f;
	
	
			/***************************/
			/* ATTACH THE MISSILE PODS */
			/***************************/

				/* LEFT POD */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_WalkerPodLeft;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	leftPod = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	leftPod->CType = CTYPE_PICKABLE;
	leftPod->HitByBulletCallback = WalkerHitByBullet;


				/* RIGHT POD */
				
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_WalkerPodRight;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	rtPod = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	rtPod->CType = CTYPE_PICKABLE;
	rtPod->HitByBulletCallback = WalkerHitByBullet;


	newObj->ChainNode = leftPod;
	leftPod->ChainHead = newObj;
	
	leftPod->ChainNode = rtPod;
	rtPod->ChainHead = leftPod;


	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 10, 10,false);

	return(true);
}


/******************** MOVE WALKER ON SPLINE ***************************/

static void MoveWalkerOnSpline(ObjNode *theNode)
{
Boolean 	isInRange; 
float		fps = gFramesPerSecondFrac;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility


again:
	switch(theNode->Skeleton->AnimNum)
	{
					/*************************/
					/* MOVE ALONG THE SPLINE */
					/*************************/

		case	WALKER_ANIM_WALK:
				if (gStopPointNum != theNode->StopPoint)		// dont do anything until we're there
					break;
					
				if (isInRange)
				{
					if (theNode->ReverseSpline)
						IncreaseSplineIndex(theNode, -200, false);
					else
						IncreaseSplineIndex(theNode, 200, false);
										
					GetObjectCoordOnSpline(theNode);

							/* SEE IF TIME TO TURN AROUND */
							
					theNode->Turn180Timer -= fps;
					if (theNode->Turn180Timer <= 0.0f)
					{
						theNode->Turn180Timer = 4.0f + RandomFloat() * 20.0f;					// set new random delay for next turnaround
						MorphToSkeletonAnim(theNode->Skeleton, WALKER_ANIM_TURN180, 8);
					}
							/* SEE IF TIME TO SHOOT */
							
					else
					{
						theNode->ShootTimer -= fps;
						if (theNode->ShootTimer <= 0.0f)
						{
							OGLPoint3D	pt;
							OGLVector3D	face;

									/* MAKE SURE NOT BEING BLOCKED */
									
							if (!OGL_DoLineSegmentCollision(&gPlayerInfo.coord, &theNode->Coord, &pt, &face, CTYPE_BUILDING))
							{
								theNode->ShootTimer = 3.0f + RandomFloat() * 5.0f;					
								StartWalkerShootAnim(theNode);
							}
							else
								theNode->ShootTimer = .5f;							
						}
					}
				}
				break;
				

				/* TURNING 180 */
				
		case	WALKER_ANIM_TURN180:
				if (theNode->Skeleton->AnimHasStopped)					// when done with turning, immediately set to walk anim
				{
					SetSkeletonAnim(theNode->Skeleton, WALKER_ANIM_WALK);
					theNode->ReverseSpline = !theNode->ReverseSpline;
					goto again;
				}
				break;
				

				/* SHOOTING */
				
		case	WALKER_ANIM_SHOOTLEFT:
		case	WALKER_ANIM_SHOOTRIGHT:
		case	WALKER_ANIM_SHOOTSTRAIGHT:
				if (theNode->ShootNow)
				{
					theNode->ShootNow = false;
					WalkerShoots(theNode);
				}
				if (theNode->Skeleton->AnimHasStopped)
				{
					MorphToSkeletonAnim(theNode->Skeleton, WALKER_ANIM_WALK, 7);				
				}
				break;
				
				
				/* DEATH */
				
		case	WALKER_ANIM_DEATH:
				break;
	}


			/****************************/
			/* UPDATE STUFF IF IN RANGE */
			/****************************/
			
	if (isInRange)
	{		
				/* ALIGN ON SPLINE */
				
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BBox.min.y;	// offset so bottom touches ground
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);
		
		AlignPods(theNode);
		
		
				/* UPDATE AUDIO */
				
		if (gStopPointNum == theNode->StopPoint)		// dont do anything until we're there
		{					
			if (theNode->EffectChannel == -1)
				theNode->EffectChannel = PlayEffect3D(EFFECT_WALKERAMBIENT, &theNode->Coord);
			else
				Update3DSoundChannel(EFFECT_WALKERAMBIENT, &theNode->EffectChannel, &theNode->Coord);
		}		
		
	}	
}



#pragma mark -

/**************** ALIGN PODS ******************/

static void AlignPods(ObjNode *walker)
{
ObjNode	*rtPod, *leftPod;
const OGLPoint3D	leftOff = {-26, 35, 21};
const OGLPoint3D	rightOff = {26, 35, 21};
OGLMatrix4x4		m, tm;

	leftPod = walker->ChainNode;
	rtPod = leftPod->ChainNode;


			/* ALIGN LEFT POD */
			
	OGLMatrix4x4_SetTranslate(&tm, leftOff.x, leftOff.y, leftOff.z);
	FindJointFullMatrix(walker, WALKER_JOINT_BODY, &m);	
	OGLMatrix4x4_Multiply(&tm, &m, &leftPod->BaseTransformMatrix);
	SetObjectTransformMatrix(leftPod);
	leftPod->Coord.x = leftPod->BaseTransformMatrix.value[M03];			// extract coords
	leftPod->Coord.y = leftPod->BaseTransformMatrix.value[M13];
	leftPod->Coord.z = leftPod->BaseTransformMatrix.value[M23];


			/* ALIGN RIGHT POD */
			
	OGLMatrix4x4_SetTranslate(&tm, rightOff.x, rightOff.y, rightOff.z);
	FindJointFullMatrix(walker, WALKER_JOINT_BODY, &m);	
	OGLMatrix4x4_Multiply(&tm, &m, &rtPod->BaseTransformMatrix);
	SetObjectTransformMatrix(rtPod);
	rtPod->Coord.x = rtPod->BaseTransformMatrix.value[M03];			// extract coords
	rtPod->Coord.y = rtPod->BaseTransformMatrix.value[M13];
	rtPod->Coord.z = rtPod->BaseTransformMatrix.value[M23];


}


/******************* START WALKER SHOOT ANIM ***********************/

static void StartWalkerShootAnim(ObjNode *walker)
{
OGLVector3D	aim, toPlayer, cross;
float		angle;
			


	walker->ShootNow = false;	

			/* DETERMINE IF NEED TO TURN LEFT OR RIGHT */
			
	OGLVector3D_Transform(&gForward, &walker->BaseTransformMatrix, &aim);
	
	toPlayer.x = gPlayerInfo.coord.x - walker->Coord.x;										// vector to player
	toPlayer.y = gPlayerInfo.coord.y - walker->Coord.y;
	toPlayer.z = gPlayerInfo.coord.z - walker->Coord.z;	
	OGLVector3D_Normalize(&toPlayer, &toPlayer);
	
				/* SEE IF SHOOT TO SIDE OR STRAIGHT */
				
	angle = acos(OGLVector3D_Dot(&aim, &toPlayer));
	if (angle > (PI/4.0f))
	{	
					/* SEE WHICH SIDE TO DO */
					
		OGLVector3D_Cross(&aim, &toPlayer, &cross);
		if (cross.y < 0.0f)
			MorphToSkeletonAnim(walker->Skeleton, WALKER_ANIM_SHOOTRIGHT, 4);
		else
			MorphToSkeletonAnim(walker->Skeleton, WALKER_ANIM_SHOOTLEFT, 4);
	}
	
			/* STRAIGHT */
	else
	{
		MorphToSkeletonAnim(walker->Skeleton, WALKER_ANIM_SHOOTSTRAIGHT, 4);	
	}
}


/******************** WALKER SHOOTS **************************/

static void WalkerShoots(ObjNode *walker)
{
ObjNode			*rtPod, *leftPod;
OGLVector3D		aim;



			/* GET PODS */
			
	leftPod = walker->ChainNode;
	rtPod = leftPod->ChainNode;



			/**************/
			/* SHOOT LEFT */
			/**************/

	OGLVector3D_Transform(&gForward, &leftPod->BaseTransformMatrix, &aim);
	ShootWalkerMissile(&leftPod->Coord, &aim);
	
}



/****************** SHOOT WALKER MISSILE *******************/

static void ShootWalkerMissile(OGLPoint3D *coord, OGLVector3D *aim)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_Missile;
	gNewObjectDefinition.scale 		= WALKER_SCALE / 4.0f;
	gNewObjectDefinition.coord		= *coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_USEALIGNMENTMATRIX;
	gNewObjectDefinition.slot 		= 494;
	gNewObjectDefinition.moveCall 	= MoveWalkerMissile;
	gNewObjectDefinition.rot 		= 0;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	SetAlignmentMatrix(&newObj->AlignmentMatrix, aim);

	newObj->Health = 6.0f;
	newObj->CType = CTYPE_PICKABLE;
	newObj->HitByBulletCallback = WalkerMissileHitByBullet;

	newObj->Damage = 3.0f;
	

				/* SET SPEED OF MISSILE */
					
	newObj->Speed3D = 400;				// movement speed
	newObj->MissileTurnSpeed = 0;		// turn speed
	

			/* GIVE IT A SHADOW */
			
//	AttachShadowToObject(newObj, 0, 1, 4, false);

	PlayEffect3D(EFFECT_LAUNCHMISSILE, coord);

}	


/**************** MOVE WALKER MISSILE **********************/

static void MoveWalkerMissile(ObjNode *theNode)
{
float				fps = gFramesPerSecondFrac;
OGLVector3D			missileAim, toPlayer, cross, face;
float				angle, speed, dist;
OGLMatrix4x4		m;
OGLPoint3D			pt;

	GetObjectInfo(theNode);

			/* SEE IF TIMED OUT */
			
	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

		/**********************/
		/* TURN TOWARD TARGET */
		/**********************/

			/* INC MISSILE TURN SPEED CAPACITY */
			
	theNode->MissileTurnSpeed += fps * 3.0f;
	if (theNode->MissileTurnSpeed > 6.0f)
		theNode->MissileTurnSpeed = 6.0f;


	OGLVector3D_Transform(&gForward, &theNode->AlignmentMatrix, &missileAim);			// get missile direction vector	

	toPlayer.x = gPlayerInfo.coord.x - gCoord.x;										// vector to player
	toPlayer.y = (gPlayerInfo.coord.y + 50.0f) - gCoord.y;
	toPlayer.z = gPlayerInfo.coord.z - gCoord.z;	
	OGLVector3D_Normalize(&toPlayer, &toPlayer);

	OGLVector3D_Cross(&missileAim, &toPlayer, &cross);									// get perpendicular axis to rotate on		
	angle = acos(OGLVector3D_Dot(&missileAim, &toPlayer));								// dot tells us how far to rotate
		
	OGLMatrix4x4_SetRotateAboutAxis(&m, &cross, angle * fps *theNode->MissileTurnSpeed);// create rotation matrix

	
	OGLMatrix4x4_Multiply(&theNode->AlignmentMatrix,&m,  &theNode->AlignmentMatrix);	// do the rotation
		


			/* ACCELERATE MISSILE */
			
	OGLVector3D_Transform(&gForward, &theNode->AlignmentMatrix, &missileAim);			// get new missile direction vector	
			
	theNode->Speed3D += fps * 1000.0f;
	if (theNode->Speed3D > MISSILE_SPEED)
		theNode->Speed3D = MISSILE_SPEED;
	

			/* MOVE IT */
		
	speed = theNode->Speed3D * fps;
		
	gCoord.x += missileAim.x * speed;
	gCoord.y += missileAim.y * speed;
	gCoord.z += missileAim.z * speed;



			/* SEE IF HIT SCENE */
			
	if (OGL_DoLineSegmentCollision(&theNode->OldCoord, &gCoord, &pt, &face, CTYPE_BUILDING))
	{
		ExplodeMissile(theNode);
		return;
	}	
	
	
		/********************/
		/* SEE IF HIT BILLY */
		/********************/

	dist = OGLPoint3D_Distance(&gCoord, &gPlayerInfo.coord);
	if (dist < 80.0f)
	{
		ShootoutPlayerHitByBulletCallback(theNode, gPlayerInfo.objNode, nil);	
		ExplodeMissile(theNode);
		MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_STUNNED, 15);
		return;
	}


	UpdateObject(theNode);
}


/******************* WALKER MISSILE HIT BY BULLET **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void WalkerMissileHitByBullet(ObjNode *bullet, ObjNode *missile, const OGLPoint3D *impactPt)
{
	(void) bullet;

	PlayEffect3D(EFFECT_BULLETHITMETAL, impactPt);			

	ExplodeMissile(missile);
}


/********************* EXPLODE MISSILE *************************/

static void ExplodeMissile(ObjNode *missile)
{
//	MakeSparkExplosion(missile->Coord.x, missile->Coord.y, missile->Coord.z,
//					 400, 1.0f, PARTICLE_SObjType_WhiteSpark4, 100, .5);

	MakeFireExplosion(&missile->Coord);
	PlayEffect3D(EFFECT_EXPLOSION, &missile->Coord);
	
	ExplodeGeometry(missile, 300.0, 0, 1, 1.0);
	DeleteObject(missile);	



}


#pragma mark -

/******************* WALKER HIT BY BULLET **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void WalkerHitByBullet(ObjNode *bullet, ObjNode *enemy, const OGLPoint3D *impactPt)
{
ObjNode	*walker;

	(void) bullet;

				/* FIND THE WALKER HEAD OBJ */
				
	walker = enemy;
	while(walker->ChainHead)
		walker = walker->ChainHead;
				
				
	PlayEffect3D(EFFECT_BULLETHITMETAL, impactPt);	
	
	
			/* HURT IT */
			
	if (walker->Skeleton->AnimNum != WALKER_ANIM_DEATH)			// is already dead?
	{
		walker->Health -= 1.0f;
		if (walker->Health <= 0.0f)
		{
			MorphToSkeletonAnim(walker->Skeleton, WALKER_ANIM_DEATH, 4);
			walker->Skeleton->AnimSpeed = .2f;
			StartLevelCompletion(5.0);
			MarkLevelWon(gCurrentArea / 2);
		}	
	}	
}















