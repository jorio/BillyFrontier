/****************************/
/*   	PARTICLES.C		    */
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

static void DeleteParticleGroup(long groupNum);
static void MoveParticleGroups(ObjNode *theNode);

static void DrawParticleGroup(ObjNode *theNode);
static void MoveBlobDroplet(ObjNode *theNode);

static void MoveSmoker(ObjNode *theNode);

static void MoveFlame(ObjNode *theNode);
static void DrawFlame(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	FIRE_TIMER	.05f
#define	SMOKE_TIMER	.07f


/*********************/
/*    VARIABLES      */
/*********************/

ParticleGroupType	*gParticleGroups[MAX_PARTICLE_GROUPS];

static float	gGravitoidDistBuffer[MAX_PARTICLES][MAX_PARTICLES];

NewParticleGroupDefType	gNewParticleGroupDef;

short			gNumActiveParticleGroups = 0;


#define FlameFrame Special[0]
#define FlameSpeed SpecialF[0]


/************************* INIT EFFECTS ***************************/

void InitEffects(void)
{
	InitParticleSystem();
	InitShardSystem();

			/* SET SPRITE BLENDING FLAGS */
			
	BlendASprite(SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_Splash);
}


#pragma mark -

/************************ INIT PARTICLE SYSTEM **************************/

void InitParticleSystem(void)
{
ObjNode	*obj;


			/* INIT GROUP ARRAY */

	for (int i = 0; i < MAX_PARTICLE_GROUPS; i++)
		gParticleGroups[i] = nil;

	gNumActiveParticleGroups = 0;



			/* LOAD SPRITES */

	LoadSpriteGroup(SPRITE_GROUP_PARTICLES);

	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);


		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE PARTICLE DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The particles need to be drawn after the fences object, but before any sprite or font objects.
		//
				
	gNewObjectDefinition.genre		= CUSTOM_GENRE;				
	gNewObjectDefinition.slot 		= PARTICLE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveParticleGroups;
	gNewObjectDefinition.scale 		= 1;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES|STATUS_BIT_NOFOG;
	
	obj = MakeNewObject(&gNewObjectDefinition);		
	obj->CustomDrawFunction = DrawParticleGroup;		


		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE SHARD DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
				
	gNewObjectDefinition.genre		= CUSTOM_GENRE;				
	gNewObjectDefinition.slot 		= PARTICLE_SLOT-1;			// best to draw before particles
	gNewObjectDefinition.moveCall 	= MoveShards;
	gNewObjectDefinition.scale 		= 1;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED;
	
	obj = MakeNewObject(&gNewObjectDefinition);		
	obj->CustomDrawFunction = DrawShards;		

}


/******************** DISPOSE PARTICLE SYSTEM **********************/

void DisposeParticleSystem(void)
{
}


/******************** DELETE ALL PARTICLE GROUPS *********************/

void DeleteAllParticleGroups(void)
{
long	i;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		DeleteParticleGroup(i);
	}
}


/******************* DELETE PARTICLE GROUP ***********************/

static void DeleteParticleGroup(long groupNum)
{
	if (gParticleGroups[groupNum])
	{
			/* NUKE GEOMETRY DATA */
			
		MO_DisposeObjectReference(gParticleGroups[groupNum]->geometryObj);
			

				/* NUKE GROUP ITSELF */
					
		SafeDisposePtr((Ptr)gParticleGroups[groupNum]);
		gParticleGroups[groupNum] = nil;

		gNumActiveParticleGroups--;
	}	
}


#pragma mark -


/********************** NEW PARTICLE GROUP *************************/
//
// INPUT:	type 	=	group type to create
//
// OUTPUT:	group ID#
//

short NewParticleGroup(NewParticleGroupDefType *def)
{
OGLTextureCoord			*uv;
MOVertexArrayData 		vertexArrayData;
MOTriangleIndecies		*t;

	
			/*************************/
			/* SCAN FOR A FREE GROUP */
			/*************************/
			
	for (int i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (gParticleGroups[i] == nil)
		{
				/* ALLOCATE NEW GROUP */
				
			gParticleGroups[i] = (ParticleGroupType *)AllocPtr(sizeof(ParticleGroupType));
			if (gParticleGroups[i] == nil)
				return(-1);									// out of memory

			
				/* INITIALIZE THE GROUP */
			
			gParticleGroups[i]->type = def->type;						// set type
			for (int p = 0; p < MAX_PARTICLES; p++)						// mark all unused
				gParticleGroups[i]->isUsed[p] = false;
			
			gParticleGroups[i]->flags 				= def->flags;
			gParticleGroups[i]->gravity 			= def->gravity;
			gParticleGroups[i]->magnetism 			= def->magnetism;
			gParticleGroups[i]->baseScale 			= def->baseScale;
			gParticleGroups[i]->decayRate 			= def->decayRate;
			gParticleGroups[i]->fadeRate 			= def->fadeRate;
			gParticleGroups[i]->magicNum 			= def->magicNum;
			gParticleGroups[i]->particleTextureNum 	= def->particleTextureNum;

			gParticleGroups[i]->srcBlend 			= def->srcBlend;
			gParticleGroups[i]->dstBlend 			= def->dstBlend;

				/*****************************/			
				/* INIT THE GROUP'S GEOMETRY */
				/*****************************/
						
					/* SET THE DATA */
					
			vertexArrayData.numMaterials 	= 1;
			vertexArrayData.materials[0]	= gSpriteGroupList[SPRITE_GROUP_PARTICLES][def->particleTextureNum].materialObject;	// set illegal ref because it is made legit below

			vertexArrayData.numPoints 		= 0;
			vertexArrayData.numTriangles 	= 0;
			vertexArrayData.points 			= (OGLPoint3D *)AllocPtr(sizeof(OGLPoint3D) * MAX_PARTICLES * 4);
			vertexArrayData.normals 		= nil;
			vertexArrayData.uvs[0]	 		= (OGLTextureCoord *)AllocPtr(sizeof(OGLTextureCoord) * MAX_PARTICLES * 4);
			vertexArrayData.colorsByte 		= (OGLColorRGBA_Byte *)AllocPtr(sizeof(OGLColorRGBA_Byte) * MAX_PARTICLES * 4);
			vertexArrayData.colorsFloat		= nil;
			vertexArrayData.triangles		= (MOTriangleIndecies *)AllocPtr(sizeof(MOTriangleIndecies) * MAX_PARTICLES * 2);
						
	
					/* INIT UV ARRAYS */
					
			uv = vertexArrayData.uvs[0];
			for (int j = 0; j < (MAX_PARTICLES*4); j += 4)
			{
				uv[j  ].u = 0;									// upper(?) left
				uv[j  ].v = 0;
				uv[j+1].u = 0;									// lower(?) left
				uv[j+1].v = 1;	
				uv[j+2].u = 1;									// lower(?) right
				uv[j+2].v = 1;	
				uv[j+3].u = 1;									// upper(?) right
				uv[j+3].v = 0;				
			}
						
					/* INIT TRIANGLE ARRAYS */
					
			t = vertexArrayData.triangles;
			for (int j = 0, k = 0; j < (MAX_PARTICLES*2); j+=2, k+=4)
			{
				t[j].vertexIndices[0] = k;							// triangle A
				t[j].vertexIndices[1] = k+1;
				t[j].vertexIndices[2] = k+2;

				t[j+1].vertexIndices[0] = k;							// triangle B
				t[j+1].vertexIndices[1] = k+2;
				t[j+1].vertexIndices[2] = k+3;			
			}


				/* CREATE NEW GEOMETRY OBJECT */

			gParticleGroups[i]->geometryObj = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &vertexArrayData);
			
			gNumActiveParticleGroups++;
			
			return(i);
		}
	}

			/* NOTHING FREE */
			
//	DoFatalAlert("NewParticleGroup: no free groups!");
	return(-1);	
}


/******************** ADD PARTICLE TO GROUP **********************/
//
// Returns true if particle group was invalid or is full.
//

Boolean AddParticleToGroup(NewParticleDefType *def)
{
short	p,group;

	group = def->groupNum;

	if ((group < 0) || (group >= MAX_PARTICLE_GROUPS))
		DoFatalAlert("AddParticleToGroup: illegal group #");

	if (gParticleGroups[group] == nil)
	{
		return(true);
	}


			/* SCAN FOR FREE SLOT */
			
	for (p = 0; p < MAX_PARTICLES; p++)
	{
		if (!gParticleGroups[group]->isUsed[p])
			goto got_it;
	}
	
			/* NO FREE SLOTS */
			
	return(true);


			/* INIT PARAMETERS */
got_it:
	gParticleGroups[group]->alpha[p] = def->alpha;
	gParticleGroups[group]->scale[p] = def->scale;
	gParticleGroups[group]->coord[p] = *def->where;
	gParticleGroups[group]->delta[p] = *def->delta;	
	gParticleGroups[group]->rotZ[p] = def->rotZ;	
	gParticleGroups[group]->rotDZ[p] = def->rotDZ;	
	gParticleGroups[group]->isUsed[p] = true;
	
	
	
	
	return(false);
}


/****************** MOVE PARTICLE GROUPS *********************/

static void MoveParticleGroups(ObjNode *theNode)
{
uint32_t		flags;
long		i,n,p,j;
float		fps = gFramesPerSecondFrac;
float		y,baseScale,oneOverBaseScaleSquared,gravity;
float		decayRate,magnetism,fadeRate;
OGLPoint3D	*coord;
OGLVector3D	*delta;

	(void) theNode;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (gParticleGroups[i])
		{
			baseScale 	= gParticleGroups[i]->baseScale;					// get base scale
			oneOverBaseScaleSquared = 1.0f/(baseScale*baseScale);
			gravity 	= gParticleGroups[i]->gravity;						// get gravity
			decayRate 	= gParticleGroups[i]->decayRate;					// get decay rate
			fadeRate 	= gParticleGroups[i]->fadeRate;						// get fade rate
			magnetism 	= gParticleGroups[i]->magnetism;					// get magnetism
			flags 		= gParticleGroups[i]->flags;
			
			n = 0;															// init counter
			for (p = 0; p < MAX_PARTICLES; p++)
			{
				if (!gParticleGroups[i]->isUsed[p])							// make sure this particle is used
					continue;
				
				n++;														// inc counter
				delta = &gParticleGroups[i]->delta[p];						// get ptr to deltas
				coord = &gParticleGroups[i]->coord[p];						// get ptr to coords
				
							/* ADD GRAVITY */
							
				delta->y -= gravity * fps;									// add gravity
				
				
						/* DO ROTATION */
						
				gParticleGroups[i]->rotZ[p] += gParticleGroups[i]->rotDZ[p] * fps;
				
				
				
				
				switch(gParticleGroups[i]->type)
				{
							/* FALLING SPARKS */
							
					case	PARTICLE_TYPE_FALLINGSPARKS:
							coord->x += delta->x * fps;						// move it
							coord->y += delta->y * fps;
							coord->z += delta->z * fps;
							break;


							/* GRAVITOIDS */
							//
							// Every particle has gravity pull on other particle
							//
							
					case	PARTICLE_TYPE_GRAVITOIDS:
							for (j = MAX_PARTICLES-1; j >= 0; j--)
							{
								float		dist,temp,x,z;
								OGLVector3D	v;
								
								if (p==j)									// dont check against self
									continue;
								if (!gParticleGroups[i]->isUsed[j])			// make sure this particle is used
									continue;
															
								x = gParticleGroups[i]->coord[j].x;
								y = gParticleGroups[i]->coord[j].y;
								z = gParticleGroups[i]->coord[j].z;
								
										/* calc 1/(dist2) */
							
								if (i < j)									// see if calc or get from buffer
								{
									temp = coord->x - x;					// dx squared
									dist = temp*temp;
									temp = coord->y - y;					// dy squared
									dist += temp*temp;
									temp = coord->z - z;					// dz squared
									dist += temp*temp;
									
									dist = sqrt(dist);						// 1/dist2
									if (dist != 0.0f)
										dist = 1.0f / (dist*dist);  

									if (dist > oneOverBaseScaleSquared)		// adjust if closer than radius
										dist = oneOverBaseScaleSquared;

									gGravitoidDistBuffer[i][j] = dist;		// remember it
								}
								else
								{
									dist = gGravitoidDistBuffer[j][i];		// use from buffer
								}
								
											/* calc vector to particle */
											
								if (dist != 0.0f)
								{
									x = x - coord->x;
									y = y - coord->y;
									z = z - coord->z;
									FastNormalizeVector(x, y, z, &v);
								}
								else
								{
									v.x = v.y = v.z = 0;
								}
																
								delta->x += v.x * (dist * magnetism * fps);		// apply gravity to particle
								delta->y += v.y * (dist * magnetism * fps);
								delta->z += v.z * (dist * magnetism * fps);
							}
														
							coord->x += delta->x * fps;						// move it
							coord->y += delta->y * fps;
							coord->z += delta->z * fps;
							break;							
				}
				
					/********************/
					/* SEE IF HAS MAX Y */
					/********************/
				
				if (flags & PARTICLE_FLAGS_HASMAXY)
				{
					if (coord->y > gParticleGroups[i]->maxY)
					{
						gParticleGroups[i]->isUsed[p] = false;
					}
				}
				
				
				/*****************/
				/* SEE IF BOUNCE */
				/*****************/

				if (!(flags & PARTICLE_FLAGS_DONTCHECKGROUND))
				{
					y = GetTerrainY(coord->x, coord->z)+10.0f;					// get terrain coord at particle x/z
					
					if (flags & PARTICLE_FLAGS_BOUNCE)
					{
						if (delta->y < 0.0f)									// if moving down, see if hit floor
						{
							if (coord->y < y)
							{
								coord->y = y;
								delta->y *= -.4f;
								
								delta->x += gRecentTerrainNormal.x * 300.0f;	// reflect off of surface
								delta->z += gRecentTerrainNormal.z * 300.0f;
								
								if (flags & PARTICLE_FLAGS_DISPERSEIFBOUNCE)	// see if disperse on impact
								{
									delta->y *= .4f;
									delta->x *= 5.0f;
									delta->z *= 5.0f;								
								}
							}
						}
					}

					/***************/
					/* SEE IF GONE */
					/***************/

					else
					{				
						if (coord->y < y)									// if hit floor then nuke particle
						{
							gParticleGroups[i]->isUsed[p] = false;				
						}
					}
				}					
					
				
					/* DO SCALE */
					
				gParticleGroups[i]->scale[p] -= decayRate * fps;			// shrink it							
				if (gParticleGroups[i]->scale[p] <= 0.0f)					// see if gone
					gParticleGroups[i]->isUsed[p] = false;				

					/* DO FADE */
					
				gParticleGroups[i]->alpha[p] -= fadeRate * fps;				// fade it							
				if (gParticleGroups[i]->alpha[p] <= 0.0f)					// see if gone
					gParticleGroups[i]->isUsed[p] = false;				
					
			}
			
				/* SEE IF GROUP WAS EMPTY, THEN DELETE */
					
			if (n == 0)
			{
				DeleteParticleGroup(i);
			}
		}
	}
}


/**************** DRAW PARTICLE GROUPS *********************/

static void DrawParticleGroup(ObjNode *theNode)
{
float				scale,baseScale;
OGLColorRGBA_Byte	*vertexColors;
MOVertexArrayData	*geoData;
OGLPoint3D		v[4],*camCoords,*coord;
static const OGLVector3D up = {0,1,0};
OGLBoundingBox	bbox;

	(void) theNode;

	v[0].z = 												// init z's to 0
	v[1].z = 
	v[2].z = 
	v[3].z = 0;

				/* SETUP ENVIRONTMENT */
		
	OGL_PushState();
		
	glEnable(GL_BLEND);
	SetColor4f(1,1,1,1);													// full white & alpha to start with

	camCoords = &gGameViewInfoPtr->cameraPlacement.cameraLocation;	

	for (int g = 0; g < MAX_PARTICLE_GROUPS; g++)
	{
		float	minX,minY,minZ,maxX,maxY,maxZ;
		int		temp;

		if (gParticleGroups[g])
		{
			uint32_t	allAim = gParticleGroups[g]->flags & PARTICLE_FLAGS_ALLAIM;
		
			geoData = &gParticleGroups[g]->geometryObj->objectData;			// get pointer to geometry object data
			vertexColors = geoData->colorsByte;								// get pointer to vertex color array
			baseScale = gParticleGroups[g]->baseScale;						// get base scale

					/********************************/
					/* ADD ALL PARTICLES TO TRIMESH */
					/********************************/
					
			minX = minY = minZ = 100000000;									// init bbox
			maxX = maxY = maxZ = -minX;

			int p, n;
			for (p = n = 0; p < MAX_PARTICLES; p++)
			{
				float			rot;
				OGLMatrix4x4	m;
				
				if (!gParticleGroups[g]->isUsed[p])							// make sure this particle is used
					continue;
	
							/* CREATE VERTEX DATA */
						
				scale = gParticleGroups[g]->scale[p] * baseScale;
				
				v[0].x = -scale;
				v[0].y = scale;

				v[1].x = -scale;
				v[1].y = -scale;

				v[2].x = scale;
				v[2].y = -scale;

				v[3].x = scale;
				v[3].y = scale;
			

					/* TRANSFORM THIS PARTICLE'S VERTICES & ADD TO TRIMESH */
					
				coord = &gParticleGroups[g]->coord[p];
				if ((n == 0) || allAim)										// only set the look-at matrix for the 1st particle unless we want to force it for all (optimization technique)
					SetLookAtMatrixAndTranslate(&m, &up, coord, camCoords);	// aim at camera & translate
				else
				{
					m.value[M03] = coord->x;								// update just the translate
					m.value[M13] = coord->y;
					m.value[M23] = coord->z;
				}

				rot = gParticleGroups[g]->rotZ[p];							// get z rotation
				if (rot != 0.0f)											// see if need to apply rotation matrix
				{
					OGLMatrix4x4	rm;
					
					OGLMatrix4x4_SetRotate_Z(&rm, rot);	
					OGLMatrix4x4_Multiply(&rm, &m, &rm);
					OGLPoint3D_TransformArray(&v[0], &rm, &geoData->points[n*4], 4);	// transform w/ rot
				}
				else																	
					OGLPoint3D_TransformArray(&v[0], &m, &geoData->points[n*4], 4);		// transform no-rot


							/* UPDATE BBOX */
						
				for (int i = 0; i < 4; i++)
				{
					int j = n*4+i;
					
					if (geoData->points[j].x < minX)
						minX = geoData->points[j].x;
					if (geoData->points[j].x > maxX)
						maxX = geoData->points[j].x;
					if (geoData->points[j].y < minY)
						minY = geoData->points[j].y;
					if (geoData->points[j].y > maxY)
						maxY = geoData->points[j].y;
					if (geoData->points[j].z < minZ)
						minZ = geoData->points[j].z;
					if (geoData->points[j].z > maxZ)
						maxZ = geoData->points[j].z;
				}
				
					/* UPDATE COLOR/TRANSPARENCY */
								
				temp = n*4;
				for (int i = temp; i < (temp+4); i++)
				{
					vertexColors[i].r = 
					vertexColors[i].g = 
					vertexColors[i].b = 0xff;
					vertexColors[i].a = gParticleGroups[g]->alpha[p] * 255.0f;		// set transparency alpha
				}

				n++;											// inc particle count
			}
	
			if (n == 0)											// if no particles, then skip
				continue;
	
				/* UPDATE FINAL VALUES */

			geoData->numTriangles = n*2;
			geoData->numPoints = n*4;
			
			bbox.min.x = minX;									// build bbox for culling test
			bbox.min.y = minY;
			bbox.min.z = minZ;
			bbox.max.x = maxX;
			bbox.max.y = maxY;
			bbox.max.z = maxZ;	

			if (OGL_IsBBoxVisible(&bbox, nil))									// do cull test on it
			{
				GLint	src,dst;
				
				src = gParticleGroups[g]->srcBlend;
				dst = gParticleGroups[g]->dstBlend;
				
					/* DRAW IT */
		
				glBlendFunc(src, dst);											// set blending mode						
				MO_DrawObject(gParticleGroups[g]->geometryObj);						// draw geometry
			}
		}
	}
	
			/* RESTORE MODES */
				
	OGL_PopState();
	SetColor4f(1,1,1,1);										// reset this
}


/**************** VERIFY PARTICLE GROUP MAGIC NUM ******************/

Boolean VerifyParticleGroupMagicNum(short group, uint32_t magicNum)
{
	if (gParticleGroups[group] == nil)
		return(false);

	if (gParticleGroups[group]->magicNum != magicNum)
		return(false);

	return(true);
}


/************* PARTICLE HIT OBJECT *******************/
//
// INPUT:	inFlags = flags to check particle types against
//

Boolean ParticleHitObject(ObjNode *theNode, uint16_t inFlags)
{
int		i,p;
uint32_t	flags;
OGLPoint3D	*coord;

	for (i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		if (!gParticleGroups[i])									// see if group active
			continue;
			
		if (inFlags)												// see if check flags
		{
			flags = gParticleGroups[i]->flags;
			if (!(inFlags & flags))
				continue;
		}
		
		for (p = 0; p < MAX_PARTICLES; p++)
		{
			if (!gParticleGroups[i]->isUsed[p])						// make sure this particle is used
				continue;
				
			if (gParticleGroups[i]->alpha[p] < .4f)				// if particle is too decayed, then skip
				continue;			 
			
			coord = &gParticleGroups[i]->coord[p];					// get ptr to coords					
			if (DoSimpleBoxCollisionAgainstObject(coord->y+40.0f,coord->y-40.0f,
												coord->x-40.0f, coord->x+40.0f,
												coord->z+40.0f, coord->z-40.0f,
												theNode))
			{
				return(true);
			}
		}
	}
	return(false);
}

#pragma mark -

/********************* MAKE PUFF ***********************/

void MakePuff(const OGLPoint3D *where, float scale, short texNum, GLint src, GLint dst, float decayRate)
{
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;

			/* white sparks */
				
	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= -20;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= scale;
	gNewParticleGroupDef.decayRate				= -1.2;
	gNewParticleGroupDef.fadeRate				= decayRate;
	gNewParticleGroupDef.particleTextureNum		= texNum;
	gNewParticleGroupDef.srcBlend				= src;
	gNewParticleGroupDef.dstBlend				= dst;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{	
		x = where->x;
		y = where->y;
		z = where->z;
		
		for (i = 0; i < 10; i++)
		{
			pt.x = x + RandomFloat2() * (2.0f * scale);
			pt.y = y + RandomFloat() * 2.0f * scale;
			pt.z = z + RandomFloat2() * (2.0f * scale);

			delta.x = RandomFloat2() * (3.0f * scale);
			delta.y = RandomFloat() * (2.0f  * scale);
			delta.z = RandomFloat2() * (3.0f * scale);
			
			
			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f; // + RandomFloat2() * .2f;
			newParticleDef.rotZ			= RandomFloat() * PI2;
			newParticleDef.rotDZ		= RandomFloat2() * 4.0f;
			newParticleDef.alpha		= FULL_ALPHA;		
			AddParticleToGroup(&newParticleDef);
		}
	}
}


/********************* MAKE SPARK EXPLOSION ***********************/

void MakeSparkExplosion(float x, float y, float z, float force, float scale, short sparkTexture, short quantityLimit, float fadeRate)
{
long					pg,i,n;
OGLVector3D				delta,v;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;

	n = force * .3f;
	
	if (quantityLimit)
		if (n > quantityLimit)
			n = quantityLimit;
			

			/* white sparks */
				
	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 200;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 15.0f * scale;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= fadeRate;
	gNewParticleGroupDef.particleTextureNum		= sparkTexture;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < n; i++)
		{
			pt.x = x + RandomFloat2() * (30.0f * scale);
			pt.y = y + RandomFloat2() * (30.0f * scale);
			pt.z = z + RandomFloat2() * (30.0f * scale);

			v.x = pt.x - x;
			v.y = pt.y - y;
			v.z = pt.z - z;
			FastNormalizeVector(v.x,v.y,v.z,&v);

			delta.x = v.x * (force * scale);
			delta.y = v.y * (force * scale);
			delta.z = v.z * (force * scale);
			
			
			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat()  * .5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;		
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}
}

/****************** MAKE STEAM ************************/

void MakeSteam(ObjNode *blob, float x, float y, float z)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

	if (gFramesPerSecond < 15.0f)
		return;

	blob->ParticleTimer -= fps;
	if (blob->ParticleTimer <= 0.0f)
	{
		blob->ParticleTimer += 0.02f;


		particleGroup 	= blob->ParticleGroup;
		magicNum 		= blob->ParticleMagicNum;
		
		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			blob->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num
			
			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_GRAVITOIDS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 90000;
			groupDef.baseScale				= 15;
			groupDef.decayRate				= -.5;
			groupDef.fadeRate				= .4;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			blob->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * (20.0f);
				p.y = y + RandomFloat2() * (20.0f);
				p.z = z + RandomFloat2() * (20.0f);

				d.x = RandomFloat2() * 190.0f;
				d.y = 150.0f + RandomFloat() * 190.0f;
				d.z = RandomFloat2() * 190.0f;
			
				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= .5;		
				if (AddParticleToGroup(&newParticleDef))
				{
					blob->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}



#pragma mark -


/************** MAKE BOMB EXPLOSION *********************/

void MakeBombExplosion(OGLPoint3D *where)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;

		/*********************/
		/* FIRST MAKE SPARKS */
		/*********************/
						
	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND | PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 300;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				= -1.5;
	gNewParticleGroupDef.fadeRate				= 2.0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = where->x;
		y = where->y;
		z = where->z;	

		for (i = 0; i < 20; i++)
		{
			d.x = RandomFloat2() * 150.0f;
			d.y = RandomFloat2() * 150.0f;
			d.z = RandomFloat2() * 150.0f;
			
			pt.x = x + (d.x * .4f);
			pt.y = y + (d.y * .4f);
			pt.z = z + (d.z * .4f);			
			
			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= RandomFloat() * PI2;
			newParticleDef.rotDZ		= RandomFloat2() * 10.0f;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);		
			AddParticleToGroup(&newParticleDef);
		}
	}
	

}




#pragma mark -

/********************* MAKE SPLASH ***********************/

void MakeSplash(float x, float y, float z, float scale)
{
long	pg,i;
OGLVector3D	delta;
OGLPoint3D	pt;
NewParticleDefType		newParticleDef;

	pt.y = y;

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 400;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 15.0f * scale;
	gNewParticleGroupDef.decayRate				=  -.6;
	gNewParticleGroupDef.fadeRate				= .8;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Splash;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

				
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 30; i++)
		{
			pt.x = x + RandomFloat2() * (30.0f * scale);
			pt.z = z + RandomFloat2() * (30.0f * scale);

			delta.x = RandomFloat2() * (200.0f * scale);
			delta.y = 100.0f + RandomFloat() * (150.0f * scale);
			delta.z = RandomFloat2() * (200.0f * scale);
			
			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2()*PI2;
			newParticleDef.alpha		= FULL_ALPHA;					
			AddParticleToGroup(&newParticleDef);
		}
	}

			/* PLAY SPLASH SOUND */

//	pt.x = x;
//	pt.z = z;
//	PlayEffect_Parms3D(EFFECT_SPLASH, &pt, NORMAL_CHANNEL_RATE, 1.5);
}


/***************** SPRAY WATER *************************/

void SprayWater(ObjNode *theNode, float x, float y, float z)
{			
float	fps = gFramesPerSecondFrac;
int		particleGroup, magicNum;

	theNode->ParticleTimer += fps;				// see if time to spew water
	if (theNode->ParticleTimer > 0.02f)
	{
		theNode->ParticleTimer += .02f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num
			
			gNewParticleGroupDef.magicNum				= magicNum;
			gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			gNewParticleGroupDef.flags					= 0;
			gNewParticleGroupDef.gravity				= 1400;
			gNewParticleGroupDef.magnetism				= 0;
			gNewParticleGroupDef.baseScale				= 35;
			gNewParticleGroupDef.decayRate				=  -1.7f;
			gNewParticleGroupDef.fadeRate				= 1.0;
			gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Splash;
			gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
			gNewParticleGroupDef.dstBlend				= GL_ONE;
			theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&gNewParticleGroupDef);
		}

	
					/* ADD PARTICLE TO GROUP */
					
		if (particleGroup != -1)
		{
			OGLPoint3D	pt;
			OGLVector3D delta;
			NewParticleDefType		newParticleDef;
						
			FastNormalizeVector(gDelta.x, 0, gDelta.z, &delta);			// calc spray delta shooting out of butt
			delta.x *= -300.0f;
			delta.z *= -300.0f;

			delta.x += (RandomFloat()-.5f) * 50.0f;						// spray delta
			delta.z += (RandomFloat()-.5f) * 50.0f;
			delta.y = 500.0f + RandomFloat() * 100.0f;

			pt.x = x + (RandomFloat()-.5f) * 80.0f;			// random noise to coord
			pt.y = y;
			pt.z = z + (RandomFloat()-.5f) * 80.0f;
			

			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;					
			AddParticleToGroup(&newParticleDef);
		}
	}
}





#pragma mark -



/****************** BURN FIRE ************************/

void BurnFire(ObjNode *theNode, float x, float y, float z, Boolean doSmoke,
			short particleType, float scale, uint32_t moreFlags)
{
float	fps = gFramesPerSecondFrac;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;


		/**************/
		/* MAKE SMOKE */
		/**************/

	if (doSmoke && (gFramesPerSecond > 20.0f))										// only do smoke if running at good frame rate
	{
		theNode->SmokeTimer -= fps;													// see if add smoke
		if (theNode->SmokeTimer <= 0.0f)
		{
			theNode->SmokeTimer += SMOKE_TIMER;										// reset timer
			
			long particleGroup 	= theNode->SmokeParticleGroup;
			uint32_t magicNum 	= (uint32_t) theNode->SmokeParticleMagic;
			
			if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
			{
				
				theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num
				
				groupDef.magicNum				= magicNum;
				groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
				groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|moreFlags;
				groupDef.gravity				= 0;
				groupDef.magnetism				= 0;
				groupDef.baseScale				= 20.0f * scale;
				groupDef.decayRate				=  -.2f;
				groupDef.fadeRate				= .2;
				groupDef.particleTextureNum		= PARTICLE_SObjType_BlackSmoke;
				groupDef.srcBlend				= GL_SRC_ALPHA;
				groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
				theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&groupDef);
			}
	
			if (particleGroup != -1)
			{
				for (int i = 0; i < 3; i++)
				{
					p.x = x + RandomFloat2() * (40.0f * scale);
					p.y = y + 200.0f + RandomFloat() * (50.0f * scale);
					p.z = z + RandomFloat2() * (40.0f * scale);
	
					d.x = RandomFloat2() * (20.0f * scale);
					d.y = 150.0f + RandomFloat() * (40.0f * scale);
					d.z = RandomFloat2() * (20.0f * scale);
				
					newParticleDef.groupNum		= particleGroup;
					newParticleDef.where		= &p;
					newParticleDef.delta		= &d;
					newParticleDef.scale		= RandomFloat() + 1.0f;
					newParticleDef.rotZ			= RandomFloat() * PI2;
					newParticleDef.rotDZ		= RandomFloat2();
					newParticleDef.alpha		= .7;		
					if (AddParticleToGroup(&newParticleDef))
					{
						theNode->SmokeParticleGroup = -1;
						break;
					}
				}
			}
		}
	}

		/*************/
		/* MAKE FIRE */
		/*************/

	theNode->FireTimer -= fps;													// see if add fire
	if (theNode->FireTimer <= 0.0f)
	{
		theNode->FireTimer += FIRE_TIMER;										// reset timer
		
		long particleGroup 	= theNode->ParticleGroup;
		uint32_t magicNum 	= theNode->ParticleMagicNum;
		
		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num
			
			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|moreFlags;
			groupDef.gravity				= -200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 30.0f * scale;
			groupDef.decayRate				=  0;
			groupDef.fadeRate				= .8;
			groupDef.particleTextureNum		= particleType;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (int i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * (30.0f * scale);
				p.y = y + RandomFloat() * (50.0f * scale);
				p.z = z + RandomFloat2() * (30.0f * scale);

				d.x = RandomFloat2() * (50.0f * scale);
				d.y = 50.0f + RandomFloat() * (60.0f * scale);
				d.z = RandomFloat2() * (50.0f * scale);
			
				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= 1.0;		
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}






#pragma mark -

/************** MAKE FIRE EXPLOSION *********************/

void MakeFireExplosion(OGLPoint3D *where)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;


		/*********************/
		/* FIRST MAKE FLAMES */
		/*********************/
						
	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND | PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 100;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 10;
	gNewParticleGroupDef.decayRate				=  -1.0;
	gNewParticleGroupDef.fadeRate				= 1.3;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = where->x;
		y = where->y;
		z = where->z;	


		for (i = 0; i < 30; i++)
		{
			pt.x = x + RandomFloat2() * 20.0f;
			pt.y = y + RandomFloat2() * 20.0f;
			pt.z = z + RandomFloat2() * 20.0f;

			d.y = RandomFloat2() * 100.0f;
			d.x = RandomFloat2() * 100.0f;
			d.z = RandomFloat2() * 100.0f;
			
			
			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 10.0f;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);		
			AddParticleToGroup(&newParticleDef);
		}
	}
	

}





#pragma mark -


/********************** MAKE SPLATTER ************************/

void MakeSplatter(OGLPoint3D *where, short modelObjType, float scale)
{
OGLVector3D	aim;
int		i;
ObjNode	*newObj;

	scale *= .6f;							// calc scale and see if too small
	if (scale < .1f)
		return;
	

	for (i = 0; i < 14; i++)
	{
				/* RANDOM AIM TO START WITH */
				
		aim.x = RandomFloat2();
		aim.y = RandomFloat() * 3.0f;
		aim.z = RandomFloat2();
		FastNormalizeVector(aim.x, aim.y, aim.z, &aim);
	
		gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
		gNewObjectDefinition.type 		= modelObjType;
		gNewObjectDefinition.coord.x	= where->x + aim.x * 25.0f;
		gNewObjectDefinition.coord.y	= where->y + aim.y * 25.0f;
		gNewObjectDefinition.coord.z	= where->z + aim.z * 25.0f;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_USEALIGNMENTMATRIX;
		gNewObjectDefinition.slot 		= 479;
		gNewObjectDefinition.moveCall 	= MoveBlobDroplet;	
		gNewObjectDefinition.rot 		= 0;	
		gNewObjectDefinition.scale 		= scale;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Special[0] = 0;						// init counter

			/* SET DELTAS */
		
		newObj->Delta.x = aim.x * 400.0f;
		newObj->Delta.y = aim.y * 400.0f;
		newObj->Delta.z = aim.z * 400.0f;

				/* SET THE ALIGNMENT MATRIX */

		SetAlignmentMatrix(&newObj->AlignmentMatrix, &aim);


			/* SET COLLISION */
			
		newObj->Damage 			= .1;
		newObj->CType 			= CTYPE_HURTME;

		CreateCollisionBoxFromBoundingBox_Maximized(newObj);
	}

}


/******************* MOVE BLOB DROPLET ********************/

static void MoveBlobDroplet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector3D	aim;


	GetObjectInfo(theNode);

			/* MOVE IT */
				
	gDelta.y -= 800.0f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SHRINK */
			
	theNode->Scale.x = 
	theNode->Scale.y = 
	theNode->Scale.z -= fps * .7f;
	if (theNode->Scale.x <= 0.0f)
	{
		DeleteObject(theNode);
		return;	
	}


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5f))
	{
//		theNode->Special[0]++;
//		if (theNode->Special[0] > 3)
//		{
//			ExplodeGeometry(theNode, 200, SHARD_MODE_FROMORIGIN, 3, .5);	
//			goto gone;
//		}
	}


		/* SET NEW ALIGNMENT & UPDATE */
		
	
	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &aim);
	SetAlignmentMatrix(&theNode->AlignmentMatrix, &aim);
	UpdateObject(theNode);
}


#pragma mark -

/******************** ADD SMOKER ****************************/

Boolean AddSmoker(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	newObj = MakeSmoker(x,z, itemPtr->parm[0]);		
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);
}


/****************** MAKE SMOKER **********************/

ObjNode *MakeSmoker(float  x, float z, int kind)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;				
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x,z, CTYPE_TERRAIN | CTYPE_WATER);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+10;
	gNewObjectDefinition.moveCall 	= MoveSmoker;
	newObj = MakeNewObject(&gNewObjectDefinition);
	
	newObj->Kind = kind;								// save smoke kind
	
	return(newObj);
}


/******************** MOVE SMOKER ************************/

static void MoveSmoker(ObjNode *theNode)
{
float				fps = gFramesPerSecondFrac;
int					i,t;
int					particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
short				smokeType;

static const short	textures[] =
{
	PARTICLE_SObjType_BlackSmoke,
	PARTICLE_SObjType_GreySmoke,
	PARTICLE_SObjType_RedFumes,
	PARTICLE_SObjType_GreenFumes
};

static const Boolean	glow[] =
{
	false,
	false,
	true,
	true
};

		/* SEE IF OUT OF RANGE */
		
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}
	
	
	theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z);		// make sure on ground (for when volcanos grow over it)
	

		/**************/
		/* MAKE SMOKE */
		/**************/

	theNode->Timer -= fps;													// see if add smoke
	if (theNode->Timer <= 0.0f)
	{
		theNode->Timer += .12f;												// reset timer
	
		t = textures[smokeType = theNode->Kind];										// get texture #
		
		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;
		
		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num
			
			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 35;
			groupDef.decayRate				= -.7;
			groupDef.fadeRate				= .18;
			groupDef.particleTextureNum		= t;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			if (glow[smokeType])
				groupDef.dstBlend				= GL_ONE;		
			else
				groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float				x,y,z;
			
			x = theNode->Coord.x;
			y = theNode->Coord.y;
			z = theNode->Coord.z;

			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 30.0f;
				p.y = y;
				p.z = z + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 60.0f;
				d.y = 110.0f + RandomFloat() * 75.0f;
				d.z = RandomFloat2() * 60.0f;
			
				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= RandomFloat2() * .4f;
				newParticleDef.alpha		= .6;		
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}


#pragma mark -

/*********************** MAKE GUN BLAST ****************************/

void MakeGunBlast(OGLPoint3D *where, OGLVector3D *aim)
{
long					pg,i;
OGLVector3D				delta,v;
NewParticleDefType		newParticleDef;
float					speed;

			/*****************************/
			/* CREATE NEW PARTICLE GROUP */
			/*****************************/
			
	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 200;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 12.0f;
	gNewParticleGroupDef.decayRate				= 0;
	gNewParticleGroupDef.fadeRate				= 2.0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark4;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
	
			/*******************/
			/* MAKE EACH SPARK */
			/*******************/
			
		for (i = 0; i < 30; i++)
		{
			/* CALC NEW VECTOR IN CONE OF AIM */
			
			OGLVector3D_RandomOffset(aim, .1, &v);
			speed = RandomFloat() * 500.0f;
		
			delta.x = v.x * speed;
			delta.y = v.y * speed;
			delta.z = v.z * speed;			
			
			newParticleDef.groupNum		= pg;
			newParticleDef.where		= where;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat()  * .5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;		
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}

}


/*************************** DO BULLET IMPACT ********************************/

void DoBulletImpact(const OGLPoint3D *entryPt, const OGLVector3D *splatVec, float scale)
{
long					pg,i;
OGLVector3D				delta,v;
NewParticleDefType		newParticleDef;
OGLMatrix4x4			m;
float					zrot,yrot, speed;
OGLPoint3D				splatPt = *entryPt;

			/*****************************/
			/* CREATE NEW PARTICLE GROUP */
			/*****************************/
			
	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_ALLAIM | PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 60;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 1.5f * scale;
	gNewParticleGroupDef.decayRate				= -1.5f;
	gNewParticleGroupDef.fadeRate				= 1.1;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_BloodSpat;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
	
			/*************************/
			/* MAKE EACH BLOOD SPECK */
			/*************************/
			
		for (i = 0; i < 150; i++)
		{
			/* CALC NEW VECTOR IN CONE OF AIM */
			
			zrot = RandomFloat2() * .15f;
			yrot = RandomFloat2() * .15f;
			OGLMatrix4x4_SetRotate_XYZ(&m, 0, yrot, zrot);
			OGLVector3D_Transform(splatVec, &m, &v);				
		
			speed = 30.0f + RandomFloat() * 200.0f * scale;
			delta.x = v.x * speed;
			delta.y = v.y * speed;
			delta.z = v.z * speed;			
			
			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &splatPt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat() * 1.5f;
			newParticleDef.rotZ			= RandomFloat() * PI2;
			newParticleDef.rotDZ		= RandomFloat() * 11.0f;
			newParticleDef.alpha		= FULL_ALPHA - RandomFloat() * .5f;		
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}
}


#pragma mark -

/************************* ADD FLAME *********************************/

Boolean AddFlame(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
		
				/* MAKE CUSTOM OBJECT */
											
	gNewObjectDefinition.genre		= CUSTOM_GENRE;				
	gNewObjectDefinition.slot 		= WATER_SLOT + 1;
	gNewObjectDefinition.moveCall 	= MoveFlame;
	gNewObjectDefinition.coord.x	= x;
	gNewObjectDefinition.coord.z	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.scale 		= 1;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_GLOW | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES;
	
	newObj = MakeNewObject(&gNewObjectDefinition);		
	newObj->CustomDrawFunction = DrawFlame;		

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->FlameFrame = RandomRange(0, 10);						// start on random frame
	newObj->FlameSpeed = 15.0f + RandomFloat() * 5.0f;				// set anim speed
	newObj->Timer = 0;

	return(true);													// item was added
}


/******************* MOVE FLAME ****************************/

static void MoveFlame(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* NEXT FRAME */
			
	theNode->Timer -= theNode->FlameSpeed * gFramesPerSecondFrac;
	if (theNode->Timer <= 0.0f)
	{
		theNode->Timer += 1.0f;
		theNode->FlameFrame++;
		if (theNode->FlameFrame > 10)
			theNode->FlameFrame = 0;	
	}


}


/****************** DRAW FLAME ***********************/

static void DrawFlame(ObjNode *theNode)
{
OGLMatrix4x4	m;
const OGLVector3D	up = {0,1,0};
const OGLPoint3D	frame[4] =
{
	{-100,200,0},
	{100,200,0},
	{100,0,0},
	{-100,0,0},
};
OGLPoint3D	verts[4];


			/* CALC VERTEX COORDS */
			
	SetLookAtMatrixAndTranslate(&m, &up, &theNode->Coord, &gGameViewInfoPtr->cameraPlacement.cameraLocation);	// aim at camera & translate
	OGLPoint3D_TransformArray(&frame[0], &m, verts, 4);

			/* SUBMIT TEXTURE */
	
//	theNode->FlameFrame = 0;	//--------
			
	gGlobalTransparency = 1.0f;
	gGlobalColorFilter.r = 1.0;
	gGlobalColorFilter.g = 0.7;
	gGlobalColorFilter.b = .6;
			
	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][PARTICLE_SObjType_Flame0 + theNode->FlameFrame].materialObject);


			/* DRAW QUAD */

	glBegin(GL_QUADS);
	glTexCoord2f(0.00f, 0.00f);		glVertex3fv(&verts[0].x);
	glTexCoord2f(0.99f, 0.00f);		glVertex3fv(&verts[1].x);
	glTexCoord2f(0.99f, 0.99f);		glVertex3fv(&verts[2].x);
	glTexCoord2f(0.00f, 0.99f);		glVertex3fv(&verts[3].x);
	glEnd();

	gGlobalTransparency = 1.0f;
	
	gGlobalColorFilter.r =gGlobalColorFilter.g = gGlobalColorFilter.b = 1;
}












