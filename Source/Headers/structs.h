//
// structs.h
//

#ifndef STRUCTS_H
#define STRUCTS_H


#include "globals.h"
#include "ogl_support.h"
#include "metaobjects.h"

#define	MAX_ANIMS			40
#define	MAX_KEYFRAMES		60
#define	MAX_JOINTS			30
#define	MAX_CHILDREN		(MAX_JOINTS-1)
#define	MAX_LIMBS	MAX_JOINTS

#define MAX_FLAGS_IN_OBJNODE			5		// # flags in ObjNode


#define	MAX_DECOMPOSED_POINTS	5000
#define	MAX_DECOMPOSED_NORMALS	5000
#define	MAX_POINT_REFS			20		// max times a point can be re-used in multiple places
#define	MAX_DECOMPOSED_TRIMESHES 10

#define	MAX_MORPH_TRIMESHES		10
#define	MAX_MORPH_POINTS		2000

#define	MAX_NODE_SPARKLES		20

#define	MAX_COLLISION_BOXES		8

#define	MAX_OBJECTS_IN_GROUP	100


			/*********************/
			/* SPLINE STRUCTURES */
			/*********************/

typedef	struct
{
	float	x,z;
}SplinePointType;

typedef struct
{
	float			placement;			// where on spline to start item (0=front, 1.0 = end)
	uint16_t		type;
	Byte			parm[4];
	uint16_t		flags;
}SplineItemType;


typedef struct
{
	int16_t			numNubs;			// # nubs in spline
	SplinePointType	**nubList;			// handle to nub list

	int32_t			numPoints;			// # points in spline
	SplinePointType	**pointList;		// handle to calculated spline points

	int16_t			numItems;			// # items on the spline
	SplineItemType	**itemList;			// handle to spline items

	Rect			bBox;				// bounding box of spline area
}SplineDefType;

typedef struct
{
	int16_t			numNubs;			// # nubs in spline
	int16_t 		_pad1;
	int32_t			_junkptr1;

	int32_t			numPoints;			// # points in spline
	int32_t			_junkptr2;

	int16_t			numItems;			// # items on the spline
	int16_t 		_pad2;
	int32_t			_junkptr3;

	Rect			bBox;				// bounding box of spline area
}File_SplineDefType;



		/* COLLISION BOX */
		
typedef struct
{
	float	left,right,front,back,top,bottom;
	float	oldLeft,oldRight,oldFront,oldBack,oldTop,oldBottom;
}CollisionBoxType;



//*************************** SKELETON *****************************************/

		/* BONE SPECIFICATIONS */
		//
		//
		// NOTE: Similar to joint definition but lacks animation, rot/scale info.
		//
		
typedef struct
{
	int32_t				parentBone;			 			// index to previous bone
	uint32_t			ignored1;
	OGLMatrix4x4		ignored2;
	uint32_t			*ignored3;
	unsigned char		ignored4[32];
	OGLPoint3D			coord;							// absolute coord (not relative to parent!) 
	uint16_t			numPointsAttachedToBone;		// # vertices/points that this bone has
	uint16_t			*pointList;						// indecies into gDecomposedPointList
	uint16_t			numNormalsAttachedToBone;		// # vertex normals this bone has
	uint16_t			*normalList;					// indecies into gDecomposedNormalsList
}BoneDefinitionType;


			/* DECOMPOSED POINT INFO */
			
typedef struct
{
	OGLPoint3D	realPoint;							// point coords as imported in 3DMF model
	OGLPoint3D	boneRelPoint;						// point relative to bone coords (offset from bone)
	
	Byte		numRefs;							// # of places this point is used in the geometry data
	Byte		whichTriMesh[MAX_POINT_REFS];		// index to trimeshes
	short		whichPoint[MAX_POINT_REFS];			// index into pointlist of triMesh above
	short		whichNormal[MAX_POINT_REFS];		// index into gDecomposedNormalsList
}DecomposedPointType;



		/* CURRENT JOINT STATE */
		
typedef struct
{
	int32_t		tick;					// time at which this state exists
	int32_t		accelerationMode;		// mode of in/out acceleration
	OGLPoint3D	coord;					// current 3D coords of joint (relative to link)
	OGLVector3D	rotation;				// current rotation values of joint (relative to link)
	OGLVector3D	scale;					// current scale values of joint mesh
}JointKeyframeType;


		/* JOINT DEFINITIONS */
		
typedef struct
{
	int8_t				numKeyFrames[MAX_ANIMS];				// # keyframes
	JointKeyframeType 	**keyFrames;							// 2D array of keyframe data keyFrames[anim#][keyframe#]
}JointKeyFrameHeader;

			/* ANIM EVENT TYPE */
			
typedef struct
{
	short	time;
	Byte	type;
	Byte	value;
}AnimEventType;


			/* SKELETON INFO */
		
		
typedef struct
{
	Byte				NumBones;						// # joints in this skeleton object
	JointKeyFrameHeader	JointKeyframes[MAX_JOINTS];		// array of joint definitions

	Byte				numChildren[MAX_JOINTS];		// # children each joint has
	Byte				childIndecies[MAX_JOINTS][MAX_CHILDREN];	// index to each child
	
	Byte				NumAnims;						// # animations in this skeleton object
	Byte				*NumAnimEvents;					// ptr to array containing the # of animevents for each anim
	AnimEventType		**AnimEventsList;				// 2 dimensional array which holds a anim event list for each anim AnimEventsList[anim#][event#]

	BoneDefinitionType	*Bones;							// data which describes bone heirarachy
	
	long				numDecomposedTriMeshes;			// # trimeshes in skeleton
	MOVertexArrayData	decomposedTriMeshes[MAX_DECOMPOSED_TRIMESHES];	// array of triMeshData

	long				numDecomposedPoints;			// # shared points in skeleton
	DecomposedPointType	*decomposedPointList;			// array of shared points

	short				numDecomposedNormals ;			// # shared normal vectors
	OGLVector3D			*decomposedNormalsList;			// array of shared normals


}SkeletonDefType;


		/* THE STRUCTURE ATTACHED TO AN OBJNODE */
		//
		// This contains all of the local skeleton data for a particular ObjNode
		//

typedef struct
{
	Boolean			JointsAreGlobal;				// true when joints are already in world-space coords
	Byte			AnimNum;						// animation #

	Boolean			IsMorphing;						// flag set when morphing from an anim to another
	float			MorphSpeed;						// speed of morphing (1.0 = normal)
	float			MorphPercent;					// percentage of morph from kf1 to kf2 (0.0 - 1.0)

	JointKeyframeType	JointCurrentPosition[MAX_JOINTS];	// for each joint, holds current interpolated keyframe values
	JointKeyframeType	MorphStart[MAX_JOINTS];		// morph start & end keyframes for each joint
	JointKeyframeType	MorphEnd[MAX_JOINTS];

	float			CurrentAnimTime;				// current time index for animation	
	float			LoopBackTime;					// time to loop or zigzag back to (default = 0 unless set by a setmarker)
	float			MaxAnimTime;					// duration of current anim
	float			AnimSpeed;						// time factor for speed of executing current anim (1.0 = normal time)
	float			PauseTimer;						// # seconds to pause animation
	Byte			AnimEventIndex;					// current index into anim event list
	Byte			AnimDirection;					// if going forward in timeline or backward			
	Byte			EndMode;						// what to do when reach end of animation
	Boolean			AnimHasStopped;					// flag gets set when anim has reached end of sequence (looping anims don't set this!)

	OGLMatrix4x4	jointTransformMatrix[MAX_JOINTS];	// holds matrix xform for each joint

	SkeletonDefType	*skeletonDefinition;						// point to skeleton's common/shared data	
	
	MOMaterialObject	*overrideTexture[MAX_DECOMPOSED_TRIMESHES];		// an illegal ref to a texture object for each trimesh in skeleton
		
}SkeletonObjDataType;


			/* TERRAIN ITEM ENTRY TYPE */

typedef struct
{
	uint32_t						x;
	uint32_t						y;
	uint16_t						type;
	Byte							parm[4];
	uint16_t						flags;
}TerrainItemEntryType;




			/****************************/
			/*  OBJECT RECORD STRUCTURE */
			/****************************/


struct ObjNode
{
	struct ObjNode	*PrevNode;			// address of previous node in linked list
	struct ObjNode	*NextNode;			// address of next node in linked list
	struct ObjNode	*ChainNode;
	struct ObjNode	*ChainHead;			// a chain's head (link back to 1st obj in chain)

	struct ObjNode	*ShadowNode;		// ptr to node's shadow (if any)
	struct ObjNode	*MPlatform;			// current moving platform

	u_short			Slot;				// sort value
	Byte			Genre;				// obj genre
	int				Type;				// obj type
	int				Group;				// obj group
	int				Kind;				// kind
	int				Mode;				// mode
	int				What;				// what
	Byte			Side;				// left or right
	u_long			StatusBits;			// various status bits
	
			/* MOVE/DRAW CALLBACKS */
			
	void			(*MoveCall)(struct ObjNode *);			// pointer to object's move routine
	void			(*SplineMoveCall)(struct ObjNode *);	// pointer to object's spline move routine
	void			(*CustomDrawFunction)(struct ObjNode *);// pointer to object's custom draw function
	void			(*HitByBulletCallback)(struct ObjNode *, struct ObjNode *, const OGLPoint3D *);	// pointer to object's pick/hit callback
	
			/* PHYSICS */
			
	OGLPoint3D		Coord;				// coord of object
	OGLPoint3D		OldCoord;			// coord @ previous frame
	OGLPoint3D		InitCoord;			// coord where was created
	OGLVector3D		Delta;				// delta velocity of object
	OGLVector3D		DeltaRot;
	OGLVector3D		Rot;				// rotation of object
	OGLVector3D		Scale;				// scale of object
	OGLVector2D		AccelVector;		// current acceleration vector
	float			Friction;			// amount of friction to apply to player if player lands on top of this (for platforms and the like).  0 = normal, 1 = slick
	
	float			Speed3D;			// length of Delta vector x,y,z (not scaled to fps)	
	float			Speed2D;			// length of Delta vector x,z (not scaled to fps)
	
	OGLVector3D		TargetOff;		
	OGLPoint3D		WeaponAutoTargetOff;	// offset from coord where weapon should auto target


			/* COLLISION INFO */
			
	u_long				CType;													// collision type bits
	u_long				CBits;													// collision attribute bits
	Byte				NumCollisionBoxes;
	CollisionBoxType	CollisionBoxes[MAX_COLLISION_BOXES];					// Array of collision rectangles
	float				LeftOff,RightOff,FrontOff,BackOff,TopOff,BottomOff;		// box offsets (only used by simple objects with 1 collision box)	

	float				BoundingSphereRadius;
	struct ObjNode 		*CurrentTriggerObj;										// set when trigger occurs

	Boolean				(*TriggerCallback)(struct ObjNode *, struct ObjNode *, Byte sides);			// callback when trigger occurs
	Boolean				(*HurtCallback)(struct ObjNode *, float damage);							// used for enemies to call their hurt function				
	
	OGLVector3D			HoldOffset;			// for Pickup objects, this is the offset that Skip needs to hold the object from
	OGLVector3D			HoldRot;			// rotation offsets for held objects
		
	float				ForceLookAtDist;
	
	Boolean				HasWorldPoints;								// true if we have the world-space coords of the model's vertices
	MOVertexArrayData	WorldMeshes[MAX_OBJECTS_IN_GROUP];			// for each mesh in the model, a copy of the master mesh but with new vertex arrays that contain world-space coords
	OGLPlaneEquation		*WorldPlaneEQs[MAX_OBJECTS_IN_GROUP];	// for each mesh, an array of plane equations for each triangle
	
	
			/* SPECS */
			
	signed char		Flag[6];
	long			Special[6];
	float			SpecialF[6];
	OGLPoint3D		SpecialPt[3];
	struct ObjNode*	SpecialObjPtr[3];
	float			Timer;				// misc use timer
	
	float			Health;				// health 0..1
	float			Damage;				// damage
	
	Byte			GunLocation;
	
		
			/* 3D CALCULATION STUFF */
			
	OGLMatrix4x4		AlignmentMatrix;		// when objects need to be aligned with a matrix rather than x,y,z rotations
		
	OGLMatrix4x4		BaseTransformMatrix;	// matrix which contains all of the transforms for the object as a whole
	MOMatrixObject		*BaseTransformObject;	// extra LEGAL object ref to BaseTransformMatrix (other legal ref is kept in BaseGroup)
	MOGroupObject		*BaseGroup;				// group containing all geometry,etc. for this object (for drawing)

	OGLBoundingBox		BBox;					// bbox for the model

	SkeletonObjDataType	*Skeleton;				// pointer to skeleton record data	
	
	
			/* TERRAIN ITEM / SPLINE INFO */
			
	TerrainItemEntryType *TerrainItemPtr;		// if item was from terrain, then this pts to entry in array
	SplineItemType 		*SplineItemPtr;			// if item was from spline, then this pts to entry in array
	u_char				SplineNum;				// which spline this spline item is on
	double				SplinePlacement;		// 0.0->.9999 for placement on spline
	short				SplineObjectIndex;		// index into gSplineObjectList of this ObjNode


				/* COLOR & TEXTURE INFO */
				
	OGLColorRGBA		ColorFilter;
	float				TextureTransformU, TextureTransformV;


			/* PARTICLE EFFECTS */
			
	short				EffectChannel;					// effect sound channel index (-1 = none)
	short				ParticleGroup;
	u_long				ParticleMagicNum;
	float				ParticleTimer;

	short				VaporTrails[MAX_JOINTS];		// indecies into vapor trail list	
	short				Sparkles[MAX_NODE_SPARKLES];	// indecies into sparkles list


			/* SPRITE INFO */
				
	MOSpriteObject		*SpriteMO;				// ref to sprite meta object for sprite genre.
	
	Byte				NumStringSprites;		// # sprites to build string (NOT SAME AS LENGTH OF STRING B/C SPACES ET.AL.)
	MOSpriteObject		*StringCharacters[31];	// sprites for each character

	float				AnaglyphZ;

		
};
typedef struct ObjNode ObjNode;

typedef void(*movecall_t)(ObjNode*);


		/* NEW OBJECT DEFINITION TYPE */
		
typedef struct
{
	Byte			genre,group,type,animNum;
	OGLPoint3D		coord;
	unsigned long	flags;
	short			slot;
	movecall_t		moveCall;
	float			rot,scale;
}NewObjectDefinitionType;


enum
{
	GUN_LOCATION_HOLSTER,
	GUN_LOCATION_HAND
};

#endif




