/****************************/
/*   	COLLISION.c		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void AllocateCollisionTriangleMemory(ObjNode *theNode, long numTriangles);

static Boolean RayIntersectTriangle(OGLPoint3D *origin, OGLVector3D *dir,
                   			OGLPoint3D *v0, OGLPoint3D *v1, OGLPoint3D *v2,
                   			Boolean	cullTest, float *t, float *u, float *v);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_COLLISIONS				60
#define	MAX_TEMP_COLL_TRIANGLES		300

enum
{
	WH_HEAD	=	1,
	WH_FOOT =	1<<1
};

/****************************/
/*    VARIABLES             */
/****************************/


CollisionRec	gCollisionList[MAX_COLLISIONS];
short			gNumCollisions = 0;
Byte			gTotalSides;
Boolean			gSolidTriggerKeepDelta;

/******************* COLLISION DETECT *********************/
//
// INPUT: startNumCollisions = value to start gNumCollisions at should we need to keep existing data in collision list
//

void CollisionDetect(ObjNode *baseNode, uint32_t CType, short startNumCollisions)
{
ObjNode 	*thisNode;
long		sideBits,cBits,cType;
short		numBaseBoxes,targetNumBoxes,target;
CollisionBoxType *baseBoxList;
CollisionBoxType *targetBoxList;
float		leftSide,rightSide,frontSide,backSide,bottomSide,topSide;

	gNumCollisions = startNumCollisions;								// clear list

			/* GET BASE BOX INFO */
			
	numBaseBoxes = baseNode->NumCollisionBoxes;
	if (numBaseBoxes == 0)
		return;
	baseBoxList = baseNode->CollisionBoxes;

	leftSide 		= baseBoxList->left;
	rightSide 		= baseBoxList->right;
	frontSide 		= baseBoxList->front;
	backSide 		= baseBoxList->back;
	bottomSide 		= baseBoxList->bottom;
	topSide 		= baseBoxList->top;
	
	
			/****************************/		
			/* SCAN AGAINST ALL OBJECTS */
			/****************************/		
		
	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		cType = thisNode->CType;	
		if (cType == INVALID_NODE_FLAG)							// see if something went wrong
			break;
	
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;
		
		if (!(cType & CType))									// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;		
					
		if (thisNode == baseNode)								// dont collide against itself
			goto next;
	
		if (baseNode->ChainNode == thisNode)					// don't collide against its own chained object
			goto next;
			
				/******************************/		
				/* NOW DO COLLISION BOX CHECK */
				/******************************/		
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// see if target has any boxes
		if (targetNumBoxes)
		{
			targetBoxList = thisNode->CollisionBoxes;
		
		
				/******************************************/
				/* CHECK BASE BOX AGAINST EACH TARGET BOX */
				/*******************************************/
				
			for (target = 0; target < targetNumBoxes; target++)
			{
						/* DO RECTANGLE INTERSECTION */
			
				if (rightSide < targetBoxList[target].left)
					continue;
					
				if (leftSide > targetBoxList[target].right)
					continue;
					
				if (frontSide < targetBoxList[target].back)
					continue;
					
				if (backSide > targetBoxList[target].front)
					continue;
					
				if (bottomSide > targetBoxList[target].top)
					continue;

				if (topSide < targetBoxList[target].bottom)
					continue;
					
									
						/* THERE HAS BEEN A COLLISION SO CHECK WHICH SIDE PASSED THRU */
			
				sideBits = 0;
				cBits = thisNode->CBits;					// get collision info bits
								
				if (!(cBits & CBITS_ALLSOLID))				// if not a solid, then add it without side info
					goto got_sides;
				
							
								/* CHECK FRONT COLLISION */
			
				if (cBits & SIDE_BITS_BACK)											// see if target has solid back
				{
					if (baseBoxList->oldFront < targetBoxList[target].oldBack)		// get old & see if already was in target (if so, skip)
					{
						if ((baseBoxList->front >= targetBoxList[target].back) &&	// see if currently in target
							(baseBoxList->front <= targetBoxList[target].front))
						{
							sideBits = SIDE_BITS_FRONT;
						}
					}
				}
				
								/* CHECK BACK COLLISION */
			
				if (cBits & SIDE_BITS_FRONT)										// see if target has solid front
				{
					if (baseBoxList->oldBack > targetBoxList[target].oldFront)		// get old & see if already was in target	
					{
						if ((baseBoxList->back <= targetBoxList[target].front) &&	// see if currently in target
							(baseBoxList->back >= targetBoxList[target].back))
						{
							sideBits = SIDE_BITS_BACK;
						}
					}
				}

		
								/* CHECK RIGHT COLLISION */
			
			
				if (cBits & SIDE_BITS_LEFT)											// see if target has solid left
				{
					if (baseBoxList->oldRight < targetBoxList[target].oldLeft)		// get old & see if already was in target	
					{
						if ((baseBoxList->right >= targetBoxList[target].left) &&	// see if currently in target
							(baseBoxList->right <= targetBoxList[target].right))
						{
							sideBits |= SIDE_BITS_RIGHT;
						}
					}
				}
				

							/* CHECK COLLISION ON LEFT */

				if (cBits & SIDE_BITS_RIGHT)										// see if target has solid right
				{
					if (baseBoxList->oldLeft > targetBoxList[target].oldRight)		// get old & see if already was in target	
					{
						if ((baseBoxList->left <= targetBoxList[target].right) &&	// see if currently in target
							(baseBoxList->left >= targetBoxList[target].left))
						{
							sideBits |= SIDE_BITS_LEFT;
						}
					}
				}	

								/* CHECK TOP COLLISION */
			
				if (cBits & SIDE_BITS_BOTTOM)										// see if target has solid bottom
				{				
					if (baseBoxList->oldTop < targetBoxList[target].oldBottom)		// get old & see if already was in target	
					{
						if ((baseBoxList->top >= targetBoxList[target].bottom) &&	// see if currently in target
							(baseBoxList->top <= targetBoxList[target].top))
						{
							sideBits |= SIDE_BITS_TOP;
						}
					}
				}

							/* CHECK COLLISION ON BOTTOM */

				
				if (cBits & SIDE_BITS_TOP)											// see if target has solid top
				{
					if (baseBoxList->oldBottom > targetBoxList[target].oldTop)		// get old & see if already was in target	
					{
						if ((baseBoxList->bottom <= targetBoxList[target].top) &&	// see if currently in target
							(baseBoxList->bottom >= targetBoxList[target].bottom))
						{
							sideBits |= SIDE_BITS_BOTTOM;
						}
					}
				}	

					/*********************************************/
					/* SEE IF ANYTHING TO ADD OR IF IMPENETRABLE */
					/*********************************************/
				
				if (!sideBits)														// if 0 then no new sides passed thru this time
				{
					if (cBits & CBITS_IMPENETRABLE)									// if its impenetrable, add to list regardless of sides
					{
						if (gCoord.x < thisNode->Coord.x)							// try to assume some side info based on which side we're on relative to the target
							sideBits |= SIDE_BITS_RIGHT;
						else
							sideBits |= SIDE_BITS_LEFT;

						if (gCoord.z < thisNode->Coord.z)
							sideBits |= SIDE_BITS_FRONT;
						else
							sideBits |= SIDE_BITS_BACK;

//						if (gCoord.y > thisNode->Coord.y)
//							sideBits |= SIDE_BITS_BOTTOM;
							 
						goto got_sides;				
					}
											 							 
					if (cBits & CBITS_ALWAYSTRIGGER)								// also always add if always trigger
						goto got_sides;	
				
					continue;
				}

						/* ADD TO COLLISION LIST */
got_sides:
				gCollisionList[gNumCollisions].baseBox 		= 0;
				gCollisionList[gNumCollisions].targetBox 	= target;
				gCollisionList[gNumCollisions].sides 		= sideBits;
				gCollisionList[gNumCollisions].type 		= COLLISION_TYPE_OBJ;
				gCollisionList[gNumCollisions].objectPtr 	= thisNode;
				gNumCollisions++;	
				gTotalSides |= sideBits;											// remember total of this
			}
		}
next:	
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);

	if (gNumCollisions > MAX_COLLISIONS)											// see if overflowed (memory corruption ensued)
		DoFatalAlert("CollisionDetect: gNumCollisions > MAX_COLLISIONS");
}


/***************** HANDLE COLLISIONS ********************/
//
// This is a generic collision handler.  Takes care of
// all processing.
//
// INPUT:  cType = CType bit mask for collision matching
//
// OUTPUT: totalSides
//

Byte HandleCollisions(ObjNode *theNode, uint32_t cType, float deltaBounce)
{
Byte		totalSides;
short		i;
float		originalX,originalY,originalZ;
float		offset,maxOffsetX,maxOffsetZ,maxOffsetY;
float		offXSign,offZSign,offYSign;
Byte		base,target;
ObjNode		*targetObj;
CollisionBoxType *baseBoxPtr,*targetBoxPtr;
float		/*leftSide,rightSide,frontSide,backSide,*/bottomSide;
CollisionBoxType *boxList;
short		numSolidHits, numPasses = 0;
Boolean		hitImpenetrable = false;
short		oldNumCollisions;
Boolean		previouslyOnGround, hitMPlatform = false;
Boolean		hasTriggered = false;	
ObjNode		*trigger = nil;			

	if (deltaBounce > 0.0f)									// make sure Brian entered a (-) bounce value!
		deltaBounce = -deltaBounce;

	theNode->MPlatform = nil;									// assume not on MPlatform

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// remember if was on ground or not
		previouslyOnGround = true;
	else
		previouslyOnGround = false;
	
	theNode->StatusBits &= ~STATUS_BIT_ONGROUND;			// assume not on anything now

	gNumCollisions = oldNumCollisions = 0;
	gTotalSides = 0;
	totalSides = 0;

again:
	originalX = gCoord.x;									// remember starting coords
	originalY = gCoord.y;									
	originalZ = gCoord.z;								

	numSolidHits = 0;

	CalcObjectBoxFromGlobal(theNode);						// calc current collision box

			/**************************/
			/* GET THE COLLISION LIST */
			/**************************/

	CollisionDetect(theNode,cType, gNumCollisions);			// get collision info
		
	maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
	offXSign = offYSign = offZSign = 0;

			/* GET BASE BOX INFO */
			
	if (theNode->NumCollisionBoxes == 0)					// it's gotta have a collision box
		return(0);
	boxList 	= theNode->CollisionBoxes;
//	leftSide 	= boxList->left;
//	rightSide 	= boxList->right;
//	frontSide 	= boxList->front;
//	backSide 	= boxList->back;
	bottomSide 	= boxList->bottom;

			/*************************************/
			/* SCAN THRU ALL RETURNED COLLISIONS */	
			/*************************************/
	
	for (i=oldNumCollisions; i < gNumCollisions; i++)		// handle all collisions
	{	
		base 		= gCollisionList[i].baseBox;			// get collision box index for base & target
		target 		= gCollisionList[i].targetBox;
		targetObj 	= gCollisionList[i].objectPtr;			// get ptr to target objnode
		
		baseBoxPtr 	= boxList + base;						// calc ptrs to base & target collision boxes
		if (targetObj)
		{
			targetBoxPtr = targetObj->CollisionBoxes;	
			targetBoxPtr += target;
		}
		else
		{
			targetBoxPtr = nil;
		}
		
					/********************************/
					/* HANDLE OBJECT COLLISIONS 	*/	
					/********************************/
				
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{			
				/* SEE IF THIS OBJECT HAS SINCE BECOME INVALID */
				
			uint32_t	targetCType = targetObj->CType;						// get ctype of hit obj
			if (targetCType == INVALID_NODE_FLAG)				
				continue;
		
						/* HANDLE TRIGGERS */
		
			if (((targetCType & CTYPE_TRIGGER) && (cType & CTYPE_TRIGGER)) ||	// target must be trigger and we must have been looking for them as well
				((targetCType & CTYPE_TRIGGERENEMYONLY) && (cType & CTYPE_TRIGGERENEMYONLY)))
	  		{
	  			gSolidTriggerKeepDelta = false;									// assume solid triggers will cause delta to stop below
	  			
	  			if (targetObj->TriggerCallback != nil)							// make sure there's a callback installed
	  			{
 					if (!targetObj->TriggerCallback(targetObj,theNode,gCollisionList[i].sides))	// returns false if handle as non-solid trigger
						gCollisionList[i].sides = 0;
						
					trigger = targetObj;										// remember which obj we triggered
				}
				
				numSolidHits++;
					
				maxOffsetX = gCoord.x - originalX;								// see if trigger caused a move
				if (maxOffsetX < 0.0f)
				{
					maxOffsetX = -maxOffsetX;
					offXSign = -1;				
				}
				else
				if (maxOffsetX > 0.0f)
					offXSign = 1;				 

				maxOffsetZ = gCoord.z - originalZ;				
				if (maxOffsetZ < 0.0f)
				{
					maxOffsetZ = -maxOffsetZ;
					offZSign = -1;				
				}
				else
				if (maxOffsetZ > 0.0f)
					offZSign = 1;				
				
				hasTriggered = true;									// dont allow multi-pass collision once there is a trigger (to avoid multiple hits on the same trigger)
				
				
				if (gSolidTriggerKeepDelta)							// if trigger's callback set this then set delta bounce to 1.0 so it'll no affect the deltas
					deltaBounce = 1.0f;
			}		
					

				/* CHECK FOR MPLATFORM */
				
			if (targetCType & CTYPE_MPLATFORM)
			{
				if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)				// only if bottom hit it
				{
					theNode->MPlatform = gCollisionList[i].objectPtr;				
					hitMPlatform = true;			
				}
			}


					/*******************/
					/* DO SOLID FIXING */
					/*******************/
					
			if (gCollisionList[i].sides & ALL_SOLID_SIDES)						// see if object with any solidness
			{
				numSolidHits++;
		
				if (targetObj->CBits & CBITS_IMPENETRABLE)							// if this object is impenetrable, then throw out any other collision offsets
				{
					hitImpenetrable = true;
					maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
					offXSign = offYSign = offZSign = 0;			
				}
			
				if (gCollisionList[i].sides & SIDE_BITS_BACK)						// SEE IF BACK HIT
				{
					offset = (targetBoxPtr->front - baseBoxPtr->back)+.01f;		// see how far over it went
					if (offset > maxOffsetZ)
					{
						maxOffsetZ = offset;
						offZSign = 1;
					}
					gDelta.z *= deltaBounce;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_FRONT)						// SEE IF FRONT HIT
				{
					offset = (baseBoxPtr->front - targetBoxPtr->back)+.01f;		// see how far over it went
					if (offset > maxOffsetZ)
					{
						maxOffsetZ = offset;
						offZSign = -1;
					}
					gDelta.z *= deltaBounce;
				}

				if (gCollisionList[i].sides & SIDE_BITS_LEFT)						// SEE IF HIT LEFT
				{
					offset = (targetBoxPtr->right - baseBoxPtr->left)+.01f;		// see how far over it went
					if (offset > maxOffsetX)
					{
						maxOffsetX = offset;
						offXSign = 1;
					}
					gDelta.x *= deltaBounce;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_RIGHT)						// SEE IF HIT RIGHT
				{
					offset = (baseBoxPtr->right - targetBoxPtr->left)+.01f;		// see how far over it went
					if (offset > maxOffsetX)
					{
						maxOffsetX = offset;
						offXSign = -1;
					}
					gDelta.x *= deltaBounce;
				}

				if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)						// SEE IF HIT BOTTOM
				{
					offset = (targetBoxPtr->top - baseBoxPtr->bottom)+.01f;		// see how far over it went
					if (offset > maxOffsetY)
					{
						maxOffsetY = offset;
						offYSign = 1;
					}
					gDelta.y = -150;					// keep some downward momentum!!
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_TOP)						// SEE IF HIT TOP
				{
					offset = (baseBoxPtr->top - targetBoxPtr->bottom)+1.0f;			// see how far over it went
					if (offset > maxOffsetY)
					{
						maxOffsetY = offset;
						offYSign = -1;
					}
					gDelta.y =0;						
				}
			}
		}

		totalSides |= gCollisionList[i].sides;				// keep sides info		
		
		if (hitImpenetrable)								// if that was impenetrable, then we dont need to check other collisions
			break;			
	}

		/* IF THERE WAS A SOLID HIT, THEN WE NEED TO UPDATE AND TRY AGAIN */
			
	if (numSolidHits > 0)
	{
				/* ADJUST MAX AMOUNTS */
				
		gCoord.x = originalX + (maxOffsetX * offXSign);
		gCoord.z = originalZ + (maxOffsetZ * offZSign);
		gCoord.y = originalY + (maxOffsetY * offYSign);			// y is special - we do some additional rouding to avoid the jitter problem
			
			
				/* SEE IF NEED TO SET GROUND FLAG */
				
		if (totalSides & SIDE_BITS_BOTTOM)
		{
			if (!previouslyOnGround)							// if not already on ground, then add some friction upon landing
			{
				if (hitMPlatform)								// special case landing on moving platforms - stop deltas
				{
					gDelta.x *= .8f; 
					gDelta.z *= .8f;
				}
			}
			theNode->StatusBits |= STATUS_BIT_ONGROUND;	
		}
	
	
				/* SEE IF DO ANOTHER PASS */

		numPasses++;		
		if ((numPasses < 3) && (!hitImpenetrable) && (!hasTriggered))	// see if can do another pass and havnt hit anything impenetrable
			goto again;
	}
	
	
			/* SEE IF UPDATE TRIGGER INFO */
			
	if (trigger)													// did we hit a trigger this time?
		theNode->CurrentTriggerObj = trigger;						// yep, so remember it
	else
		theNode->CurrentTriggerObj = false;

	
				/*************************/
				/* CHECK FENCE COLLISION */
				/*************************/
			
	if (cType & CTYPE_FENCE)
	{
		if (DoFenceCollision(theNode))
		{
			totalSides |= ALL_SOLID_SIDES;
			numPasses++;
			if (numPasses < 3)
				goto again;
		}
	}

			/******************************************/
			/* SEE IF DO AUTOMATIC TERRAIN GROUND HIT */
			/******************************************/
	
	if (cType & CTYPE_TERRAIN)
	{
		float	y = GetTerrainY(gCoord.x, gCoord.z);			// get terrain Y
		
		if (bottomSide <= y)										// see if bottom is under ground
		{
			gCoord.y += y - bottomSide;
			
			if (gDelta.y < 0.0f)								// if was going down then bounce y
			{
				gDelta.y *= deltaBounce;
				if (fabs(gDelta.y) < 30.0f)						// if small enough just make zero
					gDelta.y = 0;
			}

			theNode->StatusBits |= STATUS_BIT_ONGROUND;
			
			totalSides |= SIDE_BITS_BOTTOM;
		}
	}
	
			/* SEE IF DO WATER COLLISION TEST */
			
	if (cType & CTYPE_WATER)
	{
		int	patchNum;
		
		DoWaterCollisionDetect(theNode, gCoord.x, gCoord.y, gCoord.z, &patchNum);
	}
		
	
			
	return(totalSides);
}



#pragma mark - POINT COLLISION

/****************** IS POINT IN POLY ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInPoly2D(float pt_x, float pt_y, Byte numVerts, OGLPoint2D *polypts)
{
Byte 		oldquad,newquad;
float 		thispt_x,thispt_y,lastpt_x,lastpt_y;
signed char	wind;										// current winding number 
Byte		i;

			/************************/
			/* INIT STARTING VALUES */
			/************************/
			
	wind = 0;
    lastpt_x = polypts[numVerts-1].x;  					// get last point's coords  
    lastpt_y = polypts[numVerts-1].y;    
    
	if (lastpt_x < pt_x)								// calc quadrant of the last point
	{
    	if (lastpt_y < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (lastpt_y < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}
    

			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/
    
    for (i=0; i<numVerts; i++)
    {
   			/* GET THIS POINT INFO */
    			
		thispt_x = polypts[i].x;						// get this point's coords
		thispt_y = polypts[i].y;

		if (thispt_x < pt_x)							// calc quadrant of this point
		{
	    	if (thispt_y < pt_y)
	    		newquad = 2;
	 		else
	 			newquad = 1;
	 	}
	 	else
	    {
	    	if (thispt_y < pt_y)
	    		newquad = 3;
	 		else
	 			newquad = 0;
		}

				/* SEE IF QUADRANT CHANGED */
				
        if (oldquad != newquad)
        {
			if (((oldquad+1)&3) == newquad)				// see if advanced
            	wind++;
			else
        	if (((newquad+1)&3) == oldquad)				// see if backed up
				wind--;
    		else
			{
				float	a,b;
				
             		/* upper left to lower right, or upper right to lower left.
             		   Determine direction of winding  by intersection with x==0. */
                                             
    			a = (lastpt_y - thispt_y) * (pt_x - lastpt_x);			
                b = lastpt_x - thispt_x;
                a += lastpt_y * b;
                b *= pt_y;

				if (a > b)
                	wind += 2;
 				else
                	wind -= 2;
    		}
  		}
  		
  				/* MOVE TO NEXT POINT */
  				
   		lastpt_x = thispt_x;
   		lastpt_y = thispt_y;
   		oldquad = newquad;
	}
	

	return(wind); 										// non zero means point in poly
}





/****************** IS POINT IN TRIANGLE ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2)
{
Byte 		oldquad,newquad;
float		m;
signed char	wind;										// current winding number 

			/*********************/
			/* DO TRIVIAL REJECT */
			/*********************/
			
	m = x0;												// see if to left of triangle							
	if (x1 < m)
		m = x1;
	if (x2 < m)
		m = x2;
	if (pt_x < m)
		return(false);

	m = x0;												// see if to right of triangle							
	if (x1 > m)
		m = x1;
	if (x2 > m)
		m = x2;
	if (pt_x > m)
		return(false);

	m = y0;												// see if to back of triangle							
	if (y1 < m)
		m = y1;
	if (y2 < m)
		m = y2;
	if (pt_y < m)
		return(false);

	m = y0;												// see if to front of triangle							
	if (y1 > m)
		m = y1;
	if (y2 > m)
		m = y2;
	if (pt_y > m)
		return(false);


			/*******************/
			/* DO WINDING TEST */
			/*******************/
			
		/* INIT STARTING VALUES */
			
    
	if (x2 < pt_x)								// calc quadrant of the last point
	{
    	if (y2 < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}
    

			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/

	wind = 0;
    

//=============================================
			
	if (x0 < pt_x)									// calc quadrant of this point
	{
    	if (y0 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y0 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y2 - y0) * (pt_x - x2);			
            b = x2 - x0;
            a += y2 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
				
	oldquad = newquad;

//=============================================

	if (x1 < pt_x)							// calc quadrant of this point
	{
    	if (y1 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y1 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y0 - y1) * (pt_x - x0);			
            b = x0 - x1;
            a += y0 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
			
	oldquad = newquad;

//=============================================
			
	if (x2 < pt_x)							// calc quadrant of this point
	{
    	if (y2 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */
			
    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;
			
         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */
                                         
			a = (y1 - y2) * (pt_x - x1);			
            b = x1 - x2;
            a += y1 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}
	
	return(wind); 										// non zero means point in poly
}






/******************** DO SIMPLE POINT COLLISION *********************************/
//
// INPUT:  except == objNode to skip
//
// OUTPUT: # collisions detected
//

short DoSimplePointCollision(OGLPoint3D *thePoint, uint32_t cType, ObjNode *except)
{
ObjNode	*thisNode;
short	targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	gNumCollisions = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode == except)									// see if skip this one
			goto next;
			
		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)	// don't collide against these
			goto next;
		
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;

	
				/* GET BOX INFO FOR THIS NODE */
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;
	
	
			/***************************************/
			/* CHECK POINT AGAINST EACH TARGET BOX */
			/***************************************/
			
		for (target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */
		
			if (thePoint->x < targetBoxList[target].left)
				continue;
				
			if (thePoint->x > targetBoxList[target].right)
				continue;
				
			if (thePoint->z < targetBoxList[target].back)
				continue;
				
			if (thePoint->z > targetBoxList[target].front)
				continue;
				
			if (thePoint->y > targetBoxList[target].top)
				continue;

			if (thePoint->y < targetBoxList[target].bottom)
				continue;
				
								
					/* THERE HAS BEEN A COLLISION */

			gCollisionList[gNumCollisions].targetBox = target;
			gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
			gCollisionList[gNumCollisions].objectPtr = thisNode;
			gNumCollisions++;	
		}
		
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}


/******************** DO SIMPLE BOX COLLISION *********************************/
//
// OUTPUT: # collisions detected
//

short DoSimpleBoxCollision(float top, float bottom, float left, float right,
						float front, float back, uint32_t cType)
{
ObjNode			*thisNode;
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	gNumCollisions = 0;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)	// don't collide against these
			goto next;
		
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;

	
				/* GET BOX INFO FOR THIS NODE */
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;
	
	
			/*********************************/
			/* CHECK AGAINST EACH TARGET BOX */
			/*********************************/
			
		for (target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */
		
			if (right < targetBoxList[target].left)
				continue;
				
			if (left > targetBoxList[target].right)
				continue;
				
			if (front < targetBoxList[target].back)
				continue;
				
			if (back > targetBoxList[target].front)
				continue;
				
			if (bottom > targetBoxList[target].top)
				continue;

			if (top < targetBoxList[target].bottom)
				continue;
				
								
					/* THERE HAS BEEN A COLLISION */

			gCollisionList[gNumCollisions].targetBox = target;
			gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
			gCollisionList[gNumCollisions].objectPtr = thisNode;
			gNumCollisions++;	
		}
		
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}


/******************** DO SIMPLE BOX COLLISION AGAINST PLAYER *********************************/

Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right,
										float front, float back)
{
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	if (gPlayerIsDead)									// if dead then blown up and can't be hit
		return(false);


			/* GET BOX INFO FOR THIS NODE */
				
	targetNumBoxes = gPlayerInfo.objNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = gPlayerInfo.objNode->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/
		
	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */
	
		if (right < targetBoxList[target].left)
			continue;
			
		if (left > targetBoxList[target].right)
			continue;
			
		if (front < targetBoxList[target].back)
			continue;
			
		if (back > targetBoxList[target].front)
			continue;
			
		if (bottom > targetBoxList[target].top)
			continue;

		if (top < targetBoxList[target].bottom)
			continue;

		return(true);				
	}
	
	return(false);
}



/******************** DO SIMPLE POINT COLLISION AGAINST PLAYER *********************************/

Boolean DoSimplePointCollisionAgainstPlayer(OGLPoint3D *thePoint)
{
short	targetNumBoxes,target;
CollisionBoxType *targetBoxList;

	
	if (gPlayerIsDead)									// if dead then blown up and can't be hit
		return(false);

			/* GET BOX INFO FOR THIS NODE */
				
	targetNumBoxes = gPlayerInfo.objNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = gPlayerInfo.objNode->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/
		
	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */
	
		if (thePoint->x < targetBoxList[target].left)
			continue;
			
		if (thePoint->x > targetBoxList[target].right)
			continue;
			
		if (thePoint->z < targetBoxList[target].back)
			continue;
			
		if (thePoint->z > targetBoxList[target].front)
			continue;
			
		if (thePoint->y > targetBoxList[target].top)
			continue;

		if (thePoint->y < targetBoxList[target].bottom)
			continue;

		return(true);				
	}
	
	return(false);
}

/******************** DO SIMPLE BOX COLLISION AGAINST OBJECT *********************************/

Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right,
										float front, float back, ObjNode *targetNode)
{
short			targetNumBoxes,target;
CollisionBoxType *targetBoxList;


			/* GET BOX INFO FOR THIS NODE */
				
	targetNumBoxes = targetNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	targetBoxList = targetNode->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/
		
	for (target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */
	
		if (right < targetBoxList[target].left)
			continue;
			
		if (left > targetBoxList[target].right)
			continue;
			
		if (front < targetBoxList[target].back)
			continue;
			
		if (back > targetBoxList[target].front)
			continue;
			
		if (bottom > targetBoxList[target].top)
			continue;

		if (top < targetBoxList[target].bottom)
			continue;

		return(true);				
	}
	
	return(false);
}



/************************ FIND HIGHEST COLLISION AT XZ *******************************/
//
// Given the XY input, this returns the highest Y coordinate of any collision
// box here.
//

float FindHighestCollisionAtXZ(float x, float z, uint32_t cType)
{
ObjNode	*thisNode;
short	targetNumBoxes,target;
CollisionBoxType *targetBoxList;
float	topY = -10000000;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (!(thisNode->CType & cType))							// matching ctype
			goto next;

		if (!(thisNode->CBits & CBITS_TOP))						// only top solid objects
			goto next;

	
				/* GET BOX INFO FOR THIS NODE */
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;
	
	
			/***************************************/
			/* CHECK POINT AGAINST EACH TARGET BOX */
			/***************************************/
			
		for (target = 0; target < targetNumBoxes; target++)
		{
			if (targetBoxList[target].top < topY)				// check top
				continue;			

					/* DO RECTANGLE INTERSECTION */
		
			if (x < targetBoxList[target].left)
				continue;
				
			if (x > targetBoxList[target].right)
				continue;
				
			if (z < targetBoxList[target].back)
				continue;
				
			if (z > targetBoxList[target].front)
				continue;
				
			topY = targetBoxList[target].top + .1f;					// save as highest Y
				
		}
		
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

			/*********************/
			/* NOW CHECK TERRAIN */
			/*********************/

	if (cType & CTYPE_TERRAIN)
	{
		float	ty = GetTerrainY(x,z);
		
		if (ty > topY)
			topY = ty;
	}
	
			/*******************/
			/* NOW CHECK WATER */
			/*******************/
			
	if (cType & CTYPE_WATER)
	{
		float	wy;
		
		if (GetWaterY(x, z, &wy))
		{
			if (wy > topY)
				topY = wy;	
		}
	}
			

	return(topY);
}


#pragma mark -




/***************** RAY INTERSECT TRIANGLE *******************/

static Boolean RayIntersectTriangle(OGLPoint3D *origin, OGLVector3D *dir,
                   			OGLPoint3D *v0, OGLPoint3D *v1, OGLPoint3D *v2,
                   			Boolean	cullTest, float *t, float *u, float *v)
{
   OGLVector3D 	edge1, edge2;
   OGLVector3D 	tvec, pvec, qvec;
   float 		det,inv_det;

   /* find vectors for two edges sharing vert0 */
   
   edge1.x = v1->x - v0->x;
   edge1.y = v1->y - v0->y;
   edge1.z = v1->z - v0->z;

   edge2.x = v2->x - v0->x;
   edge2.y = v2->y - v0->y;
   edge2.z = v2->z - v0->z;


	/* begin calculating determinant - also used to calculate U parameter */

	OGLVector3D_Cross(dir, &edge2, &pvec);


	/* if determinant is near zero, ray lies in plane of triangle */

	det = (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);		// calc dot product

	if (cullTest)
	{
	   if (det < EPS)
	      return(false);

		/* calculate distance from vert0 to ray origin */
		
	   tvec.x = origin->x - v0->x;				
	   tvec.y = origin->y - v0->y;
	   tvec.z = origin->z - v0->z;


		/* calculate U parameter and test bounds */
	   
		*u = (tvec.x * pvec.x) + (tvec.y * pvec.y) + (tvec.z * pvec.z);		// calc dot product
		if ((*u < 0.0f) || (*u > det))
	  		return (false);

		
		/* prepare to test V parameter */
	   
		OGLVector3D_Cross(&tvec, &edge1, &qvec);


		/* calculate V parameter and test bounds */
	    
		*v = (dir->x * qvec.x) + (dir->y * qvec.y) + (dir->z * qvec.z);		// calc dot product
		if ((*v < 0.0f) || ((*u + *v) > det))
			return 0;

	
		/* calculate t, scale parameters, ray intersects triangle */
		
		*t = (edge2.x * qvec.x) + (edge2.y * qvec.y) + (edge2.z * qvec.z);		// calc dot product
		
		inv_det = 1.0f / det;
		*t *= inv_det;
		*u *= inv_det;
		*v *= inv_det;
	}
				/* NO CULLING */
	else
	{
		if ((det > -EPS) && (det < EPS))
			return(false);
			
		inv_det = 1.0f / det;

		/* calculate distance from vert0 to ray origin */
	   
	   	tvec.x = origin->x - v0->x;				
		tvec.y = origin->y - v0->y;
	 	tvec.z = origin->z - v0->z;

   		/* calculate U parameter and test bounds */
   		
		*u = (tvec.x * pvec.x) + (tvec.y * pvec.y) + (tvec.z * pvec.z);		// calc dot product
   		*u *= inv_det;
		
		if ((*u < 0.0f) || (*u > 1.0f))
			return(false);

		/* prepare to test V parameter */

		OGLVector3D_Cross(&tvec, &edge1, &qvec);


		/* calculate V parameter and test bounds */
	   
		*v = (dir->x * qvec.x) + (dir->y * qvec.y) + (dir->z * qvec.z);		// calc dot product
	   	*v *= inv_det;
		
		if ((*v < 0.0f) || (*u + *v > 1.0f))
			return(false);

		/* calculate t, ray intersects triangle */
		
		*t = (edge2.x * qvec.x) + (edge2.y * qvec.y) + (edge2.z * qvec.z);		// calc dot product		
		*t *= inv_det;
	}
	return(true);
}


#pragma mark -


/******************** SEE IF LINE SEGMENT HITS ANYTHING **************************/

Boolean SeeIfLineSegmentHitsAnything(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2, const ObjNode *except, uint32_t ctype)
{
ObjNode	*thisNode;
OGLPoint2D	p1,p2,crossBeamP1,crossBeamP2;
short			targetNumBoxes;
CollisionBoxType *targetBoxList;
float	ix,iz,iy;

			/* SEE IF HIT FENCE */
			
	if (ctype & CTYPE_FENCE)
	{
		if (SeeIfLineSegmentHitsFence(endPoint1, endPoint2, nil, nil, nil))
			return(true);
	}

			/***************************/
			/* SEE IF HIT ANY OBJNODES */
			/***************************/
			
	p1.x = endPoint1->x;	p1.y = endPoint1->z;				// get x/z of segment endpoints
	p2.x = endPoint2->x;	p2.y = endPoint2->z;
			
			
	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode == except)									// see if skip this one
			goto next;
			
		if (!(thisNode->CType & ctype))							// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;
		
		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;

	
				/* GET BOX INFO FOR THIS NODE */
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;


				/* CREATE SEGMENT FROM CROSSBEAM */
				
		crossBeamP1.x = targetBoxList[0].left;
		crossBeamP1.y = targetBoxList[0].back;

		crossBeamP2.x = targetBoxList[0].right;
		crossBeamP2.y = targetBoxList[0].front;


			/* SEE IF INPUT SEGMENT INTERSECTS THE CROSSBEAM SEGMENT */
				
		if (IntersectLineSegments(p1.x, p1.y, p2.x, p2.y,
			                     crossBeamP1.x, crossBeamP1.y, crossBeamP2.x, crossBeamP2.y,
	                             &ix, &iz))
	  	{
			float	dy = endPoint2->y - endPoint1->y;			// get dy of line segment

			float	d1 = CalcDistance(p1.x, p1.y, p2.x, p2.y);
			float	d2 = CalcDistance(p1.x, p1.y, ix, iz);
					
			float	ratio = d2/d1;

			iy = endPoint1->y + (dy * ratio);					// calc intersect y coord

			if ((iy <= targetBoxList[0].top) &&					// if below top & above bottom, then HIT
				(iy >= targetBoxList[0].bottom))
			{
				return(true);			
			}
	  	}
	                             
		
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


	return(false);
}



/******************** SEE IF LINE SEGMENT HITS OBJECT **************************/

Boolean SeeIfLineSegmentHitsObject(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2, ObjNode *theNode)
{
OGLPoint2D	p1, p2;
OGLPoint2D	crossBeamP1[4], crossBeamP2[4];
CollisionBoxType *collisionBox;
float	ix,iz,iy;
int		i;

	if (gPlayerIsDead)											// if player dead/gone then cant hit
		return(false);
			
	p1.x = endPoint1->x;	p1.y = endPoint1->z;				// get x/z of segment endpoints
	p2.x = endPoint2->x;	p2.y = endPoint2->z;
			
	collisionBox = &theNode->CollisionBoxes[0];

			
		/*******************************************************/
		/* FIRST SEE IF ANY ENDPOINTS ARE IN THE COLLISION BOX */
		/*******************************************************/
				
				/* CHECK P1 */
				
	if ((endPoint1->x >= collisionBox->left) && (endPoint1->x <= collisionBox->right))
		if ((endPoint1->y >= collisionBox->bottom) && (endPoint1->y <= collisionBox->top))
			if ((endPoint1->z >= collisionBox->back) && (endPoint1->z <= collisionBox->front))
				return(true);

				/* CHECK P2 */
				
	if ((endPoint2->x >= collisionBox->left) && (endPoint2->x <= collisionBox->right))
		if ((endPoint2->y >= collisionBox->bottom) && (endPoint2->y <= collisionBox->top))
			if ((endPoint2->z >= collisionBox->back) && (endPoint2->z <= collisionBox->front))
				return(true);
				
				
				/**************************/
				/* DO LINE INTERSECT TEST */
				/**************************/
			

			/* CREATE SEGMENT FROM CROSSBEAM */
						
	crossBeamP1[0].x = collisionBox->left;						// span #0
	crossBeamP1[0].y = collisionBox->back;
	crossBeamP2[0].x = collisionBox->right;
	crossBeamP2[0].y = collisionBox->front;

	crossBeamP1[1].x = collisionBox->left;						// span #1
	crossBeamP1[1].y = collisionBox->front;
	crossBeamP2[1].x = collisionBox->right;
	crossBeamP2[1].y = collisionBox->back;

	crossBeamP1[2].x = (collisionBox->left + collisionBox->right) * .5f;	// span #2
	crossBeamP1[2].y = collisionBox->front;
	crossBeamP2[2].x = crossBeamP1[2].x;
	crossBeamP2[2].y = collisionBox->back;

	crossBeamP1[3].x = collisionBox->left;						// span #3
	crossBeamP1[3].y = (collisionBox->front + collisionBox->back) * .5f;
	crossBeamP2[3].x = collisionBox->right;
	crossBeamP2[3].y = crossBeamP1[3].y;


		/* SEE IF INPUT SEGMENT INTERSECTS THE CROSSBEAM SEGMENT */
			
	for (i = 0; i < 4; i++)										// check all spans
	{
		if (IntersectLineSegments(p1.x, p1.y, p2.x, p2.y,
			                     crossBeamP1[i].x, crossBeamP1[i].y, crossBeamP2[i].x, crossBeamP2[i].y,
	                             &ix, &iz))
	  	{
			float	dy = endPoint2->y - endPoint1->y;			// get dy of line segment

			float	d1 = CalcDistance(p1.x, p1.y, p2.x, p2.y);
			float	d2 = CalcDistance(p1.x, p1.y, ix, iz);
					
			float	ratio = d2/d1;

			iy = endPoint1->y + (dy * ratio);					// calc intersect y coord

			if ((iy <= collisionBox->top) &&					// if below top & above bottom, then HIT
				(iy >= collisionBox->bottom))
			{
				return(true);			
			}
	  	}
	}

	return(false);
}


/******************** SEE IF LINE SEGMENT HITS WHAT **************************/

ObjNode *SeeIfLineSegmentHitsWhat(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2, int what)
{
ObjNode	*thisNode;
OGLPoint2D	p1,p2,crossBeamP1,crossBeamP2;
short			targetNumBoxes;
CollisionBoxType *targetBoxList;
float	ix,iz,iy;

			/***************************/
			/* SEE IF HIT ANY OBJNODES */
			/***************************/
			
	p1.x = endPoint1->x;	p1.y = endPoint1->z;				// get x/z of segment endpoints
	p2.x = endPoint2->x;	p2.y = endPoint2->z;
			
			
	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{			
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->What != what)								// see if we want to check this What
			goto next;

	
				/* GET BOX INFO FOR THIS NODE */
					
		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		targetBoxList = thisNode->CollisionBoxes;


				/* CREATE SEGMENT FROM CROSSBEAM */
				
		crossBeamP1.x = targetBoxList[0].left;
		crossBeamP1.y = targetBoxList[0].back;

		crossBeamP2.x = targetBoxList[0].right;
		crossBeamP2.y = targetBoxList[0].front;


			/* SEE IF INPUT SEGMENT INTERSECTS THE CROSSBEAM SEGMENT */
				
		if (IntersectLineSegments(p1.x, p1.y, p2.x, p2.y,
			                     crossBeamP1.x, crossBeamP1.y, crossBeamP2.x, crossBeamP2.y,
	                             &ix, &iz))
	  	{
			float	dy = endPoint2->y - endPoint1->y;			// get dy of line segment

			float	d1 = CalcDistance(p1.x, p1.y, p2.x, p2.y);
			float	d2 = CalcDistance(p1.x, p1.y, ix, iz);
					
			float	ratio = d2/d1;

			iy = endPoint1->y + (dy * ratio);					// calc intersect y coord

			if ((iy <= targetBoxList[0].top) &&					// if below top & above bottom, then HIT
				(iy >= targetBoxList[0].bottom))
			{
				return(thisNode);			
			}
	  	}
	                             
		
next:	
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


	return(nil);
}














