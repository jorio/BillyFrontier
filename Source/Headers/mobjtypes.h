//
// mobjtypes.h
//

#ifndef __MOBJT
#define __MOBJT

enum
{
	MODEL_GROUP_GLOBAL		=	0,
	MODEL_GROUP_LEVELSPECIFIC =	1,
	MODEL_GROUP_TITLE		=	1,
	MODEL_GROUP_LEVELINTRO =	2,
	MODEL_GROUP_WINNERS		= 	2,
	MODEL_GROUP_BONUS		= 	2,
	MODEL_GROUP_MAINMENU	= 	2,
	MODEL_GROUP_HIGHSCORES	=	2,
	MODEL_GROUP_LOSESCREEN	=	2,
	MODEL_GROUP_WINSCREEN	=	2,
	MODEL_GROUP_BUILDINGS 	=	3,
	
	MODEL_GROUP_SKELETONBASE				// skeleton files' models are attached here
};




/******************* GLOBAL *************************/

enum
{
	GLOBAL_ObjType_BillyHat = 0,
	GLOBAL_ObjType_BillyGun,
	
	GLOBAL_ObjType_BanditoGun,
	GLOBAL_ObjType_BanditoHat,

	GLOBAL_ObjType_RygarGun,
	GLOBAL_ObjType_RygarHat,
	
	GLOBAL_ObjType_ShortyGun,
	GLOBAL_ObjType_ShortyHat,

	GLOBAL_ObjType_Tomahawk,
	
	GLOBAL_ObjType_Bullet,
	
	GLOBAL_ObjType_Barrel,
	GLOBAL_ObjType_BarrelTNT,
	GLOBAL_ObjType_FrogBarrel,
	
	GLOBAL_ObjType_Crate,
	GLOBAL_ObjType_CrateStack,
	GLOBAL_ObjType_MetalCrate,
	GLOBAL_ObjType_MetalCrateStack,
	
	GLOBAL_ObjType_HayBale,
	GLOBAL_ObjType_HayBaleStack,

	GLOBAL_ObjType_CrateDebris0,
	GLOBAL_ObjType_CrateDebris1,
	GLOBAL_ObjType_CrateDebris2,
	GLOBAL_ObjType_CrateDebris3,
	GLOBAL_ObjType_CrateDebris4,
	GLOBAL_ObjType_CrateDebris5,
	GLOBAL_ObjType_CrateDebris6,
	GLOBAL_ObjType_CrateDebris7,
	GLOBAL_ObjType_CrateDebris8,
	GLOBAL_ObjType_CrateDebris9,

	GLOBAL_ObjType_Tumbleweed,
	GLOBAL_ObjType_AmmoBoxPOW,
	GLOBAL_ObjType_PesoPOW,
	GLOBAL_ObjType_FreeLifePOW,
	GLOBAL_ObjType_ShieldPOW,


	GLOBAL_ObjType_WoodPost,
	
	GLOBAL_ObjType_Shield,
	GLOBAL_ObjType_Boost
	
};


/******************* TOWN *************************/

enum
{
	TOWN_ObjType_Cyc = 0,
		
	TOWN_ObjType_Headstone1,
	TOWN_ObjType_Headstone2,
	TOWN_ObjType_Headstone3,
	TOWN_ObjType_Headstone4,
	TOWN_ObjType_Headstone5,
	
	TOWN_ObjType_Cactus,
	TOWN_ObjType_DeadTree,
	TOWN_ObjType_DeadTreeOnSide,

	TOWN_ObjType_RockWall,
	
	TOWN_ObjType_Coffin,
	TOWN_ObjType_CoffinStack,

	TOWN_ObjType_TallRock1,
	TOWN_ObjType_TallRock2,
	TOWN_ObjType_ShortRock1,
	TOWN_ObjType_ShortRock2,
	
	TOWN_ObjType_Alley,
	
	TOWN_ObjType_Table,
	TOWN_ObjType_Chair,
	
	TOWN_ObjType_WalkerPodLeft,
	TOWN_ObjType_WalkerPodRight,
	TOWN_ObjType_Missile


};


/******************* SWAMP *************************/

enum
{
	SWAMP_ObjType_Cyc = 0,
	SWAMP_ObjType_Cabin,

	SWAMP_ObjType_LargeRock,
	SWAMP_ObjType_MediumRock,
	SWAMP_ObjType_SmallRock,
	SWAMP_ObjType_TallRock,
	SWAMP_ObjType_Mound,
	
	SWAMP_ObjType_MushroomTree,
	SWAMP_ObjType_StickTree,
	SWAMP_ObjType_Plant1,
	SWAMP_ObjType_Plant2,

	SWAMP_ObjType_Grave,
	SWAMP_ObjType_TeePee,
	
	SWAMP_ObjType_SpearSkull,
	
	SWAMP_ObjType_ElectricFence
		
};



/******************* TARGET PRACTICE *************************/

enum
{
	PRACTICE_ObjType_CycTown = 0,
	PRACTICE_ObjType_CycSwamp,
	
	PRACTICE_ObjType_Orb_TNT,
	PRACTICE_ObjType_Orb_Points,
	PRACTICE_ObjType_Orb_Time,
	PRACTICE_ObjType_Orb_RapidFire,
	PRACTICE_ObjType_DeathSkull,
	PRACTICE_ObjType_Bottle
};

/******************* BUILDINGS *************************/

enum
{
	BUILDING_ObjType_Saloon,
	BUILDING_ObjType_Express,
	BUILDING_ObjType_Jailhouse,
	BUILDING_ObjType_Undertaker,
	BUILDING_ObjType_Livery,
	BUILDING_ObjType_Blacksmith,
	BUILDING_ObjType_Bank,
	BUILDING_ObjType_Cantina,

	BUILDING_ObjType_Saloon_Burning,
	BUILDING_ObjType_Express_Burning,
	BUILDING_ObjType_Jailhouse_Burning,
	BUILDING_ObjType_Undertaker_Burning,
	BUILDING_ObjType_Livery_Burning,
	BUILDING_ObjType_Blacksmith_Burning,
	BUILDING_ObjType_Bank_Burning,
	BUILDING_ObjType_Cantina_Burning,
	
	BUILDING_ObjType_SaloonInside
};

#endif









