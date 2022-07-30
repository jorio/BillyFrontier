//
// water.h
//

#pragma once

// TODO: Is this actually used in Billy Frontier?

#define	MAX_WATER_POINTS	100			// note:  cannot change this without breaking data files!!

// Externals
#include "game.h"

enum
{
	WATER_FLAG_FIXEDHEIGHT	= (1)
};

enum
{
	WATER_TYPE_BLUEWATER,
	WATER_TYPE_POOLWATER,
	WATER_TYPE_GARBAGE,
	
	NUM_WATER_TYPES
};


typedef struct		// NOTE: MUST MATCH OREOTERRAIN DATA!!!
{
	uint16_t		type;							// type of water
	uint32_t		flags;							// flags
	int32_t			height;							// height offset or hard-wired index
	int16_t			numNubs;						// # nubs in water
	int32_t			reserved;						// for future use
	OGLPoint2D		nubList[MAX_WATER_POINTS];		// nub list
	
	float			hotSpotX,hotSpotZ;				// hot spot coords
	Rect			bBox;							// bounding box of water area	
}WaterDefType;




//============================================

void PrimeTerrainWater(void);
void DisposeWater(void);
Boolean DoWaterCollisionDetect(ObjNode *theNode, float x, float y, float z, int *patchNum);
Boolean IsXZOverWater(float x, float z);
Boolean GetWaterY(float x, float z, float *y);

