/****************************/
/*   	PICKUPS.C		    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

#define	Explosive		Flag[0]						// set after acorn has been kicked indicating it will blow up
#define	CloverType		Special[0]

#define		Activated		Flag[0]




/********************** MOVE DEFAULT PICKUP **************************/

void MoveDefaultPickup(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (theNode->TerrainItemPtr)							// only track if can come back
	{
		if (TrackTerrainItem(theNode))							// just check to see if it's gone
		{
			DeleteObject(theNode);
			return;
		}
	}

	GetObjectInfo(theNode);

	gDelta.y -= 3000.0f * fps;
	
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE | CTYPE_WATER, .4);

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.x *= .8f;
		gDelta.z *= .8f;	
	}
	else
	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
	{
		int		patchNum;
		if (DoWaterCollisionDetect(theNode, gCoord.x, gCoord.y+theNode->BottomOff, gCoord.z, &patchNum))
		{
			gCoord.y = gWaterBBox[patchNum].max.y;
			gDelta.y = 0;
		}
	}


	UpdateObject(theNode);
	
			/* ALSO UPDATE ANY CHAINS */
			
	if (theNode->ChainNode)	
	{
		ObjNode	*child = theNode->ChainNode;
	
		child->Coord = gCoord;
		UpdateObjectTransforms(child);	
	}
}


/************************ DEFAULT DROP OBJECT **************************/
//
// The default callback for pickups.  Called when player wants to drop this.
//

void DefaultDropObject(ObjNode *player, ObjNode *held)
{
float	r = player->Rot.y;

	held->Rot.y = r;

		/* MAKE SURE NOT BEING DROPPED BEHIND A FENCE */

	if (CheckDropThruFence(player, held))
		return;
		
	held->Delta.x = gDelta.x - sin(r) * 100.0f;				// toss the object forward
	held->Delta.z = gDelta.z - cos(r) * 100.0f;
	held->Delta.y = 150.0f;
	
	UpdateObjectTransforms(held);
	CreateCollisionBoxFromBoundingBox_Rotated(held,1,1);	
}


/******************** CHECK DROP THRU FENCE ******************************/
//
// Also checks that not dropped into a solid either.
//

Boolean CheckDropThruFence(ObjNode *player, ObjNode *held)
{		
OGLVector2D	v;
OGLPoint3D	p;

			/* SEE IF INTERSECTS ANY MISC SOLIDS */
			
	CollisionDetect(held, CTYPE_MISC, 0);
	if (gNumCollisions)
		goto put_here;
	

			/* SEE IF IN FENCE */
			
	v.x = held->Coord.x - player->Coord.x;
	v.y = held->Coord.z - player->Coord.z;
	FastNormalizeVector2D(v.x, v.y, &v, true);

	p.x = held->Coord.x + v.x * 40.0f;
	p.y = held->Coord.y;
	p.z = held->Coord.z + v.y * 40.0f;
	
	if (SeeIfLineSegmentHitsFence(&player->Coord, &p, nil, nil, nil))
	{
put_here:	
		held->Coord = player->Coord;				// just drop under player to avoid being stuck in fence
		held->Delta.x = held->Delta.z = 0;
		return(true);
	}
		
	return(false);
}
	


/************************* DEFAULT GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

void DefaultGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	kickedObj->Delta.x = -sin(r) * 1100.0f;
	kickedObj->Delta.z = -cos(r) * 1100.0f;
	kickedObj->Delta.y = 600.0f;

}


















