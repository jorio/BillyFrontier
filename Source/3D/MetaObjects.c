/****************************/
/*   METAOBJECTS.C		    */
/* (c)2000 Pangea Software  */
/*   By Brian Greenstone    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

//#include <aglmacro.h>

#include "game.h"
#include "3dmath.h"
#include "infobar.h"

extern	Boolean			gMuteMusicFlag;
extern	float			gCurrentAspectRatio;
extern	SpriteType		*gSpriteGroupList[];
extern	long			gNumSpritesInGroupList[];
extern	int				gPolysThisFrame,gVRAMUsedThisFrame;
extern	Boolean			gMyState_Lighting, gIsPicking;
extern	SDL_GLContext		gAGLContext;
extern	Byte			gDebugMode;
extern	PrefsType			gGamePrefs;
extern	OGLMatrix4x4	gWorldToFrustumMatrix,gWorldToViewMatrix,gViewToFrustumMatrix;
extern	OGLMatrix4x4	*gCurrentObjMatrix;
extern	CGrafPtr				gDisplayContextGrafPtr;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];

/****************************/
/*    PROTOTYPES            */
/****************************/

static MetaObjectPtr AllocateEmptyMetaObject(u_long type, u_long subType);
static void SetMetaObjectToGroup(MOGroupObject *groupObj);
static void SetMetaObjectToGeometry(MetaObjectPtr mo, u_long subType, void *data);
static void SetMetaObjectToMaterial(MOMaterialObject *matObj, MOMaterialData *inData);
static void SetMetaObjectToVertexArrayGeometry(MOVertexArrayObject *geoObj, MOVertexArrayData *data);
static void SetMetaObjectToMatrix(MOMatrixObject *matObj, OGLMatrix4x4 *inData);
static void SetMetaObjectToPickID(MOPickIDObject *pickObj, u_long pickID);
static void MO_DetachFromLinkedList(MetaObjectPtr obj);
static void MO_DisposeObject_Group(MOGroupObject *group);
static void MO_DeleteObjectInfo_Material(MOMaterialObject *obj);
static void MO_CalcBoundingBox_Recurse(MetaObjectPtr object, OGLBoundingBox *bBox, const OGLMatrix4x4 *m);
static void SetMetaObjectToPicture(MOPictureObject *pictObj, OGLSetupOutputType *setupInfo, FSSpec *inData, int destPixelFormat);
static void MO_DeleteObjectInfo_Picture(MOPictureObject *obj);

static void SetMetaObjectToSprite(MOSpriteObject *spriteObj, OGLSetupOutputType *setupInfo, MOSpriteSetupData *inData);
static void MO_DisposeObject_Sprite(MOSpriteObject *obj);

static void MO_CalcBoundingSphere_Recurse(MetaObjectPtr object, float *bSphere);


/****************************/
/*    CONSTANTS             */
/****************************/





/*********************/
/*    VARIABLES      */
/*********************/

MetaObjectHeader	*gFirstMetaObject = nil;
MetaObjectHeader 	*gLastMetaObject = nil;
int					gNumMetaObjects = 0;

float				gGlobalTransparency = 1;			// 0 == clear, 1 = opaque
OGLColorRGB			gGlobalColorFilter = {1,1,1};
u_long				gGlobalMaterialFlags = 0;

MOMaterialObject	*gMostRecentMaterial;

u_long				gCurrentPickID = 0;


/***************** INIT META OBJECT HANDLER ******************/

void MO_InitHandler(void)
{
	gFirstMetaObject = nil;				// no meta object nodes yet
	gLastMetaObject = nil;
	gNumMetaObjects = 0;
}


#pragma mark -

/****************** MO: CREATE NEW OBJECT OF TYPE ****************/
//
// INPUT:	type = type of mo to create
//			subType = subtype to create (optional)
//			data = pointer to any data needed to create the mo (optional)
//


MetaObjectPtr	MO_CreateNewObjectOfType(u_long type, u_long subType, void *data)
{
MetaObjectPtr	mo;

			/* ALLOCATE EMPTY OBJECT */
			
	mo = AllocateEmptyMetaObject(type,subType);
	if (mo == nil)
		return(nil);


			/* SET OBJECT INFO */
			
	switch(type)
	{
		case	MO_TYPE_GROUP:
				SetMetaObjectToGroup(mo);
				break;
				
		case	MO_TYPE_GEOMETRY:
				SetMetaObjectToGeometry(mo, subType, data);
				break;
				
		case	MO_TYPE_MATERIAL:
				SetMetaObjectToMaterial(mo, data);
				break;
				
		case	MO_TYPE_MATRIX:
				SetMetaObjectToMatrix(mo, data);
				break;

		case	MO_TYPE_PICTURE:
				if (gGamePrefs.depth == 16)		// picture depth depends on display depth (no point in doing 32 bit if display is 16)
					SetMetaObjectToPicture(mo, (OGLSetupOutputType *)subType, data, GL_RGB5_A1);				
				else
					SetMetaObjectToPicture(mo, (OGLSetupOutputType *)subType, data, GL_RGB);
				break;
				
		case	MO_TYPE_SPRITE:
				SetMetaObjectToSprite(mo, (OGLSetupOutputType *)subType, data);
				break;

		case	MO_TYPE_PICKID:
				SetMetaObjectToPickID(mo, subType);
				break;
				
		default:
				DoFatalAlert("\pMO_CreateNewObjectOfType: object type not recognized");
	}
	
	return(mo);
}


/****************** ALLOCATE EMPTY META OBJECT *****************/
//
// allocates an empty meta object and connects it to the linked list
//

static MetaObjectPtr AllocateEmptyMetaObject(u_long type, u_long subType)
{
MetaObjectHeader	*mo;
int					size;

			/* DETERMINE SIZE OF DATA TO ALLOC */
			
	switch(type)
	{
		case	MO_TYPE_GROUP:
				size = sizeof(MOGroupObject);
				break;
				
		case	MO_TYPE_GEOMETRY:
				switch(subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							size = sizeof(MOVertexArrayObject);
							break;
							
					default:
							DoFatalAlert("\pAllocateEmptyMetaObject: object subtype not recognized");
				}
				break;
				
		case	MO_TYPE_MATERIAL:
				size = sizeof(MOMaterialObject);
				break;
				
		case	MO_TYPE_MATRIX:
				size = sizeof(MOMatrixObject);
				break;

		case	MO_TYPE_PICTURE:
				size = sizeof(MOPictureObject);
				break;
				
		case	MO_TYPE_SPRITE:
				size = sizeof(MOSpriteObject);
				break;
				
		case	MO_TYPE_PICKID:
				size = sizeof(MOPickIDObject);
				break;
				
		default:
				DoFatalAlert("\pAllocateEmptyMetaObject: object type not recognized");
	}
			

			/* ALLOC MEMORY FOR META OBJECT */
			
	mo = AllocPtr(size);	
	if (mo == nil)
		DoFatalAlert("\pAllocateEmptyMetaObject: AllocPtr failed!");


			/* INIT STRUCTURE */
			
	mo->cookie 		= MO_COOKIE;
	mo->type 		= type;
	mo->subType 	= subType;
	mo->data 		= nil;
	mo->nextNode 	= nil;		
	mo->refCount	= 1;							// initial reference count is always 1
	mo->parentGroup = nil;


		/***************************/
		/* ADD NODE TO LINKED LIST */
		/***************************/

		/* SEE IF IS ONLY NODE */
					
	if (gFirstMetaObject == nil)
	{
		if (gLastMetaObject)
			DoFatalAlert("\pAllocateEmptyMetaObject: gFirstMetaObject & gLastMetaObject should be nil");
	
		mo->prevNode = nil;
		gFirstMetaObject = gLastMetaObject = mo;
		gNumMetaObjects = 1;
	}
	
		/* ADD TO END OF LINKED LIST */
		
	else
	{		
		mo->prevNode = gLastMetaObject;		// point new prev to last
		gLastMetaObject->nextNode = mo;		// point old last to new
		gLastMetaObject = mo;				// set new last
		gNumMetaObjects++;
	}

	return(mo);
}

/******************* SET META OBJECT TO GROUP ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//

static void SetMetaObjectToGroup(MOGroupObject *groupObj)
{
		
			/* INIT THE DATA */
			
	groupObj->objectData.numObjectsInGroup = 0;
}


/***************** SET META OBJECT TO GEOMETRY ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//

static void SetMetaObjectToGeometry(MetaObjectPtr mo, u_long subType, void *data)
{
	switch(subType)
	{
		case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
				SetMetaObjectToVertexArrayGeometry(mo, data);
				break;
				
		default:
				DoFatalAlert("\pSetMetaObjectToGeometry: unknown subType");

	}
}


/*************** SET META OBJECT TO VERTEX ARRAY GEOMETRY ***************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.  It also boosts the ref count of
// any referenced items.
//

static void SetMetaObjectToVertexArrayGeometry(MOVertexArrayObject *geoObj, MOVertexArrayData *data)
{
int	i;
		
			/* INIT THE DATA */
		
	geoObj->objectData = *data;									// copy from input data		

	
		/* INCREASE MATERIAL REFERENCE COUNTS */
			
	for (i = 0; i < data->numMaterials; i++)
	{
		if (data->materials[i] != nil)				// make sure this material ref is valid
			MO_GetNewReference(data->materials[i]);
	}		
		
}


/******************* SET META OBJECT TO MATERIAL ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it. 
//

static void SetMetaObjectToMaterial(MOMaterialObject *matObj, MOMaterialData *inData)
{	
	
		/* COPY INPUT DATA */
		
	matObj->objectData = *inData;

#if 0
		{
			AGLContext agl_ctx = gGameViewInfoPtr->drawContext;

			MO_DrawMaterial(matObj, gGameViewInfoPtr);		// safety prime ----------
			glBegin(GL_TRIANGLES);
			glTexCoord2f(0,0); glVertex3f(0,0,0);
			glTexCoord2f(1,0); glVertex3f(0,100,0);
			glTexCoord2f(0,1); glVertex3f(0,0,1000);
			glEnd();
		}	
#endif		

}


/******************* SET META OBJECT TO MATRIX ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it. 
//

static void SetMetaObjectToMatrix(MOMatrixObject *matObj, OGLMatrix4x4 *inData)
{	
	
		/* COPY INPUT DATA */
		
	matObj->matrix = *inData;
}


/******************* SET META OBJECT TO PICTURE ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it. 
//

static void SetMetaObjectToPicture(MOPictureObject *pictObj, OGLSetupOutputType *setupInfo, FSSpec *inData, int destPixelFormat)
{
GWorldPtr	gworld;
int			width,height,depth,cellNum,numCells;
int			horizCellSize, vertCellSize,segRow,segCol;
int			numHorizCells, numVertCells;
PixMapHandle 	hPixMap;
int			bytesPerPixel;
MOPictureData	*picData = &pictObj->objectData;
Ptr			buffer,pictMapAddr;
u_long		bufferRowBytes,pictRowBytes;
MOMaterialData	matData;
Rect		r;

		/* LOAD PICTURE INTO GWORLD */

	if (DrawPictureIntoGWorld(inData , &gworld, 32))
		DoFatalAlert("\pSetMetaObjectToPicture: DrawPictureIntoGWorld failed!");


			/* GET GWORLD INFO */

	GetPortBounds(gworld, &r);


	width = r.right - r.left;		// get width/height
	height = r.bottom - r.top;

	hPixMap = GetGWorldPixMap(gworld);							// get gworld's pixmap
	pictMapAddr = GetPixBaseAddr(hPixMap);
	pictRowBytes = (u_long)(**hPixMap).rowBytes & 0x3fff;

	depth = (*hPixMap)->pixelSize;								// get pixel bitdepth
	if (depth == 32)
		bytesPerPixel = 4;
	else
		bytesPerPixel = 2;


		/***********************************/
		/* DETERMINE BEST SIZE OF SEGMENTS */
		/***********************************/

			/* DO WIDTH */
	
	switch(width)
	{
		case	32:
				horizCellSize = 32;
				numHorizCells = 1;
				break;

		case	64:
				horizCellSize = 64;
				numHorizCells = 1;
				break;

		case	320:
				horizCellSize = 64;
				numHorizCells = 5;
				break;

		case	640:
				horizCellSize = 128;
				numHorizCells = width/horizCellSize;
				break;

		case	1024:
				horizCellSize = 256;
				numHorizCells = width/horizCellSize;
				break;
				
		default:
				DoFatalAlert("\pSetMetaObjectToPicture: this width not implemented yet");
	}

			/* DO HEIGHT */
	
	switch(height)
	{
		case	32:
				vertCellSize = 32;
				numVertCells = 1;
				break;

		case	64:
				vertCellSize = 64;
				numVertCells = 1;
				break;

		case	480:
				vertCellSize = 32;
				numVertCells = height/vertCellSize;
				break;

		case	768:
				vertCellSize = 256;
				numVertCells = height/vertCellSize;
				break;
				
		default:
				DoFatalAlert("\pSetMetaObjectToPicture: this height not implemented yet");
	}
	
	
			/********************************/
			/* SET SOME PICTURE OBJECT DATA */
			/********************************/

	picData->drawCoord.x	= -1.0;						// assume upper left corner
	picData->drawCoord.y	= 1.0;	
	picData->drawCoord.z	= .999;						// assume in back
	picData->drawScaleX		= 1.0;						// scale is normal
	picData->drawScaleY		= 1.0;						

	picData->fullWidth 		= width;
	picData->fullHeight		= height;
	picData->numCellsWide 	= numHorizCells;
	picData->numCellsHigh 	= numVertCells;
	picData->cellWidth 		= horizCellSize;
	picData->cellHeight 	= vertCellSize;
	
	numCells = numHorizCells * numVertCells;
	
	picData->materials 		= (MOMaterialObject **)AllocPtr(sizeof(MOMaterialObject *) * numCells);	// alloc array of material objects
	if (picData->materials == nil)
		DoFatalAlert("\pSetMetaObjectToPicture: AllocPtr failed!");


		/*************************************/
		/* CREATE TEXTURES FOR EACH SEGMENT  */
		/*************************************/

		/* ALLOCATE SEGMENT BUFFER */
		
	buffer = AllocPtr(horizCellSize * vertCellSize * bytesPerPixel);
	if (buffer == nil)
		DoFatalAlert("\pSetMetaObjectToPicture: AllocPtr failed!");
	bufferRowBytes = horizCellSize * bytesPerPixel;

			
	cellNum = 0;
			
	for (segRow = 0; segRow < numVertCells; segRow++)
	{
		for (segCol = 0; segCol < numHorizCells; segCol++)
		{
			Ptr		srcPtr,destPtr;
			
			srcPtr = pictMapAddr + (segRow * vertCellSize * pictRowBytes) + (segCol * horizCellSize * bytesPerPixel);
			destPtr = buffer + (vertCellSize-1) * bufferRowBytes;				// start @ bottom of buffer to flip image for OpenGL


				/*********************************/
				/* COPY THIS SEGMENT INTO BUFFER */
				/*********************************/

			if (depth == 32)
			{
				u_long	r,g,b,a,x,y;	
				u_long	pixels, *dest = (u_long *)destPtr, *src = (u_long *)srcPtr;

				for (y = 0; y < vertCellSize; y++)
				{
					for (x = 0; x < horizCellSize; x++)							// copy & convert a line of pixels
					{				
						pixels = src[x];										// get pixel

//						if (pixels == 0)
//							a = 0;
//						else
							a = 0xff;											// pictures are opqaue
								
						r = (pixels & 0x00ff0000) >> 16;
						g = (pixels & 0x0000ff00) >> 8;
						b = pixels & 0xff;
						
						dest[x] = (r << 24) | (g << 16) | (b << 8) | a;			// save pixel
					}
					dest -= bufferRowBytes/4;
					src += pictRowBytes/4;
				}
			}
			else
			{
				DoFatalAlert("\pSetMetaObjectToPicture: 16-bit not implemented yet");
			}
			
					/***************************/
					/* CREATE A TEXTURE OBJECT */
					/***************************/

			matData.setupInfo		= setupInfo;
			matData.flags			= BG3D_MATERIALFLAG_TEXTURED|BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V;
			matData.diffuseColor.r	= 1;
			matData.diffuseColor.g	= 1;
			matData.diffuseColor.b	= 1;
			matData.diffuseColor.a	= 1;
			
			matData.numMipmaps		= 1;
			matData.width			= horizCellSize;
			matData.height			= vertCellSize;
			
			switch(depth)
			{
				case	32:
						matData.pixelSrcFormat	= GL_RGBA;
						break;
						
			 	default:
						matData.pixelSrcFormat	= GL_RGB;
			}
			
			matData.pixelDstFormat 	= destPixelFormat;
			matData.texturePixels[0]= nil;						// we're going to preload
			matData.textureName[0] 	= OGL_TextureMap_Load(buffer, horizCellSize, vertCellSize,
														 matData.pixelSrcFormat, destPixelFormat, GL_UNSIGNED_BYTE);
			
			picData->materials[cellNum] = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
			cellNum++;	
			
			if (cellNum > numCells)
				DoFatalAlert("\pSetMetaObjectToPicture: cellNum overflow");
				
		}
	}


			/* CLEANUP */
			
	DisposeGWorld (gworld);	
	SafeDisposePtr(buffer);	
	
}


/******************* SET META OBJECT TO SPRITE ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it. 
//

static void SetMetaObjectToSprite(MOSpriteObject *spriteObj, OGLSetupOutputType *setupInfo, MOSpriteSetupData *inData)
{
MOSpriteData	*spriteData = &spriteObj->objectData;


		/* CREATE MATERIAL OBJECT FROM FSSPEC */

	if (inData->loadFile)
	{			
		GLint	destPixelFormat = inData->pixelFormat;									// use passed in format
		
		spriteData->material = MO_GetTextureFromFile(&inData->spec, setupInfo, destPixelFormat);

		spriteData->width = spriteData->material->objectData.width;						// get dimensions of the texture
		spriteData->height = spriteData->material->objectData.width;
		spriteData->aspectRatio = spriteData->height / spriteData->width;				// calc aspect ratio
	}
	
			/* GET MATERIAL FROM SPRITE LIST */
	else
	{
		short	group,type;
	
		group = inData->group;
		type = inData->type;
	
		if (inData->type >= gNumSpritesInGroupList[group])								// make sure type is valid
			DoFatalAlert("\pSetMetaObjectToSprite: illegal type");
	
		spriteData->material = gSpriteGroupList[group][type].materialObject;
		MO_GetNewReference(spriteData->material);										// this is a new reference, so inc ref count

		spriteData->width 		= gSpriteGroupList[group][type].width;					// get width and height of texture
		spriteData->height 		= gSpriteGroupList[group][type].height;
		spriteData->aspectRatio = gSpriteGroupList[group][type].aspectRatio;			// get aspect ratio		
	}


			/*******************************/
			/* SET SOME SPRITE OBJECT DATA */
			/*******************************/

	spriteData->scaleBasis = spriteData->width / SPRITE_SCALE_BASIS_DENOMINATOR;		// calculate a scale basis to keep things scaled relative to texture size


	spriteData->coord.x		= -1.0;								// assume upper left corner
	spriteData->coord.y		= 1.0;	
	spriteData->coord.z		= 0;								// assume in front
	spriteData->scaleX		= 1.0;								// scale is normal
	spriteData->scaleY		= 1.0;
	spriteData->rot			= 0;								// rot
}

/******************* SET META OBJECT TO PICK ID ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it. 
//

static void SetMetaObjectToPickID(MOPickIDObject *pickObj, u_long pickID)
{	
	
		/* COPY INPUT DATA */
		
	pickObj->pickID = pickID;
}




#pragma mark -

/*************** SET PICTURE OBJECT COORDS TO MOUSE *******************/

void MO_SetPictureObjectCoordsToMouse(OGLSetupOutputType *info, MOPictureObject *obj)
{
MOPictureData	*picData = &obj->objectData;				//  point to pic obj's data
Point			pt;
int				x,y,w,h;

	SetPort(gDisplayContextGrafPtr);
	GetMouseCoord(&pt);										// get mouse screen coords	

			/* CONVERT SCREEN COORD TO OPENGL COORD */
			
	OGL_GetCurrentViewport(info, &x, &y, &w, &h);
	
	
	picData->drawCoord.x = -1.0f + (float)pt.h / (float)w * 2.0f;
	picData->drawCoord.y = 1.0f - (float)pt.v / (float)h * 2.0f;

}


#pragma mark -


/********************** MO: APPEND TO GROUP ****************/
//
// Attach new object to end of group
//

void MO_AppendToGroup(MOGroupObject *group, MetaObjectPtr newObject)
{
int	i;

			/* VERIFY COOKIE */

	if ((group->objectHeader.cookie != MO_COOKIE) || (((MetaObjectHeader *)newObject)->cookie != MO_COOKIE))
		DoFatalAlert("\pMO_AppendToGroup: cookie is invalid!");


	i = group->objectData.numObjectsInGroup++;		// get index into group list
	if (i >= MO_MAX_ITEMS_IN_GROUP)
		DoFatalAlert("\pMO_AppendToGroup: too many objects in group!");

	MO_GetNewReference(newObject);					// get new reference to object
	group->objectData.groupContents[i] = newObject;	// save object reference in group
}

/**************** MO: ATTACH TO GROUP START **************/
//
// Attach new object to START of group
//

void MO_AttachToGroupStart(MOGroupObject *group, MetaObjectPtr newObject)
{
int	i,j;

			/* VERIFY COOKIE */

	if ((group->objectHeader.cookie != MO_COOKIE) || (((MetaObjectHeader *)newObject)->cookie != MO_COOKIE))
		DoFatalAlert("\pMO_AttachToGroupStart: cookie is invalid!");


	i = group->objectData.numObjectsInGroup++;		// get index into group list
	if (i >= MO_MAX_ITEMS_IN_GROUP)
		DoFatalAlert("\pMO_AttachToGroupStart: too many objects in group!");

	MO_GetNewReference(newObject);					// get new reference to object
	
	
			/* SHIFT ALL EXISTING OBJECTS UP */
			
	for (j = i; j > 0; j--)
		group->objectData.groupContents[j] = group->objectData.groupContents[j-1];
	
	group->objectData.groupContents[0] = newObject;	// save object ref into group
}


#pragma mark -


/******************** MO: DRAW OBJECT ***********************/
//
// This recursive function will draw any objects submitted and parses groups.
//

void MO_DrawObject(const MetaObjectPtr object, const OGLSetupOutputType *setupInfo)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("\pMO_DrawObject: cookie is invalid!");


			/* HANDLE TYPE */

	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vObj = object;
							MO_DrawGeometry_VertexArray(&vObj->objectData, setupInfo);				
							break;
							
					default:	
						DoFatalAlert("\pMO_DrawObject: unknown sub-type!");
				}
				break;
	
		case	MO_TYPE_MATERIAL:
				MO_DrawMaterial(object, setupInfo);				
				break;

		case	MO_TYPE_GROUP:
				MO_DrawGroup(object, setupInfo);				
				break;
				
		case	MO_TYPE_MATRIX:
				MO_DrawMatrix(object, setupInfo);
				break;
				
		case	MO_TYPE_PICTURE:
				MO_DrawPicture(object, setupInfo);
				break;

		case	MO_TYPE_SPRITE:
				MO_DrawSprite(object, setupInfo);
				break;

		case	MO_TYPE_PICKID:
				if (gIsPicking)
				{
					SDL_GLContext agl_ctx = setupInfo->drawContext;
				
					MOPickIDObject *pickObj = (MOPickIDObject *)object;
					gCurrentPickID = pickObj->pickID;
					glLoadName(gCurrentPickID);
				}
				break;
				
		default:
			DoFatalAlert("\pMO_DrawObject: unknown type!");		
	}
}


/******************** MO_DRAW GROUP *************************/

void MO_DrawGroup(const MOGroupObject *object, const OGLSetupOutputType *setupInfo)
{
int	numChildren,i;

				/* VERIFY OBJECT TYPE */
				
	if (object->objectHeader.type != MO_TYPE_GROUP)
		DoFatalAlert("\pMO_DrawGroup: this isnt a group!");		


			/*************************************/
			/* PUSH CURRENT STATE ON STATE STACK */
			/*************************************/


	OGL_PushState();


				/***************/
				/* PARSE GROUP */
				/***************/

	numChildren = object->objectData.numObjectsInGroup;			// get # objects in group
	
	for (i = 0; i < numChildren; i++)
	{
		MO_DrawObject(object->objectData.groupContents[i], setupInfo);
	}


			/******************************/
			/* POP OLD STATE OFF OF STACK */
			/******************************/
			
	OGL_PopState();
}


/******************** MO: DRAW GEOMETRY - VERTEX ARRAY *************************/

void MO_DrawGeometry_VertexArray(const MOVertexArrayData *data, const OGLSetupOutputType *setupInfo)
{
Boolean		useTexture = false, multiTexture = false, texGen = false;
SDL_GLContext 	agl_ctx = setupInfo->drawContext;
u_long 		materialFlags;
short		i;
Boolean		needNormals;
	
			/**********************/
			/* SETUP VERTEX ARRAY */
			/**********************/
			
	glEnableClientState(GL_VERTEX_ARRAY);				// enable vertex arrays
	glVertexPointer(3, GL_FLOAT, 0, data->points);		// point to points array



			/***********************/
			/* SETUP VERTEX COLORS */
			/***********************/

			/* IF LIGHTING, THEN USE COLOR FLOATS */
			
	if (gMyState_Lighting)
	{
		if (data->colorsFloat)									// do we have float colors?
		{
			glColorPointer(4, GL_FLOAT, 0, data->colorsFloat);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
		if (data->colorsByte)									// no floats, so check bytes
		{
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, data->colorsByte);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
			glDisableClientState(GL_COLOR_ARRAY);				// no color data at all, so disable
	}
	
			/* IF NOT LIGHTING, THEN USE COLOR BYTES */
			
	else
	{		
		if (data->colorsByte)									// do we have byte colors?
		{
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, data->colorsByte);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
		if (data->colorsFloat)									// no bytes, so check floats
		{
			glColorPointer(4, GL_FLOAT, 0, data->colorsFloat);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
			glDisableClientState(GL_COLOR_ARRAY);				// no color data at all, so disable
	}

	if (OGL_CheckError())
		DoFatalAlert("\pMO_DrawGeometry_VertexArray: color!");





			/****************************/
			/* SEE IF ACTIVATE MATERIAL */
			/****************************/
			//
			// For now, I'm just looking at material #0.
			//


	if (data->numMaterials < 0)							// if (-), then assume texture has been manually set
	{
		goto use_current;
	}

	if (data->numMaterials > 0)									// are there any materials?	
	{
				/*************************************************/
				/* SEE IF DO PLAIN MULTI-TEXTURING FROM GEOMETRY */
				/*************************************************/

		if (data->numMaterials > 1)
		{
			useTexture = multiTexture = true;

			for (i = 0 ;i < data->numMaterials; i++)
			{
				glActiveTextureARB(GL_TEXTURE0_ARB+i);								// activate texture layer #i
				glClientActiveTextureARB(GL_TEXTURE0_ARB+i);
				glEnable(GL_TEXTURE_2D);				

				glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[i]);						// enable uv arrays
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);					

				MO_DrawMaterial(data->materials[i], setupInfo);						// submit material #n
				
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				if (i == 0)
				{
					gMostRecentMaterial = nil;							// need duplicate submits to be okay
				}
			}		
			
			goto go_here;
		}
		
		
				/* MAYBE ONLY 1 MATERIAL IN GEOMETRY */
		
		MO_DrawMaterial(data->materials[0], setupInfo);			// submit material #0 (also applies for multitexture layer 0)


			/* IF TEXTURED, THEN ALSO ACTIVATE UV ARRAY */

use_current:		
			
		materialFlags = gMostRecentMaterial->objectData.flags;	// get material flags
		if (materialFlags & BG3D_MATERIALFLAG_TEXTURED)
		{
			if (data->uvs[0])
			{
						/***************************/
						/* SEE IF DO MULTI-TEXTURE */
						/***************************/
						
				if (materialFlags & BG3D_MATERIALFLAG_MULTITEXTURE)	
				{
					u_short	multiTextureMode 	= gMostRecentMaterial->objectData.multiTextureMode;
					u_short	multiTextureCombine = gMostRecentMaterial->objectData.multiTextureCombine;
					u_short	envMapNum 		= gMostRecentMaterial->objectData.envMapNum;
					
					if (envMapNum >= gNumSpritesInGroupList[SPRITE_GROUP_SPHEREMAPS])
						DoFatalAlert("\pMO_DrawGeometry_VertexArray: illegal envMapNum");

					multiTexture = true;


					switch(multiTextureMode)
					{
								/* REFLECTION SPHERE */
								
						case	MULTI_TEXTURE_MODE_REFLECTIONSPHERE:
								for (i = 0 ;i < 2; i++)
								{
									glActiveTextureARB(GL_TEXTURE0_ARB+i);								// activate texture layer #i
									glClientActiveTextureARB(GL_TEXTURE0_ARB+i);
									glEnable(GL_TEXTURE_2D);				
							
									if (i == 0)
									{
										glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[0]);					// enable uv arrays
										glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
										glEnableClientState(GL_TEXTURE_COORD_ARRAY);					
									}
									else
									{										
										MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_SPHEREMAPS][envMapNum].materialObject, setupInfo);		// activate reflection map texture

#if USE_GL_COLOR_MATERIAL
										glEnable(GL_COLOR_MATERIAL);
#endif										
										
										switch(multiTextureCombine)														// set combining info
										{
											case	MULTI_TEXTURE_COMBINE_MODULATE:
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
													break;

											case	MULTI_TEXTURE_COMBINE_ADD:
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
													break;

											case	MULTI_TEXTURE_COMBINE_ADDALPHA:										// note, when we do this gGlobalTransparency will have no effect
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
													break;
										}
										
										glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);							// activate reflection mapping
										glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);		
										glEnable(GL_TEXTURE_GEN_S);
										glEnable(GL_TEXTURE_GEN_T);
										texGen = true;
									}
								}		
								break;
								
								
					}	
				}
							/* JUST 1 TEXTURE LAYER */
				else
				{			
					glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[0]);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);	// enable uv arrays
				}
				
				useTexture = true;

				if (OGL_CheckError())
					DoFatalAlert("\pMO_DrawGeometry_VertexArray: uv!");
			}			
		}
	}	
	else
		glDisable(GL_TEXTURE_2D);						// no materials, thus no texture, thus turn this off
		

		/* WE DONT HAVE ENOUGH INFO TO DO TEXTURES, SO DISABLE */
								
	if (!useTexture)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if (OGL_CheckError())
			DoFatalAlert("\pMO_DrawGeometry_VertexArray: glDisableClientState(GL_TEXTURE_COORD_ARRAY)!");
	}

go_here:

			/************************/
			/* SETUP VERTEX NORMALS */
			/************************/
			//
			// We do this last because we need to know some things
			// before we can determine if normals are actually needed
			//
			
	if (data->normals == nil)							// see if we even have normals to pass
		needNormals = false;
	else
	{
		if (!gMyState_Lighting)							// if no lighting, then probably don't need to pass normals
			needNormals = texGen;						// pass normals if doing texGen for sphere maps, etc.
		else											// there's lighting, so pass the normals
			needNormals = true;
	}
	
	if (needNormals)
	{
		glNormalPointer(GL_FLOAT, 0, data->normals);
		glEnableClientState(GL_NORMAL_ARRAY);			// enable normal arrays
		
#if 0
			/* SHOW VERTEX NORMALS */
		{
			int	i;
			
			OGL_PushState();
			glDisable(GL_TEXTURE_2D);
			SetColor4f(1,1,0,1);
			for (i = 0; i < data->numPoints; i++)
			{
				glBegin(GL_LINES);
		
				glVertex3fv((GLfloat *)&data->points[i]);
				glVertex3f(data->points[i].x + data->normals[i].x * 20.0f,
							data->points[i].y + data->normals[i].y * 20.0f,
							data->points[i].z + data->normals[i].z * 20.0f);
		
				glEnd();
			}
			OGL_PopState();
		}
#endif		
		
	}
	else
		glDisableClientState(GL_NORMAL_ARRAY);			// disable normal arrays

	if (OGL_CheckError())
		DoFatalAlert("\pMO_DrawGeometry_VertexArray: normals!");


			/***********/
			/* DRAW IT */
			/***********/
		
//	glLockArraysEXT(0, data->numPoints);
	glDrawElements(GL_TRIANGLES,data->numTriangles*3,GL_UNSIGNED_INT,&data->triangles[0]);

	if (OGL_CheckError())
		DoFatalAlert("\pMO_DrawGeometry_VertexArray: glDrawElements");
//	glUnlockArraysEXT();

	gPolysThisFrame += data->numTriangles;					// inc poly counter
	
	
			/* CLEANUP */
			
	if (multiTexture)
	{
		glActiveTextureARB(GL_TEXTURE1_ARB);			// turn off textureing for multi-texture layer 2 since it isnt needed anymore
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);

		glActiveTextureARB(GL_TEXTURE0_ARB);			// make sure #0 is active when we leave
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}

}





/************************ DRAW MATERIAL **************************/

void MO_DrawMaterial(MOMaterialObject *matObj, const OGLSetupOutputType *setupInfo)
{
MOMaterialData		*matData;
OGLColorRGBA		*diffuseColor,diffColor2;
Boolean				textureHasAlpha = false;
Boolean				alreadySet;
SDL_GLContext agl_ctx = setupInfo->drawContext;
u_long				matFlags;

	if (gIsPicking)							// no materials during picking
		goto bail;

			/* SEE IF THIS MATERIAL IS ALREADY SET AS CURRENT */
					
	matData = &matObj->objectData;									// point to material data

	if (matObj->objectHeader.cookie != MO_COOKIE)					// verify cookie			
		DoFatalAlert("\pMO_DrawMaterial: bad cookie!");


				/****************************/
				/* SEE IF TEXTURED MATERIAL */
				/****************************/
				
	matFlags = matData->flags | gGlobalMaterialFlags;				// check flags of material & global
				
	if (matFlags & BG3D_MATERIALFLAG_TEXTURED)
	{
		if (matData->setupInfo != setupInfo)						// make sure texture is loaded for this draw context
			DoFatalAlert("\pMO_DrawMaterial: texture is not assigned to this draw context");
	
		
		if (matData->pixelDstFormat == GL_RGBA)						// if 32bit with alpha, then turn blending on (see below)
			textureHasAlpha = true;

	
				/* ACTIVATE MATERIAL */
				
		alreadySet = (matObj == gMostRecentMaterial);
		if (alreadySet)												// see if even need to bother resetting this
		{
			glEnable(GL_TEXTURE_2D);								// just make sure textures are on
		}
		else
		{		
			OGL_Texture_SetOpenGLTexture(matData->textureName[0]);	// set this texture active
			
			if (gDebugMode)
			{
				int	size;
				size = matData->width * matData->height;
				switch(matData->pixelDstFormat)
				{
					case	GL_RGBA:
					case	GL_RGB:
							gVRAMUsedThisFrame += size * 4;
							break;
							
					case	GL_RGB5_A1:
					case	GL_UNSIGNED_SHORT_1_5_5_5_REV:
							gVRAMUsedThisFrame += size * 2;
							break;				
				}			
			}
		}
		
				/* SET TEXTURE WRAPPING MODE */
				
		if (matFlags & BG3D_MATERIALFLAG_CLAMP_U)
		    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		else
		    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

		if (matFlags & BG3D_MATERIALFLAG_CLAMP_V)
		    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		else
		    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		
	}
	else
		glDisable(GL_TEXTURE_2D);									// not textured, so disable textures
	

			/********************/
			/* COLORED MATERIAL */
			/********************/

	diffuseColor = &matData->diffuseColor;			// point to diffuse color
		
	if (gGlobalTransparency != 1.0f)				// see if need to factor in global transparency
	{
		diffColor2.r = diffuseColor->r;
		diffColor2.g = diffuseColor->g;
		diffColor2.b = diffuseColor->b;
		diffColor2.a = diffuseColor->a * gGlobalTransparency;		
	}
	else
		diffColor2 = *diffuseColor;					// copy to local so we can apply filter to it without munging original


			/* APPLY COLOR FILTER */
			
	diffColor2.r *= gGlobalColorFilter.r;
	diffColor2.g *= gGlobalColorFilter.g;
	diffColor2.b *= gGlobalColorFilter.b;


	SetColor4fv(&diffColor2);						// set current diffuse color
#if USE_GL_COLOR_MATERIAL
//	glEnable(GL_COLOR_MATERIAL);	//-------- continuously reenable this since OGL seems to have a bug where it will magically get disabled. 
#endif										


		/* SEE IF NEED TO ENABLE BLENDING */

		
	if (textureHasAlpha || (diffColor2.a != 1.0f) || (matFlags & BG3D_MATERIALFLAG_ALWAYSBLEND))		// if has alpha, then we need blending on
	    glEnable(GL_BLEND);
	else
	    glDisable(GL_BLEND);


			/* SAVE THIS STUFF */
			
bail:			
	gMostRecentMaterial = matObj;
	
	

}


/************************ DRAW MATRIX **************************/

void MO_DrawMatrix(const MOMatrixObject *matObj, const OGLSetupOutputType *setupInfo)
{
const OGLMatrix4x4		*m;
SDL_GLContext agl_ctx = setupInfo->drawContext;

	m = &matObj->matrix;							// point to matrix

				/* MULTIPLY CURRENT MATRIX BY THIS */
	
	glMultMatrixf((GLfloat *)m);

	if (OGL_CheckError())
		DoFatalAlert("\pMO_DrawMatrix: glMultMatrixf!");

}

/************************ MO: DRAW PICTURE **************************/

void MO_DrawPicture(const MOPictureObject *picObj, const OGLSetupOutputType *setupInfo)
{
int				row,col,i;
float			x,y,z;
const MOPictureData	*picData = &picObj->objectData;
long			numCellsW, numCellsH;
float			cellWidth, cellHeight, ratio, offset;
SDL_GLContext agl_ctx = setupInfo->drawContext;

			
	OGL_PushState();

			/* SET STATE */
			
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	// no filtering!
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	SetInfobarSpriteState(-5);
	


			/* GET DIMENSIONAL DATA */

	numCellsW = picData->numCellsWide;
	numCellsH = picData->numCellsHigh;
	
	cellWidth = 640.0f / (float)numCellsW;
	cellHeight = 480.0f / (float)numCellsH;

			/* CENTER VERTICALLY */
			
//	ratio = (float)picData->fullHeight / (float)picData->fullWidth;
//	offset = gCurrentAspectRatio / ratio - 1.0f;
	y = 0; //240.0f * offset;

	i = 0;
	z = 0;

	for (row = 0; row < numCellsH; row++)
	{
		x = 0;
		
		for (col = 0; col < numCellsW; col++)
		{
					/* ACTIVATE THE MATERIAL */
					
			MO_DrawMaterial(picData->materials[i++], setupInfo);		// submit material #0

			glBegin(GL_QUADS);				
			glTexCoord2f(0,0);	glVertex3f(x, y + cellHeight,z);
			glTexCoord2f(1,0);	glVertex3f(x + cellWidth, y + cellHeight,z);
			glTexCoord2f(1,1);	glVertex3f(x + cellWidth, y,z);
			glTexCoord2f(0,1);	glVertex3f(x, y, z);
			glEnd();	
						
			x += cellWidth;			
			gPolysThisFrame += 2;										// 2 more triangles
		}
		y += cellHeight;
		
	}
	
			/* RESTORE STATE */
			
	OGL_PopState();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	
}


/************************ MO: DRAW SPRITE **************************/
//
// Assume that the matrices are already set to identity
//
// Also, assume that the projection matrix is already the identity matrix.
//

void MO_DrawSprite(const MOSpriteObject *spriteObj, const OGLSetupOutputType *setupInfo)
{
const MOSpriteData	*spriteData = &spriteObj->objectData;
float			scaleX,scaleY,x,y;
SDL_GLContext agl_ctx = setupInfo->drawContext;
MOMaterialObject	*mo;
float				aspect, xoff, yoff;
OGLMatrix3x3		m;
OGLPoint2D			p[4];
				
	x = spriteData->coord.x;
	y = spriteData->coord.y;
	
	scaleX = spriteData->scaleX;
	scaleY = spriteData->scaleY;

		/* ACTIVATE THE MATERIAL */
					
	MO_DrawMaterial(mo = spriteData->material, setupInfo);			

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	xoff = scaleX*.5f;
	yoff = (scaleY*aspect)*.5f;
	
	
				/* SET COORDS */
	
	p[0].x = -xoff;		p[0].y = -yoff;
	p[1].x = xoff;		p[1].y = -yoff;
	p[2].x = xoff;		p[2].y = yoff;
	p[3].x = -xoff;		p[3].y = yoff;

	OGLMatrix3x3_SetRotate(&m, spriteData->rot);
	OGLPoint2D_TransformArray(p, &m, p, 4);

	
			/* DRAW IT */
			
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(p[0].x + x, p[0].y + y);
	glTexCoord2f(1,1);	glVertex2f(p[1].x + x, p[1].y + y);
	glTexCoord2f(1,0);	glVertex2f(p[2].x + x, p[2].y + y);
	glTexCoord2f(0,0);	glVertex2f(p[3].x + x, p[3].y + y);
	glEnd();	
			


	gPolysThisFrame += 2;						// 2 more tris
}


#pragma mark -

/******************** MO GET NEW REFERENCE *********************/

MetaObjectPtr MO_GetNewReference(MetaObjectPtr mo)
{
MetaObjectHeader *h = mo;

	h->refCount++;
	
	return(mo);
}




/********************** MO DISPOSE OBJECT REFERENCE ******************************/
//
// NOTE:  	Groups and other objects are NOT sub-recursed.  When a group is de-referenced, only that group object is affected.
//

void MO_DisposeObjectReference(MetaObjectPtr obj)
{
MetaObjectHeader	*header = obj;
MOVertexArrayObject	*vObj;

	if (obj == nil)
		DoFatalAlert("\pMO_DisposeObjectReference: obj == nil");

	if (header->cookie != MO_COOKIE)					// verify cookie			
		DoFatalAlert("\pMO_DisposeObjectReference: bad cookie!");


			/**************************************/
			/* DEC REFERENCE COUNT OF THIS OBJECT */
			/**************************************/
	
	header->refCount--;									// dec ref count
	
	if (header->refCount < 0)							// see if over did it
		DoFatalAlert("\pMO_DisposeObjectReference: refcount < 0!");
		
	if (header->refCount == 0)							// see if we can DELETE this node
	{
			/* NO MORE REFERENCES, SO DELETE DATA */

		switch(header->type)
		{
			case	MO_TYPE_GROUP:
					MO_DisposeObject_Group(obj);
					break;
		
			case	MO_TYPE_GEOMETRY:
					switch(header->subType)
					{
						case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
								vObj = obj;
								MO_DeleteObjectInfo_Geometry_VertexArray(&vObj->objectData);
								break;
								
						default:
								DoFatalAlert("\pMO_DisposeObject: unknown sub-type");
					}
					break;
					
			case	MO_TYPE_MATERIAL:
					MO_DeleteObjectInfo_Material(obj);
					break;		
					
			case	MO_TYPE_PICTURE:
					MO_DeleteObjectInfo_Picture(obj);
					break;
					
			case	MO_TYPE_SPRITE:
					MO_DisposeObject_Sprite(obj);
					break;
												
		}
	
			/* DELETE THE OBJECT NODE */
			
		MO_DetachFromLinkedList(obj);					// detach from linked list

		header->cookie = 0xdeadbeef;					// devalidate cookie
		SafeDisposePtr(obj);								// free memory used by object
		return;
	}		
}



/***************** DETACH FROM LINKED LIST *********************/

static void MO_DetachFromLinkedList(MetaObjectPtr obj)
{
MetaObjectHeader	*header = obj;
MetaObjectHeader	*prev,*next;


			/* VERIFY COOKIE */

	if (header->cookie != MO_COOKIE)
		DoFatalAlert("\pMO_DetachFromLinkedList: cookie is invalid!");


	prev = header->prevNode;
	next = header->nextNode;
	
			/* SEE IF WAS 1ST NODE IN LIST */
			
	if (prev == nil)
	{
		gFirstMetaObject = next;
		if (gFirstMetaObject)
			gFirstMetaObject->prevNode = nil;
	}
	
			/* SEE IF WAS LAST NODE IN LIST */
			
	if (next == nil)
	{
		gLastMetaObject = prev;	
		if (gLastMetaObject)
			gLastMetaObject->nextNode = nil;
	}
	
			/* SOMEWHERE IN THE MIDDLE */
	
	else		
	if (prev != nil)
	{
		prev->nextNode = next;
		next->prevNode = prev;
	}
	
	gNumMetaObjects--;	
	
	if (gNumMetaObjects < 0)
		DoFatalAlert("\pMO_DetachFromLinkedList: counter mismatch");
		
	if (gNumMetaObjects == 0)
	{
		if (prev || next)							// if all gone, then prev & next should be nil
			DoFatalAlert("\pMO_DetachFromLinkedList: prev/next should be nil!");
	
		if (gFirstMetaObject != nil)
			DoFatalAlert("\pMO_DetachFromLinkedList: gFirstMetaObject should be nil!");

		if (gLastMetaObject != nil)
			DoFatalAlert("\pMO_DetachFromLinkedList: gLastMetaObject should be nil!");
	}
}


/**************** DISPOSE OBJECT:  GROUP ************************/
//
// Decrement the references of all objects in the group.
//

static void MO_DisposeObject_Group(MOGroupObject *group)
{
int	i,n;

	n = group->objectData.numObjectsInGroup;				// get # objects in group

	for (i = 0; i < n; i++)
	{
		MO_DisposeObjectReference(group->objectData.groupContents[i]);	// dispose of this object's ref
	}
}


/****************** DISPOSE OBJECT: SPRITE *******************/
//
// Decrement reference tothe material used in this sprite
//

static void MO_DisposeObject_Sprite(MOSpriteObject *obj)
{
MOSpriteData *data = &obj->objectData;

	MO_DisposeObjectReference(data->material);
}



/****************** DELETE OBJECT INFO: GEOMETRY : VERTEX ARRAY *******************/
//
// Assumes the contents (the materials) have already been dereferenced!
//

void MO_DeleteObjectInfo_Geometry_VertexArray(MOVertexArrayData *data)
{
int					i,n;

			/* DEREFERENCE ANY MATERIALS */
			
	n = data->numMaterials;
	for (i = 0; i < n; i++)
		MO_DisposeObjectReference(data->materials[i]);	// dispose of this object's ref


		/* DISPOSE OF VARIOUS ARRAYS */
		
	if (data->points)
	{
		SafeDisposePtr((Ptr)data->points);
		data->points = nil;
	}

	if (data->normals)
	{
		SafeDisposePtr((Ptr)data->normals);
		data->normals = nil;
	}

	if (data->uvs[0])
	{
		SafeDisposePtr((Ptr)data->uvs[0]);
		data->uvs[0] = nil;
		
		if (data->numMaterials == 2)					// see if also nuke secondary uv list
		{
			if (data->uvs[1])
			{
				SafeDisposePtr((Ptr)data->uvs[1]);
				data->uvs[1] = nil;
			}		
		}
	}

	if (data->colorsByte)
	{
		SafeDisposePtr((Ptr)data->colorsByte);
		data->colorsByte = nil;
	}

	if (data->colorsFloat)
	{
		SafeDisposePtr((Ptr)data->colorsFloat);
		data->colorsFloat = nil;
	}

	if (data->triangles)
	{
		SafeDisposePtr((Ptr)data->triangles);
		data->triangles = nil;
	}
}



/****************** DELETE OBJECT INFO: MATERIAL *******************/

static void MO_DeleteObjectInfo_Material(MOMaterialObject *obj)
{
MOMaterialData		*data = &obj->objectData;
SDL_GLContext agl_ctx = gAGLContext;

		/* DISPOSE OF TEXTURE NAMES */
				
	if (data->numMipmaps > 0)
		glDeleteTextures(data->numMipmaps, &data->textureName[0]);
}


/****************** DELETE OBJECT INFO: PICTURE *******************/

static void MO_DeleteObjectInfo_Picture(MOPictureObject *obj)
{
MOPictureData		*data = &obj->objectData;
int		numCells,i;


				/* DEREFERENCE THE MATERIALS */
				
	numCells = data->numCellsWide * data->numCellsHigh;
	for (i = 0; i < numCells; i++)
		MO_DisposeObjectReference(data->materials[i]);		// dispose of this object's ref

		/* DISPOSE OF TEXTURE NAMES ARRAY */

	SafeDisposePtr((Ptr)data->materials);	
	data->materials = nil;		
}


#pragma mark -

/******************** MO_DUPLICATE VERTEX ARRAY DATA *********************/

void MO_DuplicateVertexArrayData(MOVertexArrayData *inData, MOVertexArrayData *outData)
{
int	i,n,s;
			/***********************************/
			/* GET NEW REFERENCES TO MATERIALS */
			/***********************************/
		
	outData->numMaterials = n = inData->numMaterials;

	for (i = 0; i < n; i++)
	{
		MO_GetNewReference(inData->materials[i]);
		outData->materials[i] = inData->materials[i];	
	}
	
			/************************/
			/* DUPLICATE THE ARRAYS */
			/************************/
			
			/* POINTS */
			
	n = outData->numPoints = inData->numPoints;
	s = inData->numPoints * sizeof(OGLPoint3D);
	
	if (inData->points)
	{
		outData->points = AllocPtr(s);
		if (outData->points == nil)
			DoFatalAlert("\pMO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->points, outData->points, s);
	}
	else
		outData->points = nil;


			/* NORMALS */
			
	s = n * sizeof(OGLVector3D);
	
	if (inData->normals)
	{
		outData->normals = AllocPtr(s);
		if (outData->normals == nil)
			DoFatalAlert("\pMO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->normals, outData->normals, s);
	}
	else
		outData->normals = nil;


			/* UVS */
			
	s = n * sizeof(OGLTextureCoord);
	
	if (inData->uvs[0])
	{
		outData->uvs[0] = AllocPtr(s);
		if (outData->uvs[0] == nil)
			DoFatalAlert("\pMO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->uvs[0], outData->uvs[0], s);
	}
	else
		outData->uvs[0] = nil;


			/* COLORS BYTE */
			
	s = n * sizeof(OGLColorRGBA_Byte);
	
	if (inData->colorsByte)
	{
		outData->colorsByte = AllocPtr(s);
		if (outData->colorsByte == nil)
			DoFatalAlert("\pMO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->colorsByte, outData->colorsByte, s);
	}
	else
		outData->colorsByte = nil;


			/* COLORS FLOAT */
			
	s = n * sizeof(OGLColorRGBA);
	
	if (inData->colorsFloat)
	{
		outData->colorsFloat = AllocPtr(s);
		if (outData->colorsFloat == nil)
			DoFatalAlert("\pMO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->colorsFloat, outData->colorsFloat, s);
	}
	else
		outData->colorsFloat = nil;


			/* TRIANGLES */
			
	n = outData->numTriangles = inData->numTriangles;
	s = n * sizeof(GLint) * 3;
	
	if (inData->triangles)
	{
		outData->triangles = AllocPtr(s);
		if (outData->triangles == nil)
			DoFatalAlert("\pMO_DuplicateVertexArrayData: AllocPtr failed!");
		BlockMove(inData->triangles, outData->triangles, s);
	}
	else
		outData->triangles = nil;
}

#pragma mark -


/******************** MO: CALC BOUNDING BOX ***********************/
//
// INPUT:
//			m = transform matrix to apply to verts or nil.
//

void MO_CalcBoundingBox(MetaObjectPtr object, OGLBoundingBox *bBox, OGLMatrix4x4 *m)
{
		/* INIT BBOX TO BOGUS VALUES */
		
	bBox->min.x = 
	bBox->min.y =
	bBox->min.z = 100000000;
	
	bBox->max.x = 
	bBox->max.y = 
	bBox->max.z = -bBox->min.x;

	bBox->isEmpty = false;

			/* RECURSIVELY CALC BBOX */
			
	MO_CalcBoundingBox_Recurse(object, bBox, m);
}


/******************** MO: CALC BOUNDING BOX: RECURSE ***********************/

static void MO_CalcBoundingBox_Recurse(MetaObjectPtr object, OGLBoundingBox *bBox, const OGLMatrix4x4 *m)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;
MOGroupObject		*groupObject;
MOVertexArrayData	*geoData;
int					numChildren,i,numPoints;
float				x,y,z;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("\pMO_CalcBoundingBox_Recurse: cookie is invalid!");


	switch(objHead->type)
	{
			/* CALC BBOX OF GEOMETRY */
			
		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vObj = object;
							geoData = &vObj->objectData;
							numPoints = geoData->numPoints;
							
								/* TRANSFORM POINTS */
								
							if (m)
							{
								static OGLPoint3D tpoints[1000];
								
								if (numPoints > 1000)					// make sure not overflowing buffer
								{
									DoFatalAlert("\pMO_CalcBoundingBox_Recurse: buffer overflow!");
									return;
								}

								OGLPoint3D_TransformArray(geoData->points, m, tpoints, numPoints);
								for (i = 0; i < numPoints; i++)
								{
									x = tpoints[i].x;
									y = tpoints[i].y;
									z = tpoints[i].z;
									
									if (x < bBox->min.x)
										bBox->min.x = x;
									if (x > bBox->max.x)
										bBox->max.x = x;
								
									if (y < bBox->min.y)
										bBox->min.y = y;
									if (y > bBox->max.y)
										bBox->max.y = y;

									if (z < bBox->min.z)
										bBox->min.z = z;
									if (z > bBox->max.z)
										bBox->max.z = z;							
								}							
							
							}
							
								/* NO TRANSFORM, USE RAW DATA */
								
							else
							{
								for (i = 0; i < numPoints; i++)
								{
									x = geoData->points[i].x;
									y = geoData->points[i].y;
									z = geoData->points[i].z;
									
									if (x < bBox->min.x)
										bBox->min.x = x;
									if (x > bBox->max.x)
										bBox->max.x = x;
								
									if (y < bBox->min.y)
										bBox->min.y = y;
									if (y > bBox->max.y)
										bBox->max.y = y;

									if (z < bBox->min.z)
										bBox->min.z = z;
									if (z > bBox->max.z)
										bBox->max.z = z;							
								}							
							}
							break;
							
					default:	
						DoFatalAlert("\pMO_CalcBoundingBox_Recurse: unknown sub-type!");
				}
				break;
	
	
			/* RECURSE THRU GROUP */
			
		case	MO_TYPE_GROUP:
				groupObject = object;
				numChildren = groupObject->objectData.numObjectsInGroup;				
				for (i = 0; i < numChildren; i++)
					MO_CalcBoundingBox_Recurse(groupObject->objectData.groupContents[i], bBox, m);
				break;

				
		case	MO_TYPE_MATRIX:
				DoFatalAlert("\pMO_CalcBoundingBox_Recurse: why is there a matrix here?");
				break;
	}
}



/******************** MO: CALC BOUNDING SPHERE ***********************/

void MO_CalcBoundingSphere(MetaObjectPtr object, float *bSphere)
{
	*bSphere = 0;

			/* RECURSIVELY CALC SPHERE */
			
	MO_CalcBoundingSphere_Recurse(object, bSphere);
}


/******************** MO: CALC BOUNDING SPHERE: RECURSE ***********************/

static void MO_CalcBoundingSphere_Recurse(MetaObjectPtr object, float *bSphere)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;
MOGroupObject		*groupObject;
MOVertexArrayData	*geoData;
int					numChildren,i,numPoints;
float				d;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("\pMO_CalcBoundingSphere_Recurse: cookie is invalid!");


	switch(objHead->type)
	{
			/* CALC BBOX OF GEOMETRY */
			
		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vObj = object;
							geoData = &vObj->objectData;
							numPoints = geoData->numPoints;
							
							for (i = 0; i < numPoints; i++)
							{
								d = CalcVectorLength((OGLVector3D *)&geoData->points[i]);		// calc this radius
								if (d > *bSphere)								// is this the best?
									*bSphere = d;
							}
							break;
							
					default:	
						DoFatalAlert("\pMO_CalcBoundingSphere_Recurse: unknown sub-type!");
				}
				break;
	
	
			/* RECURSE THRU GROUP */
			
		case	MO_TYPE_GROUP:
				groupObject = object;
				numChildren = groupObject->objectData.numObjectsInGroup;				
				for (i = 0; i < numChildren; i++)
					MO_CalcBoundingSphere_Recurse(groupObject->objectData.groupContents[i], bSphere);
				break;

				
		case	MO_TYPE_MATRIX:
				DoFatalAlert("\pMO_CalcBoundingSphere_Recurse: why is there a matrix here?");
				break;
	}
}


#pragma mark -



/******************* MO: GET TEXTURE FROM FILE ************************/

MOMaterialObject *MO_GetTextureFromFile(FSSpec *spec, OGLSetupOutputType *setupInfo, int destPixelFormat)
{
MetaObjectPtr	obj;
MOMaterialData	matData;
int				width,height,depth,destDepth;
GWorldPtr 		pGWorld;
PixMapHandle 	hPixMap;
Ptr				buffer;
Ptr 			pictMapAddr;
u_long 			pictRowBytes,y,x;
Boolean			destHasAlpha;
Rect			r;

		/*******************************/
		/* CREATE TEXTURE PIXEL BUFFER */
		/*******************************/

	switch(destPixelFormat)
	{
		case	GL_RGB:	
				destHasAlpha 	= false;
				destDepth 		= 32;
				break;

		case	GL_RGBA:	
				destHasAlpha 	= true;
				destDepth 		= 32;
				break;
	
		case	GL_RGB5_A1:
				destHasAlpha 	= true;
				destDepth 		= 16;
				break;
	}


		/* LOAD PICTURE INTO GWORLD */
	
	if (DrawPictureIntoGWorld(spec , &pGWorld, 32))			//--- for now, must be 32bit for OpenGL internal b/c 16bit not supported
		DoFatalAlert("\pMO_GetTextureFromFile: DrawPictureIntoGWorld failed!");
	

			/* GET GWORLD INFO */
			
	GetPortBounds(pGWorld, &r);
			
	width = r.right - r.left;		// get width/height
	height = r.bottom - r.top;
	
	if ((!IsPowerOf2(width)) || (!IsPowerOf2(height)))				// make sure its a power of 2
		DoFatalAlert("\pMO_GetTextureFromFile: dimensions not power of 2");
	
	hPixMap = GetGWorldPixMap(pGWorld);								// get gworld's pixmap
	depth = (*hPixMap)->pixelSize;									// get gworld pixel bitdepth


		/*************************************/
		/* CREATE TEXTURE OBJECT FROM PIXELS */
		/*************************************/

		/* COPY PIXELS FROM GWORLD INTO BUFFER */
		
	buffer = AllocPtr(width * height * 4);							// alloc enough for a 32bit texture
	if (buffer == nil)
		DoFatalAlert("\pMO_GetTextureFromResource: AllocPtr failed!");

	pictMapAddr = GetPixBaseAddr(hPixMap);
	pictRowBytes = (u_long)(**hPixMap).rowBytes & 0x3fff;
	pictMapAddr += pictRowBytes * (height-1);						// start @ bottom to flip texture


			/* COPY 32-BIT */

	if (depth == 32)
	{
		u_long	r,g,b,a;	
		u_long	pixels, *dest, *src;

		src = (u_long *)pictMapAddr;
		dest = (u_long *)buffer;
		
		for (y = 0; y < height; y++)
		{		
			for (x = 0; x < width; x++)
			{
				pixels = src[x];	
				if (destHasAlpha && (destDepth == 32))		// see if use file's alpha channel for 32bit destinations
					a = (pixels & 0xff000000) >> 24;
				else										// otherwise just do 0 or ff
				{
					if (pixels == 0)
						a = 0;
					else
						a = 0xff;
				}
				r = (pixels & 0x00ff0000) >> 16;
				g = (pixels & 0x0000ff00) >> 8;
				b = pixels & 0xff;
				
				dest[x] = (r << 24) | (g << 16) | (b << 8) | a;
			}
			dest += width;
			src -= pictRowBytes/4;
		}
	}
	
		/* COPY 16-BIT */
		
	else
	{
		DoFatalAlert("\pMO_GetTextureFromFile: 16 bit textures not supported yet.");
		//-------- TODO
	}

	DisposeGWorld (pGWorld);


			/* CREATE NEW TEXTURE OBJECT */

	matData.setupInfo		= setupInfo;
	matData.flags			= BG3D_MATERIALFLAG_TEXTURED;
	matData.diffuseColor.r	= 1;
	matData.diffuseColor.g	= 1;
	matData.diffuseColor.b	= 1;
	matData.diffuseColor.a	= 1;
	
	matData.numMipmaps		= 1;
	matData.width			= width;
	matData.height			= height;
	
	if (depth == 32)
		matData.pixelSrcFormat	= GL_RGBA;
	else
	{
		DoFatalAlert("\pMO_GetTextureFromFile: 16 bit textures not supported yet.");
		//-------- TODO	
	}
	
	matData.pixelDstFormat	= destPixelFormat;
	matData.texturePixels[0]= nil;						// we're going to preload
	matData.textureName[0] 	= OGL_TextureMap_Load(buffer, width, height,
												 matData.pixelSrcFormat,
												 destPixelFormat, GL_UNSIGNED_BYTE);
	
	obj = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
	
	SafeDisposePtr(buffer);									// dispose of our copy of the buffer
	
	return(obj);
}

/*************** MO: GEOMETRY OFFSET UVS *********************/

void MO_Geometry_OffserUVs(short group, short type, short geometryNum, float du, float dv)
{
MOVertexArrayObject	*mo;

	mo = gBG3DGroupList[group][type];												// point to this object

				/****************/
				/* GROUP OBJECT */
				/****************/
				
	if (mo->objectHeader.type == MO_TYPE_GROUP)										// see if need to go into group
	{
		MOGroupObject	*groupObj = (MOGroupObject *)mo;
		
		if (geometryNum >= groupObj->objectData.numObjectsInGroup)					// make sure # is valid
			DoFatalAlert("\pMO_Geometry_OffserUVs: geometryNum out of range");
		
				/* POINT TO 1ST GEOMETRY IN THE GROUP */
						
		if (geometryNum == -1)														// if -1 then assign to all textures for this model
		{
			int	i;
			for (i = 0; i < groupObj->objectData.numObjectsInGroup; i++)
			{
				MO_VertexArray_OffsetUVs(groupObj->objectData.groupContents[i], du, dv);
			}		
		}
		else
		{
			MO_VertexArray_OffsetUVs(groupObj->objectData.groupContents[geometryNum], du, dv);
		}
	}
	
			/* NOT A GROUNP, SO ASSUME GEOMETRY */
	else
	{
		MO_VertexArray_OffsetUVs(mo, du, dv);
	}	
}


/******************* MO: OBJECT OFFSET UVS ************************/

void MO_Object_OffsetUVs(MetaObjectPtr object, float du, float dv)
{
MetaObjectHeader	*objHead = object;
MOGroupData			*data;
int					i;
MOGroupObject		*group;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("\pMO_Group_OffsetUVs: cookie is invalid!");


			/* HANDLE IT */
			
	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				MO_VertexArray_OffsetUVs(object, du, dv);
				break;
				
		case	MO_TYPE_GROUP:
				group = object;
				data = &group->objectData;
							
							/* PARSE OBJECTS IN GROUP */
							
				for (i = 0; i < data->numObjectsInGroup; i++)
				{
					switch(data->groupContents[i]->type)
					{
						case	MO_TYPE_GEOMETRY:
								MO_VertexArray_OffsetUVs(data->groupContents[i], du, dv);
								break;
				
						case	MO_TYPE_GROUP:
								MO_Object_OffsetUVs(data->groupContents[i], du, dv);		// recurse this sub-group
								break;

					}
				}
				break;
	
	
		default:
			DoFatalAlert("\pMO_Group_OffsetUVs: object type is not supported.");	
	}

}


/******************* MO: VERTEX ARRAY, OFFSET UVS ************************/

void MO_VertexArray_OffsetUVs(MetaObjectPtr object, float du, float dv)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayData	*data;
int					numPoints,i;
OGLTextureCoord		*uvPtr;
MOVertexArrayObject	*vObj;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("\pMO_VertexArray_OffsetUVs: cookie is invalid!");


		/* MAKE SURE IT IS A VERTEX ARRAY */

	if ((objHead->type != MO_TYPE_GEOMETRY) || (objHead->subType != MO_GEOMETRY_SUBTYPE_VERTEXARRAY))
		DoFatalAlert("\pMO_VertexArray_OffsetUVs: object is not a Vertex Array!");	
	
	vObj = object;
	data = &vObj->objectData;						// point to data
	
	
	uvPtr = data->uvs[0];								// point to uv list
	if (uvPtr == nil)
		return;
		
	numPoints = data->numPoints;					// get # points
	
			/* OFFSET THE UV'S */
			
	for (i = 0; i < numPoints; i++)
	{
		uvPtr[i].u += du;	
		uvPtr[i].v += dv;	
	}
}


#pragma mark -


/**************************** MO: LOAD TEXTURE OBJECT FROM FILE *************************/
//
// Takes the input image file and converts it into a 32-bit texture material object
//

MOMaterialObject *MO_LoadTextureObjectFromFile(OGLSetupOutputType *setupInfo, FSSpec *spec, Boolean useAlpha)
{
GWorldPtr		gworld = nil;
OSErr			iErr;
Rect			r;
Ptr				buffer;
Ptr				pictMapAddr;
u_long			pictRowBytes,x,y;
PixMapHandle 	hPixMap;
int				width,height;
u_long			*destPtr,*srcPtr;
MOMaterialObject	*obj;


			/* DRAW PICTURE INTO A GWORLD */
			
	iErr = DrawPictureIntoGWorld(spec, &gworld, 32);
	if (iErr)
	{
		DoAlert("\pMO_LoadTextureObjectFromFile: something went wrong");
		return(nil);
	}

			/* COPY GWORLD INTO BUFFER */
			
	GetPortBounds(gworld, &r);
	width = r.right - r.left;												// get width/height
	height = r.bottom - r.top;
	
	if ((!IsPowerOf2(width)) || (!IsPowerOf2(height)))						// make sure texture is power of 2
	{
		DoAlert("\pMO_LoadTextureObjectFromFile:  texture dimensions are not power of 2");
		DisposeGWorld(gworld);
		return(nil);
	}
	
	
	buffer = AllocPtr(width * height * 4);									// allocate buffer
	destPtr = (u_long *)buffer;
	destPtr += (height-1) * width;											// start ptr at bottom of buffer
	
	hPixMap = GetGWorldPixMap(gworld);										// get gworld's pixmap
	pictMapAddr = GetPixBaseAddr(hPixMap);
	pictRowBytes = (u_long)(**hPixMap).rowBytes & 0x3fff;
	
	srcPtr = (u_long *)pictMapAddr;
	
	for (y = 0; y < height; y++)
	{	
		for (x = 0; x < width; x++)
		{
			u_long	r,g,b,a;	
			u_long pixels = srcPtr[x];										// get 32-bit pixel
	
			if (useAlpha)			
				a = (pixels & 0xff000000) >> 24;
			else
				a = 0xff;
				
			r = (pixels & 0x00ff0000) >> 16;
			g = (pixels & 0x0000ff00) >> 8;
			b = pixels & 0xff;
	
			destPtr[x] = (r << 24) | (g << 16) | (b << 8) | a;				// save pixel after twizzling components
		}
		destPtr -= width;
		srcPtr += pictRowBytes/4;
		
	}

			/***************************/
			/* CREATE A TEXTURE OBJECT */
			/***************************/

	obj = MO_CreateTextureObjectFromBuffer(setupInfo, r.right, r.bottom, buffer);


		/* CLEAN UP */
		
	DisposeGWorld(gworld);
	
	return(obj);
}



/****************** MO:  CREATE TEXTURE OBJECT FROM BUFFER **************************/

MOMaterialObject *MO_CreateTextureObjectFromBuffer(OGLSetupOutputType *setupInfo, int width, int height, Ptr buffer)
{
MOMaterialData	data;
MOMaterialObject	*obj;


			/***************************/
			/* CREATE A TEXTURE OBJECT */
			/***************************/

		/* INIT NEW MATERIAL DATA */

	data.setupInfo				= setupInfo;							// remember which draw context this material is assigned to
	data.flags 					= BG3D_MATERIALFLAG_TEXTURED;
	data.width					= width;
	data.height					= height;
	data.multiTextureMode		= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;
	data.multiTextureCombine	= MULTI_TEXTURE_COMBINE_MODULATE;
	data.envMapNum				= 0;
	data.diffuseColor.r			= 1;
	data.diffuseColor.g			= 1;
	data.diffuseColor.b			= 1;
	data.diffuseColor.a			= 1;
	data.numMipmaps				= 1;
	data.pixelSrcFormat			= GL_RGBA;
	data.pixelDstFormat			= GL_RGBA;
	data.texturePixels[0] 		= buffer;								// keep a ptr to the original buffer so we can write it to a file if needed
	data.textureName[0] 		= OGL_TextureMap_Load(buffer, width, height,
												 data.pixelSrcFormat, data.pixelDstFormat, GL_UNSIGNED_BYTE);

	/* CREATE NEW MATERIAL OBJECT */
			
	obj = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &data);


	return(obj);

}










