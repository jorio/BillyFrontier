/**********************/
/*   	FENCES.C      */
/**********************/

#include "game.h"

/***************/
/* EXTERNALS   */
/***************/

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawFences(ObjNode *theNode);
static void SubmitFence(int f, const float camX, float camZ);
static void MakeFenceGeometry(void);
static void DrawFenceNormals(short f);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_FENCES			90
#define	MAX_NUBS_IN_FENCE	80


#define	FENCE_SINK_FACTOR	10.0f

enum
{
	FENCE_TYPE_WOOD,
	FENCE_TYPE_WHITE,
	FENCE_TYPE_CANYON,
	FENCE_TYPE_TALLGRASS,
	FENCE_TYPE_SMALLGRASS,
	FENCE_TYPE_SWAMPTREE,
	FENCE_TYPE_PICKETFENCE,
	
	NUM_FENCE_TYPES
};


/**********************/
/*     VARIABLES      */
/**********************/

long			gNumFences = 0;
short			gNumFencesDrawn;
FenceDefType	*gFenceList = nil;


static const short			gFenceTexture[NUM_FENCE_TYPES][2] =
{
	SPRITE_GROUP_GLOBAL,		GLOBAL_SObjType_Fence_Wood,		// wood
	SPRITE_GROUP_GLOBAL,		GLOBAL_SObjType_Fence_White,		// white wood
	SPRITE_GROUP_STAMPEDE,		STAMPEDE_SObjType_Fence_Canyon,		// canyon
	SPRITE_GROUP_GLOBAL,		GLOBAL_SObjType_Fence_TallGrass,
	SPRITE_GROUP_GLOBAL,		GLOBAL_SObjType_Fence_SmallGrass,
	SPRITE_GROUP_GLOBAL,		GLOBAL_SObjType_Fence_SwampTree,
	SPRITE_GROUP_GLOBAL,		GLOBAL_SObjType_Fence_PicketFence,
};


static const float			gFenceHeight[] =
{
	200,					// wood
	200,					// white
	900,					// canyon
	250,					// tall grass
	100,					// small grass
	450,					// swamp tree
	200,					// picket
};

static const float			gFenceSink[] =
{
	FENCE_SINK_FACTOR,		// wood
	FENCE_SINK_FACTOR,		// 
	FENCE_SINK_FACTOR,		// 
	FENCE_SINK_FACTOR,		// 
	FENCE_SINK_FACTOR,		// 
	FENCE_SINK_FACTOR,		// 
	FENCE_SINK_FACTOR,		// 
};

static const Boolean			gFenceIsLit[] =
{
	true,					// wood
	true,					// 
	true,					// 
	false,					// tall grass
	false,					// small grass
	true,					// 
	true,					// 
};

static MOMaterialObject			*gFenceMaterials[MAX_FENCES];				// illegal refs to material for each fence in terrain

static MOVertexArrayData		gFenceTriMeshData[MAX_FENCES];
static MOTriangleIndecies		gFenceTriangles[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLPoint3D				gFencePoints[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLTextureCoord			gFenceUVs[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLColorRGBA_Byte		gFenceColors[MAX_FENCES][MAX_NUBS_IN_FENCE*2];


/********************** DISPOSE FENCES *********************/

void DisposeFences(void)
{
int		i;

	if (!gFenceList)
		return;

	for (i = 0; i < gNumFences; i++)
	{
		if (gFenceList[i].sectionVectors)
			SafeDisposePtr((Ptr)gFenceList[i].sectionVectors);			// nuke section vectors
		gFenceList[i].sectionVectors = nil;

		if (gFenceList[i].sectionNormals)
			SafeDisposePtr((Ptr)gFenceList[i].sectionNormals);			// nuke normal vectors
		gFenceList[i].sectionNormals = nil;
		
		if (gFenceList[i].nubList)
			SafeDisposePtr((Ptr)gFenceList[i].nubList);
		gFenceList[i].nubList = nil;
	}

	SafeDisposePtr((Ptr)gFenceList);
	gFenceList = nil;
	gNumFences = 0;
}



/********************* PRIME FENCES ***********************/
//
// Called during terrain prime function to initialize 
//

void PrimeFences(void)
{
long					f,i,numNubs,type, group, sprite;
FenceDefType			*fence;
OGLPoint3D				*nubs;
ObjNode					*obj;
float					sink;

	if (gNumFences > MAX_FENCES)
		DoFatalAlert("PrimeFences: gNumFences > MAX_FENCES");

	
			/******************************/
			/* ADJUST TO GAME COORDINATES */
			/******************************/
			
	for (f = 0; f < gNumFences; f++)
	{
		fence 				= &gFenceList[f];					// point to this fence
		nubs 				= fence->nubList;					// point to nub list
		numNubs 			= fence->numNubs;					// get # nubs in fence
		type 				= fence->type;						// get fence type
		
		group = gFenceTexture[type][0];							// get sprite info
		sprite = gFenceTexture[type][1];						// get sprite info
		
		if (sprite > gNumSpritesInGroupList[group])
			DoFatalAlert("PrimeFences: illegal fence sprite");
		
		if (numNubs == 1)
			DoFatalAlert("PrimeFences: numNubs == 1");
		
		if (numNubs > MAX_NUBS_IN_FENCE)
			DoFatalAlert("PrimeFences: numNubs > MAX_NUBS_IN_FENCE");
		
		sink = gFenceSink[type];								// get fence sink factor
		
		for (i = 0; i < numNubs; i++)							// adjust nubs
		{
			nubs[i].x *= gMapToUnitValue;
			nubs[i].z *= gMapToUnitValue;
			nubs[i].y = GetTerrainY(nubs[i].x,nubs[i].z) - sink;	// calc Y	
		}
		
		/* CALCULATE VECTOR FOR EACH SECTION */
		
		fence->sectionVectors = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));		// alloc array to hold vectors
		if (fence->sectionVectors == nil)
			DoFatalAlert("PrimeFences: AllocPtr failed!");

		for (i = 0; i < (numNubs-1); i++)
		{
			fence->sectionVectors[i].x = nubs[i+1].x - nubs[i].x;
			fence->sectionVectors[i].y = nubs[i+1].z - nubs[i].z;
			
			OGLVector2D_Normalize(&fence->sectionVectors[i], &fence->sectionVectors[i]);
		}		


		/* CALCULATE NORMALS FOR EACH SECTION */
		
		fence->sectionNormals = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));		// alloc array to hold vectors
		if (fence->sectionNormals == nil)
			DoFatalAlert("PrimeFences: AllocPtr failed!");

		for (i = 0; i < (numNubs-1); i++)
		{
			OGLVector3D	v;
		
			v.x = fence->sectionVectors[i].x;					// get section vector (as calculated above)
			v.z = fence->sectionVectors[i].y;
		
			fence->sectionNormals[i].x = -v.z;					//  reduced cross product to get perpendicular normal
			fence->sectionNormals[i].y = v.x;
			OGLVector2D_Normalize(&fence->sectionNormals[i], &fence->sectionNormals[i]);		
		}		
		
	}
	
			/***********************/
			/* MAKE FENCE GEOMETRY */
			/***********************/

	MakeFenceGeometry();			

		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE FENCE DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The fences need to be drawn after the Cyc object, but before any sprite or font objects.
		//
				
	gNewObjectDefinition.genre		= CUSTOM_GENRE;				
	gNewObjectDefinition.slot 		= FENCE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED|STATUS_BIT_NOLIGHTING;
	
	obj = MakeNewObject(&gNewObjectDefinition);		
	obj->CustomDrawFunction = DrawFences;		
}


/*************** MAKE FENCE GEOMETRY *********************/

static void MakeFenceGeometry(void)
{
int						f, group, sprite;
u_short					type;
float					u,height,aspectRatio,textureUOff;
long					i,numNubs,j;
FenceDefType			*fence;
OGLPoint3D				*nubs;
float					minX,minY,minZ,maxX,maxY,maxZ;

	for (f = 0; f < gNumFences; f++)
	{
				/******************/
				/* GET FENCE INFO */
				/******************/
				
		fence = &gFenceList[f];								// point to this fence
		nubs = fence->nubList;								// point to nub list	
		numNubs = fence->numNubs;							// get # nubs in fence
		type = fence->type;									// get fence type
		height = gFenceHeight[type];						// get fence height
		
		group = gFenceTexture[type][0];						// get sprite info
		sprite = gFenceTexture[type][1];
		
		aspectRatio = gSpriteGroupList[group][sprite].aspectRatio;	// get aspect ratio
	
		textureUOff = 1.0f / height * aspectRatio;			// calc UV offset

		gFenceMaterials[f] = gSpriteGroupList[group][sprite].materialObject;	// keep illegal ref to the material
		
	
					/***************************/
					/* SET VERTEX ARRAY HEADER */
					/***************************/
					
		gFenceTriMeshData[f].numMaterials		 		= -1;			// we submit these manually
		gFenceTriMeshData[f].materials[0]				= nil;
		gFenceTriMeshData[f].points 					= &gFencePoints[f][0];
		gFenceTriMeshData[f].triangles					= &gFenceTriangles[f][0];
		gFenceTriMeshData[f].uvs[0]						= &gFenceUVs[f][0];
		gFenceTriMeshData[f].normals					= nil;
		gFenceTriMeshData[f].colorsByte					= &gFenceColors[f][0];	
		gFenceTriMeshData[f].colorsFloat				= nil;		
		gFenceTriMeshData[f].numPoints = numNubs * 2;					// 2 vertices per nub
		gFenceTriMeshData[f].numTriangles = (numNubs-1) * 2;			// 2 faces per nub (minus 1st)
	
	
				/* BUILD TRIANGLE INFO */
				
		for (i = j = 0; i < MAX_NUBS_IN_FENCE; i++, j+=2)
		{
			gFenceTriangles[f][j].vertexIndices[0] = 1 + j;
			gFenceTriangles[f][j].vertexIndices[1] = 0 + j;
			gFenceTriangles[f][j].vertexIndices[2] = 3 + j;
	
			gFenceTriangles[f][j+1].vertexIndices[0] = 3 + j;
			gFenceTriangles[f][j+1].vertexIndices[1] = 0 + j;
			gFenceTriangles[f][j+1].vertexIndices[2] = 2 + j;	
	
		}

				/* INIT VERTEX COLORS */
				
		for (i = 0; i < (MAX_NUBS_IN_FENCE*2); i++)
			gFenceColors[f][i].r = gFenceColors[f][i].g = gFenceColors[f][i].b = 0xff;


				/**********************/
				/* BUILD POINTS, UV's */
				/**********************/
			
		maxX = maxY = maxZ = -1000000;									// build new bboxes while we do this
		minX = minY = minZ = -maxX;
		
		u = 0;
		for (i = j = 0; i < numNubs; i++, j+=2)
		{
			float		x,y,z,y2;
			
					/* GET COORDS */
					
			x = nubs[i].x;
			z = nubs[i].z;			
			y = nubs[i].y;			
			y2 = y + height;
		
			if (type == FENCE_TYPE_CANYON)
			{
				if ((i > 0) && (i < (numNubs-1)))		// dont futz with 1st and last nub
					y2 += RandomFloat() * 300.0f;
			}
		
		
					/* CHECK BBOX */
					
			if (x < minX)	minX = x;									// find min/max bounds for bbox
			if (x > maxX)	maxX = x;
			if (z < minZ)	minZ = z;
			if (z > maxZ)	maxZ = z;
			if (y < minY)	minY = y;
			if (y2 > maxY)	maxY = y2;
		
		
				/* SET COORDS */
					
			gFencePoints[f][j].x = x;
			gFencePoints[f][j].y = y;
			gFencePoints[f][j].z = z;
			
			if (type == FENCE_TYPE_CANYON)
			{
				if ((i > 0) && (i < (numNubs-1)))		// dont futz with 1st and last nub
				{
					x += RandomFloat2() * 400.0f;
					z += RandomFloat2() * 400.0f;
				}
			}			
			
			gFencePoints[f][j+1].x = x;
			gFencePoints[f][j+1].y = y2;
			gFencePoints[f][j+1].z = z;		

		
				/* CALC UV COORDS */
						
			if (i > 0)
			{
				u += CalcDistance3D(gFencePoints[f][j].x, gFencePoints[f][j].y, gFencePoints[f][j].z,
									gFencePoints[f][j-2].x, gFencePoints[f][j-2].y, gFencePoints[f][j-2].z) * textureUOff;
			}
						
			gFenceUVs[f][j].v 		= 1.0f;									// bottom
			gFenceUVs[f][j+1].v 	= 0.0f;									// top
			gFenceUVs[f][j].u 		= gFenceUVs[f][j+1].u = u;
		}
		
				/* SET CALCULATED BBOX */
				
		fence->bBox.min.x = minX;
		fence->bBox.max.x = maxX;
		fence->bBox.min.y = minY;
		fence->bBox.max.y = maxY;
		fence->bBox.min.z = minZ;
		fence->bBox.max.z = maxZ;
		fence->bBox.isEmpty = false;		
	}
}


#pragma mark -

/********************* DRAW FENCES ***********************/

static void DrawFences(ObjNode *theNode)
{
long			f,type;
float			cameraX, cameraZ;

	(void) theNode;


			/* GET CAMERA COORDS */
			
	cameraX = gGameViewInfoPtr->cameraPlacement.cameraLocation.x;
	cameraZ = gGameViewInfoPtr->cameraPlacement.cameraLocation.z;


			/* SET GLOBAL MATERIAL FLAGS */

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_ALWAYSBLEND;


			/*******************/
			/* DRAW EACH FENCE */
			/*******************/			

	gNumFencesDrawn = 0;

	for (f = 0; f < gNumFences; f++)
	{
		type = gFenceList[f].type;							// get type
		
					/* DO BBOX CULLING */

		if (OGL_IsBBoxVisible(&gFenceList[f].bBox, nil))	
		{
					/* CHECK LIGHTING */
			
			if (gFenceIsLit[type])
				OGL_EnableLighting();
			else
				OGL_DisableLighting();
					
				/* SUBMIT GEOMETRY */
				
			SubmitFence(f, cameraX, cameraZ);
			gNumFencesDrawn++;
						
			if (gDebugMode == 2)
			{
				DrawFenceNormals(f);
			}
		}
	}
	
	gGlobalMaterialFlags = 0;
}


/****************** DRAW FENCE NORMALS ***************************/

static void DrawFenceNormals(short f)
{
int				i,numNubs;
OGLPoint3D		*nubs;
OGLVector2D		*normals;
float			x,y,z,nx,nz;

	OGL_PushState();
	glDisable(GL_TEXTURE_2D);
	SetColor4f(1,0,0,1);
	glLineWidth(3);	
	
	numNubs  	= gFenceList[f].numNubs - 1;					// get # nubs in fence minus 1
	nubs  		= gFenceList[f].nubList;						// get ptr to nub list
	normals 	= gFenceList[f].sectionNormals;					// get ptr to normals
	
	for (i = 0; i < numNubs; i++)
	{
		glBegin(GL_LINES);

		x = nubs[i].x;
		y = nubs[i].y + 200.0f;			// show normal up a ways
		z = nubs[i].z;
		
		nx = normals[i].x * 150.0f;
		nz = normals[i].y * 150.0f;
		
		glVertex3f(x-nx,y,z-nz);
		glVertex3f(x + nx,y, z + nz);

		glEnd();
	}
	OGL_PopState();
	glLineWidth(1);	

}


/******************** SUBMIT FENCE **************************/
//
// Visibility checks have already been done, so there's a good chance the fence is visible
//

static void SubmitFence(int f, const float camX, float camZ)
{
int						doAutoFade = gAutoFadeStatusBits;
float					dist,alpha, autoFadeStart = gAutoFadeStartDist;
float					autoFadeEndDist = gAutoFadeEndDist;
float					autoFadeRangeFrac = gAutoFadeRange_Frac;
long					i,numNubs,j;
FenceDefType			*fence;
OGLPoint3D				*nubs;
Boolean					overrideAlphaFunc = false;
			
			/* GET FENCE INFO */
			
	fence = &gFenceList[f];								// point to this fence
	nubs = fence->nubList;								// point to nub list	
	numNubs = fence->numNubs;							// get # nubs in fence


				/* CALC & SET TRANSPARENCY */		
			
	for (i = j = 0; i < numNubs; i++, j+=2)
	{
				/* CALC & SET TRANSPARENCY */
		
		if (doAutoFade)														// see if this level has xparency
		{
			dist = CalcQuickDistance(camX, camZ, nubs[i].x, nubs[i].z);		// see if in fade zone
			if (dist < autoFadeStart)	
				alpha = 1.0;
			else
			{
				overrideAlphaFunc = true;
				if (dist >= autoFadeEndDist)
					alpha = 0.0;
				else
				{
					dist -= autoFadeStart;										// calc xparency %
					dist *= autoFadeRangeFrac;				
					if (dist < 0.0f)
						alpha = 0;
					else
						alpha = 1.0f - dist;
				}			
			}
		}
		else
			alpha = 1.0f;
				
		gFenceColors[f][j].a = gFenceColors[f][j+1].a = 255.0f * alpha;
	}
	
			
			
		/*******************/
		/* SUBMIT GEOMETRY */
		/*******************/
		//
		// Fences often have 1-bit alpha transparency, and we want nice sharp edges.
		// The DrawMaterial() function will set the appropriate alpha func to keep the edges sharp,
		// however, this will also cause any AutoFade vertices to vanish instead of fade out,
		// so if any part of the fence is alpha faded then we need to set the alpha func back to normal.
		//
	
		/* ACTIVATE MATERIAL */
			
	MO_DrawMaterial(gFenceMaterials[f]);

	if (overrideAlphaFunc)				// override alpha func settings if any vertex alphas are not opaque
		glAlphaFunc(GL_NOTEQUAL, 0);	
	
	
			/* SUBMIT GEO */
			
	MO_DrawGeometry_VertexArray(&gFenceTriMeshData[f]);
}	




#pragma mark -

/******************** DO FENCE COLLISION **************************/
//
// returns True if hit a fence
//

Boolean DoFenceCollision(ObjNode *theNode)
{
long			f,numFenceSegments,i,numReScans;
OGLPoint3D		*nubs;
OGLVector2D		*vectors;
double			radius;
double			oldX,oldZ,newX,newZ;
Boolean			hit = false;

			/* CALC MY MOTION LINE SEGMENT */
			
	oldX = theNode->OldCoord.x;						// from old coord
	oldZ = theNode->OldCoord.z;
	newX = gCoord.x;								// to new coord
	newZ = gCoord.z;
	radius = theNode->BoundingSphereRadius;


			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/
			
	for (f = 0; f < gNumFences; f++)
	{
		int		type;
		float	temp;
		float	r2 = radius + 20.0f;								// tweak a little to be safe

		if ((oldX == newX) && (oldZ == newZ))						// if no movement, then don't check anything
			break;
					
			
		type = gFenceList[f].type;

			
		/* QUICK CHECK TO SEE IF OLD & NEW COORDS (PLUS RADIUS) ARE OUTSIDE OF FENCE'S BBOX */

		temp = gFenceList[f].bBox.min.x - r2;
		if ((oldX < temp) && (newX < temp))
			continue;
		temp = gFenceList[f].bBox.max.x + r2;
		if ((oldX > temp) && (newX > temp))
			continue;
			
		temp = gFenceList[f].bBox.min.z - r2;
		if ((oldZ < temp) && (newZ < temp))
			continue;
		temp = gFenceList[f].bBox.max.z + r2;
		if ((oldZ > temp) && (newZ > temp))
			continue;
			
		nubs = gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence
		vectors = gFenceList[f].sectionVectors;		// point to segment vector array


				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/
			
		numReScans = 0;	
		for (i = 0; i < numFenceSegments; i++)
		{
			OGLPoint3D		p1,p2, sphereCenter, intersectPt;
			OGLVector3D		segVector;
			
					/* GET LINE SEG ENDPOINTS & SPHERE */
					
			p1.x = nubs[i].x;
			p1.y = 0;
			p1.z = nubs[i].z;
			
			p2.x = nubs[i+1].x;
			p2.y = 0;
			p2.z = nubs[i+1].z;				
	
			segVector.x = vectors[i].x;
			segVector.z = vectors[i].y;
			segVector.y = 0;
	
			sphereCenter.x = gCoord.x;
			sphereCenter.z = gCoord.z;
			sphereCenter.y = 0;
	
	
					/* SEE IF THIS FENCE SEGMENT INTERSECTS THE BOUNDING SPHERE IN THE Y=0 PLANE */
						
			if (OGL_DoesLineSegmentIntersectSphere(&p1, &p2, &segVector, &sphereCenter, radius, &intersectPt))
			{
				gCoord.x = theNode->OldCoord.x;
				gCoord.z = theNode->OldCoord.z;
			
			
				hit = true;
			
			}
		}
	}
	
	return(hit);
}

/******************** SEE IF LINE SEGMENT HITS FENCE **************************/
//
// returns True if hit a fence
//

Boolean SeeIfLineSegmentHitsFence(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2, OGLPoint3D *intersect, Boolean *overTop, float *fenceTopY)
{
float			fromX,fromZ,toX,toZ;
long			f,numFenceSegments,i;
float			segFromX,segFromZ,segToX,segToZ;
OGLPoint3D		*nubs;
Boolean			intersected;


	fromX = endPoint1->x;
	fromZ = endPoint1->z;
	toX = endPoint2->x;
	toZ = endPoint2->z;

			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/
			
	for (f = 0; f < gNumFences; f++)
	{		
		short	type;
			
				/* SEE IF IGNORE */
					
		type = gFenceList[f].type;
		switch(type)
		{
//			case	FENCE_TYPE_DOGHAIR:						// ignore these
//			case	FENCE_TYPE_CLOTH:
//					continue;
		}

					
		/* QUICK CHECK TO SEE IF OLD & NEW COORDS (PLUS RADIUS) ARE OUTSIDE OF FENCE'S BBOX */
	
		if ((fromX < gFenceList[f].bBox.min.x) && (toX < gFenceList[f].bBox.min.x))
			continue;
		if ((fromX > gFenceList[f].bBox.max.x) && (toX > gFenceList[f].bBox.max.x))
			continue;
			
		if ((fromZ < gFenceList[f].bBox.min.z) && (toZ < gFenceList[f].bBox.min.z))
			continue;
		if ((fromZ > gFenceList[f].bBox.max.z) && (toZ > gFenceList[f].bBox.max.z))
			continue;
			
		nubs = gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence


				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/
			
		for (i = 0; i < numFenceSegments; i++)
		{
			float	ix,iz;
			
					/* GET LINE SEG ENDPOINTS */
					
			segFromX = nubs[i].x;
			segFromZ = nubs[i].z;
			segToX = nubs[i+1].x;
			segToZ = nubs[i+1].z;				
					
	
					/* SEE IF THE LINES INTERSECT */
					
			intersected = IntersectLineSegments(fromX,  fromZ, toX, toZ,
						                     segFromX, segFromZ, segToX, segToZ,
				                             &ix, &iz);
	
			if (intersected)
			{
				float	fenceTop,dy,d1,d2,ratio,iy;
				
				
						/* SEE IF INTERSECT OCCURS OVER THE TOP OF THE FENCE */
			
				if (overTop || intersect || fenceTopY)
				{			
					fenceTop = GetTerrainY(ix, iz) + gFenceHeight[gFenceList[f].type];		// calc y coord @ top of fence here
					
					dy = endPoint2->y - endPoint1->y;					// get dy of line segment
					
					d1 = CalcDistance(fromX, fromZ, toX, toZ);
					d2 = CalcDistance(fromX, fromZ, ix, iz);
					
					ratio = d2/d1;
					
					iy = endPoint1->y + (dy * ratio);					// calc intersect y coord
					
					if (overTop)
					{
						if (iy >= fenceTop)
							*overTop = true;
						else
							*overTop = false;
					}
					
					if (intersect)
					{
						intersect->x = ix;						// pass back intersect coords
						intersect->y = iy;			
						intersect->z = iz;		
					}
					
					if (fenceTopY)
						*fenceTopY = fenceTop;	
				}
							
				return(true);			
			}
			
		}
	}
	
	return(false);
}




