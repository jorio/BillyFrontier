//
// player.h
//

#ifndef __PLAYER_H__
#define __PLAYER_H__

// Externals
//#include <NetSprocket.h>
#include "game.h"

#define	MAX_BUDDY_BUGS			10

#define	PLAYER_DEFAULT_SCALE	1.3f
#define	DEFAULT_PLAYER_SHADOW_SCALE	3.0f


#define	PLAYER_COLLISION_CTYPE	(CTYPE_MISC|CTYPE_TRIGGER|CTYPE_FENCE|CTYPE_ENEMY|CTYPE_HURTME|CTYPE_PLAYERONLY)

#define	PLAYER_NORMAL_MAX_SPEED	700.0f
#define	MAX_RAMMING_SPEED		(PLAYER_NORMAL_MAX_SPEED * 2.0f)
#define PLAYER_PUSH_MAX_SPEED	200.0f


#define	AMMO_CLIP_SIZE	6			// # bullets in a clip


enum
{
	PLAYER_DEATH_TYPE_TRAMPLED
};


		/* EXPLORE ANIMS */
		
enum
{
	PLAYER_ANIM_STAND,
	PLAYER_ANIM_WALK,
	PLAYER_ANIM_DRAWSHOOT,
	PLAYER_ANIM_SHOTINCHEST,
	PLAYER_ANIM_DRAWSHOOT2,
	PLAYER_ANIM_RUN,
	PLAYER_ANIM_STAMPEDESTART,
	PLAYER_ANIM_STAMPEDEJUMP,
	PLAYER_ANIM_STAMPEDETRAMPLED,
	PLAYER_ANIM_DRAWSHOOT3,
	PLAYER_ANIM_TRIPPED,
	PLAYER_ANIM_DUCK,
	PLAYER_ANIM_DRAWSHOOT4,
	PLAYER_ANIM_STUNNED
};




enum
{
	PLAYER_JOINT_BASE 		= 0,
	PLAYER_JOINT_UPPERBACK 	= 2,
	PLAYER_JOINT_HEAD 		= 3,
	PLAYER_JOINT_LEFTHIP 	= 4,
	PLAYER_JOINT_RIGHTHIP 	= 7,
	PLAYER_JOINT_LEFTHAND 	= 12,
	PLAYER_JOINT_RIGHTHAND	= 15
};



		/***************/
		/* PLAYER INFO */
		/***************/
		
typedef struct
{
	float				startX,startZ;
	float				startRotY;
	
	OGLPoint3D			coord;
	ObjNode				*objNode;
			
	float				distToFloor;
	float				mostRecentFloorY;
		
	float				knockDownTimer;
	float				invincibilityTimer;
	
	float				shieldPower;
	ObjNode				*shieldObj[2];
	float				shieldAlpha;	
	short				shieldChannel;

	int					ammoCount;
	
	OGLRect				itemDeleteWindow;
	
	OGLCameraPlacement	camera;
					

		/* TILE ATTRIBUTE PHYSICS TWEAKS */			
	
	
	float				groundTraction;
	float				groundFriction;
	float				groundAcceleration;
			
	int					waterPatch;	

	
			/* CONTROL INFO */
			
	float				analogControlX,analogControlZ;
	
	
			/* INVENTORY INFO */
			
	SInt8				lives;
	int					pesos;
			
				
}PlayerInfoType;




//=======================================================

void InitPlayerInfo_Game(void);
void InitPlayerForDuel(void);
void InitPlayerForShootout(void);
void InitPlayerForStampede(void);
void InitPlayerGlobals(void);

void KillPlayer(Byte deathType);
Boolean IsPlayerDoingWalkAnim(ObjNode *theNode);
Boolean IsPlayerDoingStandAnim(ObjNode *theNode);
void UpdatePlayerShield(void);
void PingShield(float damage);


ObjNode *CreateBilly(OGLPoint3D *where, float rotY);
void SetPlayerStandAnim(ObjNode *theNode, float speed);
void SetPlayerWalkAnim(ObjNode *theNode);

		/* BILLY */
		
void UpdateBillyAttachments(ObjNode *player);
void UpdateBilly(ObjNode *theNode);
Boolean DoBillyCollisionDetect(ObjNode *theNode, Boolean useBBoxForTerrain);
void BillyPutGunsInHands(ObjNode *player);
void CalcGunMatrixFromJointMatrix(ObjNode *gun, OGLMatrix4x4 *jointMatrix, OGLMatrix4x4 *gunMatrix);
		


#endif