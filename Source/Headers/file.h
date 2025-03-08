//
// file.h
//

#pragma once

#define	kGameID 			'ALMO'

#define PREFS_FOLDER_NAME	"BillyFrontier"
#define PREFS_FILE_NAME		"Prefs"
#define SCORES_FILE_NAME	"HighScores"
#define SAVE_FILE_NAME		"BillySave"
#define PREFS_MAGIC			"NewBillyPrefV02"	// Bump this when PrefsType changes
#define	SCORES_MAGIC		"NewBillyScorV01"	// Bump this when HighScoreType changes
#define SAVE_MAGIC			"NewBillySaveV01"	// Bump this when SaveGameType changes
#define MAX_SAVE_FILES		5

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

typedef struct
{
	// Legacy stuff left for source code compat
	struct
	{
		uint8_t anaglyph : 1;
		uint8_t anaglyphColor : 1;
	};

	Boolean	fullscreen;
	Byte	antialiasingLevel;
	Byte	displayNumMinus1;
	Byte	mouseWheelScrollSpeed;
	Boolean	invertMouseWheel;
}PrefsType;


		/* SAVE GAME */

typedef struct
{
	uint32_t	score;
	Byte		numLives;
	Byte		duelWonMask;
	Byte		levelWonMask;
	Byte		reserved;
}SaveGameType;



//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType);

void InitPrefsFolder(bool createIt);
OSErr LoadUserDataFile(const char* filename, const char* magic, long payloadLength, Ptr payloadPtr);
OSErr SaveUserDataFile(const char* filename, const char* magic, long payloadLength, Ptr payloadPtr);
OSErr LoadPrefs(void);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr);

OSErr SaveGame(int fileSlot);
OSErr DeleteSavedGame(int fileSlot);
OSErr LoadSavedGame(int fileSlot, SaveGameType* saveDataPtr);
void UseSavedGame(const SaveGameType* saveData);


void LoadTargetPracticeArt(void);
void LoadDuelArt(void);
void LoadShootoutArt(void);
void LoadStampedeArt(void);


Ptr LoadDataFile(const char* path, long* outLength);
char* LoadTextFile(const char* path, long* outLength);

char* CSVIterator(char** csvCursor, bool* eolOut);


