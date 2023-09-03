//
// effects.h
//

// Externals
#include "game.h"

#define	MAX_PARTICLE_GROUPS		70
#define	MAX_PARTICLES			253		// (note change Byte below if > 255)


		/* FIRE & SMOKE */
		
#define	FireTimer			SpecialF[5]
#define	SmokeTimer			SpecialF[4]
#define	SmokeParticleGroup	Special[0]
#define	SmokeParticleMagic	Special[1]

			/* PARTICLE GROUP TYPE */

typedef struct
{
	uint32_t			magicNum;
	Byte			isUsed[MAX_PARTICLES];
	Byte			type;
	uint32_t			flags;
	Byte			particleTextureNum;
	float			gravity;
	float			magnetism;
	float			baseScale;
	float			decayRate;						// shrink speed
	float			fadeRate;
	
	int				srcBlend,dstBlend;
	
	float			alpha[MAX_PARTICLES];
	float			scale[MAX_PARTICLES];
	float			rotZ[MAX_PARTICLES];
	float			rotDZ[MAX_PARTICLES];
	OGLPoint3D		coord[MAX_PARTICLES];
	OGLVector3D		delta[MAX_PARTICLES];
	short			vaporTrail[MAX_PARTICLES];
	
	float			maxY;
	
	MOVertexArrayObject	*geometryObj;
	
}ParticleGroupType;


enum
{
	PARTICLE_TYPE_FALLINGSPARKS,
	PARTICLE_TYPE_GRAVITOIDS
};

enum
{
	PARTICLE_FLAGS_BOUNCE 			= (1<<0),
	PARTICLE_FLAGS_HURTPLAYER 		= (1<<1),
	PARTICLE_FLAGS_HURTPLAYERBAD 	= (1<<2),	//combine with PARTICLE_FLAGS_HURTPLAYER
	PARTICLE_FLAGS_HURTENEMY 		= (1<<3),
	PARTICLE_FLAGS_DONTCHECKGROUND 	= (1<<4),
	PARTICLE_FLAGS_VAPORTRAIL 		= (1<<5),
	PARTICLE_FLAGS_DISPERSEIFBOUNCE = (1<<6),
	PARTICLE_FLAGS_ALLAIM 			= (1<<7),	// want to calc look-at matrix for all particles
	PARTICLE_FLAGS_HASMAXY			= (1<<8)	// if particle can only go so high
};


/********** PARTICLE GROUP DEFINITION **************/

typedef struct
{
	uint32_t 	magicNum;
	Byte 	type;
	uint32_t  flags;
	float 	gravity;
	float 	magnetism;
	float 	baseScale;
	float 	decayRate;
	float 	fadeRate;
	Byte 	particleTextureNum;
	int		srcBlend,dstBlend;
}NewParticleGroupDefType;

/*************** NEW PARTICLE DEFINITION *****************/

typedef struct
{
	short 		groupNum;
	OGLPoint3D 	*where;
	OGLVector3D *delta;
	float 		scale;
	float		rotZ,rotDZ;
	float 		alpha;
}NewParticleDefType;


#define	FULL_ALPHA	1.0f


void InitEffects(void);
void InitParticleSystem(void);


void DeleteAllParticleGroups(void);
short NewParticleGroup(NewParticleGroupDefType *def);
Boolean AddParticleToGroup(NewParticleDefType *def);
Boolean VerifyParticleGroupMagicNum(short group, uint32_t magicNum);
Boolean ParticleHitObject(ObjNode *theNode, uint16_t inFlags);
void DisposeParticleSystem(void);

void MakePuff(const OGLPoint3D *where, float scale, short texNum, GLint src, GLint dst, float decayRate);
void MakeSparkExplosion(float x, float y, float z, float force, float scale, short sparkTexture, short quantityLimit, float fadeRate);

void MakeBombExplosion(OGLPoint3D *where);
void MakeSplash(float x, float y, float z, float scale);

void SprayWater(ObjNode *theNode, float x, float y, float z);
void BurnFire(ObjNode *theNode, float x, float y, float z, Boolean doSmoke, short particleType, float scale, uint32_t moreFlags);


void MakeFireExplosion(OGLPoint3D *where);

void MakeSplatter(OGLPoint3D *where, short modelObjType, float scale);

void MakeSteam(ObjNode *blob, float x, float y, float z);
Boolean AddSmoker(TerrainItemEntryType *itemPtr, float  x, float z);
ObjNode *MakeSmoker(float  x, float z, int kind);

void MakeGunBlast(OGLPoint3D *where, OGLVector3D *aim);
void DoBulletImpact(const OGLPoint3D *entryPt, const OGLVector3D *splatVec, float scale);

Boolean AddFlame(TerrainItemEntryType *itemPtr, float  x, float z);

