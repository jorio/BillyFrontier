/****************************/
/*   	CAMERA.C    	    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <aglmacro.h>

#include "3DMath.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	float					gFramesPerSecond,gFramesPerSecondFrac,gCurrentAspectRatio;
extern	OGLVector3D				gWorldSunDirection;
extern	CollisionRec			gCollisionList[];
extern	FSSpec					gDataSpec;
extern	OGLMatrix4x4			gWorldToWindowMatrix;
extern	float					gGlobalTransparency,gTerrainTileDepth,gTerrainPolygonSize;
extern	PlayerInfoType			gPlayerInfo;
extern	SpriteType				*gSpriteGroupList[];
extern	OGLColorRGB			gGlobalColorFilter;
extern	float					gScratchF,gTerrainSuperTileUnitSize, gAnaglyphEyeSeparation;
extern	PrefsType			gGamePrefs;
extern	int				gGameWindowWidth, gGameWindowHeight,gScratch;
extern	Boolean					gSlowCPU, gDoneFaceOff, gIsPicking;
extern	ObjNode			*gDuelers[];
extern	SplineDefType	**gSplineList;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void ResetCameraSettings(void);
static void UpdateCamera_TopView(void);

static void MoveStampedeCameraOnSpline(ObjNode *camera);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	CAM_MINY			60.0f

#define	CAMERA_CLOSEST		75.0f
#define	CAMERA_FARTHEST		400.0f

#define	NUM_FLARE_TYPES		4
#define	NUM_FLARES			6

#define	CAMERA_DEFAULT_DIST_FROM_ME	350.0f
#define	DEFAULT_CAMERA_YOFF			100.0f

enum
{
	CAMERA_MODE_NORMAL,
	CAMERA_MODE_2,
	CAMERA_MODE_3,
	CAMERA_MODE_4,

	MAX_CAMERA_MODES
};



		/* DUELING CAMERA MODES */
		
enum
{
	DUEL_CAMERA_MODE_FRONTTRACK,
	DUEL_CAMERA_MODE_SIDETRACK,
	DUEL_CAMERA_MODE_EYES,
	DUEL_CAMERA_MODE_HAND,
	
	DUEL_CAMERA_MODE_ENEMYFACE,
	DUEL_CAMERA_MODE_ENEMYFRONT,	
	DUEL_CAMERA_MODE_EOTS,
	DUEL_CAMERA_MODE_ENEMYHAND,	
	
	DUEL_CAMERA_MODE_POTS,	
	DUEL_CAMERA_MODE_TOP,
	DUEL_CAMERA_MODE_FARSIDE,
	DUEL_CAMERA_MODE_FARSIDE2,
	
	NUM_DUEL_CAMERA_MODES

};


typedef struct
{
	int		mode;
	float	duration;
}DuelCamShotType;


/*********************/
/*    VARIABLES      */
/*********************/

static OGLCameraPlacement	gAnaglyphCameraBackup;		// backup of original camera info before offsets applied

const OGLVector3D	up = {0,1,0};

Boolean				gDrawLensFlare = true, gFreezeCameraFromXZ = false, gFreezeCameraFromY = false;

float				gCameraStartupTimer, gCameraSpinSpeed;

float				gPlayerToCameraAngle = 0.0f;
static float		gCameraLookAtAccel,gCameraFromAccelY,gCameraFromAccel;
float				gCameraDistFromMe, gCameraHeightFactor,gCameraLookAtYOff;
float				gMinHeightOffGround, gTopCamDist, gMaxCameraHeightOff;
Byte				gCameraMode;

static OGLPoint3D	gSunCoord;

static const float	gFlareOffsetTable[]=
{
	1.0,
	.6,
	.3,
	1.0/8.0,
	-.25,
	-.5
};


static const float	gFlareScaleTable[]=
{
	.3,
	.06,
	.1,
	.2,
	.03,
	.1
};

static const Byte	gFlareImageTable[]=
{
	PARTICLE_SObjType_LensFlare0,
	PARTICLE_SObjType_LensFlare1,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare3,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare1
};

static	DuelCamShotType gDualShotList[] =
{
	DUEL_CAMERA_MODE_FRONTTRACK,
	3.0f,


	DUEL_CAMERA_MODE_ENEMYFRONT,
	3.0f,

	DUEL_CAMERA_MODE_POTS,
	4.0f,

	DUEL_CAMERA_MODE_EOTS,
	4.0f,

	DUEL_CAMERA_MODE_HAND,
	3.0f,

	DUEL_CAMERA_MODE_ENEMYHAND,
	3.0f,

	DUEL_CAMERA_MODE_TOP,
	4.0f,

	DUEL_CAMERA_MODE_FRONTTRACK,
	2.0f,

	DUEL_CAMERA_MODE_FARSIDE,
	3.0f,

	DUEL_CAMERA_MODE_POTS,
	3.0,
#if 1
	
	DUEL_CAMERA_MODE_EOTS,
	3.0f,
	
	DUEL_CAMERA_MODE_ENEMYFACE,
	3.0f,

	DUEL_CAMERA_MODE_EYES,
	3.0f,

	DUEL_CAMERA_MODE_ENEMYHAND,
	2.2f,

	DUEL_CAMERA_MODE_HAND,
	2.2f,

	DUEL_CAMERA_MODE_ENEMYHAND,
	1.6f,

	DUEL_CAMERA_MODE_HAND,
	1.6f,

	DUEL_CAMERA_MODE_ENEMYFACE,
	1.2f,

	DUEL_CAMERA_MODE_EYES,
	1.2f,

	DUEL_CAMERA_MODE_ENEMYHAND,
	1.0f,

	DUEL_CAMERA_MODE_HAND,
	1.0f,

	DUEL_CAMERA_MODE_ENEMYFACE,
	.8f,

	DUEL_CAMERA_MODE_EYES,
	.8f,

	DUEL_CAMERA_MODE_ENEMYFACE,
	.8f,

	DUEL_CAMERA_MODE_EYES,
	.8f,
	
	DUEL_CAMERA_MODE_ENEMYHAND,
	.5f,
	DUEL_CAMERA_MODE_HAND,
	.5f,
	DUEL_CAMERA_MODE_ENEMYHAND,
	.5f,
	DUEL_CAMERA_MODE_HAND,
	.5f,
	DUEL_CAMERA_MODE_ENEMYHAND,
	.5f,
	DUEL_CAMERA_MODE_HAND,
	.5f,
#endif	

	DUEL_CAMERA_MODE_FARSIDE2,
	1.0,

	-1,
	0,	
};



static int		gDuelCameraMode, gDuelCameraShotIndex;
static float	gDuelCameraTimer;

static	ObjNode	*gCurrentDuelerSubject;


/*********************** DRAW LENS FLARE ***************************/

void DrawLensFlare(OGLSetupOutputType *setupInfo)
{
short			i;
float			x,y,dot;
int				x2,y2;
OGLPoint3D		sunScreenCoord,from;
float			cx,cy;
float			dx,dy,length;
OGLVector3D		axis,lookAtVector,sunVector;
static OGLColorRGBA	transColor = {1,1,1,1};
int				px,py,pw,ph;
AGLContext 		agl_ctx = gGameViewInfoPtr->drawContext;

	if (!gDrawLensFlare)
		return;

	if (gSlowCPU)					// no lens flares if slow
		return;

	if (gIsPicking)
		return;

	
			/************/
			/* SET TAGS */
			/************/

	OGL_PushState();
		
	OGL_DisableLighting();												// Turn OFF lighting
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	SetColor4f(1,1,1,1);										// full white & alpha to start with


			/* CALC SUN COORD */
			
	from = setupInfo->cameraPlacement.cameraLocation;
	gSunCoord.x = from.x - (gWorldSunDirection.x * setupInfo->yon);
	gSunCoord.y = from.y - (gWorldSunDirection.y * setupInfo->yon);
	gSunCoord.z = from.z - (gWorldSunDirection.z * setupInfo->yon);



	/* CALC DOT PRODUCT BETWEEN VIEW AND LIGHT VECTORS TO SEE IF OUT OF RANGE */

	FastNormalizeVector(from.x - gSunCoord.x,
						from.y - gSunCoord.y,
						from.z - gSunCoord.z,
						&sunVector);

	FastNormalizeVector(setupInfo->cameraPlacement.pointOfInterest.x - from.x,
						setupInfo->cameraPlacement.pointOfInterest.y - from.y,
						setupInfo->cameraPlacement.pointOfInterest.z - from.z,
						&lookAtVector);

	dot = OGLVector3D_Dot(&lookAtVector, &sunVector);
	if (dot >= 0.0f)
		goto bye;

	dot = acos(dot) * -2.0f;				// get angle & modify it
	transColor.a = cos(dot);				// get cos of modified angle


			/* CALC SCREEN COORDINATE OF LIGHT */
			
	OGLPoint3D_Transform(&gSunCoord, &gWorldToWindowMatrix, &sunScreenCoord);


				/*************************/
				/* SEE IF SUN IS BLOCKED */
				/*************************/
		
	x2 = sunScreenCoord.x;
	y2 = sunScreenCoord.y;
				
	if ((x2 >= 0.0f) && (x2 < gGameWindowWidth) &&			// see if center is in window
		(y2 >= 0.0f) && (y2 < gGameWindowHeight))
	{
		GLfloat	zbuffer;
		
					/* SEE IF CENTER IS BLOCKED */
				
		y2 = gGameWindowHeight - y2;											// flip y since 0,0 is bottom left
		
		glReadPixels(x2, y2, 1,1, GL_DEPTH_COMPONENT, GL_FLOAT, &zbuffer);		// read z-buffer to see if flare is blocked
				
//		gScratchF = zbuffer;	//---------
		if (zbuffer < .999f)
			goto bye;
	}

		
			/* CALC CENTER OF VIEWPORT */
			
	OGL_GetCurrentViewport(setupInfo, &px, &py, &pw, &ph);
	cx = pw/2 + px;
	cy = ph/2 + py;


			/* CALC VECTOR FROM CENTER TO LIGHT */
			
	dx = sunScreenCoord.x - cx;
	dy = sunScreenCoord.y - cy;
	length = sqrt(dx*dx + dy*dy);	
	FastNormalizeVector(dx, dy, 0, &axis);
	

			/***************/
			/* DRAW FLARES */
			/***************/

			/* INIT MATRICES */
					
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();		

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
				
	for (i = 0; i < NUM_FLARES; i++)
	{			
		float	sx,sy,o,fx,fy;

		if (i == 0)
			gGlobalTransparency = .99;		// sun is always full brightness (leave @ < 1 to ensure GL_BLEND)
		else
			gGlobalTransparency = transColor.a;
				
		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][gFlareImageTable[i]].materialObject, setupInfo);		// activate material


		
		if (i == 1)												// always draw sun, but fade flares based on dot
		{
			if (transColor.a <= 0.0f)							// see if faded all out
				break;
			SetColor4fv((float *)&transColor);
		}

	
		o = gFlareOffsetTable[i];
		sx = gFlareScaleTable[i];
		sy = sx * gCurrentAspectRatio;
		
		x = cx + axis.x * length * o;
		y = cy + axis.y * length * o;
				
		fx = x / (pw/2) - 1.0f;
		fy = (ph-y) / (ph/2) - 1.0f;
				
		glBegin(GL_QUADS);		
		glTexCoord2f(0,0);	glVertex2f(fx - sx, fy - sy);
		glTexCoord2f(1,0);	glVertex2f(fx + sx, fy - sy);
		glTexCoord2f(1,1);	glVertex2f(fx + sx, fy + sy);
		glTexCoord2f(0,1);	glVertex2f(fx - sx, fy + sy);
		glEnd();				
	}

			/* RESTORE MODES */
			
bye:			
	gGlobalTransparency = 1.0f;
	OGL_PopState();
	
}


//===============================================================================================================================================================

#pragma mark -

/*************** INIT CAMERA: TERRAIN ***********************/
//
// This MUST be called after the players have been created so that we know
// where to put the camera.
//

void InitCamera_Terrain(void)
{		
ObjNode	*playerObj = gPlayerInfo.objNode;
		
	ResetCameraSettings();
					
	
			/******************************/
			/* SET CAMERA STARTING COORDS */
			/******************************/
			
	if (playerObj)
	{
		float	dx,dz,x,z,r;
	
		r = playerObj->Rot.y;
	
		dx = sin(r) * gCameraDistFromMe;
		dz = cos(r) * gCameraDistFromMe;
		
		x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x = playerObj->Coord.x + dx;
		z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z = playerObj->Coord.z + dz;
		gGameViewInfoPtr->cameraPlacement.cameraLocation.y = GetTerrainY(x, z) + 200.0f;

		gGameViewInfoPtr->cameraPlacement.pointOfInterest.x = playerObj->Coord.x;
		gGameViewInfoPtr->cameraPlacement.pointOfInterest.y = playerObj->Coord.y + gCameraLookAtYOff;
		gGameViewInfoPtr->cameraPlacement.pointOfInterest.z = playerObj->Coord.z;
	}
}



/******************** RESET CAMERA SETTINGS **************************/

static void ResetCameraSettings(void)
{	
	gCameraMode = CAMERA_MODE_NORMAL;

	gTopCamDist = 300.0f;

	gCameraFromAccel 	= 4.0;
	gCameraFromAccelY	= 2.9;
	
		
	gMinHeightOffGround = 60;

			/* SPECIAL SETTINGS FOR SAUCER LEVEL */
			
	gCameraLookAtAccel 	= 10.0;

	gCameraHeightFactor = 0.2;
	gMaxCameraHeightOff = 300.0f;
	gCameraLookAtYOff 	= DEFAULT_CAMERA_YOFF; 

	gCameraDistFromMe 	= CAMERA_DEFAULT_DIST_FROM_ME;

	gFreezeCameraFromXZ = gFreezeCameraFromY = false;
}





/******************** UPDATE CAMERA: TOP VIEW **************************/

static void UpdateCamera_TopView(void)
{
float	fps = gFramesPerSecondFrac;
OGLPoint3D	from,to;

			/* MOVE CAMERA TO FINAL HEIGHT */
			
	gTopCamDist += fps * 650.0f;
	if (gTopCamDist > 2200.0f)
		gTopCamDist = 2200.0f;
	
	
			/* SET CAM COORD */
			
	if (!gFreezeCameraFromXZ)
	{
		from.x = gPlayerInfo.coord.x;
		from.z = gPlayerInfo.coord.z + (.35f * gTopCamDist);
	}
	else
	{
		from.x = gPlayerInfo.camera.cameraLocation.x;
		from.z = gPlayerInfo.camera.cameraLocation.z;
	}
		
	if (!gFreezeCameraFromY)
		from.y = gPlayerInfo.coord.y + (.8f * gTopCamDist);
	else
		from.y = gPlayerInfo.camera.cameraLocation.y;
	
	
	to.x = gPlayerInfo.coord.x;
	to.y = gPlayerInfo.coord.y;
	to.z = gPlayerInfo.coord.z;

	
		/* PIN SO DOESN'T GET TOO CLOSE TO FENCE */
			
//	if (from.z < 2000.0f)
//		from.z = 2000.0f;
//	else
	if (from.z > 30500.0f)
		from.z = 30500.0f;
		
//	gScratch = from.z;	//-------
	
	
				/* UPDATE */

	OGL_UpdateCameraFromToUp(gGameViewInfoPtr,&from,&to, &up);
				
	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;
}


#pragma mark -

/*************** INIT CAMERA: DUEL ***********************/
//
// This MUST be called after the players have been created so that we know
// where to put the camera.
//

void InitCamera_Duel(void)
{		
ObjNode	*playerObj = gPlayerInfo.objNode;

	gDuelCameraMode = DUEL_CAMERA_MODE_FRONTTRACK;
	gDuelCameraTimer = 0.0f;
	
	gDuelCameraShotIndex = 0;
	
	gCameraSpinSpeed = 0;
	
	UpdateCamera_Duel();
}



/****************** UPDATE CAMERA: DUEL **********************/

void UpdateCamera_Duel(void)
{
OGLMatrix4x4	m;
OGLPoint3D	from,to, coord;
float	fps = gFramesPerSecondFrac;
ObjNode	*player = gPlayerInfo.objNode, *enemy;
int		newMode;
float	dist;

	from = gPlayerInfo.camera.cameraLocation;								// set from/to to existing values
	to = gPlayerInfo.camera.pointOfInterest;
				
	if (gDoneFaceOff)
		goto update;



		/**************************************/
		/* SEE IF TIME TO CHANGE CAMERA MODES */
		/**************************************/

	gDuelCameraTimer -= fps;
	if (gDuelCameraTimer <= 0.0f)
	{
		newMode = gDualShotList[gDuelCameraShotIndex].mode;
		gDuelCameraTimer = gDualShotList[gDuelCameraShotIndex].duration;
		gDuelCameraShotIndex++;
		
		if (newMode == -1)											// see if end
		{
			gDoneFaceOff = true;
			goto update;
		}
		
		gDuelCameraMode = newMode;
			
				/* INIT THE NEW MODE */
						
		switch(gDuelCameraMode)
		{
			case	DUEL_CAMERA_MODE_FRONTTRACK:
					from.y = player->Coord.y - 20.0f;
					from.z = player->Coord.z;					
					gGameViewInfoPtr->fov = .9f;
					break;

			case	DUEL_CAMERA_MODE_SIDETRACK:
					to.y = player->Coord.y;
					to.z = player->Coord.z;
					from.y = to.y - 20.0f;
					from.z = to.z + 400.0f;
					gGameViewInfoPtr->fov = .6f;
					break;
	
			case	DUEL_CAMERA_MODE_EYES:
					from.y = player->Coord.y + 60.0f;
					from.z = player->Coord.z;					
					to.y = from.y + 10.0f;
					to.z = from.z;
					gGameViewInfoPtr->fov = .5f;
					break;
					
			case	DUEL_CAMERA_MODE_HAND:
					from.y = player->Coord.y - 15.0f;
					from.z = player->Coord.z - 100.0f;					
					to.y = from.y;
					to.z = player->Coord.z;
					gGameViewInfoPtr->fov = .5f;
					break;
					
			case	DUEL_CAMERA_MODE_ENEMYFACE:
					gCurrentDuelerSubject = enemy = GetRandomDueler();
	
					GetEnemyHeadCoord(enemy, &coord);
					
					switch(enemy->DuelerNum)
					{
						case	0:									// center dueler
								to.x = coord.x;				
								to.z = coord.z;
								from.x = to.x - 100.0f;
								from.z = to.z;	
								break;

						case	2:									// left dueler
								to.x = coord.x;				
								to.z = coord.z;
								from.x = to.x;
								from.z = to.z + 100.0f;	
								break;

						case	1:									// right dueler
								to.x = coord.x;				
								to.z = coord.z;
								from.x = to.x;
								from.z = to.z - 100.0f;	
								break;
								
					}

					to.y = coord.y + 15.0f;
					from.y = to.y;
					gGameViewInfoPtr->fov = .5f;	
					break;
					
			case	DUEL_CAMERA_MODE_ENEMYFRONT:
					gCurrentDuelerSubject = enemy = GetRandomDueler();
	
					switch(enemy->DuelerNum)
					{
						case	0:									// center dueler
								to.x = enemy->Coord.x;				
								to.z = enemy->Coord.z;
								from.x = to.x - 300.0f;
								from.z = to.z;	
								break;

						case	2:									// left dueler
								to.x = enemy->Coord.x;				
								to.z = enemy->Coord.z;
								from.x = to.x;
								from.z = to.z + 300.0f;	
								break;

						case	1:									// right dueler
								to.x = enemy->Coord.x;				
								to.z = enemy->Coord.z;
								from.x = to.x;
								from.z = to.z - 300.0f;	
								break;
								
					}

					to.y = enemy->Coord.y + 30.0f;
					from.y = to.y - 20.0f;
					gGameViewInfoPtr->fov = .8f;	
					break;
					
			case	DUEL_CAMERA_MODE_ENEMYHAND:
					gCurrentDuelerSubject = enemy = GetRandomDueler();
	
					GetEnemyHandCoord(enemy, &coord);

					switch(enemy->DuelerNum)
					{
						case	0:									// center dueler
								to.x = coord.x;				
								to.z = coord.z;
								from.x = to.x;
								from.z = to.z - 140.0f;
								break;

						case	2:									// left dueler
								to.x = coord.x;				
								to.z = coord.z;
								from.x = to.x - 140.0f;
								from.z = to.z;
								break;

						case	1:									// right dueler
								to.x = coord.x;				
								to.z = coord.z;
								from.x = to.x + 140.0f;
								from.z = to.z;
								break;
								
					}
					to.y = coord.y - 10.0f;
					from.y = to.y;
					gGameViewInfoPtr->fov = .5f;	
					break;
					
			case	DUEL_CAMERA_MODE_EOTS:
					enemy = gDuelers[0];							// get center dueler
						
					from.x = enemy->Coord.x + 130.0f;
					from.y = enemy->Coord.y + 70.0f;
					from.z = enemy->Coord.z - 40.0f;
					
					to.x = player->Coord.x;
					to.y = player->Coord.y;
					to.z = player->Coord.z + 100.0f;

					gGameViewInfoPtr->fov = .4f;	
					break;
					
			case	DUEL_CAMERA_MODE_POTS:
					enemy = gDuelers[0];							// get center dueler

					from.x = player->Coord.x - 130.0f;
					from.y = player->Coord.y + 50.0f;
					from.z = player->Coord.z - 70.0f;
					
					to.x = enemy->Coord.x;
					to.y = enemy->Coord.y - 40.0f;
					to.z = enemy->Coord.z + 100.0f;

					gGameViewInfoPtr->fov = .4f;	
					break;		
					
			case	DUEL_CAMERA_MODE_TOP:
						
						
					from.z = player->Coord.z;
						
					to.y = player->Coord.y;
					to.z = from.z;
						
					gGameViewInfoPtr->fov = 1.3f;	
					break;
					
			case	DUEL_CAMERA_MODE_FARSIDE:
						
					to.y = player->Coord.y;
					from.y = to.y + 90.0f;
					to.z = player->Coord.z;
												
					gGameViewInfoPtr->fov = 1.0f;	
					break;
					
					
			case	DUEL_CAMERA_MODE_FARSIDE2:
					enemy = gDuelers[0];							// get center dueler

					to.y = player->Coord.y;
					from.y = to.y + 90.0f;


					dist = OGLPoint3D_Distance(&enemy->Coord, &player->Coord);	// get dist to enemy
					from.x = (enemy->Coord.x + player->Coord.x) * .5f;
					to.x = from.x;
					from.z = (player->Coord.z - dist) * 0.95f;

					gGameViewInfoPtr->fov = 1.0f;	
					
					break;
						
		}
	}
	
			/***********************/	
			/* SET CORRECT FROM/TO */
			/***********************/
	
update:	
	switch(gDuelCameraMode)
	{
				/*****************************/
				/* CAMERA IN FRONT, TRACKING */
				/*****************************/
				
		case	DUEL_CAMERA_MODE_FRONTTRACK:
				from.x = player->Coord.x + 250.0f;				
				to = player->Coord;
				break;

		case	DUEL_CAMERA_MODE_SIDETRACK:
				from.x = player->Coord.x + 25.0f;
				to.x = from.x;
				break;
				
		case	DUEL_CAMERA_MODE_EYES:
				from.x = player->Coord.x + 90.0f;
				to.x = player->Coord.x;
				break;
				
		case	DUEL_CAMERA_MODE_HAND:
				from.x = player->Coord.x + 4.0f;
				to.x = player->Coord.x - 4.0f;
				break;

		case	DUEL_CAMERA_MODE_ENEMYFACE:
		case	DUEL_CAMERA_MODE_ENEMYFRONT:
				switch(gCurrentDuelerSubject->DuelerNum)
				{
					case	0:						// center dueler
							from.x -= fps * 6.0f;
							break;
							
					case	2:						// left
							from.z += fps * 6.0f;
							break;
							
					case	1:						// right
							from.z -= fps * 6.0f;
							break;
				}
				break;

		case	DUEL_CAMERA_MODE_ENEMYHAND:
				switch(gCurrentDuelerSubject->DuelerNum)
				{
					case	0:						// center dueler
							from.x -= fps * 3.0f;
							break;
							
					case	2:						// left
							from.z += fps * 3.0f;
							break;
							
					case	1:						// right
							from.z -= fps * 3.0f;
							break;
				}
				break;

		case	DUEL_CAMERA_MODE_EOTS:
				from.y += fps * 2.0f;
				break;

		case	DUEL_CAMERA_MODE_POTS:
				from.x += fps * 3.0f;
				from.y += fps * 2.0f;
				break;


		case	DUEL_CAMERA_MODE_TOP:
				enemy = gDuelers[0];										// get center dueler
				dist = OGLPoint3D_Distance(&enemy->Coord, &player->Coord);	// get dist to enemy
				from.x = (enemy->Coord.x + player->Coord.x) * .5f;
				from.y = player->Coord.y + dist * 1.3f;
				to.x = from.x - 10.0f;
				break;
				
		case	DUEL_CAMERA_MODE_FARSIDE:
				enemy = gDuelers[0];										// get center dueler
				dist = OGLPoint3D_Distance(&enemy->Coord, &player->Coord);	// get dist to enemy
				from.x = (enemy->Coord.x + player->Coord.x) * .5f;
				to.x = from.x;
				from.z = (player->Coord.z - dist) * 0.95f;
				break;

		case	DUEL_CAMERA_MODE_FARSIDE2:
				from.y += fps * 10.0f;
				
				gCameraSpinSpeed += fps * .1f;			// accelerate spinning
				if (gCameraSpinSpeed > .5f)
					gCameraSpinSpeed = .5;
				
				OGLMatrix4x4_SetRotateAboutPoint(&m, &to, 0, gCameraSpinSpeed * fps, 0);
				OGLPoint3D_Transform(&from, &m, &from);
				
				break;

	}


				/* UPDATE */

	OGL_UpdateCameraFromToUp(gGameViewInfoPtr,&from,&to, &up);

	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;


}


#pragma mark -



/****************** UPDATE CAMERA: SHOOTOUT **********************/

void UpdateCamera_Shootout(void)
{
OGLPoint3D	from,to;
ObjNode		*player = gPlayerInfo.objNode;
const OGLPoint3D off = {0,4,-5};
const OGLVector3D	f = {0,0,-1};
OGLMatrix4x4	m;
OGLVector3D	look;

	from 	= gPlayerInfo.camera.cameraLocation;								// set from/to to existing values
	to 		= gPlayerInfo.camera.pointOfInterest;

	FindJointFullMatrix(player,PLAYER_JOINT_HEAD,&m);
	OGLPoint3D_Transform(&off, &m, &from);
	OGLVector3D_Transform(&f, &m, &look);

	FindCoordOnJoint(player, PLAYER_JOINT_HEAD, &off, &from);	
	
	to.x = from.x + look.x;
	to.z = from.z + look.z;
	to.y = from.y + look.y;

	OGL_UpdateCameraFromToUp(gGameViewInfoPtr,&from,&to, &up);

	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;

}




#pragma mark -

/************************ PRIME STAMPEDE CAMERA *************************/

Boolean PrimeStampedeCamera(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;
ObjNode			*player = gPlayerInfo.objNode;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE EVENT OBJECT */
				
	gNewObjectDefinition.genre		= EVENT_GENRE;				
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB + 100;
	gNewObjectDefinition.moveCall 	= MoveStampedeCameraOnSpline;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;			
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveStampedeCameraOnSpline;					// set move call
	
	
	OGL_UpdateCameraFromToUp(gGameViewInfoPtr,&newObj->Coord, &player->Coord, &up);
	
	return(true);
}


/********************** MOVE STAMPEDE CAMERA ON SPLINE ********************/

static void MoveStampedeCameraOnSpline(ObjNode *camera)
{
const ObjNode		*player = gPlayerInfo.objNode;
float		distToPlayer, speedToMove;
OGLPoint3D	from,to;
int			splineNum = camera->SplineNum;

	distToPlayer = CalcDistance(camera->Coord.x, camera->Coord.z, player->Coord.x, player->Coord.z);

	speedToMove = 1500.0f - distToPlayer;


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(camera, speedToMove, true);
	GetObjectCoordOnSpline(camera);
	
	
	

			/* UPDATE CAMERA */

	from.x = camera->Coord.x;	
	from.z = camera->Coord.z;	
	from.y = camera->Coord.y = GetTerrainY(from.x, from.z) + 300.0f;

	GetCoordOnSpline2(&(*gSplineList)[splineNum], camera->SplinePlacement, -600, &to.x, &to.z, false);		// get next coord					
	to.y = from.y - 170.0f;
		
	
	OGL_UpdateCameraFromToUp(gGameViewInfoPtr,&from,&to, &up);
	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;
}



#pragma mark -

/*************** INIT CAMERA: TARGET PRACTICE ***********************/

void InitCamera_TargetPractice(void)
{		

}


#pragma mark -

/********************** PREP ANAGLYPH CAMERAS ***************************/
//
// Make a copy of the camera's real coord info before we do anaglyph offsetting.
//

void PrepAnaglyphCameras(void)
{
	gAnaglyphCameraBackup = gGameViewInfoPtr->cameraPlacement;

}

/********************** RESTORE CAMERAS FROM ANAGLYPH ***************************/

void RestoreCamerasFromAnaglyph(void)
{
	gGameViewInfoPtr->cameraPlacement = gAnaglyphCameraBackup;
}


/******************** CALC ANAGLYPH CAMERA OFFSET ***********************/

void CalcAnaglyphCameraOffset(short pass)
{
OGLVector3D	aim;
OGLVector3D	xaxis;
float		sep = gAnaglyphEyeSeparation;
const	OGLVector3D up = {0,1,0};

	if (pass > 0)
		sep = -sep;

			/* CALC CAMERA'S X-AXIS */
			
	aim.x = gAnaglyphCameraBackup.pointOfInterest.x - gAnaglyphCameraBackup.cameraLocation.x;
	aim.y = gAnaglyphCameraBackup.pointOfInterest.y - gAnaglyphCameraBackup.cameraLocation.y;
	aim.z = gAnaglyphCameraBackup.pointOfInterest.z - gAnaglyphCameraBackup.cameraLocation.z;
	OGLVector3D_Normalize(&aim, &aim);

	OGLVector3D_Cross(&up, &aim, &xaxis); 


				/* OFFSET CAMERA FROM */
				
	gGameViewInfoPtr->cameraPlacement.cameraLocation.x = gAnaglyphCameraBackup.cameraLocation.x + (xaxis.x * sep);
	gGameViewInfoPtr->cameraPlacement.cameraLocation.z = gAnaglyphCameraBackup.cameraLocation.z + (xaxis.z * sep);
	
	
				/* OFFSET CAMERA TO */
				
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.x = gAnaglyphCameraBackup.pointOfInterest.x + (xaxis.x * sep);
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.z = gAnaglyphCameraBackup.pointOfInterest.z + (xaxis.z * sep);
	
	
}







