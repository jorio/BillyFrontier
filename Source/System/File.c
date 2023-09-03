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

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType);
static void ReadDataFromPlayfieldFile(FSSpec *specPtr);

#define BYTESWAP_HANDLE(format, type, n, handle)                                  \
{                                                                                 \
	if ((n) * sizeof(type) > (unsigned long) GetHandleSize((Handle) (handle)))   \
		DoFatalAlert("BYTESWAP_HANDLE: size mismatch!\nHdl=%ld Exp=%ld\nWhen swapping %dx %s in %s:%d", \
			GetHandleSize((Handle) (handle)), \
			(n) * sizeof(type), \
			n, #type, __func__, __LINE__); \
	UnpackStructs((format), sizeof(type), (n), *(handle));                      \
}

/****************************/
/*    CONSTANTS             */
/****************************/

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1


		/* PLAYFIELD HEADER */
		
typedef struct
{
	NumVersion	version;							// version of file
	int32_t		numItems;							// # items in map
	int32_t		mapWidth;							// width of map
	int32_t		mapHeight;							// height of map
	float		tileSize;							// 3D unit size of a tile
	float		minY;								// min height value
	float		maxY;								// max height value
	int32_t		numSplines;							// # splines
	int32_t		numFences;							// # fences
	int32_t		numUniqueSuperTiles;				// # unique supertile
	int32_t		numWaterPatches;					// # water patches
	int32_t		numCheckpoints;						// # checkpoints
	int32_t		unused[10];
}PlayfieldHeaderType;


		/* FENCE STRUCTURE IN FILE */
		//
		// note: we copy this data into our own fence list
		//		since the game uses a slightly different
		//		data structure.
		//
		
typedef	struct
{
	int32_t		x,z;
}FencePointType;


typedef struct
{
	uint16_t		type;				// type of fence
	int16_t			numNubs;			// # nubs in fence
//	FencePointType	**nubList;			// handle to nub list
	uint32_t		_junkPtr;
	Rect			bBox;				// bounding box of fence area	
}FileFenceDefType;




/**********************/
/*     VARIABLES      */
/**********************/

float	g3DTileSize, g3DMinY, g3DMaxY;



/******************* LOAD SKELETON *******************/
//
// Loads a skeleton file & creates storage for it.
//
// OUTPUT:	Ptr to skeleton data
//

SkeletonDefType *LoadSkeletonFile(short skeletonType)
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
		DoFatalAlert("System Error %d opening Skel Rez file", iErr);
	}
	
	UseResFile(fRefNum);
	if (ResError())
		DoFatalAlert("Error using Rez file!");

			
			/* ALLOC MEMORY FOR SKELETON INFO STRUCTURE */
			
	skeleton = (SkeletonDefType *)AllocPtr(sizeof(SkeletonDefType));
	if (skeleton == nil)
		DoFatalAlert("Cannot alloc SkeletonInfoType");


			/* READ SKELETON RESOURCES */
			
	ReadDataFromSkeletonFile(skeleton, &fsSpec, skeletonType);
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

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType)
{
Handle				hand;
long				numJoints = 0;
long				numAnims = 0;
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
	BYTESWAP_HANDLE(">4h", SkeletonFile_Header_Type, 1, hand);
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
			LoadBonesReferenceModel(&target, skeleton, skeletonType);
		else
			DoFatalAlert("ReadDataFromSkeletonFile: Cannot find Skeleton's BG3D file!");
		ReleaseResource((Handle)alias);
	}
	else
		DoFatalAlert("ReadDataFromSkeletonFile: file is missing the Alias resource");
	


		/***********************************/
		/*  READ BONE DEFINITION RESOURCES */
		/***********************************/

	for (int i = 0; i < numJoints; i++)
	{
		File_BoneDefinitionType	*bonePtr;
		uint16_t				*indexPtr;

			/* READ BONE DATA */

		hand = GetResource('Bone',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading Bone resource!");
		BYTESWAP_HANDLE(">i32b3fHH8L", File_BoneDefinitionType, 1, hand);
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

		for (int j = 0; j < skeleton->Bones[i].numPointsAttachedToBone; j++)
			skeleton->Bones[i].pointList[j] = UnpackU16BE(&indexPtr[j]);
		ReleaseResource(hand);


			/* READ NORMAL INDEX ARRAY */
			
		hand = GetResource('BonN',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonN resource!");
		HLock(hand);
		indexPtr = (uint16_t *)(*hand);

			/* COPY NORMAL INDEX ARRAY INTO BONE STRUCT */

		for (int j = 0; j < skeleton->Bones[i].numNormalsAttachedToBone; j++)
			skeleton->Bones[i].normalList[j] = UnpackU16BE(&indexPtr[j]);
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

	int numDecomposedPointsDeducted = (int) (GetHandleSize(hand) / sizeof(OGLPoint3D));
	BYTESWAP_HANDLE(">fff", OGLPoint3D, skeleton->numDecomposedPoints, hand);

	if (numDecomposedPointsDeducted != skeleton->numDecomposedPoints)
		DoFatalAlert("# of points in Reference Model has changed!");
	else
		for (int i = 0; i < skeleton->numDecomposedPoints; i++)
			skeleton->decomposedPointList[i].boneRelPoint = pointPtr[i];

	ReleaseResource(hand);


			/*********************/
			/* READ ANIM INFO   */
			/*********************/

	for (int i = 0; i < numAnims; i++)
	{
				/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);
		if (hand == nil)
			DoFatalAlert("Error getting anim header resource");
		HLock(hand);
		BYTESWAP_HANDLE(">b32bxh", SkeletonFile_AnimHeader_Type, 1, hand);
		animHeaderPtr = (SkeletonFile_AnimHeader_Type *)*hand;

		skeleton->NumAnimEvents[i] = animHeaderPtr->numAnimEvents;			// copy # anim events in anim	
		ReleaseResource(hand);

			/* READ ANIM-EVENT DATA */
			
		hand = GetResource('Evnt',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading anim-event data resource!");
		BYTESWAP_HANDLE(">hbb", AnimEventType, skeleton->NumAnimEvents[i], hand);
		animEventPtr = (AnimEventType *)*hand;
		for (int j = 0;  j < skeleton->NumAnimEvents[i]; j++)
			skeleton->AnimEventsList[i][j] = *animEventPtr++;
		ReleaseResource(hand);		


			/* READ # KEYFRAMES PER JOINT IN EACH ANIM */
					
		hand = GetResource('NumK',1000+i);									// read array of #'s for this anim
		if (hand == nil)
			DoFatalAlert("Error reading # keyframes/joint resource!");
		for (int j = 0; j < numJoints; j++)
			skeleton->JointKeyframes[j].numKeyFrames[i] = (*hand)[j];
		ReleaseResource(hand);
	}


	for (int j = 0; j < numJoints; j++)
	{
				/* ALLOC 2D ARRAY FOR KEYFRAMES */
				
		Alloc_2d_array(JointKeyframeType,skeleton->JointKeyframes[j].keyFrames,	numAnims,MAX_KEYFRAMES);
		
		if ((skeleton->JointKeyframes[j].keyFrames == nil) || (skeleton->JointKeyframes[j].keyFrames[0] == nil))
			DoFatalAlert("ReadDataFromSkeletonFile: Error allocating Keyframe Array.");

					/* READ THIS JOINT'S KF'S FOR EACH ANIM */

		for (int i = 0; i < numAnims; i++)
		{
			unsigned int numKeyframes = skeleton->JointKeyframes[j].numKeyFrames[i];		// get actual # of keyframes for this joint
			if (numKeyframes > MAX_KEYFRAMES)
				DoFatalAlert("Error: numKeyframes > MAX_KEYFRAMES");
		
					/* READ A JOINT KEYFRAME */
					
			hand = GetResource('KeyF',1000+(i*100)+j);
			if (hand == nil)
				DoFatalAlert("Error reading joint keyframes resource!");
			BYTESWAP_HANDLE(">ii3f3f3f", JointKeyframeType, numKeyframes, hand);
			keyFramePtr = (JointKeyframeType *)*hand;
			for (unsigned int k = 0; k < numKeyframes; k++)									// copy this joint's keyframes for this anim
				skeleton->JointKeyframes[j].keyFrames[i][k] = *keyFramePtr++;
			ReleaseResource(hand);		
		}
	}
}

#pragma mark -


/******************** FIND PREFS FOLDER **********************/

void InitPrefsFolder(bool createIt)
{
	OSErr iErr;
	long createdDirID;

			/* CHECK PREFERENCES FOLDER */

	iErr = FindFolder(kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder,		// locate the folder
		&gPrefsFolderVRefNum, &gPrefsFolderDirID);
	if (iErr != noErr)
		DoAlert("Warning: Cannot locate the Preferences folder.");

	if (createIt)
	{
		iErr = DirCreate(gPrefsFolderVRefNum, gPrefsFolderDirID, PREFS_FOLDER_NAME, &createdDirID);		// make folder in there
	}
}

/********* MAKE FSSPEC FOR USER FILE IN PREFS FOLDER ***********/

static OSErr MakeFSSpecForUserDataFile(const char* filename, FSSpec* spec)
{
	char path[256];
	snprintf(path, sizeof(path), ":%s:%s", PREFS_FOLDER_NAME, filename);

	return FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, path, spec);
}

/********* LOAD STRUCT FROM USER FILE IN PREFS FOLDER ***********/

OSErr LoadUserDataFile(const char* filename, const char* magic, long payloadLength, Ptr payloadPtr)
{
OSErr		iErr;
short		refNum;
FSSpec		file;
long		count;
long		eof = 0;
char		fileMagic[64];
long		magicLength = (long) strlen(magic) + 1;		// including null-terminator

	GAME_ASSERT(magicLength < (long) sizeof(fileMagic));

	InitPrefsFolder(false);


				/*************/
				/* READ FILE */
				/*************/

	MakeFSSpecForUserDataFile(filename, &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr)
		return iErr;

				/* CHECK FILE LENGTH */

	GetEOF(refNum, &eof);

	if (eof != magicLength + payloadLength)
		goto fileIsCorrupt;

				/* READ HEADER */

	count = magicLength;
	iErr = FSRead(refNum, &count, fileMagic);
	if (iErr ||
		count != magicLength ||
		0 != strncmp(magic, fileMagic, magicLength-1))
	{
		goto fileIsCorrupt;
	}

				/* READ PAYLOAD */

	count = payloadLength;
	iErr = FSRead(refNum, &count, payloadPtr);
	if (iErr || count != payloadLength)
	{
		goto fileIsCorrupt;
	}

	FSClose(refNum);
	return noErr;

fileIsCorrupt:
	printf("File '%s' appears to be corrupt!\n", file.cName);
	FSClose(refNum);
	return badFileFormat;
}


/********* SAVE STRUCT TO USER FILE IN PREFS FOLDER ***********/

OSErr SaveUserDataFile(const char* filename, const char* magic, long payloadLength, Ptr payloadPtr)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

	InitPrefsFolder(true);

				/* CREATE BLANK FILE */

	MakeFSSpecForUserDataFile(filename, &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, kGameID, 'Pref', smSystemScript);					// create blank file
	if (iErr)
	{
		return iErr;
	}

				/* OPEN FILE */

	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
		FSpDelete(&file);
		return iErr;
	}

				/* WRITE MAGIC */

	count = (long) strlen(magic) + 1;
	iErr = FSWrite(refNum, &count, (Ptr) magic);
	if (iErr)
	{
		FSClose(refNum);
		return iErr;
	}

				/* WRITE DATA */

	count = payloadLength;
	iErr = FSWrite(refNum, &count, payloadPtr);
	FSClose(refNum);

	printf("Wrote %s\n", file.cName);

	return iErr;
}


#pragma mark -

/******************** LOAD PREFS **********************/
//
// Load in standard preferences
//

OSErr LoadPrefs(void)
{
	OSErr err = LoadUserDataFile(PREFS_FILE_NAME, PREFS_MAGIC, sizeof(PrefsType), (Ptr) &gGamePrefs);

	if (err != noErr)
	{
		InitDefaultPrefs();
	}

//	memcpy(&gDiskShadowPrefs, &gGamePrefs, sizeof(gDiskShadowPrefs));

	return err;
}


/******************** SAVE PREFS **********************/

void SavePrefs(void)
{
	// If prefs didn't change relative to what's on disk, don't bother rewriting them
	//if (0 == memcmp(&gDiskShadowPrefs, &gGamePrefs, sizeof(gGamePrefs)))
	//{
	//	return;
	//}

	SaveUserDataFile(PREFS_FILE_NAME, PREFS_MAGIC, sizeof(PrefsType), (Ptr)&gGamePrefs);

	//memcpy(&gDiskShadowPrefs, &gGamePrefs, sizeof(gGamePrefs));
}


#pragma mark -

/******************* LOAD PLAYFIELD *******************/

void LoadPlayfield(FSSpec *specPtr)
{
	
	gDisableHiccupTimer = true;
	
			/* READ PLAYFIELD RESOURCES */
						
	ReadDataFromPlayfieldFile(specPtr);
		
		
				/* DO ADDITIONAL SETUP */
	
	CreateSuperTileMemoryList();				// allocate memory for the supertile geometry
	CalculateSplitModeMatrix();					// precalc the tile split mode matrix
	InitSuperTileGrid();						// init the supertile state grid
		
	BuildTerrainItemList();						// build list of items & find player start coords


			/* CAST ITEM SHADOWS */
			
	DoItemShadowCasting();
}


/********************** READ DATA FROM PLAYFIELD FILE ************************/

static void ReadDataFromPlayfieldFile(FSSpec *specPtr)
{
Handle					hand;
PlayfieldHeaderType		**header;
long					row,col,j,i,size;
float					yScale;
short					fRefNum;
OSErr					iErr;	
Ptr						tempBuffer16 = nil;

				/* OPEN THE REZ-FORK */
			
	fRefNum = FSpOpenResFile(specPtr, fsRdPerm);
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
	BYTESWAP_HANDLE(">4b iii fff iiiii 10i", PlayfieldHeaderType, 1, hand);
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
		int16_t *src = (int16_t *)*hand;
		BYTESWAP_HANDLE(">h", int16_t, gNumSuperTilesDeep*gNumSuperTilesWide, hand);

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
		float* src = (float *)*hand;
		BYTESWAP_HANDLE(">f", float, (gTerrainTileWidth+1)*(gTerrainTileDepth+1), hand);

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

		BYTESWAP_HANDLE(">IIH4BH", TerrainItemEntryType, gNumTerrainItems, hand);
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

		// SOURCE PORT NOTE: we have to convert this structure manually,
		// because the original contains 4-byte pointers
		BYTESWAP_HANDLE(">hxxi ii hxxi 4h", File_SplineDefType, gNumSplines, hand);

		gSplineList = (SplineDefType **) NewHandleClear(gNumSplines * sizeof(SplineDefType));

		for (i = 0; i < gNumSplines; i++)
		{
			const File_SplineDefType*	srcSpline = &(*((File_SplineDefType **) hand))[i];
			SplineDefType*				dstSpline = &(*gSplineList)[i];

			dstSpline->numNubs		= srcSpline->numNubs;
			dstSpline->numPoints	= srcSpline->numPoints;
			dstSpline->numItems		= srcSpline->numItems;
			dstSpline->bBox			= srcSpline->bBox;
		}

		DisposeHandle(hand);
	}
	else
	{
		gNumSplines = 0;
		gSplineList = nil;
	}


			/* READ SPLINE POINT LIST */
			
	for (i = 0; i < gNumSplines; i++)
	{
		// If spline has 0 points, skip the byteswapping, but do alloc an empty handle, which the game expects.
		if ((*gSplineList)[i].numPoints == 0)
		{
#if _DEBUG
			printf("WARNING: Spline #%ld has 0 points\n", i);
#endif
			(*gSplineList)[i].pointList = (SplinePointType**) AllocHandle(0);
			continue;
		}

		hand = GetResource('SpPt',1000+i);
		if (hand)
		{
			DetachResource(hand);
			HLockHi(hand);
			BYTESWAP_HANDLE(">ff", SplinePointType, (*gSplineList)[i].numPoints, hand);
			(*gSplineList)[i].pointList = (SplinePointType **)hand;
		}
		else
			DoFatalAlert("ReadDataFromPlayfieldFile: cant get spline points rez");
	}	


			/* READ SPLINE ITEM LIST */
			
	for (i = 0; i < gNumSplines; i++)
	{
		// If spline has 0 items, skip the byteswapping, but do alloc an empty handle, which the game expects.
		if ((*gSplineList)[i].numItems == 0)
		{
#if _DEBUG
			printf("WARNING: Spline #%ld has 0 items\n", i);
#endif
			(*gSplineList)[i].itemList = (SplineItemType**) AllocHandle(0);
			continue;
		}

		hand = GetResource('SpIt',1000+i);
		if (hand)
		{
			DetachResource(hand);
			HLockHi(hand);
			BYTESWAP_HANDLE(">fH4bH", SplineItemType, (*gSplineList)[i].numItems, hand);
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

		BYTESWAP_HANDLE(">Hhi4h", FileFenceDefType, gNumFences, hand);
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
	{
		gNumFences = 0;
		gFenceList = nil;
	}

	
			/* READ FENCE NUB LIST */
			
	for (i = 0; i < gNumFences; i++)
	{
		hand = GetResource('FnNb',1000+i);					// get rez
		HLock(hand);
		if (hand)
		{
   			FencePointType *fileFencePoints = (FencePointType *)*hand;
			BYTESWAP_HANDLE(">ii", FencePointType, gFenceList[i].numNubs, hand);

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
		GAME_ASSERT(MAX_WATER_POINTS == 100);		// if this changes, the byteswap format below needs to change as well
		BYTESWAP_HANDLE(">Hxx L i hxx i 200f ff 4h", WaterDefType, gNumWaterPatches, hand);
		gWaterList = *gWaterListHandle;
	}
	else
	{
		gNumWaterPatches = 0;
		gWaterListHandle = nil;
		gWaterList = nil;
	}


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
			BYTESWAP_HANDLE(">hh 2f 2f", LineMarkerDefType, gNumLineMarkers, hand);
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
		{
			gNumLineMarkers = 0;
		}
	}



			/* CLOSE REZ FILE */
			
	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);


		
		/********************************************/
		/* READ SUPERTILE IMAGE DATA FROM DATA FORK */
		/********************************************/


				/* ALLOC BUFFERS */
		
	size = SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 2;						// calc size of supertile 16-bit texture
	tempBuffer16 = AllocPtr(size);
	if (tempBuffer16 == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed!");


				/* OPEN THE DATA FORK */
				
	iErr = FSpOpenDF(specPtr, fsRdPerm, &fRefNum);
	if (iErr)
		DoFatalAlert("ReadDataFromPlayfieldFile: FSpOpenDF failed!");
	

			
	for (i = 0; i < gNumUniqueSuperTiles; i++)
	{
		int32_t	compressedSize;
		long	sizeoflong = sizeof(compressedSize);
		int	width,height;
		MOMaterialData	matData;
			
		
				/* READ THE SIZE OF THE NEXT COMPRESSED SUPERTILE TEXTURE */
						
		iErr = FSRead(fRefNum, &sizeoflong, (Ptr) &compressedSize);
		if (iErr)
			DoFatalAlert("ReadDataFromPlayfieldFile: FSRead failed!");
		compressedSize = UnpackI32BE(&compressedSize);

				/* READ & DECOMPRESS IT */

		size_t decompressedSize = LZSS_Decode(fRefNum, tempBuffer16, compressedSize);
		width = SUPERTILE_TEXMAP_SIZE;
		height = SUPERTILE_TEXMAP_SIZE;
		GAME_ASSERT(decompressedSize == (size_t)(2 * width * height));
#if !(__BIG_ENDIAN__)
		ByteswapInts(sizeof(uint16_t), width*height, tempBuffer16);
#endif


				/**************************/
				/* CREATE MATERIAL OBJECT */
				/**************************/

			/* USE PACKED PIXEL TYPE */

		matData.pixelSrcFormat 	= GL_BGRA_EXT;
		matData.pixelDstFormat 	= GL_RGB;		// Billy Frontier's terrain textures are always opaque. This isn't necessarily the case in all Pangea games (e.g. Otto)
		matData.textureName[0] 	= OGL_TextureMap_Load(tempBuffer16, width, height,
												 GL_BGRA_EXT, GL_RGB, GL_UNSIGNED_SHORT_1_5_5_5_REV);


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
			
		MO_DrawMaterial(gSuperTileTextureObjects[i]);			//--------------
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
}


#pragma mark -

/***************************** SAVE GAME ********************************/
//
// Returns true if saving was successful
//

OSErr SaveGame(int fileSlot)
{
	char path[64];
	SaveGameType saveData;

	memset(&saveData, 0, sizeof(saveData));

//	saveData.version		= SAVE_GAME_VERSION;				// save file version #
	saveData.score 			= gScore;
	saveData.numLives 		= gPlayerInfo.lives;
	saveData.duelWonMask	= gDuelWonMask;
	saveData.levelWonMask	= gLevelWonMask;

	snprintf(path, sizeof(path), "%s%d", SAVE_FILE_NAME, fileSlot);

	return SaveUserDataFile(path, SAVE_MAGIC, sizeof(SaveGameType), (Ptr) & saveData);
}

/***************************** DELETE SAVED GAME ********************************/

OSErr DeleteSavedGame(int fileSlot)
{
	FSSpec spec;
	OSErr iErr;
	char path[64];

	snprintf(path, sizeof(path), "%s%d", SAVE_FILE_NAME, fileSlot);

	iErr = MakeFSSpecForUserDataFile(path, &spec);

	if (noErr == iErr)
	{
		printf("Deleting %s\n", path);
		iErr = FSpDelete(&spec);
	}

	return iErr;
}

/***************************** LOAD SAVED GAME ********************************/

OSErr LoadSavedGame(int fileSlot, SaveGameType* saveDataPtr)
{
	char path[64];
	snprintf(path, sizeof(path), "%s%d", SAVE_FILE_NAME, fileSlot);

	return LoadUserDataFile(path, SAVE_MAGIC, sizeof(SaveGameType), (Ptr) saveDataPtr);
}

/***************************** USE SAVED GAME ********************************/

void UseSavedGame(const SaveGameType* saveData)
{
	gLoadedScore = gScore = saveData->score;
	gPlayerInfo.lives = saveData->numLives;
	if (gPlayerInfo.lives > 20)							// check for corruption
		gPlayerInfo.lives = 0;

	gDuelWonMask = saveData->duelWonMask;
	gLevelWonMask = saveData->levelWonMask;
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
