//
// sobjtypes.h
//

#pragma once

enum
{
	SPRITE_GROUP_SPHEREMAPS 		= 	0,
	SPRITE_GROUP_INFOBAR			=	1,
	SPRITE_GROUP_LEVELSPECIFIC		= 	2,
	SPRITE_GROUP_PARTICLES			=	3,
	SPRITE_GROUP_BONUS				= 	4,
	SPRITE_GROUP_HIGHSCORES			=	4,
	SPRITE_GROUP_LOSE				=	4,
	SPRITE_GROUP_WIN				= 	4,
	SPRITE_GROUP_TITLE				=	4,
	SPRITE_GROUP_MAINMENU			=	4,
	SPRITE_GROUP_BIGBOARD			=	4,
	SPRITE_GROUP_GLOBAL				= 	5,
	SPRITE_GROUP_FONT				= 	6,
	MAX_SPRITE_GROUPS
};

		/* GLOBAL SPRITES */
		
enum
{
	GLOBAL_SObjType_Shadow_Circular,
	GLOBAL_SObjType_Shadow_CircularDark,
	GLOBAL_SObjType_Shadow_Square,
	
	GLOBAL_SObjType_Fence_Wood,
	GLOBAL_SObjType_Fence_White,
	GLOBAL_SObjType_Fence_TallGrass,
	GLOBAL_SObjType_Fence_SmallGrass,
	GLOBAL_SObjType_Fence_SwampTree,
	GLOBAL_SObjType_Fence_PicketFence,
	
	GLOBAL_SObjType_SwampWater
	
};





		/* SPHEREMAP SPRITES */
		
enum
{
	SPHEREMAP_SObjType_Satin,
	SPHEREMAP_SObjType_Sea,
	SPHEREMAP_SObjType_DarkDusk,
	SPHEREMAP_SObjType_Medow,
	SPHEREMAP_SObjType_Sheen,
	SPHEREMAP_SObjType_DarkYosemite,
	SPHEREMAP_SObjType_Red,
	SPHEREMAP_SObjType_Tundra,
	SPHEREMAP_SObjType_SheenAlpha
};



		/* PARTICLE SPRITES */
		
enum
{
	PARTICLE_SObjType_WhiteSpark,
	PARTICLE_SObjType_WhiteSpark2,
	PARTICLE_SObjType_WhiteSpark3,
	PARTICLE_SObjType_WhiteSpark4,
	PARTICLE_SObjType_WhiteGlow,

	PARTICLE_SObjType_RedGlint,
	PARTICLE_SObjType_GreenGlint,
	PARTICLE_SObjType_BlueGlint,
	PARTICLE_SObjType_YellowGlint,

	PARTICLE_SObjType_RedSpark,
	PARTICLE_SObjType_GreenSpark,
	PARTICLE_SObjType_BlueSpark,

	PARTICLE_SObjType_GreySmoke,
	PARTICLE_SObjType_BlackSmoke,
	PARTICLE_SObjType_RedFumes,
	PARTICLE_SObjType_GreenFumes,
	PARTICLE_SObjType_Dust,
	
	PARTICLE_SObjType_Splash,
	PARTICLE_SObjType_SnowFlakes,
	PARTICLE_SObjType_Fire,
	
	PARTICLE_SObjType_BloodSpat,
	
	PARTICLE_SObjType_LensFlare0,
	PARTICLE_SObjType_LensFlare1,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare3,
	
	PARTICLE_SObjType_Flame0,
	PARTICLE_SObjType_Flame1,
	PARTICLE_SObjType_Flame2,
	PARTICLE_SObjType_Flame3,
	PARTICLE_SObjType_Flame4,
	PARTICLE_SObjType_Flame5,
	PARTICLE_SObjType_Flame6,
	PARTICLE_SObjType_Flame7,
	PARTICLE_SObjType_Flame8,
	PARTICLE_SObjType_Flame9,
	PARTICLE_SObjType_Flame10
};

/******************* DIALOG SOBJTYPES *************************/

enum
{
	FONT_SObjType_Comma,
	FONT_SObjType_Dash,
	FONT_SObjType_Period,
	FONT_SObjType_QuestionMark,
	FONT_SObjType_ExclamationMark,
	FONT_SObjType_ExclamationMark2,
	FONT_SObjType_Apostrophe,
	
	FONT_SObjType_UU,
	FONT_SObjType_uu,
	FONT_SObjType_ua,
	FONT_SObjType_OO,
	FONT_SObjType_oo,
	FONT_SObjType_AA,
	FONT_SObjType_AO,
	FONT_SObjType_NN,
	FONT_SObjType_nn,
	FONT_SObjType_EE,
	FONT_SObjType_ee,
	FONT_SObjType_ev,
	FONT_SObjType_Ax,
	FONT_SObjType_ax,
	FONT_SObjType_av,
	FONT_SObjType_au,
	FONT_SObjType_ao,
	FONT_SObjType_aa,
	FONT_SObjType_Ox,
	FONT_SObjType_Oa,
	FONT_SObjType_oa,
	FONT_SObjType_beta,
	FONT_SObjType_ia,
	
	FONT_SObjType_0,
	FONT_SObjType_1,
	FONT_SObjType_2,
	FONT_SObjType_3,
	FONT_SObjType_4,
	FONT_SObjType_5,
	FONT_SObjType_6,
	FONT_SObjType_7,
	FONT_SObjType_8,
	FONT_SObjType_9,

	FONT_SObjType_a,
	FONT_SObjType_b,
	FONT_SObjType_c,
	FONT_SObjType_d,
	FONT_SObjType_e,
	FONT_SObjType_f,
	FONT_SObjType_g,
	FONT_SObjType_h,
	FONT_SObjType_i,
	FONT_SObjType_j,
	FONT_SObjType_k,
	FONT_SObjType_l,
	FONT_SObjType_m,
	FONT_SObjType_n,
	FONT_SObjType_o,
	FONT_SObjType_p,
	FONT_SObjType_q,
	FONT_SObjType_r,
	FONT_SObjType_s,
	FONT_SObjType_t,
	FONT_SObjType_u,
	FONT_SObjType_v,
	FONT_SObjType_w,
	FONT_SObjType_x,
	FONT_SObjType_y,
	FONT_SObjType_z,
	
	FONT_SObjType_A,
	FONT_SObjType_B,
	FONT_SObjType_C,
	FONT_SObjType_D,
	FONT_SObjType_E,
	FONT_SObjType_F,
	FONT_SObjType_G,
	FONT_SObjType_H,
	FONT_SObjType_I,
	FONT_SObjType_J,
	FONT_SObjType_K,
	FONT_SObjType_L,
	FONT_SObjType_M,
	FONT_SObjType_N,
	FONT_SObjType_O,
	FONT_SObjType_P,
	FONT_SObjType_Q,
	FONT_SObjType_R,
	FONT_SObjType_S,
	FONT_SObjType_T,
	FONT_SObjType_U,
	FONT_SObjType_V,
	FONT_SObjType_W,
	FONT_SObjType_X,
	FONT_SObjType_Y,
	FONT_SObjType_Z,
	
	FONT_SObjType_Cursor

};

/******************* INFOBAR SOBJTYPES *************************/

enum
{

	INFOBAR_SObjType_HealthFrame,
	INFOBAR_SObjType_HealthAmmoFrame,
	INFOBAR_SObjType_HealthAmmoShieldFrame,

	INFOBAR_SObjType_Bullet,
	INFOBAR_SObjType_Heart,
	INFOBAR_SObjType_Skull,
	INFOBAR_SObjType_StarOn,
	
	INFOBAR_SObjType_0,
	INFOBAR_SObjType_1,
	INFOBAR_SObjType_2,
	INFOBAR_SObjType_3,
	INFOBAR_SObjType_4,
	INFOBAR_SObjType_5,
	INFOBAR_SObjType_6,
	INFOBAR_SObjType_7,
	INFOBAR_SObjType_8,
	INFOBAR_SObjType_9,
	
	INFOBAR_SObjType_PausedFrame,
	INFOBAR_SObjType_PausedDot,
	
	INFOBAR_SObjType_TimerFrame	
};




/******************* DUEL SPRITES *************************/

enum
{
	DUEL_SObjType_StatBar,
	
	DUEL_SObjType_UpArrow,
	DUEL_SObjType_RightArrow,
	DUEL_SObjType_DownArrow,
	DUEL_SObjType_LeftArrow,

	DUEL_SObjType_UpArrowOff,
	DUEL_SObjType_RightArrowOff,
	DUEL_SObjType_DownArrowOff,
	DUEL_SObjType_LeftArrowOff,
	
	DUEL_SObjType_ReflexDot,
	DUEL_SObjType_FullLight
};

/******************* SHOOTOUT SPRITES *************************/

enum
{
	SHOOTOUT_SObjType_Crosshairs,
	SHOOTOUT_SObjType_Continue
};

/******************* STAMPEDE SPRITES *************************/

enum
{
	STAMPEDE_SObjType_Fence_Canyon
};


/******************* TARGET PRACTICE SPRITES *************************/

enum
{
	TARGET_SObjType_Sight
};






