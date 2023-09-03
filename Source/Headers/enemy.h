//
// enemy.h
//

// Externals
#include "game.h"

#include "terrain.h"
#include "splineitems.h"


#define	DEFAULT_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_HURTENEMY|CTYPE_ENEMY|CTYPE_TRIGGER2|CTYPE_FENCE|CTYPE_PLAYER)
#define	DEATH_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_ENEMY|CTYPE_FENCE)

#define ENEMY_GRAVITY		3500.0f
#define	ENEMY_SLOPE_ACCEL		3000.0f


		/* ENEMY KIND */
		
enum
{
	ENEMY_KIND_BANDITO,
	ENEMY_KIND_RYGAR,
	ENEMY_KIND_SHORTY,
	ENEMY_KIND_TREMORALIEN,
	ENEMY_KIND_TREMORGHOST,
	ENEMY_KIND_FROGMAN,
	NUM_ENEMY_KINDS
};

	

enum
{
	KANGA_ANIM_STAND = 0,
	KANGA_ANIM_STAMPEDE,
	KANGA_ANIM_FLAIL

};


#define StopPoint		Special[0]		
#define	ActionType	Special[1]
#define	ShootNow	Flag[0]
#define	EnemyIsDead	Flag[4]



//=====================================================================
//=====================================================================
//=====================================================================


			/* ENEMY */
			
ObjNode *MakeEnemySkeleton(Byte skeletonType, short animNum, float x, float z, float scale, float rot, movecall_t moveCall, uint32_t flags);
extern	void DeleteEnemy(ObjNode *theEnemy);
Boolean DoEnemyCollisionDetect(ObjNode *theEnemy, uint32_t ctype, Boolean useBBoxBottom);
void EnemyTouchedPlayer(ObjNode *enemy, ObjNode *player);
extern	void UpdateEnemy(ObjNode *theNode);
extern	void InitEnemyManager(void);
void DetachEnemyFromSpline(ObjNode *theNode, movecall_t moveCall);
ObjNode *FindClosestEnemy(OGLPoint3D *pt, float *dist);
Boolean	IsWaterInFrontOfEnemy(float r);
void GetEnemyHeadCoord(ObjNode *enemy, OGLPoint3D *coord);
void GetEnemyHandCoord(ObjNode *enemy, OGLPoint3D *coord);
void UpdateEnemyAttachments(ObjNode *theNode);
void ShootEnemyBullet(OGLPoint3D *muzzlePt, OGLVector3D *aim);

void DecEnemiesAtStopPoint(void);

//==========================================================================================

		/***********/
		/* BANDITO */
		/***********/
		
enum
{
	BANDITO_JOINT_CHEST=	1,
	BANDITO_JOINT_UPPERSPINE	= 	2,
	BANDITO_JOINT_HEAD			= 	3,
	BANDITO_JOINT_RIGHTHIP 		= 	4,
	BANDITO_JOINT_RIGHTSHOULDER	=	10,
	BANDITO_JOINT_RIGHTHAND		=	12,
	BANDITO_JOINT_LEFTSHOULDER	=	13
};

enum
{
	BANDITO_ANIM_STANDDUEL = 0,
	BANDITO_ANIM_SHOTINCHEST,
	BANDITO_ANIM_DRAWANDSHOOT,
	BANDITO_ANIM_DUCK,
	BANDITO_ANIM_STANDSHOOT,
	BANDITO_ANIM_GOTSHOT2
};
		
ObjNode *MakeBandito(float x, float z, float rot, short animNum, movecall_t moveCall, Boolean gunInHand);
void UpdateBanditoAttachments(ObjNode *enemy);
void BanditoPutGunInHand(ObjNode *enemy);
Boolean AddBandito_Shootout(TerrainItemEntryType *itemPtr, float  x, float z);

	
//==========================================================================================

		/*********/
		/* RYGAR */
		/*********/
		
enum
{
	RYGAR_JOINT_CHEST			=	2,
	RYGAR_JOINT_HEAD	 		= 	4,
	RYGAR_JOINT_RIGHTHIP 		= 	5,
	RYGAR_JOINT_LEFTHIP 		= 	8,
	RYGAR_JOINT_RIGHTSHOULDER	=	10,
	RYGAR_JOINT_RIGHTHAND		=	13,
	RYGAR_JOINT_LEFTSHOULDER	=	14,
	RYGAR_JOINT_LEFTELBOW		=	15,
	RYGAR_JOINT_LEFTHAND		=	17
};

enum
{
	RYGAR_ANIM_STANDDUEL = 0,
	RYGAR_ANIM_DRAWANDSHOOT,
	RYGAR_ANIM_SHOTINCHEST
};
		
ObjNode *MakeRygar(float x, float z, float rot, short animNum, movecall_t moveCall, Boolean gunInHand);
void UpdateRygarAttachments(ObjNode *enemy);
void RygarPutGunsInHands(ObjNode *enemy);


//==========================================================================================

		/***********/
		/* SHORTY  */
		/***********/
		
enum
{
	SHORTY_JOINT_CHEST			=	2,
	SHORTY_JOINT_HEAD			= 	3,
	SHORTY_JOINT_RIGHTSHOULDER	=	4,
	SHORTY_JOINT_RIGHTHAND		=	6,
	SHORTY_JOINT_LEFTSHOULDER	=	7,
	SHORTY_JOINT_RIGHTHIP 		= 	10
};

enum
{
	SHORTY_ANIM_STANDDUEL = 0,
	SHORTY_ANIM_DRAWANDSHOOT,
	SHORTY_ANIM_SHOTINCHEST,	
	SHORTY_ANIM_DUCK,
	SHORTY_ANIM_STANDSHOOT,
	SHORTY_ANIM_DUCKSHOOTRIGHT,
	SHORTY_ANIM_DUCKSHOOTLEFT,
	SHORTY_ANIM_TOSSED
};
		
ObjNode *MakeShorty(float x, float z, float rot, short animNum, movecall_t moveCall, Boolean gunInHand);
void UpdateShortyAttachments(ObjNode *enemy);
void ShortyPutGunInHand(ObjNode *enemy);
Boolean AddShorty_Shootout(TerrainItemEntryType *itemPtr, float  x, float z);
		
		
		
		/****************/
		/* TREMOR ALIEN */
		/****************/
		
ObjNode *MakeTremorAlien(float x, float z, float rot, short animNum, movecall_t moveCall, Boolean gunInHand);
Boolean PrimeTremorAlien(long splineNum, SplineItemType *itemPtr);
Boolean AddTremorAlien_Shootout(TerrainItemEntryType *itemPtr, float  x, float z);
void UpdateTremorAlienAttachments(ObjNode *enemy);



		/****************/
		/* TREMOR GHOST */
		/****************/
		
enum
{
	TREMORGHOST_ANIM_FLOAT,
	TREMORGHOST_ANIM_xxx,
	TREMORGHOST_ANIM_RISE,
	TREMORGHOST_ANIM_ATTACK,
	TREMORGHOST_ANIM_GOTHIT,
	TREMORGHOST_ANIM_FLAIL
};
		
		
Boolean AddTremorGrave(TerrainItemEntryType *itemPtr, float  x, float z);
		

			/***********/		
			/* FROGMAN */
			/***********/		

enum
{
	FROGMAN_ANIM_STAND = 0,
	FROGMAN_ANIM_THROW,
	FROGMAN_ANIM_SHOTDEAD,
	FROGMAN_ANIM_STUNNED,
	FROGMAN_ANIM_TARGETPRACTICE
};




Boolean AddFrogMan_Shootout(TerrainItemEntryType *itemPtr, float  x, float z);
		
		
		
//=============================================

Boolean PrimeWalker(long splineNum, SplineItemType *itemPtr);
Boolean AddWalker(TerrainItemEntryType *itemPtr, float  x, float z);

		
		
		
		
		
