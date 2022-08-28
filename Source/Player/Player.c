/****************************/
/*   	PLAYER.C   			*/
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

static void MoveShieldSphere(ObjNode *theNode);
static void BulletHitShieldCallback(ObjNode *bullet, ObjNode *shieldObj, const OGLPoint3D *impactPt);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/


PlayerInfoType	gPlayerInfo;

float	gDeathTimer = 0;


Boolean	gPlayerIsDead = false;



/******************** INIT PLAYER INFO ***************************/
//
// Called once at beginning of game
//

void InitPlayerInfo_Game(void)
{
			/* INIT SOME THINGS IF NOT LOADING SAVED GAME */

	if (!gPlayingFromSavedGame)
	{
		gLevelWonMask				= 0;			// no levels completed yet
		gDuelWonMask				= 0;
		gPlayerInfo.lives			= 3;
		gPlayerInfo.pesos			= 0;
		gScore						= 0;
	}

	gDeathTimer = 0;

	gPlayerInfo.startX 			= 0;
	gPlayerInfo.startZ 			= 0;
	gPlayerInfo.coord.x 		= 0;
	gPlayerInfo.coord.y 		= 0;
	gPlayerInfo.coord.z 		= 0;
}




/******************* INIT PLAYER FOR DUEL ********************/

void InitPlayerForDuel(void)
{
ObjNode	*player;

		/* FIRST PRIME THE TERRAIN TO CAUSE ALL OBJECTS TO BE GENERATED BEFORE WE PUT THE PLAYER DOWN */
			
	InitCurrentScrollSettings();
	DoPlayerTerrainUpdate(gPlayerInfo.coord.x, gPlayerInfo.coord.z);					

	
			/* THEN CREATE THE PLAYER */
			
	player = CreateBilly(&gPlayerInfo.coord, gPlayerInfo.startRotY);
	player->MoveCall = MovePlayer_Duel;
	
	SetPlayerWalkAnim(player);
	player->Skeleton->AnimSpeed = .7;

	InitPlayerGlobals();
}


/******************* INIT PLAYER FOR SHOOTOUT ********************/

void InitPlayerForShootout(void)
{
ObjNode	*player;

		/* FIRST PRIME THE TERRAIN TO CAUSE ALL OBJECTS TO BE GENERATED BEFORE WE PUT THE PLAYER DOWN */
			
	InitCurrentScrollSettings();
	DoPlayerTerrainUpdate(gPlayerInfo.coord.x, gPlayerInfo.coord.z);					

	
			/* THEN CREATE THE PLAYER */
			
	player = CreateBilly(&gPlayerInfo.coord, gPlayerInfo.startRotY);
	
	player->MoveCall = MovePlayer_Shootout;
		
	player->SplineNum = 0;
	player->SplinePlacement = 0.0;

	InitPlayerGlobals();
	
//	SetObjectCollisionBounds(player, player->BBox.max.y, gPlayerBottomOff = player->BBox.min.y, -30, 30, 30, -30);
	

			/* SPECIAL HIDE */
			//
			// Since we want enemy bullets to hit us we cannot just hide the player.
			// instead, we'll set the transparency to 0.0 which will halt the rendering
			//
					
	player->ColorFilter.a = 0.0;
	
	SetPlayerWalkAnim(player);


		/* SET GOT-SHOT CALLBACK */
			
	player->HitByBulletCallback = ShootoutPlayerHitByBulletCallback;

	CreateCollisionBoxFromBoundingBox(player, 1,1);

}


/******************* INIT PLAYER FOR STAMPEDE ********************/

void InitPlayerForStampede(void)
{
ObjNode	*player;

		/* FIRST PRIME THE TERRAIN TO CAUSE ALL OBJECTS TO BE GENERATED BEFORE WE PUT THE PLAYER DOWN */
			
	InitCurrentScrollSettings();
	DoPlayerTerrainUpdate(gPlayerInfo.coord.x, gPlayerInfo.coord.z);					

	
			/* THEN CREATE THE PLAYER */
			
	player = CreateBilly(&gPlayerInfo.coord, gPlayerInfo.startRotY);
	SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAMPEDESTART);
	player->Timer = 2.1;								// time to stay on the start anim before beginning the running
	
	player->MoveCall = MovePlayer_Stampede;
	
	player->BoundingSphereRadius *= .33f;				// tweak the radius for fence collision
	
	InitPlayerGlobals();
	
	gCurrentMaxSpeed = gTargetMaxSpeed = PLAYER_STAMPEDE_SPEED;
	
}



/******************** INIT PLAYER GLOBALS ************************/
//
// common init stuff for all Init calls above
//

void InitPlayerGlobals(void)
{
	gPlayerInfo.ammoCount		= AMMO_CLIP_SIZE * 12;

	gPlayerInfo.invincibilityTimer = 0;
		
	gPlayerInfo.shieldPower = MAX_SHIELD;
	gPlayerInfo.shieldObj[0] = nil;
	gPlayerInfo.shieldObj[1] = nil;
	gPlayerInfo.shieldChannel = -1;
	gPlayerInfo.shieldAlpha = 0;
	
	gPlayerIsDead = false;

	gFreezeCameraFromY 		= false;					// assume no camera freeze
	gFreezeCameraFromXZ		= false;

}


#pragma mark -




/****************** KILL PLAYER *************************/

void KillPlayer(Byte deathType)
{
ObjNode	*player = gPlayerInfo.objNode;


		/* VERIFY ANIM IF ALREADY DEAD */
		//
		// This should assure us that we don't get kicked out of our death anim accidentally
		//
		
	if (gPlayerIsDead)						// see if already dead
	{
		return;
	}
		
		
			/* KILL US NOW */
			
	gPlayerIsDead = true;
	player->Health = 0;					// make sure this is set correctly

	switch(deathType)
	{
		case	PLAYER_DEATH_TYPE_TRAMPLED:
				MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAMPEDETRAMPLED, 8);
				gDeathTimer = gPlayerInfo.invincibilityTimer = 3.0f;
				break;

	}
}




#pragma mark -

/*********************** SET PLAYER WALK ANIM *******************************/

void SetPlayerWalkAnim(ObjNode *theNode)
{
	MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_WALK, 9);
}


/********************* IS PLAYER DOING WALK ANIM *********************/

Boolean IsPlayerDoingWalkAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_WALK)
		return(true);

	return(false);
}

/*********************** SET PLAYER STAND ANIM *******************************/

void SetPlayerStandAnim(ObjNode *theNode, float speed)
{
	MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_STAND, speed);
		
	theNode->Timer = 2.0f + RandomFloat() * 5.0f;			// set random personality delay timer (see below)
}


/********************* IS PLAYER DOING STAND ANIM *********************/

Boolean IsPlayerDoingStandAnim(ObjNode *theNode)
{
	switch(theNode->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_STAND:
				return(true);

		default:
				return(false);
	}
}







#pragma mark -


/********************** UPDATE PLAYER SHIELD ***********************/
//
// Called from player update function to see if need to maintain shield.
//

void UpdatePlayerShield(void)
{
int		i;
ObjNode *newObj;
	
			/********************************/
			/* SEE IF SHIELD SHOULD BE GONE */
			/********************************/
			
	if (gPlayerInfo.shieldPower <= 0.0f)
	{
		gPlayerInfo.shieldPower = 0.0f;
		StopAChannel(&gPlayerInfo.shieldChannel);

		if (gPlayerInfo.shieldObj[0])
		{
			DeleteObject(gPlayerInfo.shieldObj[0]);
			gPlayerInfo.shieldObj[0] = nil;			
		}
		if (gPlayerInfo.shieldObj[1])
		{
			DeleteObject(gPlayerInfo.shieldObj[1]);
			gPlayerInfo.shieldObj[1] = nil;			
		}
		return;
	}
	
		
		/* UPDATE SOUND */
		
//	if (gPlayerInfo.shieldChannel == -1)
//		gPlayerInfo.shieldChannel = PlayEffect(EFFECT_SHIELD);	

		/*****************************/
		/* MAKE SURE WE HAVE SPHERES */
		/*****************************/

	for (i = 0; i < 2; i++)
	{
		if (gPlayerInfo.shieldObj[i] == nil)
		{
			gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
			gNewObjectDefinition.type 		= GLOBAL_ObjType_Shield;
			gNewObjectDefinition.scale 		= 1.2f;
			gNewObjectDefinition.coord	 	= gPlayerInfo.coord;
			gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_GLOW |
											 STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_DOUBLESIDED;
			gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 1;
			gNewObjectDefinition.moveCall 	= MoveShieldSphere;
			gNewObjectDefinition.rot 		= 0;	
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
			gPlayerInfo.shieldObj[i] = newObj;
			
			
			
			
			if (i == 0)
			{
				newObj->CType = CTYPE_HITENEMYBULLET;
				newObj->HitByBulletCallback = BulletHitShieldCallback;
			
				newObj->ColorFilter.r = 1.0f;
				newObj->ColorFilter.g = .3f;
				newObj->ColorFilter.b = 1.0f;
				
				newObj->Scale.x =
				newObj->Scale.y =
				newObj->Scale.z = newObj->Scale.x * 1.06f;

				newObj->Rot.y = PI;	
				newObj->DeltaRot.y = -3.5;
			}
			else
			{
				newObj->ColorFilter.r = 1;
				newObj->ColorFilter.g = .8;
				newObj->ColorFilter.b = .6;			

				newObj->Rot.y = 0;	
				newObj->DeltaRot.y = 3.5;

			}
		}
		gPlayerInfo.shieldObj[i]->ColorFilter.a = gPlayerInfo.shieldAlpha;		
	}			

			/* UPDATE ALPHA */
			
	gPlayerInfo.shieldAlpha -= gFramesPerSecondFrac * 1.6f;							// fade out after "ping'
	if (gPlayerInfo.shieldAlpha < 0.0f)
		gPlayerInfo.shieldAlpha = 0.0f;
		
		
}


/********************* MOVE SHIELD SPHERE ****************************/

static void MoveShieldSphere(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*player = gPlayerInfo.objNode;
	

	theNode->Rot.y += fps * theNode->DeltaRot.y;
	theNode->Rot.x += fps * .1f;

	theNode->Coord.x = player->Coord.x + (player->BBox.max.x + player->BBox.min.x) * .5f;
	theNode->Coord.y = player->Coord.y + (player->BBox.max.y + player->BBox.min.y) * .5f;
	theNode->Coord.z = player->Coord.z + (player->BBox.max.z + player->BBox.min.z) * .5f;

	UpdateObjectTransforms(theNode);
}

/****************** BULLET HIT SHIELD CALLBACK ***********************/

static void BulletHitShieldCallback(ObjNode *bullet, ObjNode *shieldObj, const OGLPoint3D *impactPt)
{
	(void) bullet;
	(void) shieldObj;

	MakePuff(impactPt, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);			
	PingShield(bullet->Damage);
}


/******************* PING SHIELD ******************************/
//
// Called when something hits the shield.  This makes it visible momentarily.
// 

void PingShield(float damage)
{
	PlayEffect(EFFECT_SHIELDHIT);
	gPlayerInfo.shieldAlpha = .8f;

			/* LOSE SHIELD POWER */
			
	gPlayerInfo.shieldPower -= damage * 2.0f;
	if (gPlayerInfo.shieldPower < 0.0f)
		gPlayerInfo.shieldPower = 0.0f;

}


















