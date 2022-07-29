/****************************/
/*   	INFOBAR.C		    */
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include	"infobar.h"
#include	"3dmath.h"


extern	float					gCurrentAspectRatio,gGlobalTransparency,gFramesPerSecondFrac, gTargetPracticeTimer;
extern	PlayerInfoType			gPlayerInfo;
extern	FSSpec					gDataSpec;
extern	long					gTerrainUnitWidth,gTerrainUnitDepth;
extern	OGLColorRGB				gGlobalColorFilter;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	PrefsType				gGamePrefs;
extern	u_long					gGlobalMaterialFlags;
extern	SpriteType				*gSpriteGroupList[];
extern	AGLContext				gAGLContext;
extern	int						gDuelKeySequenceLength, gDuelKeyBufferIndex,gCurrentArea,gDuelReflex;
extern	Byte					gDuelKeySequence[], gDuelKeySequenceMode, gShootoutMode;
extern	long					gNumSpritesInGroupList[];
extern	int						gGameWindowWidth,gGameWindowHeight, gPepperCount;
extern	OGLPoint2D				gCrosshairsCoord;
extern	Boolean					gIsPicking, gShootoutCanProceedToNextStopPoint, gNeedToReloadNextAmmoClip;
extern	u_long					gScore;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawDuelInfobar(void);

static void DrawInfobarSprite_Rotated(float x, float y, float size, short texNum, float rot);
static void DrawInfobarSprite_Centered(float x, float y, float size, short texNum);
static void DrawInfobarSprite_Scaled(float x, float y, float scaleX, float scaleY, short texNum);
//static void Infobar_DrawPesos(void);
static void Infobar_DrawAmmo(void);
static void Infobar_DrawDuelSequence(void);
static void Infobar_DrawReflex(void);
static void DrawShootoutInfobar(void);
static void Infobar_DrawCrosshairs(void);
static void Infobar_DrawShield(void);
static void Infobar_DrawLives(void);
static void Infobar_DrawScore(void);
static void DrawStampedeInfobar(void);
static void DrawTargetPracticeInfobar(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DIGIT_WIDTH		.4f

#define	STAT_BAR_HEIGHT	40.0f

#define MAP_SCALE	150.0f
#define	MAP_X		410.0f			//(640.0f - MAP_SCALE)
#define	MAP_Y		0.0f

#define	HEALTH_X		22.0f
#define	HEALTH_Y		20.0f
#define	HEALTH_SCALE	16.0f


#define	DUEL_SEQ_X		171.5f
#define DUEL_SEQ_Y		430.5f
#define DUEL_SEQ_SCALE	33.0f

#define	REFLEX_X		156.0f
#define	REFLEX_Y		455.0f
#define	REFLEX_SCALE	20.0f

#define	SHIELD_X		50.0f
#define	SHIELD_Y		7.0f
#define	SHIELD_SCALE	20.0f

#define	SCORE_X			(STAT_BAR_HEIGHT * 1.21f)
#define	SCORE_Y			(STAT_BAR_HEIGHT * 0.5f)
#define SCORE_SCALE		(STAT_BAR_HEIGHT * .4f)

#define	CLIPS_X			(STAT_BAR_HEIGHT * 3.3f)
#define	CLIPS_Y			(STAT_BAR_HEIGHT * 0.23f)
#define CLIPS_SCALE		(STAT_BAR_HEIGHT * .46f)

#define	LIVES_X			(STAT_BAR_HEIGHT * .93f)
#define	LIVES_Y			(STAT_BAR_HEIGHT * 0.15f)
#define LIVES_SCALE		(STAT_BAR_HEIGHT * .53f)


#define	TIMER_X			530.0f
#define	TIMER_Y			2.0f
#define	TIMER_SCALE		32.0f


/*********************/
/*    VARIABLES      */
/*********************/


/********************* INIT INFOBAR ****************************/
//
// Called at beginning of level
//

void InitInfobar(void)
{		



}



/***************** SET INFOBAR SPRITE STATE *******************/

void SetInfobarSpriteState(float anaglyphZ)
{
AGLContext agl_ctx = gAGLContext;

	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);							
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures
	

			/* INIT MATRICES */
					
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (gGamePrefs.anaglyph)
	{
		if (gAnaglyphPass == 0)
			glOrtho(-anaglyphZ, 640-anaglyphZ, 480.0f, 0, 0, 1);	
		else
			glOrtho(anaglyphZ, 640+anaglyphZ, 480.0f, 0, 0, 1);						
	}
	else
		glOrtho(0, 640, 480.0f, 0, 0, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();					
}


/********************** DRAW INFOBAR ****************************/

void DrawInfobar(void)
{
AGLContext agl_ctx = gAGLContext;

	if (gIsPicking)
		return;
		

		/************/
		/* SET TAGS */
		/************/

	OGL_PushState();
				
	if (gGameViewInfoPtr->useFog)
		glDisable(GL_FOG);
		
	SetInfobarSpriteState(0);
							


		/***************/
		/* DRAW THINGS */
		/***************/



	switch(gCurrentArea)
	{
		case	AREA_TOWN_DUEL1:
		case	AREA_TOWN_DUEL2:
		case	AREA_TOWN_DUEL3:
		case	AREA_SWAMP_DUEL1:
		case	AREA_SWAMP_DUEL2:
		case	AREA_SWAMP_DUEL3:
				DrawDuelInfobar();
				break;
		
		case	AREA_TOWN_SHOOTOUT:
		case	AREA_SWAMP_SHOOTOUT:
				DrawShootoutInfobar();
				break;

		case	AREA_TOWN_STAMPEDE:
		case	AREA_SWAMP_STAMPEDE:
				DrawStampedeInfobar();
				break;

		case	AREA_TARGETPRACTICE1:
		case	AREA_TARGETPRACTICE2:
				DrawTargetPracticeInfobar();
				break;
				
	}
	
			
			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
	if (gGameViewInfoPtr->useFog)
		glEnable(GL_FOG);
}


#pragma mark -


/******************** DRAW INFOBAR SPRITE **********************/

void DrawInfobarSprite(float x, float y, float size, short texNum)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */
				
	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, gGameViewInfoPtr);			

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();	
}


/******************** DRAW INFOBAR SPRITE 3 **********************/
//
// Same as above, but where size is the vertical size, not horiz.
//

void DrawInfobarSprite3(float x, float y, float size, short texNum)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */
				
	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, gGameViewInfoPtr);			

	aspect = (float)mo->objectData.width / (float)mo->objectData.height;

			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 					y);
	glTexCoord2f(1,1);	glVertex2f(x+(size*aspect), 	y);
	glTexCoord2f(1,0);	glVertex2f(x+(size*aspect),		y+size);
	glTexCoord2f(0,0);	glVertex2f(x,					y+size);
	glEnd();	
}


/******************** DRAW INFOBAR SPRITE: CENTERED **********************/
//
// Coords are for center of sprite, not upper left
//

static void DrawInfobarSprite_Centered(float x, float y, float size, short texNum)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */
				
	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, gGameViewInfoPtr);			

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	x -= size*.5f;
	y -= (size*aspect)*.5f;

			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();	
}


/******************** DRAW INFOBAR SPRITE 2 **********************/
//
// This version lets user pass in the sprite group
//

void DrawInfobarSprite2(float x, float y, float size, short group, short texNum)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */
				
	mo = gSpriteGroupList[group][texNum].materialObject;
	MO_DrawMaterial(mo, gGameViewInfoPtr);			

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();	
}

/******************** DRAW INFOBAR SPRITE 2: CENTERED **********************/
//
// This version lets user pass in the sprite group
//

void DrawInfobarSprite2_Centered(float x, float y, float size, short group, short texNum)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

	if (texNum >= gNumSpritesInGroupList[group])
	{
		DoAlert("DrawInfobarSprite2_Centered: sprite # > max in group");
		ShowSystemErr_NonFatal(texNum);
		ShowSystemErr(gNumSpritesInGroupList[group]);
	}

		/* ACTIVATE THE MATERIAL */
				
	mo = gSpriteGroupList[group][texNum].materialObject;
	MO_DrawMaterial(mo, gGameViewInfoPtr);			

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	x -= size*.5f;
	y -= (size*aspect)*.5f;

			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();	
}



/******************** DRAW INFOBAR SPRITE: ROTATED **********************/

static void DrawInfobarSprite_Rotated(float x, float y, float size, short texNum, float rot)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect, xoff, yoff;
OGLPoint2D			p[4];
OGLMatrix3x3		m;

		/* ACTIVATE THE MATERIAL */
				
	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, gGameViewInfoPtr);			

				/* SET COORDS */
				
	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	xoff = size*.5f;
	yoff = (size*aspect)*.5f;

	p[0].x = -xoff;		p[0].y = -yoff;
	p[1].x = xoff;		p[1].y = -yoff;
	p[2].x = xoff;		p[2].y = yoff;
	p[3].x = -xoff;		p[3].y = yoff;

	OGLMatrix3x3_SetRotate(&m, rot);
	OGLPoint2D_TransformArray(p, &m, p, 4);


			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(p[0].x + x, p[0].y + y);
	glTexCoord2f(1,1);	glVertex2f(p[1].x + x, p[1].y + y);
	glTexCoord2f(1,0);	glVertex2f(p[2].x + x, p[2].y + y);
	glTexCoord2f(0,0);	glVertex2f(p[3].x + x, p[3].y + y);
	glEnd();	
}


/******************** DRAW INFOBAR SPRITE: SCALED **********************/

static void DrawInfobarSprite_Scaled(float x, float y, float scaleX, float scaleY, short texNum)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */
				
	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, gGameViewInfoPtr);			

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 			y);
	glTexCoord2f(1,1);	glVertex2f(x+scaleX, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+scaleX, 	y+(scaleY*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,			y+(scaleY*aspect));
	glEnd();	
}



/*************** INFOBAR: DRAW NUMBER ************************/

void Infobar_DrawNumber(int number, float x, float y, float scale, int numDigits, Boolean showLeading)
{
int		i,n,r;
float	sep = scale * DIGIT_WIDTH;
	
	
	x += (numDigits-1) * sep;			// start on right
	
	for (i = 0; i < numDigits; i++)
	{
		n = number / 10;
		r = (number - (n * 10));
		if (r == 10)
			r = 0;
		number = n;
	
		DrawInfobarSprite2(x, y, scale, SPRITE_GROUP_FONT, FONT_SObjType_0 + r);
	
		x -= sep;
	
		if (!showLeading)
		{
			if (number == 0)
				return;
		}
	
	}
	
}

#pragma mark -




/*********************** INFOBAR: DRAW LIVES ***********************/

static void Infobar_DrawLives(void)
{
int		i,n;
float	y = LIVES_Y;

	n = gPlayerInfo.lives;
	if (n > 3)
		n = 3;

	for (i = 0; i < n; i++)
	{
		DrawInfobarSprite_Centered(LIVES_X, y, LIVES_SCALE, INFOBAR_SObjType_Heart);

		y += STAT_BAR_HEIGHT * .3f;
	}
}



#pragma mark -

/********************** DRAW DUEL INFOBAR *****************************/

static void DrawDuelInfobar(void)
{
	DrawInfobarSprite3(0, 0, STAT_BAR_HEIGHT, INFOBAR_SObjType_HealthFrame);
	Infobar_DrawScore();
	Infobar_DrawLives();
	

	DrawInfobarSprite2(0, 480-(640/8), 640, SPRITE_GROUP_LEVELSPECIFIC, DUEL_SObjType_StatBar);
	Infobar_DrawDuelSequence();	
	Infobar_DrawReflex();
}


/***************** INFOBAR:  DRAW DUEL SEQUENCE *************************/

static void Infobar_DrawDuelSequence(void)
{
int		i;
float	x, spacing = 42.5f;

	if (gDuelKeySequenceMode == DUEL_KEY_SEQUENCE_MODE_SHRINK)		// see if done
		return;

	if (gDuelKeySequenceMode != DUEL_KEY_SEQUENCE_MODE_NONE)		// is there a key sequence to draw yet
	{

				/* DRAW THE KEY SEQUENCE */
				
		x = DUEL_SEQ_X + ((MAX_DUEL_KEY_SEQUENCE_LENGTH - gDuelKeySequenceLength >> 1) * spacing);
				
		for (i = 0; i < gDuelKeySequenceLength; i++)				// create new sequence
		{
			int	key = gDuelKeySequence[i];
			
			if (gDuelKeyBufferIndex > i)							// set completed keys as semi-trans
				DrawInfobarSprite2_Centered(x, DUEL_SEQ_Y, DUEL_SEQ_SCALE, SPRITE_GROUP_LEVELSPECIFIC, DUEL_SObjType_UpArrowOff + key);
			else
				DrawInfobarSprite2_Centered(x, DUEL_SEQ_Y, DUEL_SEQ_SCALE, SPRITE_GROUP_LEVELSPECIFIC, DUEL_SObjType_UpArrow + key);
			
			x += spacing;
		}
	}
}


/*********************** INFOBAR: DRAW REFLEX ***********************/

static void Infobar_DrawReflex(void)
{
int		i, n;
float	x;
static float	blinkTimer = 0;

			/* DRAW THE SMALL DOTS */
			
	x = REFLEX_X;

	n = 0;
	for (i = 0; i < MAX_REFLEX_DOTS; i++)
	{
		if (i >= gDuelReflex)
			continue;
			
		DrawInfobarSprite2(x, REFLEX_Y, REFLEX_SCALE, SPRITE_GROUP_LEVELSPECIFIC, DUEL_SObjType_ReflexDot);

		if (i == 7)
			x = 337.0f;
		else
			x += 18.1f;	
			
		n++;
	}

	
		/* DRAW THE MAIN LIGHT */

	if (n == MAX_REFLEX_DOTS)						// are we full?
	{
		AGLContext agl_ctx = gGameViewInfoPtr->drawContext;

		blinkTimer -= gFramesPerSecondFrac;
		if (blinkTimer <= 0.0f)
			blinkTimer += .5f;
			
		if (blinkTimer < .3f)
		{
			DrawInfobarSprite2_Centered(320.5, 463, 41, SPRITE_GROUP_LEVELSPECIFIC, DUEL_SObjType_FullLight);
		
		
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			gGlobalTransparency = .9;
			DrawInfobarSprite2_Centered(320.5, 463, 100, SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_RedSpark);
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			gGlobalTransparency = 1.0f;	
		}	
	}

}





#pragma mark -

/********************** DRAW SHOOUTOUT INFOBAR *****************************/

static void DrawShootoutInfobar(void)
{
	DrawInfobarSprite3(0, 0, STAT_BAR_HEIGHT, INFOBAR_SObjType_HealthAmmoShieldFrame);
//	Infobar_DrawPesos();

	Infobar_DrawShield();
	Infobar_DrawAmmo();
	Infobar_DrawLives();

	if (gShootoutMode != SHOOTOUT_MODE_PLAYERKILLED)
		Infobar_DrawCrosshairs();
	
	if (gShootoutCanProceedToNextStopPoint)
	{
		DrawInfobarSprite2_Centered(640.0/2, 420.0, 80, SPRITE_GROUP_LEVELSPECIFIC, SHOOTOUT_SObjType_Continue);
	}
	
	Infobar_DrawScore();
	
}



/********************** INFOBAR: DRAW AMMO *****************************/

static void Infobar_DrawAmmo(void)
{
int		i, numBullets, numClips;
OGLPoint2D	bulletCoords[6] =
{
	STAT_BAR_HEIGHT * 2.57f,		STAT_BAR_HEIGHT * .23f,
	STAT_BAR_HEIGHT * 2.36f,		STAT_BAR_HEIGHT * .35f,
	STAT_BAR_HEIGHT * 2.36f,		STAT_BAR_HEIGHT * .59f,

	STAT_BAR_HEIGHT * 2.58f,		STAT_BAR_HEIGHT * .72f,
	STAT_BAR_HEIGHT * 2.8f,			STAT_BAR_HEIGHT * .59f,
	STAT_BAR_HEIGHT * 2.8f,			STAT_BAR_HEIGHT * .35,
};

	numClips = gPlayerInfo.ammoCount / AMMO_CLIP_SIZE;
	numBullets = gPlayerInfo.ammoCount % AMMO_CLIP_SIZE;
	
	
				/********************************************/
				/* ARE WE WAITING FOR THE PLAYER TO RELOAD? */
				/********************************************/
				
	if (gNeedToReloadNextAmmoClip)
	{
		if (GetNewKeyState_Real(KEY_SPACE))					// user pressed the reload button?
		{
			gNeedToReloadNextAmmoClip = false;
			PlayEffect(EFFECT_RELOAD);
		}
		else
			DrawFontString("RELOAD", 640/2, 480/2, 30.0);
		
	
	}
	
	
				/****************************/
				/* STILL SHOOTING THIS CLIP */
				/****************************/
	else
	{
		if ((numBullets == 0) && (numClips > 0))
		{
			numBullets = AMMO_CLIP_SIZE;
			numClips--;
		}
		
				/* DRAW BULLETS */
				
		for (i = 0; i < numBullets; i++)
		{
			DrawInfobarSprite_Centered(bulletCoords[i].x, bulletCoords[i].y, STAT_BAR_HEIGHT * .2f, INFOBAR_SObjType_Bullet);
		}	
	}



			/* DRAW # OF CLIPS */
			
	Infobar_DrawNumber(numClips, CLIPS_X, CLIPS_Y, CLIPS_SCALE, 2, true);
}



/*********************** INFOBAR: DRAW CROSSHAIRS ***********************/

static void Infobar_DrawCrosshairs(void)
{
		DrawInfobarSprite2_Centered(gCrosshairsCoord.x, gCrosshairsCoord.y, 50, SPRITE_GROUP_LEVELSPECIFIC, SHOOTOUT_SObjType_Crosshairs);
}



/********************** INFOBAR: DRAW SHIELD **********************/

static void Infobar_DrawShield(void)
{
AGLContext agl_ctx = gAGLContext;
float	y = 33.5;
const	x = 203.0f;

	glDisable(GL_TEXTURE_2D);

	glColor3f(1,0,0);
	
	glBegin(GL_QUADS);
	
	glVertex2f(x, y);
	glVertex2f(x-7, y);
		
	y -= gPlayerInfo.shieldPower / MAX_SHIELD * 30.1f;
	
	
	
	glVertex2f(x-7, y);
	glVertex2f(x, y);
	
	glEnd();


	glColor3f(1,1,1);

}




#pragma mark -

/********************** DRAW STAMPEDE INFOBAR *****************************/

static void DrawStampedeInfobar(void)
{
	DrawInfobarSprite3(0, 0, STAT_BAR_HEIGHT, INFOBAR_SObjType_HealthFrame);
	Infobar_DrawScore();
	Infobar_DrawLives();


//	DrawInfobarSprite3(0, 0, STAT_BAR_HEIGHT, INFOBAR_SObjType_HealthAmmoShieldFrame);
//	Infobar_DrawShield();
//	Infobar_DrawAmmo();
//	Infobar_DrawLives();
//	Infobar_DrawScore();
}


/******************* INFOBAR: DRAW TARGET PRACTICE ********************/

static void DrawTargetPracticeInfobar(void)
{
	gGlobalTransparency = 1.0f;			// set this to see if it fixes the random invisible cursor problem

	DrawInfobarSprite3(0, 0, STAT_BAR_HEIGHT, INFOBAR_SObjType_HealthAmmoShieldFrame);

	Infobar_DrawScore();
	
	Infobar_DrawAmmo();
	Infobar_DrawLives();
	Infobar_DrawShield();

	
			/* DRAW TIMER */
			
	DrawInfobarSprite3(475, 0, STAT_BAR_HEIGHT+2, INFOBAR_SObjType_TimerFrame);
	Infobar_DrawNumber(gTargetPracticeTimer + .5f, TIMER_X, TIMER_Y, TIMER_SCALE, 3, false);
	
			/* DRAW PEPPER COUNT */

	Infobar_DrawNumber(gPepperCount, TIMER_X + 86.0f, TIMER_Y, TIMER_SCALE, 1, false);
			

	Infobar_DrawCrosshairs();	
	
	gGlobalTransparency = 1.0f;	
}


/****************** INFOBAR: DRAW SCORE *******************/

static void Infobar_DrawScore(void)
{
	Infobar_DrawNumber(gScore, SCORE_X, SCORE_Y, SCORE_SCALE, SCORE_NUM_DIGITS, true);


}










