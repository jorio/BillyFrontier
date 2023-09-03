//
// Object.h
//

#pragma once

#define INVALID_NODE_FLAG	0xdeadbeef			// put into CType when node is deleted

#define	TERRAIN_SLOT	1
#define	FENCE_SLOT		20
#define	PLAYER_SLOT		200
#define	ENEMY_SLOT		(PLAYER_SLOT+10)
#define	SLOT_OF_DUMB	3000
#define	SPRITE_SLOT		(SLOT_OF_DUMB+100)
#define	FADEPANE_SLOT	(SLOT_OF_DUMB+1000)
#define	PARTICLE_SLOT	(SPRITE_SLOT-2)
#define	WATER_SLOT		(PARTICLE_SLOT - 1)

enum
{
	SKELETON_GENRE,
	DISPLAY_GROUP_GENRE,
	SPRITE_GENRE,
	CUSTOM_GENRE,
	EVENT_GENRE,
	FONTSTRING_GENRE
};


enum
{
	SHADOW_TYPE_CIRCULAR,
};


enum
{
	WHAT_UNDEFINED = 0,
	
	WHAT_PLAYERBULLET,
	WHAT_ENEMYBULLET,
	
	WHAT_COINORB,
	WHAT_TIMEORB,
	WHAT_RAPIDFIRE,
	WHAT_SKULL,
	WHAT_KANGACOW,
	WHAT_SHORTY,
	WHAT_AMMOBOX,
	WHAT_FROGMAN,
	WHAT_TREMORGHOST,
	WHAT_BOTTLE,
	WHAT_TNT,
	WHAT_PEPPER
};


#define	DEFAULT_GRAVITY		5000.0f


#define	ShadowScaleX	SpecialF[0]
#define	ShadowScaleZ	SpecialF[1]
#define	CheckForBlockers	Flag[0]

//========================================================

extern	void InitObjectManager(void);
extern	ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef);
extern	void MoveObjects(void);
void DrawObjects(void);

extern	void DeleteAllObjects(void);
extern	void DeleteObject(ObjNode	*theNode);
void DetachObject(ObjNode *theNode, Boolean subrecurse);
extern	void GetObjectInfo(ObjNode *theNode);
extern	void UpdateObject(ObjNode *theNode);
extern	ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef);
extern	void AttachGeometryToDisplayGroupObject(ObjNode *theNode, MetaObjectPtr geometry);
extern	void CreateBaseGroup(ObjNode *theNode);
extern	void UpdateObjectTransforms(ObjNode *theNode);
extern	void SetObjectTransformMatrix(ObjNode *theNode);
extern	void DisposeObjectBaseGroup(ObjNode *theNode);
extern	void ResetDisplayGroupObject(ObjNode *theNode);
void AttachObject(ObjNode *theNode, Boolean recurse);

void MoveStaticObject(ObjNode *theNode);
void MoveStaticObject2(ObjNode *theNode);
void MoveStaticObject3(ObjNode *theNode);
void HideObject(ObjNode *theNode);
void ShowObject(ObjNode *theNode);

void CalcNewTargetOffsets(ObjNode *theNode, float scale);
void CalcObjectRadiusFromBBox(ObjNode *theNode);

//===================


extern	void CalcObjectBoxFromNode(ObjNode *theNode);
extern	void CalcObjectBoxFromGlobal(ObjNode *theNode);
void SetObjectCollisionBounds(ObjNode *theNode, float top, float bottom, float left,
							 float right, float front, float back);
ObjNode	*AttachStaticShadowToObject(ObjNode *theNode, int shadowType, float scaleX, float scaleZ);
extern	void UpdateShadow(ObjNode *theNode);
extern	void CullTestAllObjects(void);
ObjNode	*AttachShadowToObject(ObjNode *theNode, int shadowType, float scaleX, float scaleZ, Boolean checkBlockers);
void CreateCollisionBoxFromBoundingBox(ObjNode *theNode, float tweakXZ, float tweakY);
void CreateCollisionBoxFromBoundingBox_Maximized(ObjNode *theNode);
void CreateCollisionBoxFromBoundingBox_Rotated(ObjNode *theNode, float tweakXZ, float tweakY);
void CreateCollisionBoxFromBoundingBox_Update(ObjNode *theNode, float tweakXZ, float tweakY);
ObjNode *FindClosestCType(OGLPoint3D *pt, uint32_t ctype, Boolean notThruSolid);
ObjNode *FindClosestCType3D(OGLPoint3D *pt, uint32_t ctype);
extern	void KeepOldCollisionBoxes(ObjNode *theNode);
void AddCollisionBoxToObject(ObjNode *theNode, float top, float bottom, float left,
							 float right, float front, float back);
void DoObjectFriction(ObjNode *theNode, float friction);

void CalcDisplayGroupWorldPoints(ObjNode *theNode);


ObjNode* MakeBackgroundPictureObject(const char* path);
