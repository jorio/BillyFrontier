//
// file.h
//

// Externals
#include "game.h"

#define	kGameID 			'ALMO'
#define	kSavedGameFileType	'ALsv'

#define PREFS_FOLDER_NAME	"BillyFrontier"
#define PREFS_MAGIC			"Billy Frontier Prefs v01"

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

#define	CURRENT_PREFS_VERS	0xA0E1

typedef struct
{
	uint32_t version;

	// Legacy stuff left for source code compat
	struct
	{
		uint8_t anaglyph : 1;
		uint8_t anaglyphColor : 1;
	};

	Boolean	fullscreen;
	Byte	antialiasingLevel;
	Byte	monitorNum;

}PrefsType;





//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType);

void InitPrefsFolder(bool createIt);
OSErr LoadUserDataFile(const char* filename, const char* magic, long payloadLength, Ptr payloadPtr);
OSErr SaveUserDataFile(const char* filename, const char* magic, long payloadLength, Ptr payloadPtr);
OSErr LoadPrefs(void);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr);
OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth);
void SetDefaultDirectory(void);

Boolean SaveGame(void);
Boolean LoadSavedGame(void);


void LoadTargetPracticeArt(void);
void LoadDuelArt(void);
void LoadShootoutArt(void);
void LoadStampedeArt(void);


Ptr LoadDataFile(const char* path, long* outLength);
char* LoadTextFile(const char* path, long* outLength);

char* CSVIterator(char** csvCursor, bool* eolOut);


