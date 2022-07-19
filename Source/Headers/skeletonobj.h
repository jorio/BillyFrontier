//
// SkeletonObj.h
//

#ifndef __SKELOBJ
#define __SKELOBJ

// Externals
#include "game.h"

enum
{
	SKELETON_TYPE_BILLY = 0,
	SKELETON_TYPE_BANDITO,
	SKELETON_TYPE_RYGAR,
	SKELETON_TYPE_SHORTY,
	SKELETON_TYPE_KANGACOW,
	SKELETON_TYPE_KANGAREX,
	SKELETON_TYPE_WALKER,
	SKELETON_TYPE_TREMORALIEN,
	SKELETON_TYPE_TREMORGHOST,
	SKELETON_TYPE_FROGMAN,
	
	MAX_SKELETON_TYPES				
};




//===============================

extern	ObjNode	*MakeNewSkeletonObject(NewObjectDefinitionType *newObjDef);
extern	void AllocSkeletonDefinitionMemory(SkeletonDefType *skeleton);
extern	void InitSkeletonManager(void);
void LoadASkeleton(Byte num, OGLSetupOutputType *setupInfo);
extern	void FreeSkeletonFile(Byte skeletonType);
extern	void FreeAllSkeletonFiles(short skipMe);
extern	void FreeSkeletonBaseData(SkeletonObjDataType *data);
void DrawSkeleton(ObjNode *theNode, const OGLSetupOutputType *setupInfo);


#endif