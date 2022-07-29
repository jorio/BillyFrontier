//
// main.h
//


// Externals
#include "game.h"

#ifndef __MAIN
#define __MAIN
  
#define	GAME_FOV		1.0f
   
#define	MIN_FPS			10
#define MAX_FPS			100
   
#define	NORMAL_GRAVITY	4500.0f  

enum
{
	LANGUAGE_ENGLISH = 0,
	LANGUAGE_FRENCH,
	LANGUAGE_GERMAN,
	LANGUAGE_SPANISH,
	LANGUAGE_ITALIAN,
	LANGUAGE_SWEDISH,
	LANGUAGE_DUTCH,
	
	MAX_LANGUAGES
};
  
  
enum
{
	AREA_TOWN_DUEL1 = 0,
	AREA_TOWN_SHOOTOUT,
	
	AREA_TOWN_DUEL2,
	AREA_TOWN_STAMPEDE,

	AREA_TOWN_DUEL3,
	AREA_TARGETPRACTICE1,


	AREA_SWAMP_DUEL1,
	AREA_SWAMP_SHOOTOUT,
	
	AREA_SWAMP_DUEL2,
	AREA_SWAMP_STAMPEDE,

	AREA_SWAMP_DUEL3,
	AREA_TARGETPRACTICE2

};



#define	DuelerNum	Special[4]
  
#define	POINTS_DUEL_BANDIT		150  
#define	POINTS_PESO				10
#define	POINTS_ANIMAL			30  
#define	POINTS_ORB				75
#define	POINTS_SHOOTOUTENEMY	50
  
#define	NUM_LEVELS		6
  
//=================================================


		/* MAIN */
		
void GameMain(void);
extern	void ToolBoxInit(void);
void InitDefaultPrefs(void);
void StartLevelCompletion(float coolDownTimer);
void DefaultDrawCallback(OGLSetupOutputType *setupInfo);


		
		/* DUEL */
		
enum
{
	DUEL_KEY_SEQUENCE_MODE_NONE,
	DUEL_KEY_SEQUENCE_MODE_PROCESS,
	DUEL_KEY_SEQUENCE_MODE_GOOD,
	DUEL_KEY_SEQUENCE_MODE_BAD,
	DUEL_KEY_SEQUENCE_MODE_SHRINK
};

#define	MAX_REFLEX_DOTS	16
#define	MAX_DUEL_KEY_SEQUENCE_LENGTH	8

		
void PlayDuel(Byte difficulty);
Boolean AddDueler(TerrainItemEntryType *itemPtr, float  x, float z);
void MovePlayer_Duel(ObjNode *player);
ObjNode *GetRandomDueler(void);


		/* SHOOTOUT */
	
#define	MAX_SHIELD	20.0f
		
enum
{
	SHOOTOUT_MODE_WALK,
	SHOOTOUT_MODE_BATTLE,
	SHOOTOUT_MODE_PLAYERKILLED
};


enum
{
	ACTION_TYPE_DUCKDOWN,
	ACTION_TYPE_DUCKLEFT,
	ACTION_TYPE_DUCKRIGHT
};
		
		
void PlayShootout(void);
void MovePlayer_Shootout(ObjNode *player);
Boolean AddShootoutSaloon(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddShootoutAlley(TerrainItemEntryType *itemPtr, float  x, float z);
void ShootoutPlayerHitByBulletCallback(ObjNode *bullet, ObjNode *player, const OGLPoint3D *impactPt);
void GoToNextStopPoint(void);

		
		/* STAMPEDE */
		
#define	PLAYER_STAMPEDE_SPEED	850.0f		
		
void PlayStampede(void);
void MovePlayer_Stampede(ObjNode *player);
Boolean PrimeStampedeKangaCow(long splineNum, SplineItemType *itemPtr);
Boolean PrimeStampedeKangaRex(long splineNum, SplineItemType *itemPtr);
Boolean AddBoost(TerrainItemEntryType *itemPtr, float  x, float z);


		/* TARGET PRACTICE */
		
void PlayTargetPractice(void);
		
		
		

#endif
