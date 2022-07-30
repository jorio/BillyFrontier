/****************************/
/*      FILE ROUTINES       */
/* By Brian Greenstone      */
/* (c)2003 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include 	"bones.h"
#include 	"lzss.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType, OGLSetupOutputType *setupInfo);
static void ReadDataFromPlayfieldFile(FSSpec *specPtr, OGLSetupOutputType *setupInfo);
static void	ConvertTexture16To24(u_short *textureBuffer2, u_char *textureBuffer3, int width, int height);
static void	ConvertTexture16To16(u_short *textureBuffer, int width, int height);
static void	ConvertTexture16To32(u_short *srcBuff, u_char *destBuff, int width, int height);

static void HalfTexture16(u_short *buff, int width, int height);

#define BYTESWAP_HANDLE(format, type, n, handle)                                  \
{                                                                                 \
	if ((n) * sizeof(type) > (unsigned long) GetHandleSize((Handle) (handle)))   \
		DoFatalAlert("BYTESWAP_HANDLE: size mismatch!\nHdl=%ld Exp=%ld\nWhen swapping %dx %s in %s:%d", \
			GetHandleSize((Handle) (handle)), \
			(n) * sizeof(type), \
			n, #type, __func__, __LINE__); \
	ByteswapStructs((format), sizeof(type), (n), *(handle));                      \
}

/****************************/
/*    CONSTANTS             */
/****************************/

#define	TILE_DEPTH			16

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1

#define	SAVE_GAME_VERSION	0x0100		// 1.0

		/* SAVE GAME */
		
typedef struct
{
	u_long		version;
	u_long		score;
	short		realLevel;
	short		numLives;
	Boolean		levels[NUM_LEVELS];
	Boolean		duels[NUM_LEVELS];
}SaveGameType;


		/* PLAYFIELD HEADER */
		
typedef struct
{
	NumVersion	version;							// version of file
	long		numItems;							// # items in map
	long		mapWidth;							// width of map
	long		mapHeight;							// height of map	
	float		tileSize;							// 3D unit size of a tile
	float		minY,maxY;							// min/max height values
	long		numSplines;							// # splines
	long		numFences;							// # fences
	long		numUniqueSuperTiles;				// # unique supertile
	long        numWaterPatches;                    // # water patches
	long		numCheckpoints;						// # checkpoints
	long        unused[10];
}PlayfieldHeaderType;


		/* FENCE STRUCTURE IN FILE */
		//
		// note: we copy this data into our own fence list
		//		since the game uses a slightly different
		//		data structure.
		//
		
typedef	struct
{
	int		x,z;
}FencePointType;


typedef struct
{
	u_short			type;				// type of fence
	short			numNubs;			// # nubs in fence
	FencePointType	**nubList;			// handle to nub list	
	Rect			bBox;				// bounding box of fence area	
}FileFenceDefType;




/**********************/
/*     VARIABLES      */
/**********************/

static	Str255		gBasePathName = "NewGame";

float	g3DTileSize, g3DMinY, g3DMaxY;

static 	FSSpec		gSavedGameSpec;



/****************** SET DEFAULT DIRECTORY ********************/
//
// This function needs to be called for OS X because OS X doesnt automatically
// set the default directory to the application directory.
//

void SetDefaultDirectory(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
ProcessSerialNumber serial;
ProcessInfoRec info;
FSSpec	app_spec;
WDPBRec wpb;
OSErr	iErr;		
		
	serial.highLongOfPSN = 0;
	serial.lowLongOfPSN = kCurrentProcess;
	
	
	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processName = NULL;
	info.processAppSpec = &app_spec;

	iErr = GetProcessInformation(&serial, & info);

	wpb.ioVRefNum = app_spec.vRefNum;
	wpb.ioWDDirID = app_spec.parID;
	wpb.ioNamePtr = NULL;
	
	iErr = PBHSetVolSync(&wpb);
	
	
		/* ALSO SET SAVED GAME SPEC TO DESKTOP */
		
	iErr = FindFolder(kOnSystemDisk,kDesktopFolderType,kDontCreateFolder,			// locate the desktop folder
					&gSavedGameSpec.vRefNum,&gSavedGameSpec.parID);
	gSavedGameSpec.name[0] = 0;
#endif
}



/******************* LOAD SKELETON *******************/
//
// Loads a skeleton file & creates storage for it.
// 
// NOTE: Skeleton types 0..NUM_CHARACTERS-1 are reserved for player character skeletons.
//		Skeleton types NUM_CHARACTERS and over are for other skeleton entities.
//
// OUTPUT:	Ptr to skeleton data
//

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo)
{
QDErr		iErr;
short		fRefNum;
FSSpec		fsSpec;
SkeletonDefType	*skeleton;					
const char*	fileNames[MAX_SKELETON_TYPES] =
{
	[SKELETON_TYPE_BILLY]		= ":Skeletons:billy.skeleton",
	[SKELETON_TYPE_BANDITO]		= ":Skeletons:bandito.skeleton",
	[SKELETON_TYPE_RYGAR]		= ":Skeletons:rygar.skeleton",
	[SKELETON_TYPE_SHORTY]		= ":Skeletons:shorty.skeleton",
	[SKELETON_TYPE_KANGACOW]	= ":Skeletons:kangacow.skeleton",
	[SKELETON_TYPE_KANGAREX]	= ":Skeletons:kangarex.skeleton",
	[SKELETON_TYPE_WALKER]		= ":Skeletons:walker.skeleton",
	[SKELETON_TYPE_TREMORALIEN]	= ":Skeletons:tremoralien.skeleton",
	[SKELETON_TYPE_TREMORGHOST]	= ":Skeletons:tremorghost.skeleton",
	[SKELETON_TYPE_FROGMAN]		= ":Skeletons:frogman.skeleton",
};

	GAME_ASSERT(skeletonType >= 0);
	GAME_ASSERT(skeletonType < MAX_SKELETON_TYPES);


	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, fileNames[skeletonType], &fsSpec);


			/* OPEN THE FILE'S REZ FORK */

	fRefNum = FSpOpenResFile(&fsSpec,fsRdPerm);
	if (fRefNum == -1)
	{
		iErr = ResError();
		DoAlert("Error opening Skel Rez file");
		ShowSystemErr(iErr);
	}
	
	UseResFile(fRefNum);
	if (ResError())
		DoFatalAlert("Error using Rez file!");

			
			/* ALLOC MEMORY FOR SKELETON INFO STRUCTURE */
			
	skeleton = (SkeletonDefType *)AllocPtr(sizeof(SkeletonDefType));
	if (skeleton == nil)
		DoFatalAlert("Cannot alloc SkeletonInfoType");


			/* READ SKELETON RESOURCES */
			
	ReadDataFromSkeletonFile(skeleton,&fsSpec,skeletonType,setupInfo);
	PrimeBoneData(skeleton);
	
			/* CLOSE REZ FILE */
			
	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);
		
	return(skeleton);
}


/************* READ DATA FROM SKELETON FILE *******************/
//
// Current rez file is set to the file. 
//

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType, OGLSetupOutputType *setupInfo)
{
Handle				hand;
int					i,k,j;
long				numJoints,numAnims,numKeyframes;
AnimEventType		*animEventPtr;
JointKeyframeType	*keyFramePtr;
SkeletonFile_Header_Type	*headerPtr;
short				version;
AliasHandle				alias;
OSErr					iErr;
FSSpec					target;
Boolean					wasChanged;
OGLPoint3D				*pointPtr;
SkeletonFile_AnimHeader_Type	*animHeaderPtr;


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	GAME_ASSERT(hand);
	BYTESWAP_HANDLE("4h", SkeletonFile_Header_Type, 1, hand);
	headerPtr = (SkeletonFile_Header_Type *)*hand;
	version = headerPtr->version;
	GAME_ASSERT_MESSAGE(version == SKELETON_FILE_VERS_NUM, "Skeleton file has wrong version #");
	
	numAnims = skeleton->NumAnims = headerPtr->numAnims;			// get # anims in skeleton
	numJoints = skeleton->NumBones = headerPtr->numJoints;			// get # joints in skeleton
	ReleaseResource(hand);

	if (numJoints > MAX_JOINTS)										// check for overload
		DoFatalAlert("ReadDataFromSkeletonFile: numJoints > MAX_JOINTS");


				/*************************************/
				/* ALLOCATE MEMORY FOR SKELETON DATA */
				/*************************************/

	AllocSkeletonDefinitionMemory(skeleton);



		/********************************/
		/* 	LOAD THE REFERENCE GEOMETRY */
		/********************************/
		
	alias = (AliasHandle)GetResource(rAliasType,1000);				// alias to geometry BG3D file
	if (alias != nil)
	{
		iErr = ResolveAlias(fsSpec, alias, &target, &wasChanged);	// try to resolve alias
		if (!iErr)
			LoadBonesReferenceModel(&target,skeleton, skeletonType, setupInfo);
		else
			DoFatalAlert("ReadDataFromSkeletonFile: Cannot find Skeleton's BG3D file!");
		ReleaseResource((Handle)alias);
	}
	else
		DoFatalAlert("ReadDataFromSkeletonFile: file is missing the Alias resource");
	


		/***********************************/
		/*  READ BONE DEFINITION RESOURCES */
		/***********************************/

	for (i=0; i < numJoints; i++)
	{
		File_BoneDefinitionType	*bonePtr;
		uint16_t				*indexPtr;

			/* READ BONE DATA */

		hand = GetResource('Bone',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading Bone resource!");
		BYTESWAP_HANDLE("i32b3fHH8L", File_BoneDefinitionType, 1, hand);
		HLock(hand);
		bonePtr = (File_BoneDefinitionType *)*hand;

			/* COPY BONE DATA INTO ARRAY */

		skeleton->Bones[i].parentBone = bonePtr->parentBone;								// index to previous bone
		skeleton->Bones[i].coord = bonePtr->coord;											// absolute coord (not relative to parent!)
		skeleton->Bones[i].numPointsAttachedToBone = bonePtr->numPointsAttachedToBone;		// # vertices/points that this bone has
		skeleton->Bones[i].numNormalsAttachedToBone = bonePtr->numNormalsAttachedToBone;	// # vertex normals this bone has		
		ReleaseResource(hand);

			/* ALLOC THE POINT & NORMALS SUB-ARRAYS */

		skeleton->Bones[i].pointList = (uint16_t *)AllocPtr(sizeof(uint16_t) * (int)skeleton->Bones[i].numPointsAttachedToBone);
		if (skeleton->Bones[i].pointList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/pointList failed!");

		skeleton->Bones[i].normalList = (uint16_t *)AllocPtr(sizeof(uint16_t) * (int)skeleton->Bones[i].numNormalsAttachedToBone);
		if (skeleton->Bones[i].normalList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/normalList failed!");

			/* READ POINT INDEX ARRAY */

		hand = GetResource('BonP',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonP resource!");
		HLock(hand);
		indexPtr = (uint16_t *)(*hand);

			/* COPY POINT INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numPointsAttachedToBone; j++)
			skeleton->Bones[i].pointList[j] = Byteswap16(&indexPtr[j]);
		ReleaseResource(hand);


			/* READ NORMAL INDEX ARRAY */
			
		hand = GetResource('BonN',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonN resource!");
		HLock(hand);
		indexPtr = (uintptr_t *)(*hand);

			/* COPY NORMAL INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numNormalsAttachedToBone; j++)
			skeleton->Bones[i].normalList[j] = Byteswap16(&indexPtr[j]);
		ReleaseResource(hand);
	}


		/*******************************/
		/* READ POINT RELATIVE OFFSETS */
		/*******************************/
		//
		// The "relative point offsets" are the only things
		// which do not get rebuilt in the ModelDecompose function.
		// We need to restore these manually.

	hand = GetResource('RelP', 1000);
	if (hand == nil)
		DoFatalAlert("Error reading RelP resource!");
	HLock(hand);
	pointPtr = (OGLPoint3D *)*hand;

	i = GetHandleSize(hand) / sizeof(OGLPoint3D);
	BYTESWAP_HANDLE("fff", OGLPoint3D, skeleton->numDecomposedPoints, hand);

	if (i != skeleton->numDecomposedPoints)
		DoFatalAlert("# of points in Reference Model has changed!");
	else
		for (i = 0; i < skeleton->numDecomposedPoints; i++)
			skeleton->decomposedPointList[i].boneRelPoint = pointPtr[i];

	ReleaseResource(hand);


			/*********************/
			/* READ ANIM INFO   */
			/*********************/

	for (i=0; i < numAnims; i++)
	{
				/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);
		if (hand == nil)
			DoFatalAlert("Error getting anim header resource");
		HLock(hand);
		BYTESWAP_HANDLE("b32bxh", SkeletonFile_AnimHeader_Type, 1, hand);
		animHeaderPtr = (SkeletonFile_AnimHeader_Type *)*hand;

		skeleton->NumAnimEvents[i] = animHeaderPtr->numAnimEvents;			// copy # anim events in anim	
		ReleaseResource(hand);

			/* READ ANIM-EVENT DATA */
			
		hand = GetResource('Evnt',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading anim-event data resource!");
		BYTESWAP_HANDLE("hbb", AnimEventType, skeleton->NumAnimEvents[i], hand);
		animEventPtr = (AnimEventType *)*hand;
		for (j=0;  j < skeleton->NumAnimEvents[i]; j++)
			skeleton->AnimEventsList[i][j] = *animEventPtr++;
		ReleaseResource(hand);		


			/* READ # KEYFRAMES PER JOINT IN EACH ANIM */
					
		hand = GetResource('NumK',1000+i);									// read array of #'s for this anim
		if (hand == nil)
			DoFatalAlert("Error reading # keyframes/joint resource!");
		for (j=0; j < numJoints; j++)
			skeleton->JointKeyframes[j].numKeyFrames[i] = (*hand)[j];
		ReleaseResource(hand);
	}


	for (j=0; j < numJoints; j++)
	{
				/* ALLOC 2D ARRAY FOR KEYFRAMES */
				
		Alloc_2d_array(JointKeyframeType,skeleton->JointKeyframes[j].keyFrames,	numAnims,MAX_KEYFRAMES);
		
		if ((skeleton->JointKeyframes[j].keyFrames == nil) || (skeleton->JointKeyframes[j].keyFrames[0] == nil))
			DoFatalAlert("ReadDataFromSkeletonFile: Error allocating Keyframe Array.");

					/* READ THIS JOINT'S KF'S FOR EACH ANIM */
					
		for (i=0; i < numAnims; i++)								
		{
			numKeyframes = skeleton->JointKeyframes[j].numKeyFrames[i];					// get actual # of keyframes for this joint
			if (numKeyframes > MAX_KEYFRAMES)
				DoFatalAlert("Error: numKeyframes > MAX_KEYFRAMES");
		
					/* READ A JOINT KEYFRAME */
					
			hand = GetResource('KeyF',1000+(i*100)+j);
			if (hand == nil)
				DoFatalAlert("Error reading joint keyframes resource!");
			BYTESWAP_HANDLE("ii3f3f3f", JointKeyframeType, numKeyframes, hand);
			keyFramePtr = (JointKeyframeType *)*hand;
			for (k = 0; k < numKeyframes; k++)												// copy this joint's keyframes for this anim
				skeleton->JointKeyframes[j].keyFrames[i][k] = *keyFramePtr++;
			ReleaseResource(hand);		
		}
	}
	
}

#pragma mark -



/******************** LOAD PREFS **********************/
//
// Load in standard preferences
//

OSErr LoadPrefs(PrefsType *prefBlock)
{
OSErr		iErr;
short		refNum;
FSSpec		file;
long		count;
				
				/*************/
				/* READ FILE */
				/*************/
					
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Billy:Prefs2", &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);	
	if (iErr)
		return(iErr);

	count = sizeof(PrefsType);
	iErr = FSRead(refNum, &count,  (Ptr)prefBlock);		// read data from file
	if (iErr)
	{
		FSClose(refNum);			
		return(iErr);
	}
	
	FSClose(refNum);			
	
			/****************/
			/* VERIFY PREFS */
			/****************/

	if ((gGamePrefs.depth != 16) && (gGamePrefs.depth != 32))
		goto err;
	
	if (gGamePrefs.version != CURRENT_PREFS_VERS)
		goto err;
	
	return(noErr);
	
err:
	InitDefaultPrefs();
	return(noErr);	
}



/******************** SAVE PREFS **********************/

void SavePrefs(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;
		
						
				/* CREATE BLANK FILE */
				
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":Billy:Prefs2", &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, kGameID, 'Pref', smSystemScript);					// create blank file
	if (iErr)
		return;

				/* OPEN FILE */
					
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
		FSpDelete(&file);
		return;
	}
		
				/* WRITE DATA */

	count = sizeof(PrefsType);
	FSWrite(refNum, &count, &gGamePrefs);	
	FSClose(refNum);
	
	
}

#pragma mark -




/**************** DRAW PICTURE INTO GWORLD ***********************/
//
// Uses Quicktime to load any kind of picture format file and draws
// it into the GWorld
//
//
// INPUT: myFSSpec = spec of image file
//
// OUTPUT:	theGWorld = gworld contining the drawn image.
//

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth)
{
#if 1
	IMPLEMENT_ME_SOFT();
	return unimpErr;
#else
OSErr						iErr;
GraphicsImportComponent		gi;
Rect						r;
ComponentResult				result;
PixMapHandle 				hPixMap;
	

			/* PREP IMPORTER COMPONENT */
			
	result = GetGraphicsImporterForFile(myFSSpec, &gi);		// load importer for this image file
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GetGraphicsImporterForFile failed!  You do not have Quicktime properly installed, reinstall Quicktime and do a FULL install.");
		return(result);
	}
	if (GraphicsImportGetBoundsRect(gi, &r) != noErr)		// get dimensions of image
		DoFatalAlert("DrawPictureIntoGWorld: GraphicsImportGetBoundsRect failed!");


			/* MAKE GWORLD */
	
	iErr = NewGWorld(theGWorld, depth, &r, nil, nil, 0);					// try app mem
	if (iErr)
	{
		DoAlert("DrawPictureIntoGWorld: using temp mem");
		iErr = NewGWorld(theGWorld, depth, &r, nil, nil, useTempMem);		// try sys mem
		if (iErr)
		{
			DoAlert("DrawPictureIntoGWorld: MakeMyGWorld failed");
			return(1);
		}
	}

	if (depth == 32)
	{
		hPixMap = GetGWorldPixMap(*theGWorld);				// get gworld's pixmap
		(**hPixMap).cmpCount = 4;							// we want full 4-component argb (defaults to only rgb)
	}


			/* DRAW INTO THE GWORLD */
	
	DoLockPixels(*theGWorld);	
	GraphicsImportSetGWorld(gi, *theGWorld, nil);			// set the gworld to draw image into
	GraphicsImportSetQuality(gi,codecLosslessQuality);		// set import quality

	result = GraphicsImportDraw(gi);						// draw into gworld
	CloseComponent(gi);										// cleanup
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GraphicsImportDraw failed!");
		ShowSystemErr(result);
		DisposeGWorld (*theGWorld);
		*theGWorld= nil;
		return(result);
	}
	return(noErr);
#endif
}


#pragma mark -

/******************* LOAD PLAYFIELD *******************/

void LoadPlayfield(FSSpec *specPtr, OGLSetupOutputType *setupInfo)
{
	
	gDisableHiccupTimer = true;
	
			/* READ PLAYFIELD RESOURCES */
						
	ReadDataFromPlayfieldFile(specPtr, setupInfo);
		
		
				/* DO ADDITIONAL SETUP */
	
	CreateSuperTileMemoryList();				// allocate memory for the supertile geometry
	CalculateSplitModeMatrix();					// precalc the tile split mode matrix
	InitSuperTileGrid();						// init the supertile state grid
		
	BuildTerrainItemList();						// build list of items & find player start coords


			/* CAST ITEM SHADOWS */
			
	DoItemShadowCasting(setupInfo);
}


/********************** READ DATA FROM PLAYFIELD FILE ************************/

static void ReadDataFromPlayfieldFile(FSSpec *specPtr, OGLSetupOutputType *setupInfo)
{
Handle					hand;
PlayfieldHeaderType		**header;
long					row,col,j,i,size;
float					yScale;
float					*src;
short					fRefNum;	
OSErr					iErr;	
Ptr						tempBuffer16 = nil;		
Ptr						tempBuffer24 = nil;		
			
				/* OPEN THE REZ-FORK */
			
	fRefNum = FSpOpenResFile(specPtr,fsCurPerm);
	if (fRefNum == -1)
		DoFatalAlert("LoadPlayfield: FSpOpenResFile failed.  You seem to have a corrupt or missing file.  Please reinstall the game.");
	UseResFile(fRefNum);

	
			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	if (hand == nil)
	{
		DoAlert("ReadDataFromPlayfieldFile: Error reading header resource!");
		return;
	}
	
	header = (PlayfieldHeaderType **)hand;	
	gNumTerrainItems		= (**header).numItems;
	gTerrainTileWidth		= (**header).mapWidth;
	gTerrainTileDepth		= (**header).mapHeight;	
	g3DTileSize				= (**header).tileSize;
	g3DMinY					= (**header).minY;
	g3DMaxY					= (**header).maxY;
	gNumSplines				= (**header).numSplines;
	gNumFences				= (**header).numFences;
	gNumWaterPatches		= (**header).numWaterPatches;
	gNumUniqueSuperTiles	= (**header).numUniqueSuperTiles;
	gNumLineMarkers			= (**header).numCheckpoints;
	
	ReleaseResource(hand);

	if ((gTerrainTileWidth % SUPERTILE_SIZE) != 0)		// terrain must be non-fractional number of supertiles in w/h
		DoFatalAlert("ReadDataFromPlayfieldFile: terrain width not a supertile multiple");
	if ((gTerrainTileDepth % SUPERTILE_SIZE) != 0)		
		DoFatalAlert("ReadDataFromPlayfieldFile: terrain depth not a supertile multiple");

				
				/* CALC SOME GLOBALS HERE */
				
	gTerrainTileWidth = (gTerrainTileWidth/SUPERTILE_SIZE)*SUPERTILE_SIZE;		// round size down to nearest supertile multiple
	gTerrainTileDepth = (gTerrainTileDepth/SUPERTILE_SIZE)*SUPERTILE_SIZE;		
	gTerrainUnitWidth = gTerrainTileWidth*gTerrainPolygonSize;					// calc world unit dimensions of terrain
	gTerrainUnitDepth = gTerrainTileDepth*gTerrainPolygonSize;
	gNumSuperTilesDeep = gTerrainTileDepth/SUPERTILE_SIZE;						// calc size in supertiles
	gNumSuperTilesWide = gTerrainTileWidth/SUPERTILE_SIZE;	


			/*******************************/
			/* SUPERTILE RELATED RESOURCES */
			/*******************************/
	
			/* READ SUPERTILE GRID MATRIX */
			
	if (gSuperTileTextureGrid)														// free old array
		Free_2d_array(gSuperTileTextureGrid);
	Alloc_2d_array(short, gSuperTileTextureGrid, gNumSuperTilesDeep, gNumSuperTilesWide);
	
	hand = GetResource('STgd',1000);												// load grid from rez
	if (hand == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: Error reading supertile rez resource!");
	else																			// copy rez into 2D array
	{
		short *src = (short *)*hand;					
		
		for (row = 0; row < gNumSuperTilesDeep; row++)
			for (col = 0; col < gNumSuperTilesWide; col++)
			{
				gSuperTileTextureGrid[row][col] = *src++;
			}
		ReleaseResource(hand);
	}		
	

			/*******************************/
			/* MAP LAYER RELATED RESOURCES */
			/*******************************/
		
	
			/* READ HEIGHT DATA MATRIX */
	
	yScale = gTerrainPolygonSize / g3DTileSize;											// need to scale original geometry units to game units
	
	
	
	Alloc_2d_array(float, gMapYCoords, gTerrainTileDepth+1, gTerrainTileWidth+1);			// alloc 2D array for map
	Alloc_2d_array(float, gMapYCoordsOriginal, gTerrainTileDepth+1, gTerrainTileWidth+1);	// and the copy of it
	
	hand = GetResource('YCrd',1000);
	if (hand == nil)
		DoAlert("ReadDataFromPlayfieldFile: Error reading height data resource!");
	else
	{
		src = (float *)*hand;					
		for (row = 0; row <= gTerrainTileDepth; row++)
			for (col = 0; col <= gTerrainTileWidth; col++)
				gMapYCoordsOriginal[row][col] = gMapYCoords[row][col] = *src++ * yScale;
		ReleaseResource(hand);
	}
	
				/**************************/
				/* ITEM RELATED RESOURCES */
				/**************************/
	
				/* READ ITEM LIST */

	hand = GetResource('Itms',1000);
	if (hand == nil)
		DoAlert("ReadDataFromPlayfieldFile: Error reading itemlist resource!");
	else
	{
		DetachResource(hand);							// lets keep this data around		
		HLockHi(hand);									// LOCK this one because we have the lookup table into this
		gMasterItemList = (TerrainItemEntryType **)hand;
	}
	
				/* CONVERT COORDINATES */
				
	for (i = 0; i < gNumTerrainItems; i++)
	{
		(*gMasterItemList)[i].x *= gMapToUnitValue;
		(*gMasterItemList)[i].y *= gMapToUnitValue;	
	}


	

			/****************************/
			/* SPLINE RELATED RESOURCES */
			/****************************/
	
			/* READ SPLINE LIST */
			
	hand = GetResource('Spln',1000);
	if (hand)
	{	
		DetachResource(hand);
		HLockHi(hand);
		gSplineList = (SplineDefType **)hand;
	}
	else
		gNumSplines = 0;	

	
			/* READ SPLINE POINT LIST */
			
	for (i = 0; i < gNumSplines; i++)
	{
		hand = GetResource('SpPt',1000+i);
		if (hand)
		{	
			DetachResource(hand);
			HLockHi(hand);
			(*gSplineList)[i].pointList = (SplinePointType **)hand;
		}
		else
			DoFatalAlert("ReadDataFromPlayfieldFile: cant get spline points rez");
	}	


			/* READ SPLINE ITEM LIST */
			
	for (i = 0; i < gNumSplines; i++)
	{
		hand = GetResource('SpIt',1000+i);
		if (hand)
		{	
			DetachResource(hand);
			HLockHi(hand);
			(*gSplineList)[i].itemList = (SplineItemType **)hand;
		}
		else
			DoFatalAlert("ReadDataFromPlayfieldFile: cant get spline items rez");
	}	
	
			/****************************/
			/* FENCE RELATED RESOURCES */
			/****************************/
	
			/* READ FENCE LIST */
		
	hand = GetResource('Fenc',1000);
	if (hand)
	{	
		FileFenceDefType *inData;

		gFenceList = (FenceDefType *)AllocPtr(sizeof(FenceDefType) * gNumFences);	// alloc new ptr for fence data
		if (gFenceList == nil)
			DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed");
			
		inData = (FileFenceDefType *)*hand;								// get ptr to input fence list
		
		for (i = 0; i < gNumFences; i++)								// copy data from rez to new list
		{
			gFenceList[i].type 		= inData[i].type;
			gFenceList[i].numNubs 	= inData[i].numNubs;
			gFenceList[i].nubList 	= nil;
			gFenceList[i].sectionVectors = nil;
		}		
		ReleaseResource(hand);
	}
	else
		gNumFences = 0;	

	
			/* READ FENCE NUB LIST */
			
	for (i = 0; i < gNumFences; i++)
	{
		hand = GetResource('FnNb',1000+i);					// get rez
		HLock(hand);
		if (hand)
		{
   			FencePointType *fileFencePoints = (FencePointType *)*hand;

			gFenceList[i].nubList = (OGLPoint3D *)AllocPtr(sizeof(FenceDefType) * gFenceList[i].numNubs);	// alloc new ptr for nub array
			if (gFenceList[i].nubList == nil)
				DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed");
		
		
		
			for (j = 0; j < gFenceList[i].numNubs; j++)		// convert x,z to x,y,z
			{
				gFenceList[i].nubList[j].x = fileFencePoints[j].x;
				gFenceList[i].nubList[j].z = fileFencePoints[j].z;
				gFenceList[i].nubList[j].y = 0;				
			}
			ReleaseResource(hand);
		}
		else
			DoFatalAlert("ReadDataFromPlayfieldFile: cant get fence nub rez");
	}


			/****************************/
			/* WATER RELATED RESOURCES */
			/****************************/

			/* READ WATER LIST */
		
	hand = GetResource('Liqd',1000);
	if (hand)
	{	
		DetachResource(hand);
		HLockHi(hand);
		gWaterListHandle = (WaterDefType **)hand;
		gWaterList = *gWaterListHandle;
	}
	else
		gNumWaterPatches = 0;	
	
	

			/*************************/
			/* LINE MARKER RESOURCES */
			/*************************/
	
	if (gNumLineMarkers > 0)
	{
		if (gNumLineMarkers > MAX_LINEMARKERS)
			DoFatalAlert("ReadDataFromPlayfieldFile: gNumLineMarkers > MAX_LINEMARKERS");
			
				/* READ CHECKPOINT LIST */
				
		hand = GetResource('CkPt',1000);
		if (hand)
		{	
			HLock(hand);
			BlockMove(*hand, &gLineMarkerList[0], GetHandleSize(hand));		
			ReleaseResource(hand);
			
						/* CONVERT COORDINATES */
						
			for (i = 0; i < gNumLineMarkers; i++)
			{
				gLineMarkerList[i].x[0] *= gMapToUnitValue;
				gLineMarkerList[i].z[0] *= gMapToUnitValue;	
				gLineMarkerList[i].x[1] *= gMapToUnitValue;
				gLineMarkerList[i].z[1] *= gMapToUnitValue;	
			}			
		}
		else
			gNumLineMarkers = 0;	
	}	
	


			/* CLOSE REZ FILE */
			
	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);


		
		/********************************************/
		/* READ SUPERTILE IMAGE DATA FROM DATA FORK */
		/********************************************/


				/* ALLOC BUFFERS */
		
	size = SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 2;						// calc size of supertile 32-bit texture
	tempBuffer16 = AllocPtr(size);
	if (tempBuffer16 == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed!");

	tempBuffer24 = AllocPtr(SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 3);		// alloc for 24bit pixels
	if (tempBuffer24 == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed!");


				/* OPEN THE DATA FORK */
				
	iErr = FSpOpenDF(specPtr, fsCurPerm, &fRefNum);	
	if (iErr)
		DoFatalAlert("ReadDataFromPlayfieldFile: FSpOpenDF failed!");
	

			
	for (i = 0; i < gNumUniqueSuperTiles; i++)
	{
		static long	sizeoflong = 4;
		long	compressedSize,decompressedSize;
		long	width,height;
		MOMaterialData	matData;
			
		
				/* READ THE SIZE OF THE NEXT COMPRESSED SUPERTILE TEXTURE */
						
		iErr = FSRead(fRefNum, &sizeoflong, &compressedSize);
		if (iErr)
			DoFatalAlert("ReadDataFromPlayfieldFile: FSRead failed!");
	
				
				/* READ & DECOMPRESS IT */
			
#if TILE_DEPTH == 16		
		decompressedSize = LZSS_Decode(fRefNum, tempBuffer16, compressedSize);				
#else
		decompressedSize = LZSS_Decode(fRefNum, tempBuffer24, compressedSize);				
#endif
		width = SUPERTILE_TEXMAP_SIZE;
		height = SUPERTILE_TEXMAP_SIZE;		


				/**************************/
				/* CREATE MATERIAL OBJECT */
				/**************************/

#if TILE_DEPTH == 16		

			/* USE PACKED PIXEL TYPE */

		ConvertTexture16To16((u_short *)tempBuffer16, width, height);
		
//		if (gGamePrefs.lowVRAMMode)
//		{
//			HalfTexture16((u_short *)tempBuffer16, width, height);
//			width /= 2;
//			height /= 2;
//		}

		matData.pixelSrcFormat 	= GL_BGRA_EXT;
		matData.pixelDstFormat 	= GL_RGBA;
		matData.textureName[0] 	= OGL_TextureMap_Load(tempBuffer16, width, height,
												 GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);



#else

		matData.pixelSrcFormat 	= GL_RGB;
		matData.pixelDstFormat 	= GL_RGB;
		matData.textureName[0] 	= OGL_TextureMap_Load(tempBuffer24, width, height,
												 matData.pixelSrcFormat, matData.pixelDstFormat, GL_UNSIGNED_BYTE);
#endif


			/* INIT NEW MATERIAL DATA */

		matData.drawContext				= gAGLContext;								// remember which draw context this material is assigned to
		matData.flags 					= 	BG3D_MATERIALFLAG_CLAMP_U|
											BG3D_MATERIALFLAG_CLAMP_V|
											BG3D_MATERIALFLAG_TEXTURED;
																								
		matData.multiTextureMode		= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;
		matData.multiTextureCombine		= MULTI_TEXTURE_COMBINE_ADD;
		matData.diffuseColor.r			= 1;
		matData.diffuseColor.g			= 1;
		matData.diffuseColor.b			= 1;
		matData.diffuseColor.a			= 1;
		matData.numMipmaps				= 1;										// 1 texture
		matData.width					= width;
		matData.height					= height;		
		matData.texturePixels[0] 		= nil;										// the original pixels are gone (or will be soon)
		gSuperTileTextureObjects[i] 	= MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);		// create the new object


			/* PRE-LOAD */
			
		MO_DrawMaterial(gSuperTileTextureObjects[i], setupInfo);			//--------------
		glBegin(GL_TRIANGLES);
		glVertex3f(0,0,0);
		glVertex3f(1,0,0);
		glVertex3f(0,0,1);
		glEnd();

	}
	
			/* CLOSE THE FILE */
			
	FSClose(fRefNum);	
		
	if (tempBuffer16)
		SafeDisposePtr(tempBuffer16);
	if (tempBuffer24)
		SafeDisposePtr(tempBuffer24);
//	if (tempBuffer32)
//		SafeDisposePtr(tempBuffer32);
}


/************************* HALF TEXTURE 16 *********************************/

static void HalfTexture16(u_short *buff, int width, int height)
{
int	r,c;
u_short	*p, *p2;

			/* SHRINK VERTICAL */

	p = buff;
	p2 = p; // + (width * 2);
	height /= 2;
		
#if 1
	for (r = 0; r < height; r++)
	{
		for (c = 0; c < width; c++)
			p[c] = p2[c];
			
		p += width;
		p2 += (width *2);
	}
#endif	


		/* SHRINK HORIZ */

#if 1

	width /= 2;
	p = buff;
	p2 = p; //+2;
			
	for (r = 0; r < height; r++)
	{
		for (c = 0; c < width; c++)
		{
			*p = *p2;
			p++;
			p2 += 2;
		}
			
//		p++;
	}
#endif	
}


/*********************** CONVERT TEXTURE; 16 TO 24 ***********************************/

static void	ConvertTexture16To24(u_short *textureBuffer2, u_char *textureBuffer3, int width, int height)
{
int		x,y;
u_short	srcPixel;
u_long	r,g,b;
u_char	*dest;

	textureBuffer3 += (width * 3) * (height - 1);				// flip Y while we do this!

	for (y = 0; y < height; y++)
	{
		dest = textureBuffer3;
		
		for (x = 0; x < width; x++)
		{
			srcPixel = textureBuffer2[x];						// get 16bit pixel

			b = srcPixel & 0x1f;
			g = (srcPixel & (0x1f << 5)) >> 5;
			r = (srcPixel & (0x1f << 10)) >> 10;
			
			*dest++ = r<<3;			// save red byte
			*dest++ = g<<3;			// save green byte
			*dest++ = b<<3;			// save blue byte
		}
		
		textureBuffer2 += width;
		textureBuffer3 -= (width*3);
	}
}		


/*********************** CONVERT TEXTURE; 16 TO 32 ***********************************/

static void	ConvertTexture16To32(u_short *srcBuff, u_char *destBuff, int width, int height)
{
int		x,y;
u_short	srcPixel;
u_long	r,g,b;
u_char	*dest;

	destBuff += (width * 4) * (height - 1);				// flip Y while we do this!

	for (y = 0; y < height; y++)
	{
		dest = destBuff;
		
		for (x = 0; x < width; x++)
		{
			srcPixel = srcBuff[x];						// get 16bit pixel

			b = srcPixel & 0x1f;
			g = (srcPixel & (0x1f << 5)) >> 5;
			r = (srcPixel & (0x1f << 10)) >> 10;
			
			*dest++ = r<<3;			// save red byte
			*dest++ = g<<3;			// save green byte
			*dest++ = b<<3;			// save blue byte
			
			if (srcPixel & 0x8000)		// see if set alpha
				*dest++ = 0xff;
			else
				*dest++ = 0x00;
		}
		
		srcBuff += width;
		destBuff -= width*4;
	}
}		


/*********************** CONVERT TEXTURE; 16 TO 16 ***********************************/
//
// Simply flips Y since OGL Textures are screwey
//

static void	ConvertTexture16To16(u_short *textureBuffer, int width, int height)
{
int		x,y;
u_short	pixel,*bottom;
u_short	*dest;
Boolean	blackOpaq;

	blackOpaq = true;	//(gLevelNum != LEVEL_NUM_CLOUD);			// make black transparent on cloud


	bottom = textureBuffer + ((height - 1) * width);

	for (y = 0; y < height / 2; y++)
	{
		dest = bottom;
		
		for (x = 0; x < width; x++)
		{
			pixel = textureBuffer[x];						// get 16bit pixel
			if ((pixel & 0x7fff) || blackOpaq)				// check/set alpha
				pixel |= 0x8000;
			else
				pixel &= 0x7fff;
				
			textureBuffer[x] = bottom[x];
			if ((textureBuffer[x] & 0x7fff) || blackOpaq)
				textureBuffer[x] |= 0x8000;
			else
				textureBuffer[x] &= 0x7fff;
			bottom[x] = pixel;
		}

		textureBuffer += width;
		bottom -= width;
	}
}		


#pragma mark -

/***************************** SAVE GAME ********************************/
//
// Returns true if saving was successful
//

Boolean SaveGame(void)
{
SaveGameType	saveData;
short			fRefNum;
FSSpec			*specPtr;
long			count, i;
Boolean			success = false;

			/* NO SAVING IN DEMO MODE */
			
	if (!gGameIsRegistered)
	{
		DoAlert("Sorry, you cannot save games in demo mode.  Please buy a serial number to unlock this feature.");
		return(false);	
	}


	Enter2D(true);
			
			/*************************/	
			/* CREATE SAVE GAME DATA */
			/*************************/	
					
	saveData.version		= SAVE_GAME_VERSION;				// save file version #
	saveData.score 			= gScore;
	saveData.numLives 		= gPlayerInfo.lives;

	for (i = 0; i < NUM_LEVELS; i++)
	{
		saveData.levels[i] = gLevelWon[i];
		saveData.duels[i] = gDuelWon[i];
	}	

		/*******************/
		/* DO NAV SERVICES */
		/*******************/

#if 1
	IMPLEMENT_ME();
#else
	if (PutFileWithNavServices(&navReply, &gSavedGameSpec))
		goto bail;	
	specPtr = &gSavedGameSpec;	
	if (navReply.replacing)										// see if delete old
		FSpDelete(specPtr);



				/* CREATE & OPEN THE REZ-FORK */
			
	if (FSpCreate(specPtr, kGameID,kSavedGameFileType,nil) != noErr)
	{
		DoAlert("Error creating Save file");
		goto bail;
	}
	
	FSpOpenDF(specPtr,fsRdWrPerm, &fRefNum);
	if (fRefNum == -1)
	{
		DoAlert("Error opening Save file");
		goto bail;
	}
				
	
				/* WRITE TO FILE */

	count = sizeof(SaveGameType);
	if (FSWrite(fRefNum, &count, (Ptr)&saveData) != noErr)
	{
		DoAlert("Error writing Save file");
		FSClose(fRefNum);
		goto bail;
	}

			/* CLOSE FILE */
			
	FSClose(fRefNum);						


			/* CLEANUP NAV SERVICES */
				
	NavCompleteSave(&navReply, kNavTranslateInPlace);
	
	success = true;
	
bail:	
	NavDisposeReply(&navReply);
	HideCursor();
	Exit2D();
	TurnOnISp();
#endif
	return(success);
}


/***************************** LOAD SAVED GAME ********************************/

Boolean LoadSavedGame(void)
{
SaveGameType	saveData;
short			fRefNum;
long			count, i;
Boolean			success = false;

	Enter2D(true);
	MyFlushEvents();	
	

				/* GET FILE WITH NAVIGATION SERVICES */
#if 1
	IMPLEMENT_ME();
#else
	if (GetFileWithNavServices(&gSavedGameSpec) != noErr)
		goto bail;
	
	
				/* OPEN THE REZ-FORK */
			
	FSpOpenDF(&gSavedGameSpec,fsRdPerm, &fRefNum);
	if (fRefNum == -1)
	{
		DoAlert("Error opening Save file");
		goto bail;
	}

				/* READ FROM FILE */

	count = sizeof(SaveGameType);
	if (FSRead(fRefNum, &count, &saveData) != noErr)
	{
		DoAlert("Error reading Save file");
		FSClose(fRefNum);
		goto bail;
	}

			/* CLOSE FILE */
			
	FSClose(fRefNum);						

	
			/**********************/	
			/* USE SAVE GAME DATA */
			/**********************/	
					
	gLoadedScore = gScore = saveData.score;
	gPlayerInfo.lives 	= saveData.numLives;
	if (gPlayerInfo.lives > 20)							// check for corruption
		gPlayerInfo.lives = 0;
		
	for (i = 0; i < NUM_LEVELS; i++)
	{
		gLevelWon[i] = saveData.levels[i];
		gDuelWon[i] = saveData.duels[i];
	}
		
	success = true;
			
		
bail:
	Exit2D();
	HideCursor();
	TurnOnISp();
	
#endif
	return(success);
}



#pragma mark -


/*********************** LOAD DATA FILE INTO MEMORY ***********************************/
//
// Use SafeDisposePtr when done.
//

Ptr LoadDataFile(const char* path, long* outLength)
{
	FSSpec spec;
	OSErr err;
	short refNum;
	long fileLength = 0;
	long readBytes = 0;

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
	GAME_ASSERT_MESSAGE(!err, path);

	err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	GAME_ASSERT_MESSAGE(!err, path);

	// Get number of bytes until EOF
	GetEOF(refNum, &fileLength);

	// Prep data buffer
	// Alloc 1 extra byte so LoadTextFile can return a null-terminated C string!
	Ptr data = AllocPtrClear(fileLength + 1);

	// Read file into data buffer
	readBytes = fileLength;
	err = FSRead(refNum, &readBytes, (Ptr) data);
	GAME_ASSERT_MESSAGE(err == noErr, path);
	FSClose(refNum);

	GAME_ASSERT_MESSAGE(fileLength == readBytes, path);

	if (outLength)
	{
		*outLength = fileLength;
	}

	return data;
}

/*********************** LOAD TEXT FILE INTO MEMORY ***********************************/
//
// Use SafeDisposePtr when done.
//

char* LoadTextFile(const char* spec, long* outLength)
{
	return LoadDataFile(spec, outLength);
}



#pragma mark -

/*********************** PARSE CSV *****************************/
//
// Call this function repeatedly to iterate over cells in a CSV table.
// THIS FUNCTION MODIFIES THE INPUT BUFFER!
//
// Sample usage:
//
//		char* csvText = LoadTextFile("hello.csv", NULL);
//		char* csvReader = csvText;
//		bool endOfLine = false;
//
//		while (csvReader != NULL)
//		{
//			char* column = CSVIterator(&csvReader, &endOfLine);
//			puts(column);
//		}
//
//		SafeDisposePtr(csvText);
//

char* CSVIterator(char** csvCursor, bool* eolOut)
{
	enum
	{
		kCSVState_Stop,
		kCSVState_WithinQuotedString,
		kCSVState_WithinUnquotedString,
		kCSVState_AwaitingSeparator,
	};

	const char SEPARATOR = ',';
	const char QUOTE_DELIMITER = '"';

	GAME_ASSERT(csvCursor);
	GAME_ASSERT(*csvCursor);

	const char* reader = *csvCursor;
	char* writer = *csvCursor;		// we'll write over the column as we read it
	char* columnStart = writer;
	bool eol = false;

	if (reader[0] == '\0')
	{
		reader = NULL;			// signify the parser should stop
		columnStart = NULL;		// signify nothing more to read
		eol = true;
	}
	else
	{
		int state;

		if (*reader == QUOTE_DELIMITER)
		{
			state = kCSVState_WithinQuotedString;
			reader++;
		}
		else
		{
			state = kCSVState_WithinUnquotedString;
		}

		while (*reader && state != kCSVState_Stop)
		{
			if (reader[0] == '\r' && reader[1] == '\n')
			{
				// windows CRLF -- skip the \r; we'll look at the \n later
				reader++;
				continue;
			}

			switch (state)
			{
				case kCSVState_WithinQuotedString:
					if (*reader == QUOTE_DELIMITER)
					{
						state = kCSVState_AwaitingSeparator;
					}
					else
					{
						*writer = *reader;
						writer++;
					}
					break;

				case kCSVState_WithinUnquotedString:
					if (*reader == SEPARATOR)
					{
						state = kCSVState_Stop;
					}
					else if (*reader == '\n')
					{
						eol = true;
						state = kCSVState_Stop;
					}
					else
					{
						*writer = *reader;
						writer++;
					}
					break;

				case kCSVState_AwaitingSeparator:
					if (*reader == SEPARATOR)
					{
						state = kCSVState_Stop;
					}
					else if (*reader == '\n')
					{
						eol = true;
						state = kCSVState_Stop;
					}
					else
					{
						GAME_ASSERT_MESSAGE(false, "unexpected token in CSV file");
					}
					break;
			}

			reader++;
		}
	}

	*writer = '\0';	// terminate string

	if (reader != NULL)
	{
		GAME_ASSERT_MESSAGE(reader >= writer, "writer went past reader!!!");
	}

	*csvCursor = (char*) reader;
	*eolOut = eol;
	return columnStart;
}
