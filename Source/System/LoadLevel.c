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

void LoadDuelArt(OGLSetupOutputType *setupInfo)
{
FSSpec	spec;



			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/

#if 0
				/* LOAD AUDIO */
						
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Garden.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
#endif
				

			/* LOAD GLOBAL BG3D GEOMETRY */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, setupInfo);


			/* LOAD LEVEL BG3D */
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_DUEL1:
		case	AREA_TOWN_DUEL2:
		case	AREA_TOWN_DUEL3:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:town.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);

				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:buildings.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_BUILDINGS, setupInfo);
				break;
				
		case	AREA_SWAMP_DUEL1:
		case	AREA_SWAMP_DUEL2:
		case	AREA_SWAMP_DUEL3:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:swamp.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);
				
	}


			/* LOAD SPRITES */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:duel.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:font.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, setupInfo);


			/* LOAD PLAYER SKELETON */
			
	LoadASkeleton(SKELETON_TYPE_BILLY, setupInfo);

	LoadASkeleton(SKELETON_TYPE_BANDITO, setupInfo);
	LoadASkeleton(SKELETON_TYPE_RYGAR, setupInfo);
	LoadASkeleton(SKELETON_TYPE_SHORTY, setupInfo);




			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_DUEL1:
		case	AREA_TOWN_DUEL2:
		case	AREA_TOWN_DUEL3:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:town_duel.ter", &spec);
				break;
				
		case	AREA_SWAMP_DUEL1:
		case	AREA_SWAMP_DUEL2:
		case	AREA_SWAMP_DUEL3:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:swamp_duel.ter", &spec);
				break;
	}
	
	LoadPlayfield(&spec, setupInfo);

}


/************************** LOAD SHOOTOUT ART ***************************/

void LoadShootoutArt(OGLSetupOutputType *setupInfo)
{
FSSpec	spec;



			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/

#if 0
				/* LOAD AUDIO */
						
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Garden.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
#endif
				

			/* LOAD GLOBAL BG3D GEOMETRY */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, setupInfo);


	switch(gCurrentArea)
	{
		case	AREA_TOWN_SHOOTOUT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:town.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);

				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:buildings.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_BUILDINGS, setupInfo);
				break;
				
		case	AREA_SWAMP_SHOOTOUT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:swamp.bg3d", &spec);
				ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);
				break;
	}


			/* LOAD SPRITES */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:shootout.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:font.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, setupInfo);


			/* LOAD PLAYER SKELETON */
			
	LoadASkeleton(SKELETON_TYPE_BILLY, setupInfo);

	switch(gCurrentArea)
	{
		case	AREA_TOWN_SHOOTOUT:
				LoadASkeleton(SKELETON_TYPE_BANDITO, setupInfo);
				LoadASkeleton(SKELETON_TYPE_SHORTY, setupInfo);
				LoadASkeleton(SKELETON_TYPE_WALKER, setupInfo);
				LoadASkeleton(SKELETON_TYPE_KANGACOW, setupInfo);
				break;
			
		case	AREA_SWAMP_SHOOTOUT:
				LoadASkeleton(SKELETON_TYPE_KANGAREX, setupInfo);
				LoadASkeleton(SKELETON_TYPE_TREMORALIEN, setupInfo);
				LoadASkeleton(SKELETON_TYPE_TREMORGHOST, setupInfo);
				LoadASkeleton(SKELETON_TYPE_FROGMAN, setupInfo);
				LoadASkeleton(SKELETON_TYPE_BANDITO, setupInfo);
				LoadASkeleton(SKELETON_TYPE_SHORTY, setupInfo);
				break;
				
	}



			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_SHOOTOUT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:town_shootout.ter", &spec);
				break;

		case	AREA_SWAMP_SHOOTOUT:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:swamp_shootout.ter", &spec);
				break;
	}

	LoadPlayfield(&spec, setupInfo);
	
	
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_PesoPOW,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			
	

}


/************************** LOAD STAMPEDE ART ***************************/

void LoadStampedeArt(OGLSetupOutputType *setupInfo)
{
FSSpec	spec;



			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/

#if 0
				/* LOAD AUDIO */
						
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Garden.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
#endif
				

			/* LOAD LEVEL BG3D */
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_STAMPEDE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:town.bg3d", &spec);
				break;			

		case	AREA_SWAMP_STAMPEDE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:swamp.bg3d", &spec);
				break;			
	}
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);


			/* LOAD GLOBAL BG3D GEOMETRY */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, setupInfo);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_Boost,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			


			/* LOAD SPRITES */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:stampede.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:font.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, setupInfo);


			/* LOAD PLAYER SKELETON */
						
	LoadASkeleton(SKELETON_TYPE_BILLY, setupInfo);
	
	switch(gCurrentArea)
	{
		case	AREA_TOWN_STAMPEDE:
				LoadASkeleton(SKELETON_TYPE_KANGACOW, setupInfo);
				break;			

		case	AREA_SWAMP_STAMPEDE:
				LoadASkeleton(SKELETON_TYPE_KANGAREX, setupInfo);
				break;			
	}
	




			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//
			
	switch(gCurrentArea)
	{
		case	AREA_TOWN_STAMPEDE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:town_stampede.ter", &spec);
				break;
				
		case	AREA_SWAMP_STAMPEDE:
				FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":terrain:swamp_stampede.ter", &spec);
				break;
	}

	LoadPlayfield(&spec, setupInfo);



	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_PesoPOW,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_Boost,
								0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);			

}




/************************** LOAD TARGET PRACTICE ART ***************************/

void LoadTargetPracticeArt(OGLSetupOutputType *setupInfo)
{
FSSpec	spec;



			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/

			/* LOAD GLOBAL BG3D GEOMETRY */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":models:targetpractice.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);



			/* LOAD SPRITES */
			
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:targetpractice.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprites:font.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, setupInfo);


			/* LOAD PLAYER SKELETON */
	
	if (gCurrentArea == AREA_TARGETPRACTICE1)
	{			
		LoadASkeleton(SKELETON_TYPE_KANGACOW, setupInfo);
		LoadASkeleton(SKELETON_TYPE_SHORTY, setupInfo);
	}
	else
	{
		LoadASkeleton(SKELETON_TYPE_FROGMAN, setupInfo);
		LoadASkeleton(SKELETON_TYPE_TREMORGHOST, setupInfo);
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



