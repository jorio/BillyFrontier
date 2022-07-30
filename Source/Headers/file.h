//
// file.h
//

// Externals
#include "game.h"

#define	kGameID 			'ALMO'
#define	kSavedGameFileType	'ALsv'

		/***********************/
		/* RESOURCE STURCTURES */
		/***********************/
		
			/* Hedr */
			
typedef struct
{
	int16_t	version;			// 0xaa.bb
	int16_t	numAnims;			// gNumAnims
	int16_t	numJoints;			// gNumJoints
	int16_t	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing 
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.
			
typedef struct
{
	int32_t				parentBone;			 		// index to previous bone
	char				name[32];					// text string name for bone
	OGLPoint3D			coord;						// absolute coord (not relative to parent!) 
	uint16_t			numPointsAttachedToBone;	// # vertices/points that this bone has
	uint16_t			numNormalsAttachedToBone;	// # vertex normals this bone has
	uint32_t			reserved[8];				// reserved for future use
}File_BoneDefinitionType;




			/* AnHd */
			
typedef struct
{
	Str32	animName;
	int16_t	numAnimEvents;
}SkeletonFile_AnimHeader_Type;



		/* PREFERENCES */
		
#define	MAX_HTTP_NOTES	1000
		
#define	CURRENT_PREFS_VERS	0xA0E0		
		
typedef struct
{
	Byte	difficulty;
	Boolean	showScreenModeDialog;
	short	depth;
	int		screenWidth;
	int		screenHeight;
	double	videoHz;
	Byte	language;
	Boolean	deepZ;
	Boolean	hasConfiguredISpControls;
	Boolean	oldOSWarned;
	Boolean	anaglyph;
	Boolean	anaglyphColor;
	u_long	version;
}PrefsType;





//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr, OGLSetupOutputType *setupInfo);
OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth);
void SetDefaultDirectory(void);

Boolean SaveGame(void);
Boolean LoadSavedGame(void);


void LoadTargetPracticeArt(OGLSetupOutputType *setupInfo);
void LoadDuelArt(OGLSetupOutputType *setupInfo);
void LoadShootoutArt(OGLSetupOutputType *setupInfo);
void LoadStampedeArt(OGLSetupOutputType *setupInfo);


Ptr LoadDataFile(const char* path, long* outLength);
char* LoadTextFile(const char* path, long* outLength);

char* CSVIterator(char** csvCursor, bool* eolOut);


