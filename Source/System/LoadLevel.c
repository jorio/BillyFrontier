/****************************/
/*      LOAD LEVEL.C        */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/



/************************** LOAD DUEL ART ***************************/

void LoadDuelArt(void)
{
FSSpec	spec;



			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/


			/* LOAD GLOBAL BG3D GEOMETRY */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);


			/* LOAD LEVEL BG3D */
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_DUEL1:
		case	AREA_TOWN_DUEL2:
		case	AREA_TOWN_DUEL3:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:town.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:buildings.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_BUILDINGS);
				break;
				
		case	AREA_SWAMP_DUEL1:
		case	AREA_SWAMP_DUEL2:
		case	AREA_SWAMP_DUEL3:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:swamp.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);
				
	}


			/* LOAD SPRITES */
			
	LoadSpriteGroup(SPRITE_GROUP_INFOBAR);
	LoadSpriteGroup(SPRITE_GROUP_GLOBAL);
	LoadSpriteGroup(SPRITE_GROUP_SPHEREMAPS);
	LoadSpriteGroup(SPRITE_GROUP_FONT);
	LoadSpriteGroup(SPRITE_GROUP_DUEL);


			/* LOAD PLAYER SKELETON */
			
	LoadASkeleton(SKELETON_TYPE_BILLY);
	LoadASkeleton(SKELETON_TYPE_BANDITO);
	LoadASkeleton(SKELETON_TYPE_RYGAR);
	LoadASkeleton(SKELETON_TYPE_SHORTY);




			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_DUEL1:
		case	AREA_TOWN_DUEL2:
		case	AREA_TOWN_DUEL3:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Terrain:town_duel.ter", &spec);
				break;
				
		case	AREA_SWAMP_DUEL1:
		case	AREA_SWAMP_DUEL2:
		case	AREA_SWAMP_DUEL3:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Terrain:swamp_duel.ter", &spec);
				break;
	}
	
	LoadPlayfield(&spec);

}


/************************** LOAD SHOOTOUT ART ***************************/

void LoadShootoutArt(void)
{
FSSpec	spec;



			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/
				

			/* LOAD GLOBAL BG3D GEOMETRY */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);


	switch(gCurrentArea)
	{
		case	AREA_TOWN_SHOOTOUT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:town.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:buildings.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_BUILDINGS);
				break;
				
		case	AREA_SWAMP_SHOOTOUT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:swamp.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);
				break;
	}


			/* LOAD SPRITES */
			
	LoadSpriteGroup(SPRITE_GROUP_INFOBAR);
	LoadSpriteGroup(SPRITE_GROUP_GLOBAL);
	LoadSpriteGroup(SPRITE_GROUP_SPHEREMAPS);
	LoadSpriteGroup(SPRITE_GROUP_CURSOR);
	LoadSpriteGroup(SPRITE_GROUP_FONT);


			/* LOAD PLAYER SKELETON */
			
	LoadASkeleton(SKELETON_TYPE_BILLY);

	switch(gCurrentArea)
	{
		case	AREA_TOWN_SHOOTOUT:
				LoadASkeleton(SKELETON_TYPE_BANDITO);
				LoadASkeleton(SKELETON_TYPE_SHORTY);
				LoadASkeleton(SKELETON_TYPE_WALKER);
				LoadASkeleton(SKELETON_TYPE_KANGACOW);
				break;
			
		case	AREA_SWAMP_SHOOTOUT:
				LoadASkeleton(SKELETON_TYPE_KANGAREX);
				LoadASkeleton(SKELETON_TYPE_TREMORALIEN);
				LoadASkeleton(SKELETON_TYPE_TREMORGHOST);
				LoadASkeleton(SKELETON_TYPE_FROGMAN);
				LoadASkeleton(SKELETON_TYPE_BANDITO);
				LoadASkeleton(SKELETON_TYPE_SHORTY);
				break;
				
	}



			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_SHOOTOUT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Terrain:town_shootout.ter", &spec);
				break;

		case	AREA_SWAMP_SHOOTOUT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Terrain:swamp_shootout.ter", &spec);
				break;
	}

	LoadPlayfield(&spec);
	
	
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_PesoPOW,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			
}


/************************** LOAD STAMPEDE ART ***************************/

void LoadStampedeArt(void)
{
FSSpec	spec;



			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/
				

			/* LOAD LEVEL BG3D */
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_STAMPEDE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:town.bg3d", &spec);
				break;			

		case	AREA_SWAMP_STAMPEDE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:swamp.bg3d", &spec);
				break;			
	}
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);


			/* LOAD GLOBAL BG3D GEOMETRY */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_Boost,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			


			/* LOAD SPRITES */

	LoadSpriteGroup(SPRITE_GROUP_INFOBAR);
	LoadSpriteGroup(SPRITE_GROUP_GLOBAL);
	LoadSpriteGroup(SPRITE_GROUP_SPHEREMAPS);
	LoadSpriteGroup(SPRITE_GROUP_STAMPEDE);
	LoadSpriteGroup(SPRITE_GROUP_FONT);


			/* LOAD PLAYER SKELETON */
						
	LoadASkeleton(SKELETON_TYPE_BILLY);
	
	switch(gCurrentArea)
	{
		case	AREA_TOWN_STAMPEDE:
				LoadASkeleton(SKELETON_TYPE_KANGACOW);
				break;			

		case	AREA_SWAMP_STAMPEDE:
				LoadASkeleton(SKELETON_TYPE_KANGAREX);
				break;			
	}
	




			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_STAMPEDE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Terrain:town_stampede.ter", &spec);
				break;
				
		case	AREA_SWAMP_STAMPEDE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Terrain:swamp_stampede.ter", &spec);
				break;
	}

	LoadPlayfield(&spec);



	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_PesoPOW,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_Boost,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			
}




/************************** LOAD TARGET PRACTICE ART ***************************/

void LoadTargetPracticeArt(void)
{
FSSpec	spec;



			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/

			/* LOAD GLOBAL BG3D GEOMETRY */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:targetpractice.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);



			/* LOAD SPRITES */
			
	LoadSpriteGroup(SPRITE_GROUP_INFOBAR);
	LoadSpriteGroup(SPRITE_GROUP_GLOBAL);
	LoadSpriteGroup(SPRITE_GROUP_SPHEREMAPS);
	LoadSpriteGroup(SPRITE_GROUP_CURSOR);
	LoadSpriteGroup(SPRITE_GROUP_FONT);


			/* LOAD PLAYER SKELETON */
	
	if (gCurrentArea == AREA_TARGETPRACTICE1)
	{			
		LoadASkeleton(SKELETON_TYPE_KANGACOW);
		LoadASkeleton(SKELETON_TYPE_SHORTY);
	}
	else
	{
		LoadASkeleton(SKELETON_TYPE_FROGMAN);
		LoadASkeleton(SKELETON_TYPE_TREMORGHOST);
	}


	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_Boost,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_PesoPOW,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PRACTICE_ObjType_Bottle,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PRACTICE_ObjType_DeathSkull,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
}



