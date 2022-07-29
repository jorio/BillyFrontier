/****************************/
/*   		ITEMS.C		    */
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "3dmath.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gCurrentAspectRatio,gTerrainPolygonSize;
extern	OGLPoint3D			gCoord;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLBoundingBox 		gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLSetupOutputType	*gGameViewInfoPtr;
extern	u_long				gAutoFadeStatusBits,gGlobalMaterialFlags;
extern	Boolean				gG4;
extern	PlayerInfoType		gPlayerInfo;
extern	SplineDefType	**gSplineList;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	short				gNumEnemies;
extern	SpriteType	*gSpriteGroupList[];
extern	AGLContext		gAGLContext;
extern	int					gCurrentArea;
extern	u_long				gScore;


/****************************/
/*    PROTOTYPES            */
/****************************/


static void BulletHitWoodCrateCallback(ObjNode *bullet, ObjNode *crate, const OGLPoint3D *impactPt);
static void MakeShieldPOW(float x, float z, float rot);
static void BulletHitShieldPOW(ObjNode *bullet, ObjNode *pow, const OGLPoint3D *impactPt);
static void MovePOW_Spin(ObjNode *theNode);
static void MoveShieldPOW_Vanish(ObjNode *theNode);
static void MakeAmmoPOW(float x, float z, float rot, int numBullets);
static void BulletHitAmmoPOW(ObjNode *bullet, ObjNode *pow, const OGLPoint3D *impactPt);
static void MovePOW_Vanish(ObjNode *theNode);
static void MoveCrateDebris(ObjNode *theNode);
static ObjNode *MakePesoPOW(float x, float z);
static void MovePesoPOW(ObjNode *theNode);
static void BulletHitPesoPOW(ObjNode *bullet, ObjNode *pow, const OGLPoint3D *impactPt);
static ObjNode *MakeFreeLifePOW(float x, float z, float rot);
static void BulletHitFreeLifePOW(ObjNode *bullet, ObjNode *pow, const OGLPoint3D *impactPt);
static Boolean DoTrig_FreeLife(ObjNode *item, ObjNode *who, Byte sideBits);
static void MoveTumbleweedOnSpline(ObjNode *theNode);
static Boolean DoTrig_ExplodeItem(ObjNode *item, ObjNode *who, Byte sideBits);
static Boolean DoTrig_Peso(ObjNode *item, ObjNode *who, Byte sideBits);
static void KangaHitByBulletCallback(ObjNode *bullet, ObjNode *cow, const OGLPoint3D *impactPt);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	LEAF_DEFAULT_WOBBLE_MAG		4.0f
#define	LEAF_DEFAULT_WOBBLE_SPEED	1.0f

/*********************/
/*    VARIABLES      */
/*********************/

#define	WaveXIndex	SpecialF[0]
#define	WaveZIndex	SpecialF[1]



#define	Wobble		SpecialF[0]
#define WobbleMag	SpecialF[1]
#define	WobbleSpeed	SpecialF[2]

#define	NumBullets	Special[0]


ObjNode	*gCyc = nil;


/********************* INIT ITEMS MANAGER *************************/

void InitItemsManager(void)
{
		
	CreateCyclorama();
	
}


/************************* CREATE CYCLORAMA *********************************/

void CreateCyclorama(void)
{

	switch(gCurrentArea)
	{
		case	AREA_SWAMP_STAMPEDE:
		case	AREA_TOWN_STAMPEDE:
		case	AREA_TARGETPRACTICE1:
		case	AREA_TARGETPRACTICE2:
				return;
	}

	gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 	= 0;								// cyc is always 1st model in level bg3d files
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags 	= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 	= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale 	= gGameViewInfoPtr->yon * .995f / 10.0f;
	gCyc = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gCyc->CustomDrawFunction = DrawCyclorama;		
}


/********************** DRAW CYCLORAMA *************************/

void DrawCyclorama(ObjNode *theNode, const OGLSetupOutputType *setupInfo)
{
OGLPoint3D cameraCoord = setupInfo->cameraPlacement.cameraLocation;

		/* UPDATE CYCLORAMA COORD INFO */
		
	theNode->Coord.x = cameraCoord.x;
	theNode->Coord.y = cameraCoord.y + theNode->TargetOff.y;
	theNode->Coord.z = cameraCoord.z;
	UpdateObjectTransforms(theNode);	



			/* DRAW THE OBJECT */
			
	MO_DrawObject(theNode->BaseGroup, setupInfo);
}


#pragma mark -


/************************* ADD BUILDING *********************************/

Boolean AddBuilding(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_BUILDINGS;	
	gNewObjectDefinition.type 		= BUILDING_ObjType_Saloon + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;

	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y;

	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/4);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE|CTYPE_HITENEMYBULLET|CTYPE_BUILDING;

	return(true);													// item was added
}


#pragma mark -


/************************* ADD DUEL ROCKWALL *********************************/

Boolean AddDuelRockWall(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_RockWall;
	gNewObjectDefinition.scale 		= 18.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 80;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


	return(true);													// item was added
}


/************************* ADD HEADSTONE *********************************/

Boolean AddHeadStone(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_Headstone1 + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= .6f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 57;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);													// item was added
}


/************************* ADD PLANT *********************************/

Boolean AddPlant(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	switch(gCurrentArea)
	{
		case	AREA_TOWN_SHOOTOUT:
		case	AREA_TOWN_DUEL1:
		case	AREA_TOWN_DUEL2:
		case	AREA_TOWN_DUEL3:
		case	AREA_TOWN_STAMPEDE:
				gNewObjectDefinition.type 		= TOWN_ObjType_Cactus + itemPtr->parm[0];
				break;
				
		case	AREA_SWAMP_SHOOTOUT:
		case	AREA_SWAMP_DUEL1:
		case	AREA_SWAMP_DUEL2:
		case	AREA_SWAMP_DUEL3:
		case	AREA_SWAMP_STAMPEDE:
				gNewObjectDefinition.type 		= SWAMP_ObjType_MushroomTree + itemPtr->parm[0];
				break;
	}		
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.scale 		= 2.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 57;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list



	newObj->CType = CTYPE_MISC | CTYPE_TRIGGERENEMYONLY;
	newObj->CBits = CBITS_ALLSOLID | CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox(newObj, .8,1);
	newObj->TriggerCallback = DoTrig_ExplodeItem;

	return(true);													// item was added
}


/************************* ADD COFFIN *********************************/

Boolean AddCoffin(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_Coffin + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 2.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 101;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE;
	newObj->HitByBulletCallback = DefaultBulletHitCallback;
	newObj->Health = 3.0f;

	return(true);													// item was added
}



/************************* ADD BARREL *********************************/

Boolean AddBarrel(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Barrel + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 210;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE;
	newObj->HitByBulletCallback = DefaultBulletHitCallback;
	newObj->Health = 2.0f;
	
	if (itemPtr->parm[0] == 1)		// is TNT?
		newObj->What = WHAT_TNT;
		
	return(true);													// item was added
}


/************************* ADD CRATE *********************************/

Boolean AddWoodCrate(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];
									
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Crate + type;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 101;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE | CTYPE_MISC;
	CalcObjectBoxFromNode(newObj);
	
	if (type >= 2)
		newObj->HitByBulletCallback = nil;		// metal crates cannot be busted
	else
		newObj->HitByBulletCallback = BulletHitWoodCrateCallback;

	newObj->Kind = itemPtr->parm[2];								// remember what kind of contents are in this crate

	return(true);													// item was added
}

/******************* BULLET HIT WOOD CRATE CALLBACK **********************/

static void BulletHitWoodCrateCallback(ObjNode *bullet, ObjNode *crate, const OGLPoint3D *impactPt)
{
OGLPoint3D	puffPt;
OGLVector3D	v;
ObjNode		*newObj;
int			i;

	bullet;

			/*****************/
			/* MAKE CONTENTS */
			/*****************/

	switch(crate->Kind)
	{
		case	0:								// empty			
				break;

				/* SHIELD POW */
				
		case	1:
				MakeShieldPOW(crate->Coord.x, crate->Coord.z, crate->Rot.y);
				break;


				/* AMMO POW - 6 SIX */
				
		case	2:
				MakeAmmoPOW(crate->Coord.x, crate->Coord.z, crate->Rot.y, 6);
				break;


				/* PESO POW */
				
		case	3:
				for (i = 0; i < 8; i++)
					MakePesoPOW(crate->Coord.x, crate->Coord.z);
				break;

				/* FREE LIFE POW */
				
		case	4:
				MakeFreeLifePOW(crate->Coord.x, crate->Coord.z, crate->Rot.y);
				break;


				/* AMMO POW - 12 SIX */
				
		case	5:
				MakeAmmoPOW(crate->Coord.x, crate->Coord.z, crate->Rot.y, 12);
				break;


	}	


			/*****************/
			/* BLOW UP CRATE */
			/*****************/

	for (i = 0; i < 10; i++)
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
		gNewObjectDefinition.type 		= GLOBAL_ObjType_CrateDebris0 + i;
		gNewObjectDefinition.scale 		= 1.0f;
		gNewObjectDefinition.coord		= crate->Coord;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall 	= MoveCrateDebris;
		gNewObjectDefinition.rot 		= RandomFloat() * PI2;	
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		
		newObj->Delta.x = RandomFloat2() * 300.0f;
		newObj->Delta.z = RandomFloat2() * 300.0f;
		newObj->Delta.y = RandomFloat() * 300.0f;
				
		newObj->DeltaRot.x = RandomFloat2() * PI2;
		newObj->DeltaRot.y = RandomFloat2() * PI2;
		newObj->DeltaRot.z = RandomFloat2() * PI2;
				
		newObj->Health = 1.0f;
		
		UpdateObjectTransforms(newObj);
		
		AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2, 2,false);
		
	}

	PlayEffect3D(EFFECT_CRATEEXPLODE, &crate->Coord);
			
	DeleteObject(crate);


			/********************/
			/* MAKE IMPACT PUFF */
			/********************/

		/* MOVE PUFF AWAY FROM IMPACT PT A BIT */
						
	v.x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x - impactPt->x;
	v.y = gGameViewInfoPtr->cameraPlacement.cameraLocation.y - impactPt->y;
	v.z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z - impactPt->z;
	FastNormalizeVector(v.x, v.y, v.z, &v);
		
	puffPt.x = impactPt->x + (v.x * 20.0f);
	puffPt.y = impactPt->y + (v.y * 20.0f);
	puffPt.z = impactPt->z + (v.z * 20.0f);
			
	MakePuff(&puffPt, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);				
}


/************************* MOVE CRATE DEBRIS *****************************/

static void MoveCrateDebris(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	y;

	GetObjectInfo(theNode);

	gDelta.y -= 1000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
	
	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;

#if 1
	y = GetTerrainY(gCoord.x, gCoord.z);
	if (gCoord.y <= y)
	{
		theNode->Health -= .3f;
		if (theNode->Health <= 0.0f)
		{
			DeleteObject(theNode);
			return;
		}
		gCoord.y = y;
		gDelta.y *= -.5f;
		
	}
#endif	

	UpdateObject(theNode);
}


#pragma mark -

/******************** MAKE SHIELD POW **************************/

static void MakeShieldPOW(float x, float z, float rot)
{
ObjNode		*newObj;

			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_ShieldPOW;
	gNewObjectDefinition.scale 		= .3;
	gNewObjectDefinition.coord.x	= x;
	gNewObjectDefinition.coord.z	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 199;
	gNewObjectDefinition.moveCall 	= MovePOW_Spin;
	gNewObjectDefinition.rot 		= rot;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
			
	newObj->HitByBulletCallback = BulletHitShieldPOW;
							
	newObj->CType = CTYPE_PICKABLE;


}



/******************* BULLET HIT SHIELD POW **********************/

static void BulletHitShieldPOW(ObjNode *bullet, ObjNode *pow, const OGLPoint3D *impactPt)
{
	if (bullet->What == WHAT_ENEMYBULLET)			// only player bullets can get these
		return;

	MakePuff(impactPt, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);				

	pow->MoveCall = MoveShieldPOW_Vanish;
	
	pow->CType &= ~CTYPE_PICKABLE;			// cant shoot it again
	
	gPlayerInfo.shieldPower += 3.0f;
	if (gPlayerInfo.shieldPower > MAX_SHIELD)
		gPlayerInfo.shieldPower = MAX_SHIELD;
}


/******************** MOVE SHIELD POW: VANISH ********************/

static void MoveShieldPOW_Vanish(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*node2;

			/* FADE OUT */
			
	theNode->ColorFilter.a -= fps;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

			/* MOVE SPHERE 1 */
			
	GetObjectInfo(theNode);

	gDelta.y += 3500.0f * fps;
	gCoord.y += gDelta.y * fps;
	
	theNode->Rot.y += fps * theNode->DeltaRot.y;
	theNode->Rot.x += fps * .1f;

	UpdateObject(theNode);
	
	
			/* MOVE SPHERE 2 */
			
	node2 = theNode->ChainNode;
	if (node2)
	{
		node2->Rot.y += fps * node2->DeltaRot.y;
		node2->Rot.x += fps * .1f;
		UpdateObject(node2);
		
		node2->ColorFilter.a = theNode->ColorFilter.a;
	}
}


#pragma mark -

/******************** MAKE AMMO POW **************************/

static void MakeAmmoPOW(float x, float z, float rot, int numBullets)
{
ObjNode		*newObj;

			/* MAKE SPHERE 1 */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_AmmoBoxPOW;
	gNewObjectDefinition.scale 		= .5f;
	gNewObjectDefinition.coord.x	= x;
	gNewObjectDefinition.coord.z	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 499;
	gNewObjectDefinition.moveCall 	= MovePOW_Spin;
	gNewObjectDefinition.rot 		= rot;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CType = CTYPE_PICKABLE;			
	newObj->HitByBulletCallback = BulletHitAmmoPOW;
	
	newObj->NumBullets = numBullets;
						
}


/******************* BULLET HIT AMMO POW **********************/

static void BulletHitAmmoPOW(ObjNode *bullet, ObjNode *pow, const OGLPoint3D *impactPt)
{
	if (bullet->What == WHAT_ENEMYBULLET)			// only player bullets can get these
		return;

	MakePuff(impactPt, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);				
	PlayEffect3D(EFFECT_RELOAD, impactPt);

	pow->MoveCall = MovePOW_Vanish;
	
	pow->CType &= ~CTYPE_PICKABLE;			// cant shoot it again
	
	gPlayerInfo.ammoCount += pow->NumBullets;
}


/******************** MOVE POW: SPIN *********************/

static void MovePOW_Spin(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Rot.y += gFramesPerSecondFrac * PI;

	UpdateObjectTransforms(theNode);
	UpdateShadow(theNode);										// prime it
}


/******************** MOVE POW: VANISH ********************/

static void MovePOW_Vanish(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* FADE OUT */
			
	theNode->ColorFilter.a -= fps;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

			/* MOVE SPHERE 1 */
			
	GetObjectInfo(theNode);

	gDelta.y += 3500.0f * fps;
	gCoord.y += gDelta.y * fps;
	
	theNode->Rot.y += fps * 17.0f;

	UpdateObject(theNode);
	
	
}




#pragma mark -

/****************** ADD FREE LIFE POW ************************/

Boolean AddFreeLifePOW(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	newObj = MakeFreeLifePOW(x,z, RandomFloat() * PI2);
									
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);													// item was added
}


/******************** MAKE FREE LIFE POW **************************/

static ObjNode *MakeFreeLifePOW(float x, float z, float rot)
{
ObjNode		*newObj;
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_FreeLifePOW;
	gNewObjectDefinition.scale 		= .6f;
	gNewObjectDefinition.coord.x	= x;
	gNewObjectDefinition.coord.z	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 420;
	gNewObjectDefinition.moveCall 	= MovePOW_Spin;
	gNewObjectDefinition.rot 		= rot;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CType = CTYPE_PICKABLE | CTYPE_TRIGGER;			
	newObj->CBits = CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);
	newObj->HitByBulletCallback = BulletHitFreeLifePOW;		
	newObj->TriggerCallback = DoTrig_FreeLife;
		
		
	return(newObj);				
}


/******************* BULLET HIT FREE LIFE POW **********************/

static void BulletHitFreeLifePOW(ObjNode *bullet, ObjNode *pow, const OGLPoint3D *impactPt)
{
	if (bullet)
		if (bullet->What == WHAT_ENEMYBULLET)			// only player bullets can get these
			return;

	MakePuff(impactPt, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);				
	PlayEffect3D(EFFECT_RELOAD, impactPt);

	pow->MoveCall = MovePOW_Vanish;
	
	pow->CType = 0;			// cant shoot or touch it again
	
	gPlayerInfo.lives++;
//	if (gPlayerInfo.lives > 3)
//		gPlayerInfo.lives = 3;
}

/************** DO TRIGGER - FREE LIFE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_FreeLife(ObjNode *item, ObjNode *who, Byte sideBits)
{
	who; sideBits;
	
	BulletHitFreeLifePOW(nil, item, &item->Coord);
	
	return(false);
}


#pragma mark -


/************************* ADD PESO *********************************/

Boolean AddPeso(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
			
			
	newObj = MakePesoPOW(x, z);
									
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Delta.x = newObj->Delta.y = newObj->Delta.z = 0;

	return(true);													// item was added
}

/******************** MAKE PESO POW **************************/

static ObjNode *MakePesoPOW(float x, float z)
{
ObjNode		*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_PesoPOW;
	gNewObjectDefinition.scale 		= .4f;
	gNewObjectDefinition.coord.x	= x;
	gNewObjectDefinition.coord.z	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 20.0f;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 381;
	gNewObjectDefinition.moveCall 	= MovePesoPOW;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CType = CTYPE_PICKABLE | CTYPE_TRIGGER;			
	newObj->CBits = CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);
	newObj->HitByBulletCallback = BulletHitPesoPOW;
	newObj->TriggerCallback = DoTrig_Peso;
	
	newObj->Delta.x = RandomFloat2() * 200.0f;
	newObj->Delta.z = RandomFloat2() * 200.0f;
	newObj->Delta.y = 400.0f  + RandomFloat() * 400.0f;

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2, 2, false);




	return(newObj);						
}


/********************* MOVE PESO POW ****************************/

static void MovePesoPOW(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;


	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}	

	GetObjectInfo(theNode);

	gDelta.y -= 2000.0f * fps;							// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
	
	ApplyFrictionToDeltasXZ(100.0f, &gDelta);
	
	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC, .5);
		

	theNode->Rot.y += 9.0f * fps;

	UpdateObject(theNode);
}


/******************* BULLET HIT PESO POW **********************/

static void BulletHitPesoPOW(ObjNode *bullet, ObjNode *pow, const OGLPoint3D *impactPt)
{
	if (bullet->What == WHAT_ENEMYBULLET)			// only player bullets can get these
		return;

	MakePuff(impactPt, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);				
	PlayEffect3D(EFFECT_GETCOIN, impactPt);

	pow->MoveCall = MovePOW_Vanish;
	
	pow->CType &= ~CTYPE_PICKABLE;			// cant shoot it again
	
	gPlayerInfo.pesos += 1;
	
	
	gScore += POINTS_PESO;

}


/************** DO TRIGGER - PESO ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Peso(ObjNode *item, ObjNode *who, Byte sideBits)
{
	who; sideBits;
	
	item->CType = 0;
	item->MoveCall = MovePOW_Vanish;

	gPlayerInfo.pesos += 1;

	gScore += POINTS_PESO;
	
	PlayEffect3D(EFFECT_GETCOIN, &item->Coord);
	

	return(false);
}



#pragma mark -

/************************* ADD HAY BALE *********************************/

Boolean AddHayBale(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_HayBale + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 101;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE;			
	newObj->HitByBulletCallback = DefaultBulletHitCallback;

	newObj->Health = 2.0f;

	return(true);													// item was added
}


/************************* ADD POST *********************************/

Boolean AddPost(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];

	switch(type)
	{
		case	0:							// wood fence post	
				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
				gNewObjectDefinition.type 		= GLOBAL_ObjType_WoodPost;
				gNewObjectDefinition.scale 		= 2.0f;
				break;
	}
					
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 101;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE;			
	newObj->HitByBulletCallback = DefaultBulletHitCallback;

	return(true);													// item was added
}


#pragma mark -


/******************* DEFAULT BULLET HIT CALLBACK **********************/

void DefaultBulletHitCallback(ObjNode *bullet, ObjNode *hitObj, const OGLPoint3D *impactPt)
{
OGLPoint3D	puffPt;
OGLVector3D	v;

	bullet;


			/* DESTROY IT ? */

	hitObj->Health -= 1.0f;
	if (hitObj->Health <= 0.0f)
	{	
				/* BLOW IT UP */
				
		if (hitObj->What == WHAT_TNT)				// did we hit a TNT barrel?
		{
			PlayEffect3D(EFFECT_EXPLOSION, &hitObj->Coord);
			ExplodeGeometry(hitObj, 500.0, SHARD_MODE_FROMORIGIN, 1, .3);
			MakeSparkExplosion(hitObj->Coord.x, hitObj->Coord.y, hitObj->Coord.z, 250.0f, 30.0, PARTICLE_SObjType_YellowGlint, 500, .5);
		}
		else
		{
			ExplodeGeometry(hitObj, 100.0, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .4);
			PlayEffect3D(EFFECT_CRATEEXPLODE, &hitObj->Coord);
		}
		DeleteObject(hitObj);
	}
	else
		PlayEffect3D(EFFECT_RICOCHET, impactPt);
	


			/********************/
			/* MAKE IMPACT PUFF */
			/********************/

		/* MOVE PUFF AWAY FROM IMPACT PT A BIT */
						
	v.x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x - impactPt->x;
	v.y = gGameViewInfoPtr->cameraPlacement.cameraLocation.y - impactPt->y;
	v.z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z - impactPt->z;
	FastNormalizeVector(v.x, v.y, v.z, &v);
		
	puffPt.x = impactPt->x + (v.x * 20.0f);
	puffPt.y = impactPt->y + (v.y * 20.0f);
	puffPt.z = impactPt->z + (v.z * 20.0f);
			
	MakePuff(&puffPt, 10.0, PARTICLE_SObjType_GreySmoke, GL_SRC_ALPHA, GL_ONE, 1.0);	
}


#pragma mark -

/******************** SCENERY KANGA COW *****************************/

Boolean AddSceneryKangaCow(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
	
			/****************************/
			/* MAKE NEW SKELETON OBJECT */
			/****************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_KANGACOW;
	gNewObjectDefinition.animNum 	= 0;				
	gNewObjectDefinition.scale 		= 2.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= ENEMY_SLOT + SKELETON_TYPE_KANGACOW;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
	
	newObj->Coord.y -= newObj->BBox.min.y;							// offset so bottom touches ground
	UpdateObjectTransforms(newObj);
	
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 5, 12,false);

	newObj->CType = CTYPE_PICKABLE;			
	newObj->HitByBulletCallback = KangaHitByBulletCallback;

	return(true);
}


/******************* KANGA HIT BY BULLET CALLBACK **********************/
//
// gCoord & gDelta are currently set to bullet's data since the bullet Move function called this
//

static void KangaHitByBulletCallback(ObjNode *bullet, ObjNode *cow, const OGLPoint3D *impactPt)
{
OGLVector3D	splatVec;

	bullet;	cow;

			/* MAKE BULLET IMPACT SPLAT */
			
	splatVec.x = -gDelta.x;
	splatVec.y = -gDelta.y;
	splatVec.z = -gDelta.z;
	OGLVector3D_Normalize(&splatVec, &splatVec);
	DoBulletImpact(impactPt, &splatVec, 1.0);

	PlayEffect3D(EFFECT_BULLETHIT, impactPt);				
}


#pragma mark -


/************************* ADD TABLE *********************************/

Boolean AddTable(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_Table;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 384;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Health = 4.0f;

	newObj->CType = CTYPE_PICKABLE|CTYPE_HITENEMYBULLET;			
	newObj->HitByBulletCallback = DefaultBulletHitCallback;

	return(true);													// item was added
}


/************************* ADD CHAIR *********************************/

Boolean AddChair(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_Chair;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 384;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_PICKABLE|CTYPE_HITENEMYBULLET;			
	newObj->HitByBulletCallback = DefaultBulletHitCallback;
	newObj->Health = 2.0f;

	return(true);													// item was added
}


#pragma mark -

/************************* ADD DEAD TREE *********************************/

Boolean AddDeadTree(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= TOWN_ObjType_DeadTree + itemPtr->parm[1];
	gNewObjectDefinition.scale 		= 2.5f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= FENCE_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* CONFORM TO TERRAIN */
			
	RotateOnTerrain(newObj, 0, nil);							// set transform matrix
	SetObjectTransformMatrix(newObj);

	newObj->CType = CTYPE_MISC | CTYPE_TRIGGERENEMYONLY;
	newObj->CBits = CBITS_ALLSOLID | CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, .9,.7);
	newObj->TriggerCallback = DoTrig_ExplodeItem;

	return(true);													// item was added
}


/************** DO TRIGGER - EXPLODE ITEM ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_ExplodeItem(ObjNode *item, ObjNode *who, Byte sideBits)
{

	who; sideBits;
	
	PlayEffect3D(EFFECT_CRATEEXPLODE, &item->Coord);
	ExplodeGeometry(item, 300.0, 0, 1, .8);
	DeleteObject(item);
	

	return(false);
}



/************************* ADD ROCK *********************************/

Boolean AddRock(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int	type = itemPtr->parm[0];
Boolean	conform;

	switch(gCurrentArea)
	{
		case	AREA_TOWN_SHOOTOUT:
		case	AREA_TOWN_DUEL1:
		case	AREA_TOWN_DUEL2:
		case	AREA_TOWN_DUEL3:
		case	AREA_TOWN_STAMPEDE:
				if (type > 3)
					DoAlert("AddRock: illegal rock type");
				gNewObjectDefinition.type  = TOWN_ObjType_TallRock1 + type;
				conform = (type >= 2);					// short rocks conform to terrain
				break;
				
		case	AREA_SWAMP_SHOOTOUT:
		case	AREA_SWAMP_DUEL1:
		case	AREA_SWAMP_DUEL2:
		case	AREA_SWAMP_DUEL3:
		case	AREA_SWAMP_STAMPEDE:
				gNewObjectDefinition.type = SWAMP_ObjType_LargeRock + type;
				conform = true;
				break;
	}		
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.scale 		= 2.5f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0) - 
									 gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= FENCE_SLOT-2;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* CONFORM TO TERRAIN */
			
	if (conform)
	{
		RotateOnTerrain(newObj, 0, nil);							// set transform matrix
		SetObjectTransformMatrix(newObj);
		CalcObjectBoxFromNode(newObj);
	}

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, .9,1);


	return(true);													// item was added
}




#pragma mark -


/************************* ADD TUMBLEWEED *********************************/

Boolean AddTumbleweed(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Tumbleweed;
	gNewObjectDefinition.scale 		= 1.2f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= FENCE_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Rot.x = RandomFloat() * PI2;
	newObj->Rot.z = RandomFloat() * PI2;	
	UpdateObjectTransforms(newObj);
	CalcObjectBoxFromNode(newObj);


	return(true);													// item was added
}

/************************ PRIME TUMBLEWEED *************************/

Boolean PrimeTumbleweed(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;	
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE OBJ */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Tumbleweed;
	gNewObjectDefinition.scale 		= .6f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);	
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED;
	gNewObjectDefinition.slot 		= FENCE_SLOT-1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
								
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;			
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveTumbleweedOnSpline;					// set move call

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */
			
	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 4,false);

	return(true);
}


/******************** MOVE TUMBLEWEED ON SPLINE ***************************/

static void MoveTumbleweedOnSpline(ObjNode *theNode)
{
Boolean 	isInRange; 
const ObjNode	*player = gPlayerInfo.objNode;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 110.0f, false);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF IN RANGE */
			
	if (isInRange)
	{		
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);		

		theNode->Rot.x -= gFramesPerSecondFrac * 8.0f;
		

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - (theNode->BottomOff * .9f);	// calc y coord
		
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);	
		CalcObjectBoxFromNode(theNode);
	}	
}






#pragma mark -




/************************* ADD TEE PEE *********************************/

Boolean AddTeePee(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= SWAMP_ObjType_TeePee;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 188;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	CalcObjectBoxFromNode(newObj);

	newObj->CType = CTYPE_PICKABLE|CTYPE_HITENEMYBULLET;

	return(true);													// item was added
}

/************************* ADD SWAMP CABIN *********************************/

Boolean AddSwampCabin(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= SWAMP_ObjType_Cabin;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 188;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	CalcObjectBoxFromNode(newObj);

	newObj->CType = CTYPE_PICKABLE|CTYPE_HITENEMYBULLET;


	return(true);													// item was added
}




/************************* ADD SPEAR SKULL *********************************/

Boolean AddSpearSkull(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= SWAMP_ObjType_SpearSkull;
	gNewObjectDefinition.scale 		= 1.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 677;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	CalcObjectBoxFromNode(newObj);

	newObj->CType = CTYPE_PICKABLE|CTYPE_HITENEMYBULLET;

	return(true);													// item was added
}


#pragma mark -

/************************* ADD ELECTRIC FENCE *********************************/

Boolean AddElectricFence(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
									
	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
	gNewObjectDefinition.type 		= SWAMP_ObjType_ElectricFence;
	gNewObjectDefinition.scale 		= 12.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);	
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= FENCE_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);	
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* CONFORM TO TERRAIN */
			
	RotateOnTerrain(newObj, 0, nil);							// set transform matrix
	SetObjectTransformMatrix(newObj);
	CalcObjectBoxFromNode(newObj);


			/* MAKE TRIGGER */			

	newObj->CType = CTYPE_MISC | CTYPE_TRIGGERENEMYONLY;
	newObj->CBits = CBITS_ALLSOLID | CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1,.7);
	newObj->TriggerCallback = DoTrig_ExplodeItem;


	return(true);													// item was added
}






