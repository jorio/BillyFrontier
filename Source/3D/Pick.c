/****************************/
/*   		PICK.C    	    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "bones.h"
#include "3dmath.h"

extern	GLuint				gPickBuffer[];
extern	int					gNumPickHits,gNumWorldCalcsThisFrame;
extern	MOVertexArrayData	**gLocalTriMeshesOfSkelType;
extern	ObjNode				*gFirstNodePtr;
extern	long				gNumSuperTilesDeep,gNumSuperTilesWide;
extern	SuperTileStatus		**gSuperTileStatusGrid;
extern	SuperTileMemoryType	gSuperTileMemoryList[MAX_SUPERTILES];

/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean OGL_RayGetHitInfo_DisplayGroup(OGLRay *ray, ObjNode *theNode, OGLPoint3D *worldHitCoord);
static Boolean	OGL_DoesRayIntersectMesh(OGLRay *ray, MOVertexArrayData *mesh, OGLPoint3D *intersectionPt, float *distToIntersection);

static void OGLTriangle_3D2DComponentProjectionPoints(const OGLVector3D *triangleNormal,	const OGLPoint3D *point3D, const OGLPoint3D	*triPoints,
													 OGLPoint2D *point2D, OGLPoint2D *verts2D);

static void MO_RayTestGeometry_VertexArray(MOVertexArrayData *data);
static void MO_RayTestMatrix(const MOMatrixObject *matObj);
static void MO_RayTestGroup(const MOGroupObject *object);
static void MO_RayTestObject(const MetaObjectPtr object);




#define	EmVector3D_Member_Dot(_nx,_ny,_nz,_p)							\
			((_nx * _p.x) + (_ny * _p.y) + (_nz * _p.z))


static Boolean ObjectIsInFrontOfRay(ObjNode *theNode, OGLRay *ray);
static Boolean OGL_DoesRayIntersectSphere(OGLRay *ray, OGLPoint3D *sphereCenter, float sphereRadius, OGLPoint3D *intersectPt);

static Boolean	OGL_DoesLineSegIntersectMesh(MOVertexArrayData *mesh, OGLPoint3D *intersectionPt, float *distToIntersection);
static Boolean	OGL_DoesLineSegIntersectMesh2(OGLPlaneEquation *planeEQ, MOVertexArrayData *mesh, OGLPoint3D *intersectionPt, float *distToIntersection);
static Boolean OGL_LineSegGetHitInfo_DisplayGroup(OGLPoint3D *p1, OGLPoint3D *p2, ObjNode *theNode, OGLPoint3D *worldHitCoord, float *distToHit);
static void MO_LineSegTestMatrix(const MOMatrixObject *matObj);
static void MO_LineSegTestGeometry_VertexArray(MOVertexArrayData *data);
static void MO_LineSegTestGroup(const MOGroupObject *object);
static void MO_LineSegTestObject(const MetaObjectPtr object);
static Boolean	OGL_LineSegIntersectsTriangle(OGLPoint3D *trianglePoints, OGLPoint3D *intersectPt, float *distFromP1ToPlane);
static Boolean	OGL_LineSegIntersectsTriangle2(OGLPlaneEquation *planeEQ, OGLPoint3D *trianglePoints, OGLPoint3D *intersectPt, float *distFromP1ToPlane);
static Boolean OGL_DoesLineSegIntersectTrianglePlane(OGLPoint3D	triWorldPoints[], float *distFromP1ToPlane);
static Boolean OGL_DoesLineSegIntersectTrianglePlane2(OGLPlaneEquation *planeEQ, float *distFromP1ToPlane);

static Boolean OGL_PickAndGetHitInfo_Skeleton(OGLRay *ray, ObjNode *theNode, OGLPoint3D *worldHitCoord);
static Boolean OGL_PickAndGetInfo_Terrain(OGLSetupOutputType *setupInfo, OGLPoint2D *point, OGLPoint3D *worldHitCoord);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static OGLPoint3D		gP1, gP2;
static OGLRay		gPickRay;
static OGLPoint3D	gClosestHitCoord;
static float		gClosestHitDist;
static Boolean		gGotAHit;
static	OGLPlaneEquation	gRecentPlaneEq;
static OGLVector3D	gBestTriangleNormal;


/**************** OGL: DO RAY COLLISION ************************/
//
// Checks to see if the input ray hits any eligible and visible objNodes in the scene.
//
// INPUT: 	ray = world-space ray to collide with
//			statusFilter = STATUS_BIT flags used for filtering out objects we don't care about (like hidden or culled objects)
//			cTypes = which objects do we want to collide against?
//
// OUTPUT:  ObjNode of object picked or nil
//			worldHitCoord = world-space coords of the pick intersection
//			ray->distance = distance from ray origin to the intersection point
//

ObjNode *OGL_DoRayCollision(OGLRay *ray, OGLPoint3D *worldHitCoord, u_long statusFilter, u_long cTypes)
{
ObjNode		*thisNodePtr;
ObjNode		*bestObj = nil;
float		bestDist = 100000000;
OGLPoint3D	hitPt;

	thisNodePtr = gFirstNodePtr;
	
	do
	{
				/* VERIFY NODE */
			
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)								// stop here
			break;
			
		if (thisNodePtr->StatusBits & statusFilter)	//(STATUS_BIT_ISCULLED | STATUS_BIT_HIDDEN))		// only check on visible objects
			goto next;				
		if (thisNodePtr->CType == INVALID_NODE_FLAG)									// make sure the node is even valid
			goto next;
						
		if (thisNodePtr->CType & cTypes)							// only if pickable
		{
					/* IF THE PICK RAY HITS THE OBJECT'S BOUNDING SPHERE THEN SEE IF WE HIT THE GEOMETRY */
				
			if (OGL_DoesRayIntersectSphere(ray, &thisNodePtr->Coord, thisNodePtr->BoundingSphereRadius, nil))					
			{
			
						/* NOW PARSE THE OBJNODE AND DO RAY-TRIANGLE TESTS TO SEE WHERE WE HIT */

				switch(thisNodePtr->Genre)
				{		
					case	SKELETON_GENRE:		
							if (OGL_PickAndGetHitInfo_Skeleton(ray, thisNodePtr, &hitPt))		// does ray intersect skeleton?
							{
								if (ray->distance < bestDist)								// is this the best hit so far?
								{
									bestDist = ray->distance;
									bestObj = thisNodePtr;
									*worldHitCoord = hitPt;
								}
							}
							break;

					case	DISPLAY_GROUP_GENRE:		
							if (OGL_RayGetHitInfo_DisplayGroup(ray, thisNodePtr, &hitPt))	// does ray hit display group geometry?
							{
								if (ray->distance < bestDist)								// is this the best hit so far?
								{
									bestDist = ray->distance;
									bestObj = thisNodePtr;
									*worldHitCoord = hitPt;
								}
							}
							break;
							
					case	CUSTOM_GENRE:													// ignore this or do custom handling
							break;
							
					default:
							DoFatalAlert("\pOGL_DoRayCollision: unsupported genre");
				}
			}		
		}

next:
		thisNodePtr = thisNodePtr->NextNode;														// next node
	}
	while (thisNodePtr != nil);

	ray->distance = bestDist;								// return the best distance in the ray

	return(bestObj);
}


/**************************** OBJECT IS IN FRONT OF RAY ****************************/

static Boolean ObjectIsInFrontOfRay(ObjNode *theNode, OGLRay *ray)
{
OGLVector3D	v;
float		dot, x, y, z, r;
OGLPoint3D	pt;

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;

			/* FIRST JUST SEE IF THE OBJECT'S COORDS ARE IN FRONT */

	v.x = x - ray->origin.x;							// calc vector from origin to object coords
	v.y = y - ray->origin.y;			
	v.z = z - ray->origin.z;			
	FastNormalizeVector(v.x, v.y, v.z, &v);
	
	dot = OGLVector3D_Dot(&v, &ray->direction);			// calc angle between vectors
	if (dot < 0.0f)
		return(true);	


			/* NOW SEE IF BOUNDING SPHERE IS ALSO IN FRONT OR NOT */
	
	r = theNode->BoundingSphereRadius;
	pt.x = x - (v.x * r);								// calc point on bounding sphere
	pt.y = y - (v.y * r);
	pt.z = z - (v.z * r);

	v.x = pt.x - ray->origin.x;							// calc vector from origin to sphere coords
	v.y = pt.y - ray->origin.y;			
	v.z = pt.z - ray->origin.z;			
	FastNormalizeVector(v.x, v.y, v.z, &v);
	
	dot = OGLVector3D_Dot(&v, &ray->direction);			// calc angle between vectors
	if (dot < 0.0f)
		return(true);	

	return(false);
}


/******************** OGL:  DOES RAY INTERSECT SPHERE *************************/
//
// Returns TRUE if the input ray intersects the input sphere.
// also returns the intersect point if intersectPt != nil
//

static Boolean OGL_DoesRayIntersectSphere(OGLRay *ray, OGLPoint3D *sphereCenter, float sphereRadius, OGLPoint3D *intersectPt)
{
OGLVector3D			sphereToRay, intersectVector;
float				d, q, t, l2, r2, m2;

	// Prepare to intersect
	//
	// First calculate the vector from the sphere to the ray origin, its
	// length squared, the projection of this vector onto the ray direction,
	// and the squared radius of the sphere.
	
	OGLPoint3D_Subtract(sphereCenter, &ray->origin, &sphereToRay);
	l2 = OGLVector3D_Dot_NoPin(&sphereToRay, &sphereToRay);			// the dot of itself gives us the length squared
	d  = OGLVector3D_Dot_NoPin(&sphereToRay, &ray->direction);
	r2 = sphereRadius * sphereRadius;



	// If the sphere is behind the ray origin, they don't intersect
	if (d < 0.0f && l2 > r2)
		return(false);



	// Calculate the squared distance from the sphere center to the projection.
	// If it's greater than the radius then they don't intersect.
	m2 = (l2 - (d * d));
	if (m2 > r2)
		return(false);


	if (intersectPt != nil)
	{
		// Calculate the distance along the ray to the intersection point
		q = (float) sqrt(r2 - m2);
		if (l2 > r2)
			t = d - q;
		else
			t = d + q;

		// Calculate the intersection point
		OGLVector3D_Scale(&ray->direction, t, &intersectVector);
		OGLPoint3D_Vector3D_Add(&ray->origin, &intersectVector, intersectPt);
	}
	
	return(true);
}


/******************** OGL:  DOES LINE SEGMENT INTERSECT SPHERE *************************/
//
// Returns TRUE if the input line seg intersects the input sphere.
// also returns the intersect point if intersectPt != nil
//
// This is actually a variant of the Ray intersect function above.  A line segment
// is actually 2 opposite rays.
//

Boolean OGL_DoesLineSegmentIntersectSphere(OGLPoint3D *p1, OGLPoint3D *p2, OGLVector3D *segVector, OGLPoint3D *sphereCenter, float sphereRadius, OGLPoint3D *intersectPt)
{
OGLVector3D			sphereToEndpoint, intersectVector, rayDir;
float				d, l2, r2, m2;

					/*********************************************************************/
					/* DO THE USUAL RAY->SPHERE INTERSECT TEST WITH ONE OF THE ENDPOINTS */
					/*********************************************************************/
					
	// create a ray vector from p1 -> p2
	
	if (segVector == nil)
	{
		rayDir.x = p2->x - p1->x;
		rayDir.y = p2->y - p1->y;
		rayDir.z = p2->z - p1->z;
		OGLVector3D_Normalize(&rayDir, &rayDir);
	}
	else
		rayDir = *segVector;


	// Calculate the vector from the sphere to the p1 endpoint, its
	// length squared, the projection of this vector onto the ray direction,
	// and the squared radius of the sphere.
	
	OGLPoint3D_Subtract(sphereCenter, p1, &sphereToEndpoint);
	l2 = OGLVector3D_Dot_NoPin(&sphereToEndpoint, &sphereToEndpoint);			// the dot of itself gives us the length squared
	d  = OGLVector3D_Dot_NoPin(&sphereToEndpoint, &rayDir);
	r2 = sphereRadius * sphereRadius;



	// If the sphere is behind the endpoint, they don't intersect
	if (d < 0.0f && l2 > r2)
		return(false);



	// Calculate the squared distance from the sphere center to the projection.
	// If it's greater than the radius then they don't intersect.
	m2 = (l2 - (d * d));
	if (m2 > r2)
		return(false);


				/******************************/
				/* NOW CHECK THE 2ND ENDPOINT */
				/******************************/
				//
				// We now know that the ray from p1->p2 does intersect
				// the sphere, so if p2 is also good then we have a line seg hit
				//

	{
		float	l2b, db;
		
		rayDir.x = -rayDir.x;													// negate the ray direction
		rayDir.y = -rayDir.y;
		rayDir.z = -rayDir.z;

		OGLPoint3D_Subtract(sphereCenter, p2, &sphereToEndpoint);
		l2b = OGLVector3D_Dot_NoPin(&sphereToEndpoint, &sphereToEndpoint);		// the dot of itself gives us the length squared
		db  = OGLVector3D_Dot_NoPin(&sphereToEndpoint, &rayDir);
		
		if (db < 0.0f && l2b > r2)												// If the sphere is behind the endpoint, they don't intersect
			return(false);
	}


				/*****************/
				/* WE HAVE A HIT */
				/*****************/

	if (intersectPt != nil)
	{
		float	t;
		float 	q = (float) sqrt(r2 - m2);							// Calculate the distance along the p1 ray to the intersection point
		if (l2 > r2)
			t = d - q;
		else
			t = d + q;

		// Calculate the intersection point
		OGLVector3D_Scale(&rayDir, t, &intersectVector);
		OGLPoint3D_Vector3D_Add(p1, &intersectVector, intersectPt);
	}
	
	return(true);
}



/**************** PICK AND GET HIT INFO ************************/
//
//
// INPUT: 	point = grafPort coords of click
//			drawFun = callback to object drawing function
//
// OUTPUT:  ObjNode of object picked or nil
//			worldHitCoord = world-space coords of the pick intersection
//

ObjNode *OGL_PickAndGetHitInfo(OGLSetupOutputType *setupInfo, OGLPoint2D *point, void *drawFunc, OGLPoint3D *worldHitCoord)
{
u_long	pickID;
ObjNode	*pickedObj;
OGLRay	ray;
				/* USE OPENGL TO DETERMINE WHAT WE HIT */
				
	pickID = OGL_PickAndGetPickID(setupInfo, point, drawFunc);
	if (pickID == 0)
		return(nil);


			/* GET THE PICKING RAY IN WORLD-SPACE */
			
	OGL_GetWorldRayAtScreenPoint(point, &ray, setupInfo);

	
			/* NOW PARSE THE OBJNODE AND DO RAY-TRIANGLE TESTS TO SEE WHERE WE HIT */

	pickedObj = (ObjNode *)pickID;										// this pick ID is actually the ObjNode * of the object that was picked

	switch(pickedObj->Genre)
	{		
		case	SKELETON_GENRE:		
				if (OGL_PickAndGetHitInfo_Skeleton(&ray, pickedObj, worldHitCoord) == false)
				{
					pickedObj = nil;									// hmm... should probably never do this since OpenGL says there is a hit, but our triangle test says otherwise
				}
				break;
	
		case	DISPLAY_GROUP_GENRE:		
				OGL_RayGetHitInfo_DisplayGroup(&ray, pickedObj, worldHitCoord);
				break;
				
		case	CUSTOM_GENRE:
				if (pickedObj->CustomDrawFunction == DrawTerrain)		// is this the terrain?
				{				
					OGL_PickAndGetInfo_Terrain(setupInfo, point, worldHitCoord);
				}
				break;
				
		default:
				DoFatalAlert("\pOGL_PickAndGetHitInfo: unsupported genre");
	}

	return(pickedObj);
}

/******************** OGL: PICK AND GET HIT INFO: SKELETON *********************/
//
// Called from above when we know we've picked a Skeleton objNode.
// Now we just need to parse thru all of the Skeleton's triangles and see if our pick ray hits any.
// Then we keep track of the closest hit coord and that's what we'll return.
//

static Boolean OGL_PickAndGetHitInfo_Skeleton(OGLRay *ray, ObjNode *theNode, OGLPoint3D *worldHitCoord)
{
short		i,numTriMeshes;
short			skelType;
OGLPoint3D	where;
float		thisDist, bestDist = 100000000;
Boolean		gotHit = false;


				/* GET SKELETON DATA */
				
	UpdateSkinnedGeometry(theNode);													// update skeleton geometry to be sure that this ObjNode's data is what's in the skeleton buffers!
	numTriMeshes = theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
	skelType = theNode->Type;	

	
			/***********************************/
			/* CHECK EACH MESH IN THE SKELETON */
			/***********************************/
			
	for (i = 0; i < numTriMeshes; i++)
	{
		if (OGL_DoesRayIntersectMesh(ray, &gLocalTriMeshesOfSkelType[skelType][i], &where, &thisDist))
		{
				/* IS THIS INTERSECTION PT THE BEST ONE? */		
				
			if (thisDist < bestDist)
			{
				bestDist = thisDist;
				*worldHitCoord = where;							// pass back the intersection point since it's the best one we've found so far.		
			}
			
			gotHit = true;
		}
	}
	
	ray->distance = bestDist;							// pass back the best dist too
	return(gotHit);
}


/******************** OGL: LINE SEGMENT GET HIT INFO: SKELETON *********************/
//
// Called from above when we know we've picked a Skeleton objNode.
// Now we just need to parse thru all of the Skeleton's triangles and see if our line seg hits any.
// Then we keep track of the closest hit coord and that's what we'll return.
//

static Boolean OGL_LineSegGetHitInfo_Skeleton(OGLPoint3D *p1, OGLPoint3D *p2, ObjNode *theNode, OGLPoint3D *worldHitCoord, float *hitDist)
{
short		i,numTriMeshes;
short			skelType;
OGLPoint3D	where;
float		thisDist, bestDist = 100000000;
Boolean		gotHit = false;


		/* CREATE A GLOBAL RAY */
		
	gPickRay.origin = *p1;	
	gPickRay.direction.x = p2->x - p1->x;
	gPickRay.direction.y = p2->y - p1->y;
	gPickRay.direction.z = p2->z - p1->z;
	OGLVector3D_Normalize(&gPickRay.direction, &gPickRay.direction);
	
	gP1 = *p1;
	gP2 = *p2;	
	gClosestHitDist = 10000000;
	gGotAHit 		= false;


				/* GET SKELETON DATA */
				
	UpdateSkinnedGeometry(theNode);													// update skeleton geometry to be sure that this ObjNode's data is what's in the skeleton buffers!
	numTriMeshes = theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
	skelType = theNode->Type;	

	
			/***********************************/
			/* CHECK EACH MESH IN THE SKELETON */
			/***********************************/
			
	for (i = 0; i < numTriMeshes; i++)
	{
		if (OGL_DoesLineSegIntersectMesh(&gLocalTriMeshesOfSkelType[skelType][i], &where, &thisDist))
		{
				/* IS THIS INTERSECTION PT THE BEST ONE? */		
				
			if (thisDist < bestDist)
			{
				bestDist = thisDist;
				*worldHitCoord = where;							// pass back the intersection point since it's the best one we've found so far.		
			}
			
			gotHit = true;
		}
	}
	
	*hitDist = bestDist;
	
	return(gotHit);
}



/**************** OGL: PICK AND GET INFO : TERRAIN **************************/

static Boolean OGL_PickAndGetInfo_Terrain(OGLSetupOutputType *setupInfo, OGLPoint2D *point, OGLPoint3D *worldHitCoord)
{
int				r,c;
int				i;
OGLRay			ray;
float		thisDist, bestDist = 100000000;
Boolean		gotHit = false;
OGLPoint3D		thisPt;

			/* GET THE PICKING RAY IN WORLD-SPACE */
			
	OGL_GetWorldRayAtScreenPoint(point, &ray, setupInfo);

			
	/******************************************************************/
	/* SCAN THE SUPERTILE GRID AND LOOK FOR USED & VISIBLE SUPERTILES */
	/******************************************************************/
			
	for (r = 0; r < gNumSuperTilesDeep; r++)
	{
		for (c = 0; c < gNumSuperTilesWide; c++)
		{
			if (gSuperTileStatusGrid[r][c].statusFlags & SUPERTILE_IS_USED_THIS_FRAME)			// see if used
			{
				i = gSuperTileStatusGrid[r][c].supertileIndex;					// extract supertile #
						
				if (gSuperTileMemoryList[i].culledLastDraw)						// was it culled the last time it was drawn?
					continue;
						
				if (OGL_DoesRayIntersectMesh(&ray, gSuperTileMemoryList[i].meshData, &thisPt, &thisDist))
				{
					if (thisDist < bestDist)									// is this the closest hit so far?
					{
						bestDist = thisDist;									// remember some info about this hit
						*worldHitCoord = thisPt;
						gotHit = true;
					}
				}														
			}
		}	
	}	
	
	return(gotHit);
}


/******************* OGL:  DOES RAY INTERSECT MESH ***************************/
//
// ASSUMES THE MESH IS IN WORLD COORDINATES ALREADY!!!
//
// Determines if the input ray intersects any of the triangles in the input mesh,
// and returns the closest intersection coordinate if so.  We also return the
// distance to the intersection point.
//

static Boolean	OGL_DoesRayIntersectMesh(OGLRay *ray, MOVertexArrayData *mesh, OGLPoint3D *intersectionPt, float *distToIntersection)
{
int			numTriangles = mesh->numTriangles;
int			t,i;
OGLPoint3D	triPts[3];
OGLPoint3D	thisCoord;
Boolean		gotHit = false;
float		bestDist = 10000000;

			/***************************/
			/* SCAN THRU ALL TRIANGLES */
			/***************************/
			
	for (t = 0; t < numTriangles; t++)
	{
				/* GET TRIANGLE POINTS */
				
		i = mesh->triangles[t].vertexIndices[0];
		triPts[0] = mesh->points[i];
		
		i = mesh->triangles[t].vertexIndices[1];
		triPts[1] = mesh->points[i];
		
		i = mesh->triangles[t].vertexIndices[2];
		triPts[2] = mesh->points[i];

				/* DOES OUR RAY HIT IT? */
				
		if (OGL_RayIntersectsTriangle(&triPts[0], ray, &thisCoord))
		{
			if (ray->distance < bestDist)							// is this hit closer than any previous hit?
			{
				bestDist = ray->distance;
				*intersectionPt = thisCoord;						// pass back this intersection point since it's the best so far
			}
			gotHit = true;
		}
	}
		
	*distToIntersection = bestDist;									// pass back the best distance that we found (if any)
	return(gotHit);
}


#pragma mark -


/*************** OGL: PICK AND GET PICK ID ********************/
//
// This function simply returns the user name data that was picked out of the entire scene. 
// No useful information such as pick hit x,y,z info is given.
//
// INPUT: 	point = grafPort coords of click
//			drawFun = callback to object drawing function
//

u_long OGL_PickAndGetPickID(OGLSetupOutputType *setupInfo, OGLPoint2D *point, void *drawFunc)
{
int		i,j;

				/* DO THE PICKING */
							
	OGL_PickScene(setupInfo, drawFunc, point->x, point->y, 4, 4);
	
			
			/* SEE WHETHER ANY HITS OCCURRED */	


	if (gNumPickHits > 0)
	{
		int		q = 0;
		float	bestZ = 100000;
		u_long	bestPick;
		
		
		for (i = 0; i < gNumPickHits; i++)								// scan all hits for closest z
		{
			int		numNamesInPick;
			float	z1,z2;
						
			numNamesInPick 	= gPickBuffer[q];							// get # names returned
			
			for (j = 0; j < numNamesInPick; j++)
			{
				z1 				= (float)gPickBuffer[q+1] / 0x7fffffff;		// get z1
				z2 				= (float)gPickBuffer[q+2] / 0x7fffffff;		// get z2
				if (z1 < bestZ)
				{
					bestZ = z1;
					bestPick = gPickBuffer[q+3];						// get the ObjNode from this value					
				}
				q += 3;
			}
			q += 1;
		}	

		return(bestPick);
	}


			/* DIDNT HIT ANYTHING */

	return(nil);
}


#pragma mark -

/*************** OGL: GET WORLD RAY AT SCREEN POINT *********************/
//
// Used for picking, this function returns the world-space ray at the screenCoord.
// screenCoord is in grafPort coordinates.
//

void OGL_GetWorldRayAtScreenPoint(OGLPoint2D *screenCoord, OGLRay *ray, const OGLSetupOutputType *setupInfo)
{
GLdouble	model_view[16];
GLdouble	projection[16];
GLint		viewport[4];
float		realy;
double		dx,dy,dz;

			/* GET STATE DATA */
			
	glGetDoublev(GL_MODELVIEW_MATRIX, model_view);	
	glGetDoublev(GL_PROJECTION_MATRIX, projection);	
	glGetIntegerv(GL_VIEWPORT, viewport);	


			/* FLIP Y */
			
	realy = (viewport[3] - (GLint)screenCoord->y) - 1;			// flip Y ([3] is the height value)


		/* GET 3D COORDINATES @ BACK PLANE */
		
	gluUnProject(screenCoord->x, realy, 1.0, model_view, projection, viewport, &dx, &dy ,&dz); 



			/* CONVERT TO RAY */
	
	ray->origin = setupInfo->cameraPlacement.cameraLocation;		// ray origin @ camera location
	ray->direction.x = dx - ray->origin.x;							// calc vector of ray
	ray->direction.y = dy - ray->origin.y;							
	ray->direction.z = dz - ray->origin.z;							
	OGLVector3D_Normalize(&ray->direction, &ray->direction);		// normalize the ray vector

}


/******************** OGL: RAY INTERSECTS TRIANGLE ************************/

Boolean	OGL_RayIntersectsTriangle(OGLPoint3D *trianglePoints, OGLRay *ray, OGLPoint3D *intersectPt)
{
OGLPlaneEquation	planeEquation;
float				distance;
	 
	 			/* SEE IF RAY INTERSECTS THE TRIANGLE'S PLANE */
	 			
	if (OGL_DoesRayIntersectTrianglePlane(trianglePoints, ray, &planeEquation))
	{
					/* CALC INTERSECTION POINT ON PLANE */
					
		distance = ray->distance;
		intersectPt->x = ray->origin.x + ray->direction.x * distance;
		intersectPt->y = ray->origin.y + ray->direction.y * distance;
		intersectPt->z = ray->origin.z + ray->direction.z * distance;


				/* IS THE INTERSECTION PT INSIDE THE TRIANGLE? */
					
		if (OGLPoint3D_InsideTriangle3D(intersectPt, trianglePoints, &planeEquation.normal))
			return (true);
	}

	return (false);

}


//******************** OGL: DOES RAY INTERSECT TRIANGLE PLANE ***********************/
//
// Returns true if the input ray intersects the plane of the triangle.
//

Boolean OGL_DoesRayIntersectTrianglePlane(const OGLPoint3D	triWorldPoints[], OGLRay *ray, OGLPlaneEquation	*planeEquation)
{
float	nDotD, nDotO,t;
float				nx, ny, nz;

 	OGL_ComputeTrianglePlaneEquation(triWorldPoints, planeEquation);

	nx = planeEquation->normal.x;
	ny = planeEquation->normal.y;
	nz = planeEquation->normal.z;

	nDotD = EmVector3D_Member_Dot(nx, ny, nz, ray->direction);

	if (OGLIsZero(nDotD))								// is ray parallel to plane?
		return (false);

	nDotO = EmVector3D_Member_Dot(nx, ny, nz, ray->origin);

	t = -(planeEquation->constant + nDotO) / nDotD;
	if (t < 0.0f)
		return(false);		

	ray->distance = t;

	return (true);
}

			


/**************** OGL POINT3D: INSIDE TRIANGLE 3D *************************/
//
// Is the point which lies on the triangle plane insdie the triangle?

Boolean OGLPoint3D_InsideTriangle3D(const OGLPoint3D *point3D, const OGLPoint3D *trianglePoints, const OGLVector3D	*triangleNormal)
{
OGLPoint2D			point2D, verts[3];
Boolean				intersects;
float				alpha, beta;
float				u0, u1, u2, v0, v1, v2;

	OGLTriangle_3D2DComponentProjectionPoints(triangleNormal, point3D, trianglePoints, &point2D, verts);

	intersects = false;

	u0 = point2D.x  - verts[0].x;
	v0 = point2D.y  - verts[0].y;
	u1 = verts[1].x - verts[0].x;
	v1 = verts[1].y - verts[0].y;
	u2 = verts[2].x - verts[0].x;
	v2 = verts[2].y - verts[0].y;

	if (OGLIsZero(u1))
	{
		beta = u0 / u2;
		
		if ((-EPS <= beta) && (beta <= (1.0f + EPS)))								/* Test if 0.0 <= beta <= 1.0 */
		{
			alpha = (v0 - beta * v2) / v1;
			intersects = ((alpha >= -EPS) && ((alpha + beta) <= (1.0f + EPS))) ? true : false;
		}
	}
	else
	{
		beta = (v0 * u1 - u0 * v1) / (v2 * u1 - u2 * v1);
		
		if ( (-EPS <= beta) && (beta <= (1.0f + EPS)) )								/* Test if  0.0 <= beta <= 1.0 */
		{
			alpha = (u0 - beta * u2) / u1;

			intersects = ((alpha >= -EPS) && ((alpha + beta) <= (1.0f + EPS))) ? true : false;
		}
	}

	return intersects;
}


/*********** OGL TRIANGLE 3D3D COMPONENT PROJECTION POINTS ****************/

static void OGLTriangle_3D2DComponentProjectionPoints(const OGLVector3D *triangleNormal,	const OGLPoint3D *point3D, const OGLPoint3D	*triPoints,
													 OGLPoint2D *point2D, OGLPoint2D *verts2D)
{
float 	xComp, yComp, zComp;

	xComp = fabs(triangleNormal->x);
	yComp = fabs(triangleNormal->y);
	zComp = fabs(triangleNormal->z);

	if (xComp > yComp)
	{
		if (xComp > zComp)
		{
			/* Maximal X */
			point2D->x = point3D->y;
			point2D->y = point3D->z;

			verts2D[0].x = triPoints[0].y;
			verts2D[0].y = triPoints[0].z;

			verts2D[1].x = triPoints[1].y;
			verts2D[1].y = triPoints[1].z;

			verts2D[2].x = triPoints[2].y;
			verts2D[2].y = triPoints[2].z;
		}
		else
		{
			/* Maximal Z */
			point2D->x = point3D->x;
			point2D->y = point3D->y;

			verts2D[0].x = triPoints[0].x;
			verts2D[0].y = triPoints[0].y;

			verts2D[1].x = triPoints[1].x;
			verts2D[1].y = triPoints[1].y;

			verts2D[2].x = triPoints[2].x;
			verts2D[2].y = triPoints[2].y;
		}
	}
	else
	{
		if (yComp > zComp)
		{
			/* Maximal Y */
			point2D->x = point3D->z;
			point2D->y = point3D->x;

			verts2D[0].x = triPoints[0].z;
			verts2D[0].y = triPoints[0].x;

			verts2D[1].x = triPoints[1].z;
			verts2D[1].y = triPoints[1].x;

			verts2D[2].x = triPoints[2].z;
			verts2D[2].y = triPoints[2].x;
		}
		else
		{
			/* Maximal Z */
			point2D->x = point3D->x;
			point2D->y = point3D->y;

			verts2D[0].x = triPoints[0].x;
			verts2D[0].y = triPoints[0].y;

			verts2D[1].x = triPoints[1].x;
			verts2D[1].y = triPoints[1].y;

			verts2D[2].x = triPoints[2].x;
			verts2D[2].y = triPoints[2].y;
		}
	}
}


#pragma mark -

/******************** OGL: PICK AND GET HIT INFO: DISPLAY GROUP *********************/
//
// Called from above when we know we've picked a Display Group genre objNode.
// Now we just need to traverse the Base Group, transform the data to world-space, and and see if our pick ray hits anything.
// Then we keep track of the closest hit coord and that's what we'll return.
//

static Boolean OGL_RayGetHitInfo_DisplayGroup(OGLRay *ray, ObjNode *theNode, OGLPoint3D *worldHitCoord)
{
int	i;
float	distToHit;

	gPickRay 		= *ray;	
	gClosestHitDist = 10000000;
	gGotAHit 		= false;


		/* MAKE SURE WE HAVE WORLD-SPACE DATA FOR THIS OBJNODE */
			
	if (!theNode->HasWorldPoints)
		CalcDisplayGroupWorldPoints(theNode);



			/* SCAN THRU OBJNODE'S WORLD-SPACE DATA FOR A HIT */
			
	for (i = 0; i < MAX_OBJECTS_IN_GROUP; i++)
	{
		if (theNode->WorldMeshes[i].points)														// does this mesh exist?
		{
			if (OGL_DoesRayIntersectMesh(ray, &theNode->WorldMeshes[i], worldHitCoord, &distToHit))// does the ray hit this mesh?
			{
				if (distToHit < gClosestHitDist)												// is this the closest hit so far?
				{
					gClosestHitDist = distToHit;												// remember some info about this hit
					gClosestHitCoord = *worldHitCoord;
					gGotAHit = true;
				}
			}
		}
	}


	
	if (gGotAHit)
	{
		*worldHitCoord = gClosestHitCoord;
		ray->distance = gClosestHitDist;					// pass back the best dist in the ray
		return(true);	
	}
	
	
	return(false);
	
}

/******************** MO: PICK OBJECT ***********************/

static void MO_RayTestObject(const MetaObjectPtr object)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("\pMO_RayTestObject: cookie is invalid!");


			/* HANDLE TYPE */

	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				vObj = object;
				MO_RayTestGeometry_VertexArray(&vObj->objectData);				
				break;
	
		case	MO_TYPE_GROUP:
				MO_RayTestGroup(object);	
				break;
				
		case	MO_TYPE_MATRIX:
				MO_RayTestMatrix(object);
				break;				
	}
}



/******************** MO_PICK GROUP *************************/

void MO_RayTestGroup(const MOGroupObject *object)
{
int	numChildren,i;


		/* PUSH MATRIES WITH OPENGL */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();


				/***************/
				/* PARSE GROUP */
				/***************/

	numChildren = object->objectData.numObjectsInGroup;			// get # objects in group
	
	for (i = 0; i < numChildren; i++)
	{
		MO_RayTestObject(object->objectData.groupContents[i]);
	}


		/* RETREIVE OPENGL MATRICES */
		
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


/************************ OGL PICK MATRIX **************************/

void MO_RayTestMatrix(const MOMatrixObject *matObj)
{
const OGLMatrix4x4		*m;

	m = &matObj->matrix;							// point to matrix

				/* MULTIPLY CURRENT MATRIX BY THIS */
	
	glMultMatrixf((GLfloat *)m);
}


/******************** MO: PICK GEOMETRY - VERTEX ARRAY *************************/
//
// We need to transform this Geometry to world space and then do a ray pick on it.
//

static void MO_RayTestGeometry_VertexArray(MOVertexArrayData *data)
{
int				numPoints;
OGLPoint3D		worldBuffer[3000];
OGLMatrix4x4	localToWorld;
OGLPoint3D		*localPts;
float			distToHit;
OGLPoint3D		hitPt;

			/* GET # POINTS TO XFORM & VERIFY THAT IT'LL FIT INTO OUR WORLD-SPACE BUFFER */
			
	numPoints = data->numPoints;
	if (numPoints > 3000)
		DoFatalAlert("\pMO_RayTestGeometry_VertexArray: numPoints > buffer size");


			/************************************************/	
			/* TRANSFORM ALL OF THESE POINTS TO WORLD-SPACE */
			/************************************************/	
			
				/* GET THE TRANSFORM MATRIX WE'VE BUILT */
					
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&localToWorld);	
			
				/* TRANSFORM EACH POINT INTO THE BUFFER */
	
	OGLPoint3D_TransformArray(&data->points[0], &localToWorld,	&worldBuffer[0],  numPoints);



				/******************/
				/* DO THE PICKING */
				/******************/
				
	localPts = data->points;								// get the old pointer
	data->points = &worldBuffer[0];							// point to buffer that contains our new world-space data

	if (OGL_DoesRayIntersectMesh(&gPickRay, data, &hitPt, &distToHit))
	{
		if (distToHit < gClosestHitDist)					// is this the closest hit so far?
		{
			gClosestHitDist = distToHit;					// remember some info about this hit
			gClosestHitCoord = hitPt;
			gGotAHit = true;
		}
	}
	
	data->points = localPts;								// restore the old pointer
}


#pragma mark -


/**************** OGL: DO LINE SEGMENT COLLISION ************************/
//
// Checks to see if the input line segment hits any eligible and visible objNodes in the scene.
//
// INPUT: 	p1/p2 = world-space line seg to collide with
//			cTypes = which objects do we want to collide against?
//
// OUTPUT:  ObjNode of object picked or nil
//			worldHitCoord = world-space coords of the pick intersection
//			ray->distance = distance from ray origin to the intersection point
//

ObjNode *OGL_DoLineSegmentCollision(OGLPoint3D *p1, OGLPoint3D *p2, OGLPoint3D *worldHitCoord, OGLVector3D *worldHitFaceNormal, u_long cTypes)
{
ObjNode		*thisNodePtr;
ObjNode		*bestObj = nil;
float		bestDist = 100000000;
OGLPoint3D	hitPt;



	thisNodePtr = gFirstNodePtr;
	
	do
	{
				/* VERIFY NODE */
	
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)								// stop here
			break;
			
		if (thisNodePtr->CType == INVALID_NODE_FLAG)						// make sure the node is even valid
			goto next;

		if (thisNodePtr->StatusBits & STATUS_BIT_HIDDEN)					// skip it if hidden
			goto next;
						
		if (thisNodePtr->CType & cTypes)									// only if pickable
		{		
		
					/* IF THE LINE SEG HITS THE OBJECT'S BOUNDING SPHERE THEN SEE IF WE HIT THE GEOMETRY */
				
			if (OGL_DoesLineSegmentIntersectSphere(p1, p2, nil, &thisNodePtr->Coord, thisNodePtr->BoundingSphereRadius, nil))					
			{
				float	hitDist;
							
						/* NOW PARSE THE OBJNODE AND DO TRIANGLE TESTS TO SEE WHERE WE HIT */

				switch(thisNodePtr->Genre)
				{
					case	SKELETON_GENRE:		
							if (OGL_LineSegGetHitInfo_Skeleton(p1, p2, thisNodePtr, &hitPt, &hitDist))		// does ray intersect skeleton?
							{
								if (hitDist < bestDist)								// is this the best hit so far?
								{
									bestDist = hitDist;
									bestObj = thisNodePtr;
									*worldHitCoord = hitPt;
								}
							}
							break;
				
					case	DISPLAY_GROUP_GENRE:		
							if (OGL_LineSegGetHitInfo_DisplayGroup(p1, p2, thisNodePtr, &hitPt, &hitDist))	// does line seg hit display group geometry?
							{
								if (hitDist < bestDist)						// is this the best hit so far?
								{
									bestDist = hitDist;
									bestObj = thisNodePtr;
									*worldHitCoord = hitPt;
								}
							}
							break;
							
					case	CUSTOM_GENRE:									// ignore this or do custom handling
							break;
							
					default:
							DoFatalAlert("\pOGL_DoLineSegmentCollision: unsupported genre");
				}
			}		
		}

next:
		thisNodePtr = thisNodePtr->NextNode;								// next node
	}
	while (thisNodePtr != nil);

	if (bestObj)
		*worldHitFaceNormal = gBestTriangleNormal;

	return(bestObj);
}


/******************** OGL: PICK AND GET HIT INFO: DISPLAY GROUP *********************/
//
// Called from above when we know we've picked a Display Group genre objNode.
// Now we just need to traverse the Base Group, transform the data to world-space, and and see if our pick ray hits anything.
// Then we keep track of the closest hit coord and that's what we'll return.
//

static Boolean OGL_LineSegGetHitInfo_DisplayGroup(OGLPoint3D *p1, OGLPoint3D *p2, ObjNode *theNode, OGLPoint3D *worldHitCoord, float *distToHit)
{
int		i;

		/* CREATE A GLOBAL RAY */
		
	gPickRay.origin = *p1;	
	gPickRay.direction.x = p2->x - p1->x;
	gPickRay.direction.y = p2->y - p1->y;
	gPickRay.direction.z = p2->z - p1->z;
	OGLVector3D_Normalize(&gPickRay.direction, &gPickRay.direction);
	
	gP1 = *p1;
	gP2 = *p2;	
	gClosestHitDist = 10000000;
	gGotAHit 		= false;
	
	
		/* MAKE SURE WE HAVE WORLD-SPACE DATA FOR THIS OBJNODE */
			
	if (!theNode->HasWorldPoints)
		CalcDisplayGroupWorldPoints(theNode);

	
#if 1

			/* SCAN THRU OBJNODE'S WORLD-SPACE DATA FOR A HIT */
			
	for (i = 0; i < MAX_OBJECTS_IN_GROUP; i++)
	{
		if (theNode->WorldMeshes[i].points)														// does this mesh exist?
		{
			if (OGL_DoesLineSegIntersectMesh2(theNode->WorldPlaneEQs[i], &theNode->WorldMeshes[i], worldHitCoord, distToHit))// does the line segment hit this mesh?
			{
				if (*distToHit < gClosestHitDist)												// is this the closest hit so far?
				{
					gClosestHitDist = *distToHit;												// remember some info about this hit
					gClosestHitCoord = *worldHitCoord;
					gGotAHit = true;
				}
			}
		}
	}



#else	

			/* RECURSIVELY PICK THE DATA IN THE BASE GROUP */
				
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	MO_LineSegTestObject(theNode->BaseGroup);
	
	glPopMatrix();
#endif
	

	if (gGotAHit)
	{
		*worldHitCoord = gClosestHitCoord;
		*distToHit = gClosestHitDist;
		return(true);	
	}

	return(false);	
}


/******************** MO: PICK OBJECT ***********************/

static void MO_LineSegTestObject(const MetaObjectPtr object)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("\pMO_LineSegTestObject: cookie is invalid!");


			/* HANDLE TYPE */

	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				vObj = object;
				MO_LineSegTestGeometry_VertexArray(&vObj->objectData);				
				break;
	
		case	MO_TYPE_GROUP:
				MO_LineSegTestGroup(object);	
				break;
				
		case	MO_TYPE_MATRIX:
				MO_LineSegTestMatrix(object);
				break;				
	}
}



/******************** MO_PICK GROUP *************************/

static void MO_LineSegTestGroup(const MOGroupObject *object)
{
int	numChildren,i;


		/* PUSH MATRIES WITH OPENGL */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();


				/***************/
				/* PARSE GROUP */
				/***************/

	numChildren = object->objectData.numObjectsInGroup;			// get # objects in group
	
	for (i = 0; i < numChildren; i++)
	{
		MO_LineSegTestObject(object->objectData.groupContents[i]);
	}


		/* RETREIVE OPENGL MATRICES */
		
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


/************************ OGL PICK MATRIX **************************/

static void MO_LineSegTestMatrix(const MOMatrixObject *matObj)
{
const OGLMatrix4x4		*m;

	m = &matObj->matrix;							// point to matrix

				/* MULTIPLY CURRENT MATRIX BY THIS */
	
	glMultMatrixf((GLfloat *)m);
}


/******************** MO: PICK GEOMETRY - VERTEX ARRAY *************************/
//
// We need to transform this Geometry to world space and then do a ray pick on it.
//

static void MO_LineSegTestGeometry_VertexArray(MOVertexArrayData *data)
{
int				numPoints;
OGLPoint3D		worldBuffer[3000];
OGLMatrix4x4	localToWorld;
OGLPoint3D		*localPts;
float			distToHit;
OGLPoint3D		hitPt;

			/* GET # POINTS TO XFORM & VERIFY THAT IT'LL FIT INTO OUR WORLD-SPACE BUFFER */
			
	numPoints = data->numPoints;
	if (numPoints > 3000)
		DoFatalAlert("\pMO_LineSegTestGeometry_VertexArray: numPoints > buffer size");


			/************************************************/	
			/* TRANSFORM ALL OF THESE POINTS TO WORLD-SPACE */
			/************************************************/	
			
				/* GET THE TRANSFORM MATRIX WE'VE BUILT */
					
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&localToWorld);	
			
				/* TRANSFORM EACH POINT INTO THE BUFFER */
	
	OGLPoint3D_TransformArray(&data->points[0], &localToWorld,	&worldBuffer[0],  numPoints);



				/******************/
				/* DO THE PICKING */
				/******************/
				
	localPts = data->points;								// get the old pointer
	data->points = &worldBuffer[0];							// point to buffer that contains our new world-space data

	if (OGL_DoesLineSegIntersectMesh(data, &hitPt, &distToHit))
	{
		if (distToHit < gClosestHitDist)					// is this the closest hit so far?
		{
			gClosestHitDist = distToHit;					// remember some info about this hit
			gClosestHitCoord = hitPt;
			gGotAHit = true;
		}
	}
	
	data->points = localPts;								// restore the old pointer
}


/******************* OGL:  DOES LINE SEGMENT INTERSECT MESH ***************************/
//
// ASSUMES THE MESH IS IN WORLD COORDINATES ALREADY!!!
//
// Determines if the global seg intersects any of the triangles in the input mesh,
// and returns the closest intersection coordinate if so.  We also return the
// distance to the intersection point.
//

static Boolean	OGL_DoesLineSegIntersectMesh(MOVertexArrayData *mesh, OGLPoint3D *intersectionPt, float *distToIntersection)
{
int			numTriangles = mesh->numTriangles;
int			t,i;
OGLPoint3D	triPts[3];
OGLPoint3D	thisCoord;
Boolean		gotHit = false;
float		bestDist = 10000000;
float		distFromP1ToPlane;

			/***************************/
			/* SCAN THRU ALL TRIANGLES */
			/***************************/
			
	for (t = 0; t < numTriangles; t++)
	{
				/* GET TRIANGLE POINTS */
				
		i = mesh->triangles[t].vertexIndices[0];
		triPts[0] = mesh->points[i];
		
		i = mesh->triangles[t].vertexIndices[1];
		triPts[1] = mesh->points[i];
		
		i = mesh->triangles[t].vertexIndices[2];
		triPts[2] = mesh->points[i];

				/* DOES OUR RAY HIT IT? */
				
		if (OGL_LineSegIntersectsTriangle(&triPts[0], &thisCoord, &distFromP1ToPlane))
		{
			if (distFromP1ToPlane < bestDist)							// is this hit closer than any previous hit?
			{
				bestDist = distFromP1ToPlane;
				gBestTriangleNormal = gRecentPlaneEq.normal;				// keep the best face normal that we've hit
				*intersectionPt = thisCoord;							// pass back this intersection point since it's the best so far
			}
			gotHit = true;
		}
	}
		
	*distToIntersection = bestDist;									// pass back the best distance that we found (if any)
	return(gotHit);
}


/******************* OGL:  DOES LINE SEGMENT INTERSECT MESH 2***************************/
//
// Same as above, but also takes an PlaneEQ array
//


static Boolean	OGL_DoesLineSegIntersectMesh2(OGLPlaneEquation *planeEQ, MOVertexArrayData *mesh, OGLPoint3D *intersectionPt, float *distToIntersection)
{
int			numTriangles = mesh->numTriangles;
int			t,i;
OGLPoint3D	triPts[3];
OGLPoint3D	thisCoord;
Boolean		gotHit = false;
float		bestDist = 10000000;
float		distFromP1ToPlane;

			/***************************/
			/* SCAN THRU ALL TRIANGLES */
			/***************************/
			
	for (t = 0; t < numTriangles; t++)
	{
				/* GET TRIANGLE POINTS */
				
		i = mesh->triangles[t].vertexIndices[0];
		triPts[0] = mesh->points[i];
		
		i = mesh->triangles[t].vertexIndices[1];
		triPts[1] = mesh->points[i];
		
		i = mesh->triangles[t].vertexIndices[2];
		triPts[2] = mesh->points[i];

				/* DOES OUR RAY HIT IT? */
				
		if (OGL_LineSegIntersectsTriangle2(&planeEQ[t], &triPts[0], &thisCoord, &distFromP1ToPlane))
		{
			if (distFromP1ToPlane < bestDist)							// is this hit closer than any previous hit?
			{
				bestDist = distFromP1ToPlane;
				gBestTriangleNormal = planeEQ[t].normal;				// keep the best face normal that we've hit
				*intersectionPt = thisCoord;							// pass back this intersection point since it's the best so far
			}
			gotHit = true;
		}
	}
		
	*distToIntersection = bestDist;									// pass back the best distance that we found (if any)
	return(gotHit);
}


/******************** OGL: LINE SEGMENT INTERSECTS TRIANGLE ************************/

static Boolean	OGL_LineSegIntersectsTriangle(OGLPoint3D *trianglePoints, OGLPoint3D *intersectPt, float *distFromP1ToPlane)
{
	 
	 			/* SEE IF RAY INTERSECTS THE TRIANGLE'S PLANE */
	 			
	if (OGL_DoesLineSegIntersectTrianglePlane(trianglePoints, distFromP1ToPlane))
	{
					/* CALC INTERSECTION POINT ON PLANE */
					
		intersectPt->x = gP1.x + gPickRay.direction.x * *distFromP1ToPlane;
		intersectPt->y = gP1.y + gPickRay.direction.y * *distFromP1ToPlane;
		intersectPt->z = gP1.z + gPickRay.direction.z * *distFromP1ToPlane;


				/* IS THE INTERSECTION PT INSIDE THE TRIANGLE? */
					
		if (OGLPoint3D_InsideTriangle3D(intersectPt, trianglePoints, &gRecentPlaneEq.normal))
			return (true);
	}

	return (false);

}


/******************** OGL: LINE SEGMENT INTERSECTS TRIANGLE 2************************/
//
// This version is passed the plane EQ
//

static Boolean	OGL_LineSegIntersectsTriangle2(OGLPlaneEquation *planeEQ, OGLPoint3D *trianglePoints, OGLPoint3D *intersectPt, float *distFromP1ToPlane)
{
	 
	 			/* SEE IF RAY INTERSECTS THE TRIANGLE'S PLANE */
	 			
	if (OGL_DoesLineSegIntersectTrianglePlane2(planeEQ, distFromP1ToPlane))
	{
					/* CALC INTERSECTION POINT ON PLANE */
					
		intersectPt->x = gP1.x + gPickRay.direction.x * *distFromP1ToPlane;
		intersectPt->y = gP1.y + gPickRay.direction.y * *distFromP1ToPlane;
		intersectPt->z = gP1.z + gPickRay.direction.z * *distFromP1ToPlane;


				/* IS THE INTERSECTION PT INSIDE THE TRIANGLE? */
					
		if (OGLPoint3D_InsideTriangle3D(intersectPt, trianglePoints, &gRecentPlaneEq.normal))
			return (true);
	}

	return (false);

}



//******************** OGL: DOES LINE SEGMENT INTERSECT TRIANGLE PLANE ***********************/
//
// Returns true if the global line data intersects the plane of the triangle.
//
// NOTE:  this only works for DIRECTIONAL line segments!!!  Line segments which go from P1 to P2.
//		Line segments which intersect from P2 to P1 will not return a valid hit.
//

static Boolean OGL_DoesLineSegIntersectTrianglePlane(OGLPoint3D	triWorldPoints[], float *distFromP1ToPlane)
{
float	nDotD, nDotO,t;
float	nx, ny, nz, oneOverDotD;

			/* GET TRIANGLE NORMAL */
			
 	OGL_ComputeTrianglePlaneEquation(triWorldPoints, &gRecentPlaneEq);
	nx = gRecentPlaneEq.normal.x;
	ny = gRecentPlaneEq.normal.y;
	nz = gRecentPlaneEq.normal.z;


			/* IS PARALLEL TO OR BEHIND PLANE? */
				
	nDotD = EmVector3D_Member_Dot(nx, ny, nz, gPickRay.direction);		// calc dot between normal and the line ray
	if (nDotD >= EPS)													// if ray is pointing away from plane then bail since we're not interested in rays hitting the triangle from behind
		return (false);

	oneOverDotD = -1.0f / nDotD;										// let's calculate the -1/d since we use it twice

			/* SEE IF RAY FROM P1 HITS IT */
			
	nDotO = EmVector3D_Member_Dot(nx, ny, nz, gP1);
	t = (gRecentPlaneEq.constant + nDotO) * oneOverDotD;
	if (t < 0.0f)
		return(false);		

	*distFromP1ToPlane = t;


		/* IF P2 ALSO HITS THEN BOTH PTS ARE ON SAME SIDE OF PLANE, THUS NO INTERSECT */
		//
		// We know that the vector from p1 intersects the plane, but if the same vector from p2
		// also hits the plane then both endpoints were in front of the plane.  The line segment
		// can only be intersecting the plane if each endpoint is on opposite sides.
		//
			
	nDotO = EmVector3D_Member_Dot(nx, ny, nz, gP2);
	t = (gRecentPlaneEq.constant + nDotO) * oneOverDotD;
	if (t >= 0.0f)
		return(false);


	return (true);
}

//******************** OGL: DOES LINE SEGMENT INTERSECT TRIANGLE PLANE 2 ***********************/
//
// Returns true if the global line data intersects the plane of the triangle.
//
// NOTE:  this only works for DIRECTIONAL line segments!!!  Line segments which go from P1 to P2.
//		Line segments which intersect from P2 to P1 will not return a valid hit.
//

static Boolean OGL_DoesLineSegIntersectTrianglePlane2(OGLPlaneEquation *planeEQ, float *distFromP1ToPlane)
{
float	nDotD, nDotO,t;
float	nx, ny, nz, oneOverDotD;

	gRecentPlaneEq = *planeEQ;										// keep global copy for later use
	
	nx = planeEQ->normal.x;
	ny = planeEQ->normal.y;
	nz = planeEQ->normal.z;


			/* IS PARALLEL TO OR BEHIND PLANE? */
				
	nDotD = EmVector3D_Member_Dot(nx, ny, nz, gPickRay.direction);		// calc dot between normal and the line ray
	if (nDotD >= EPS)													// if ray is pointing away from plane then bail since we're not interested in rays hitting the triangle from behind
		return (false);

	oneOverDotD = -1.0f / nDotD;										// let's calculate the -1/d since we use it twice

			/* SEE IF RAY FROM P1 HITS IT */
			
	nDotO = EmVector3D_Member_Dot(nx, ny, nz, gP1);
	t = (planeEQ->constant + nDotO) * oneOverDotD;
	if (t < 0.0f)
		return(false);		

	*distFromP1ToPlane = t;


		/* IF P2 ALSO HITS THEN BOTH PTS ARE ON SAME SIDE OF PLANE, THUS NO INTERSECT */
		//
		// We know that the vector from p1 intersects the plane, but if the same vector from p2
		// also hits the plane then both endpoints were in front of the plane.  The line segment
		// can only be intersecting the plane if each endpoint is on opposite sides.
		//
			
	nDotO = EmVector3D_Member_Dot(nx, ny, nz, gP2);
	t = (planeEQ->constant + nDotO) * oneOverDotD;
	if (t >= 0.0f)
		return(false);


	return (true);
}
























