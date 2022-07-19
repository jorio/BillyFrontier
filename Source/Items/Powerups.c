/****************************/
/*   	POWERUPS.C		    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "3dmath.h"
#include <aglmacro.h>


extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGravity,gPlayerBottomOff,gGlobalTransparency;
extern	OGLPoint3D			gCoord;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLBoundingBox 		gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLSetupOutputType	*gGameViewInfoPtr;
extern	u_long				gAutoFadeStatusBits;
extern	Boolean				gG4;
extern	PlayerInfoType		gPlayerInfo;
extern	SplineDefType	**gSplineList;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	float				gCameraDistFromMe,gCameraLookAtYOff;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MovePowerupVanish(ObjNode *pow);



/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

#define	FlapIndex	SpecialF[0]
#define	HoverWobble	SpecialF[1]			// small local wobble
#define	Metabolism	SpecialF[2]
#define	FlyWobble	SpecialF[3]			// larger up/down wobble in butterfly flight

#define	SpinRadius	SpecialF[0]
#define	SpinRot		SpecialF[1]

#define	POWKind		Special[0]
#define	Regenerate	Flag[0]


#pragma mark -


/************************* MAKE POWERUP ***************************/

ObjNode *MakePOW(int powKind, OGLPoint3D *where)
{
ObjNode	*pow;
static	short	powToModel[1] =
{
//	GLOBAL_ObjType_HealthPOW,
};


				/********************/
				/* MAKE GENERAL POW */
				/********************/

	gNewObjectDefinition.type 		= powToModel[powKind];
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
				
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= POW_SLOT;
	gNewObjectDefinition.moveCall 	= MovePowerup;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0f;
	
	switch(powKind)								// set scale of POW item
	{
		case	POW_KIND_HEALTH:
				gNewObjectDefinition.scale 		= 1.6f;
				break;

		case	POW_KIND_GREENCLOVER:
		case	POW_KIND_BLUECLOVER:
		case	POW_KIND_GOLDCLOVER:
				gNewObjectDefinition.flags 		|= STATUS_BIT_DOUBLESIDED;
				gNewObjectDefinition.scale 		= .4f;
				break;
				
		case	POW_KIND_MAP:
				gNewObjectDefinition.flags 		|= STATUS_BIT_DOUBLESIDED;
				gNewObjectDefinition.scale 		= 3.0f;
				break;
				
		case	POW_KIND_SHIELD:
				break;

	}
	
	pow = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	pow->Kind = PICKUP_KIND_POW;			// pickup kind is a POW
	pow->POWKind = powKind;					// also set POW kind


			/* SET COLLISION STUFF */

	pow->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
	pow->CBits		= CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox_Maximized(pow);

	pow->TriggerCallback = DoTrig_Powerup;

	switch(powKind)								// set timer before trigger becomes active
	{
		case	POW_KIND_GREENCLOVER:
		case	POW_KIND_BLUECLOVER:
		case	POW_KIND_GOLDCLOVER:	
				pow->Timer = .05;					// clovers are instant
				break;
				
		default:
				pow->Timer = .4f;	
	}							


			/* MAKE SHADOW */
			
	AttachShadowToObject(pow, 0, 1.5,1.5, true);

	return(pow);
}


/********************** MOVE POWERUP *****************************/

void MovePowerup(ObjNode *theNode)
{
			/* MAKE TRIGGER ACTIVE */
			
	if (theNode->Timer > 0.0f)
	{
		theNode->Timer -= gFramesPerSecondFrac;
		if (theNode->Timer <= 0.0f)
			theNode->CType |= CTYPE_TRIGGER;
	}

			/* DO CUSTOM STUFF */
			
	switch(theNode->POWKind)
	{
		case	POW_KIND_SHIELD:
				theNode->Rot.y += gFramesPerSecondFrac * 7.0f;
				break;


	}
	
	
			/* DO DEFAULT MOVE CALL */
			
	MoveDefaultPickup(theNode);
}


/************** DO TRIGGER - POWERUP ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_Powerup(ObjNode *pow, ObjNode *who, Byte sideBits)
{
	who; sideBits;
			
	switch(pow->POWKind)
	{
				/* GET HEALTH */
				
		case	POW_KIND_HEALTH:
				gPlayerInfo.health += .15f;
				if (gPlayerInfo.health > 1.0f)
					gPlayerInfo.health = 1.0f;
				break;

				

				/* FREE LIFE */
				
		case	POW_KIND_FREELIFE:
				gPlayerInfo.lives++;
				break;

				
	}
			
			
	StartPowerupVanish(pow);
	return(true);
}


#pragma mark -

/********************** START POWERUP VANISH **********************/

void StartPowerupVanish(ObjNode *pow)
{
	pow->InitCoord = pow->Coord;							// set central axis

	pow->SpinRadius = 10.0f;
	pow->SpinRot = 0;
	pow->DeltaRot.y = 20.0f;

	pow->MoveCall = MovePowerupVanish; 
	pow->StatusBits &= ~STATUS_BIT_NOMOVE;			// make sure it can move
	pow->CType = 0;
	
	if (pow->ChainHead)								// see if was attached to something
	{
		pow->ChainHead->ChainNode = nil;			// detach from chain
		pow->ChainHead = nil;	
	}
	
}


/******************** MOVE POWERUP VANISH ********************/

static void MovePowerupVanish(ObjNode *pow)
{
float	fps = gFramesPerSecondFrac;
float	r,r2;

	GetObjectInfo(pow);

	r2 = pow->SpinRadius += fps * 30.0f;	
	
	pow->DeltaRot.y += fps * 15.0f;
	r = pow->SpinRot += pow->DeltaRot.y * fps;
		
	pow->Rot.y -= (pow->DeltaRot.y * fps) * .1f;
	pow->Rot.x -= (pow->DeltaRot.y * fps) * .15f;
		
	gCoord.x = pow->InitCoord.x + sin(r) * r2;
	gCoord.z = pow->InitCoord.z + cos(r) * r2;
	gCoord.y += 3.0f * (pow->DeltaRot.y * fps);
	
	pow->ColorFilter.a -= fps * .6f;
	if (pow->ColorFilter.a <= 0.0f)
	{
		DeleteObject(pow);
		return;
	}
	
	UpdateObject(pow);
}


#pragma mark -


/******************* ADD POW **************************/

Boolean AddPOW(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*pow;
OGLPoint3D	where;

	where.x	= x;
	where.y = FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN|CTYPE_WATER) + 30.0f;
	where.z	= z;

	pow = MakePOW(itemPtr->parm[0], &where);
	if (pow)
		pow->TerrainItemPtr = itemPtr;			// keep ptr to item list

	return(true);
}












