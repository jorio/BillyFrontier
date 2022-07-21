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
	short	version;			// 0xaa.bb
	short	numAnims;			// gNumAnims
	short	numJoints;			// gNumJoints
	short	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing 
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.
			
typedef struct
{
	long 				parentBone;			 		// index to previous bone
	unsigned char		name[32];					// text string name for bone
	OGLPoint3D			coord;						// absolute coord (not relative to parent!) 
	u_short				numPointsAttachedToBone;	// # vertices/points that this bone has
	u_short				numNormalsAttachedToBone;	// # vertex normals this bone has
	u_long				reserved[8];				// reserved for future use
}File_BoneDefinitionType;



			/* Joit */
			
typedef struct
{
	OGLVector3D		maxRot;						// max rot values of joint
	OGLVector3D		minRot;						// min rot values of joint
	long 			parentBone; 		// index to previous link joint definition
	unsigned char	name[32];						// text string name for joint
	long			limbIndex;					// index into limb list
}Joit_Rez_Type;




			/* AnHd */
			
typedef struct
{
	Str32	animName;			
	short	numAnimEvents;	
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
	//DateTimeRec	lastVersCheckDate;
	Byte	didThisNote[MAX_HTTP_NOTES];
	Boolean	hasConfiguredISpControls;
	Boolean	oldOSWarned;
	Boolean	anaglyph;
	Boolean	anaglyphColor;
	u_long	version;
	
	int		reserved[8];
}PrefsType;





//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr, OGLSetupOutputType *setupInfo);
OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth);
void GetDemoTimer(void);
void SaveDemoTimer(void);
void SetDefaultDirectory(void);

Boolean SaveGame(void);
Boolean LoadSavedGame(void);


void LoadTargetPracticeArt(OGLSetupOutputType *setupInfo);
void LoadDuelArt(OGLSetupOutputType *setupInfo);
void LoadShootoutArt(OGLSetupOutputType *setupInfo);
void LoadStampedeArt(OGLSetupOutputType *setupInfo);













