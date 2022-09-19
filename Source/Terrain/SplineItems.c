/****************************/
/*   	SPLINE ITEMS.C      */
/****************************/

#include "game.h"

/***************/
/* EXTERNALS   */
/***************/

/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NilPrime(long splineNum, SplineItemType *itemPtr);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SPLINE_OBJECTS		100



/**********************/
/*     VARIABLES      */
/**********************/

SplineDefType	**gSplineList = nil;
long			gNumSplines = 0;

static long		gNumSplineObjects = 0;
static ObjNode	*gSplineObjectList[MAX_SPLINE_OBJECTS];


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_SPLINE_ITEM_NUM		34				// for error checking!

Boolean (*gSplineItemPrimeRoutines[MAX_SPLINE_ITEM_NUM+1])(long, SplineItemType *) =
{
		NilPrime,							// My Start Coords
		NilPrime,							// 1:
		NilPrime,							// 2:
		NilPrime,							// 3:
		NilPrime,							// 4:
		NilPrime,							// 5:
		NilPrime,							// 6:
		NilPrime,							// 7:
		NilPrime,							// 8:
		NilPrime,							// 9:
		NilPrime,							// 10:
		NilPrime,							// 11:
		NilPrime,							// 12:
		NilPrime,							// 13:
		NilPrime,							// 14:
		NilPrime,							// 15:
		NilPrime,							// 16:
		NilPrime,							// 17:
		NilPrime,							// 18:
		NilPrime,							// 19:		
		PrimeStampedeKangaCow,				// 20:
		NilPrime,							// 21:		
		PrimeStampedeCamera,				// 22:	stampede camera
		PrimeWalker,						// 23: walker
		NilPrime,							// 24:		
		NilPrime,							// 25:		
		NilPrime,							// 26:		
		PrimeTumbleweed,					// 27: tumbleweed		
		NilPrime,							// 28:		
		NilPrime,							// 29:		
		NilPrime,							// 30:		
		PrimeTremorAlien,					// 31:	tremor alien	
		NilPrime,							// 32:		
		NilPrime,							// 33:				
		PrimeStampedeKangaRex,				// 34: kanga rex
};



/********************* PRIME SPLINES ***********************/
//
// Called during terrain prime function to initialize 
// all items on the splines and recalc spline coords
//

void PrimeSplines(void)
{
long			s,i,type;
SplineItemType 	*itemPtr;
SplineDefType	*spline;
Boolean			flag;
SplinePointType	*points;


			/* ADJUST SPLINE TO GAME COORDINATES */
			
	for (s = 0; s < gNumSplines; s++)
	{
		spline = &(*gSplineList)[s];							// point to this spline
		points = (*spline->pointList);							// point to points list
		
		for (i = 0; i < spline->numPoints; i++)
		{
			points[i].x *= gMapToUnitValue;
			points[i].z *= gMapToUnitValue;
		}

	}	
	
	
				/* CLEAR SPLINE OBJECT LIST */
				
	gNumSplineObjects = 0;										// no items in spline object node list yet

	for (s = 0; s < gNumSplines; s++)
	{
		spline = &(*gSplineList)[s];							// point to this spline
		
				/* SCAN ALL ITEMS ON THIS SPLINE */
				
		HLockHi((Handle)spline->itemList);						// make sure this is permanently locked down
		for (i = 0; i < spline->numItems; i++)
		{
			itemPtr = &(*spline->itemList)[i];					// point to this item
			type = itemPtr->type;								// get item type
			if (type > MAX_SPLINE_ITEM_NUM)
				DoFatalAlert("PrimeSplines: type > MAX_SPLINE_ITEM_NUM");
	
			flag = gSplineItemPrimeRoutines[type](s,itemPtr); 	// call item's Prime routine
			if (flag)
				itemPtr->flags |= ITEM_FLAGS_INUSE;				// set in-use flag	
		}
	}
}


/******************** NIL PRIME ***********************/
//
// nothing prime
//

static Boolean NilPrime(long splineNum, SplineItemType *itemPtr)
{
	(void) splineNum;
	(void) itemPtr;

	return false;
}


/*********************** GET COORD ON SPLINE FROM INDEX **********************/

void GetCoordOnSplineFromIndex(SplineDefType *splinePtr, double findex, float *x, float *z)
{
SplinePointType	*points;
int				i,i2, numPointsInSpline;
double			ratio,oneMinusRatio;

			/* CALC INDEX OF THIS PT AND NEXT */

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	i = findex;													// round down to int
	i2 = i+1;
	if (i2 >= numPointsInSpline)								// make sure not go too far
		i2 = numPointsInSpline-1;

	points = *splinePtr->pointList;								// point to point list


			/* INTERPOLATE */

	ratio = findex - (double)i;									// calc 0.0 - .999 remainder for weighing the points
	oneMinusRatio = 1.0 - ratio;

	*x = points[i].x * oneMinusRatio + points[i2].x * ratio;	// calc interpolated coord
	*z = points[i].z * oneMinusRatio + points[i2].z * ratio;
}


/*********************** GET COORD ON SPLINE **********************/

void GetCoordOnSpline(SplineDefType *splinePtr, double placement, float *x, float *z)
{
double				numPointsInSpline;
double			findex;

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	findex = numPointsInSpline * placement;						// calc float index
	
	GetCoordOnSplineFromIndex(splinePtr, findex, x, z);
}


/*********************** GET NEXT COORD ON SPLINE **********************/
//
// Same as above except returns coord of the next point on the spline instead of the exact
// current one.
//

void GetNextCoordOnSpline(SplineDefType *splinePtr, double placement, float *x, float *z)
{
double			numPointsInSpline;
double			findex;

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	
	findex = (double)numPointsInSpline * placement;				// get index
	findex += 1.0;												// bump it up +1
	
	if (findex >= (double)numPointsInSpline)						// see if wrap around
		findex = 0;
	
	GetCoordOnSplineFromIndex(splinePtr, findex, x, z);
}


/*********************** GET COORD ON SPLINE 2 **********************/
//
// Same as above except takes in input spline index offset
//

void GetCoordOnSpline2(SplineDefType *splinePtr, double placement, double offset, float *x, float *z, Boolean allowWrap)
{
double			numPointsInSpline;
int				findex;

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	
	findex = (double)numPointsInSpline * placement;				// get index
	findex += offset;											// bump it up
	
	if (findex >= (double)numPointsInSpline)					// see if wrap around
	{
		if (allowWrap)
			findex -= (double)numPointsInSpline;
		else
			findex = numPointsInSpline-1;
	}
	else
	if (findex < 0)
	{
		if (allowWrap)
			findex += (double)numPointsInSpline;
		else
			findex = 0;
	}

	GetCoordOnSplineFromIndex(splinePtr, findex, x, z);	
}


/********************* IS SPLINE ITEM ON ACTIVE TERRAIN ********************/
//
// Returns true if the input objnode is in visible range.
// Also, this function handles the attaching and detaching of the objnode
// as needed.
//

Boolean UpdateSplineItemVisibilityOnActiveTerrain(ObjNode *theNode)
{
Boolean	visible = true;
long	row,col;
	
	
			/* IF IS ON AN ACTIVE SUPERTILE, THEN ASSUME VISIBLE */

	row = theNode->Coord.z * gTerrainSuperTileUnitSizeFrac;	// calc supertile row,col
	col = theNode->Coord.x * gTerrainSuperTileUnitSizeFrac;
	
	if ((row < 0) || (row >= gNumSuperTilesDeep) || (col < 0) || (col >= gNumSuperTilesWide))		// make sure in bounds
		visible = false;
	else
	{
		if (gSuperTileStatusGrid[row][col].playerHereFlag)
			visible = true;
		else
			visible = false;
	}
			/* HANDLE OBJNODE UPDATES */			

	if (visible)
	{
		if (theNode->StatusBits & STATUS_BIT_DETACHED)			// see if need to insert into linked list
		{
			AttachObject(theNode, true);
		}
	}
	else
	{
		if (!(theNode->StatusBits & STATUS_BIT_DETACHED))		// see if need to remove from linked list
		{
			DetachObject(theNode, true);
		}
	}

	return(visible);
}


#pragma mark ======= SPLINE OBJECTS ================

/******************* ADD TO SPLINE OBJECT LIST ***************************/
//
// Called by object's primer function to add the detached node to the spline item master
// list so that it can be maintained.
//

void AddToSplineObjectList(ObjNode *theNode, Boolean setAim)
{

	if (gNumSplineObjects >= MAX_SPLINE_OBJECTS)
		DoFatalAlert("AddToSplineObjectList: too many spline objects");

	theNode->SplineObjectIndex = gNumSplineObjects;					// remember where in list this is

	gSplineObjectList[gNumSplineObjects++] = theNode;	


			/* SET INITIAL AIM */
				
	if (setAim)
		SetSplineAim(theNode);
}

/************************ SET SPLINE AIM ***************************/

void SetSplineAim(ObjNode *theNode)
{
float	x,z;

	GetCoordOnSpline2(&(*gSplineList)[theNode->SplineNum], theNode->SplinePlacement, 10, &x, &z, false);			// get coord of next point on spline
	theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->Coord.x, theNode->Coord.z,x,z);	// calc y rot aim


}

/****************** REMOVE FROM SPLINE OBJECT LIST **********************/
//
// OUTPUT:  true = the obj was on a spline and it was removed from it
//			false = the obj was not on a spline.
//

Boolean RemoveFromSplineObjectList(ObjNode *theNode)
{
	theNode->StatusBits &= ~STATUS_BIT_ONSPLINE;		// make sure this flag is off

	if (theNode->SplineObjectIndex != -1)
	{
		gSplineObjectList[theNode->SplineObjectIndex] = nil;			// nil out the entry into the list
		theNode->SplineObjectIndex = -1;
		theNode->SplineItemPtr = nil;
		theNode->SplineMoveCall = nil;
		return(true);
	}
	else
	{
		return(false);	
	}
}


/**************** EMPTY SPLINE OBJECT LIST ***********************/
//
// Called by level cleanup to dispose of the detached ObjNode's in this list.
//

void EmptySplineObjectList(void)
{
int	i;
ObjNode	*o;

	for (i = 0; i < gNumSplineObjects; i++)
	{
		o = gSplineObjectList[i];
		if (o)
			DeleteObject(o);			// This will dispose of all memory used by the node.
										// RemoveFromSplineObjectList will be called by it. 
	}
	gNumSplineObjects = 0;
}


/******************* MOVE SPLINE OBJECTS **********************/

void MoveSplineObjects(void)
{
long	i;
ObjNode	*theNode;

	for (i = 0; i < gNumSplineObjects; i++)
	{
		theNode = gSplineObjectList[i];
		if (theNode)
		{
			if (theNode->SplineMoveCall)
			{
				KeepOldCollisionBoxes(theNode);					// keep old boxes & other stuff
				theNode->SplineMoveCall(theNode);				// call object's spline move routine
			}
		}
	}
}


/*********************** GET OBJECT COORD ON SPLINE **********************/
//
// OUTPUT: 	x,y = coords
//

void GetObjectCoordOnSpline(ObjNode *theNode)
{
double			placement;
SplineDefType	*splinePtr;

	placement = theNode->SplinePlacement;						// get placement
	if (placement < 0.0)
		placement = 0;
	else
	if (placement >= 1.0)
		placement = .999;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline

	GetCoordOnSpline(splinePtr, placement, &theNode->Coord.x, &theNode->Coord.z);		// get coord

	theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc delta
	theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
	
}


/*********************** GET OBJECT COORD ON SPLINE 2 **********************/

void GetObjectCoordOnSpline2(ObjNode *theNode, float *x, float *z)
{
double			placement;
SplineDefType	*splinePtr;

	placement = theNode->SplinePlacement;						// get placement
	if (placement < 0.0)
		placement = 0;
	else
	if (placement >= 1.0)
		placement = .999;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline

	GetCoordOnSpline(splinePtr, placement, x, z);		// get coord
}



/******************* INCREASE SPLINE INDEX *********************/
//
// Moves objects on spline at given speed
//
//

Boolean IncreaseSplineIndex(ObjNode *theNode, double speed, Boolean noWrapping)
{
SplineDefType	*splinePtr;
double			numPointsInSpline;
Boolean			atEnd = false;

	speed *= gFramesPerSecondFrac;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline
	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

	theNode->SplinePlacement += speed / numPointsInSpline;
	
	if (theNode->SplinePlacement < 0.0f)
	{
		if (noWrapping)
		{
			theNode->SplinePlacement = 0.0f;
			atEnd = true;
		}
		else
			theNode->SplinePlacement += 1.0f;
	}
	else
	if (theNode->SplinePlacement > .999f)
	{
		if (noWrapping)
		{
			theNode->SplinePlacement = .999f;
			atEnd = true;
		}
		else
			theNode->SplinePlacement -= 1.0f;
	}
	
	return(atEnd);
}


/******************* INCREASE SPLINE INDEX ZIGZAG *********************/
//
// Moves objects on spline at given speed, but zigzags
//

void IncreaseSplineIndexZigZag(ObjNode *theNode, double speed)
{
SplineDefType	*splinePtr;
double			numPointsInSpline;

	speed *= gFramesPerSecondFrac;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline
	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

			/* GOING BACKWARD */
			
	if (theNode->StatusBits & STATUS_BIT_REVERSESPLINE)			// see if going backward
	{
		theNode->SplinePlacement -= speed / numPointsInSpline;
		if (theNode->SplinePlacement <= 0.0f)
		{
			theNode->SplinePlacement = 0;
			theNode->StatusBits ^= STATUS_BIT_REVERSESPLINE;	// toggle direction
		}
	}
	
		/* GOING FORWARD */
		
	else
	{
		theNode->SplinePlacement += speed / numPointsInSpline;
		if (theNode->SplinePlacement >= .999f)
		{
			theNode->SplinePlacement = .999f;
			theNode->StatusBits ^= STATUS_BIT_REVERSESPLINE;	// toggle direction
		}
	}
}


/******************** DETACH OBJECT FROM SPLINE ***********************/

void DetachObjectFromSpline(ObjNode *theNode, movecall_t moveCall)
{

	if (!(theNode->StatusBits & STATUS_BIT_ONSPLINE))
		return;

		/***********************************************/
		/* MAKE SURE ALL COMPONENTS ARE IN LINKED LIST */
		/***********************************************/
				
	AttachObject(theNode, true);
	

			/* REMOVE FROM SPLINE */
			
	RemoveFromSplineObjectList(theNode);
			
	theNode->InitCoord  = theNode->Coord;			// remember where started
	
	theNode->MoveCall = moveCall;

}




