//
// shards.h
//

// Externals
#include "game.h"

enum
{
	SHARD_MODE_UPTHRUST		=	1,
	SHARD_MODE_HEAVYGRAVITY = (1<<1),
	SHARD_MODE_BOUNCE 		= (1<<2),
	SHARD_MODE_FROMORIGIN 	= (1<<3)

};



void InitShardSystem(void);
void ExplodeGeometry(ObjNode *theNode, float boomForce, Byte particleMode, long particleDensity, float particleDecaySpeed);
void MoveShards(ObjNode *theNode);
void DrawShards(ObjNode *theNode);


