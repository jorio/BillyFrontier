/****************************/
/*   ENEMY: RYGAR.C	*/
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

static void	RygarPutgunInHolster(ObjNode *gun);
static void	RygarPutHatOnHead(ObjNode *hat);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_RYGARS				8

#define	RYGAR_SCALE				1.1f


/*********************/
/*    VARIABLES      */
/*********************/

#define	SparkleWobble	SpecialF[0]


/************************* MAKE RYGAR ****************************/

ObjNode *MakeRygar(float x, float z, float rot, short animNum, void *moveCall, Boolean gunInHand)
{
ObjNode	*newObj, *rightGun, *leftGun, *hat;
int		i;
				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_RYGAR,animNum, x,z, RYGAR_SCALE, rot, moveCall, gAutoFadeStatusBits );
	


				/* SET BETTER INFO */

	newObj->Kind 		= ENEMY_KIND_RYGAR;
	newObj->CType		|= CTYPE_PICKABLE;

	
				/* SET COLLISION INFO */
								
	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	




		/********************/
		/* MAKE ATTACHMENTS */
		/********************/

			/* RIGHT GUN */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_RygarGun;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	rightGun = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	rightGun->Side = SIDE_RIGHT;
	RygarPutgunInHolster(rightGun);

			/* LEFT GUN */
			
	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;	
	gNewObjectDefinition.type 		= GLOBAL_ObjType_RygarGun;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	leftGun = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	leftGun->Side = SIDE_LEFT;
	RygarPutgunInHolster(leftGun);
	

			/* HAT */
			
	gNewObjectDefinition.type 		= GLOBAL_ObjType_RygarHat;
	hat = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	RygarPutHatOnHead(hat);
	
			/* CHAIN THEM */
			
	newObj->ChainNode = rightGun;
	rightGun->ChainNode = leftGun;
	leftGun->ChainNode = hat;


			/******************/
			/* MAKE ARM LIGHT */					
			/******************/

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = 0; //SPARKLE_FLAG_FLICKER;
		gSparkles[i].where = newObj->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 30.0f;
		gSparkles[i].separation = 20.0f;
		
		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}
	
	newObj->SparkleWobble = RandomFloat() * PI2;
	


				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 4,false);
				

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_RYGAR]++;

	if (gunInHand)
		RygarPutGunsInHands(newObj);
				
	return(newObj);

}



/************************* PUT HAT ON HEAD *************************/

static void	RygarPutHatOnHead(ObjNode *hat)
{
	hat->HoldOffset.x = 0;
	hat->HoldOffset.y = 27;
	hat->HoldOffset.z = -3;

	hat->HoldRot.x = 0;
	hat->HoldRot.y = 0;
	hat->HoldRot.z = 0;
}



/************************* PUT GUN IN HOLSTER *************************/

static void	RygarPutgunInHolster(ObjNode *gun)
{
	switch(gun->Side)
	{
		case	SIDE_RIGHT:
				gun->HoldOffset.x = 7;
				gun->HoldOffset.y = 1;
				gun->HoldOffset.z = -10;

				gun->HoldRot.x = .75;
				gun->HoldRot.y = 3.7;
				gun->HoldRot.z = 0;
				break;
				
		case	SIDE_LEFT:
				gun->HoldOffset.x = -6;
				gun->HoldOffset.y = 1;
				gun->HoldOffset.z = -10;

				gun->HoldRot.x = .75;
				gun->HoldRot.y = -3.7;
				gun->HoldRot.z = 0;
				break;
				
	}

	gun->GunLocation = GUN_LOCATION_HOLSTER;
}


/*********************** RYGAR PUT GUNS INTO HANDS ***********************/

void RygarPutGunsInHands(ObjNode *enemy)
{
ObjNode 		*rightGun, *leftGun;

	rightGun = enemy->ChainNode;
	leftGun = rightGun->ChainNode;


		/****************/
		/* DO RIGHT GUN */
		/****************/

			/* IN HANDS */
				
	rightGun->GunLocation = GUN_LOCATION_HAND;


		/* GUN ALIGNMENT */

	rightGun->HoldOffset.x = 2.5;
	rightGun->HoldOffset.y = -20;
	rightGun->HoldOffset.z = -10;

	rightGun->HoldRot.x = .3;
	rightGun->HoldRot.y = 0;
	rightGun->HoldRot.z = 0;


		/***************/
		/* DO LEFT GUN */
		/***************/

			/* IN HANDS */
				
	leftGun->GunLocation = GUN_LOCATION_HAND;


		/* GUN ALIGNMENT */

	leftGun->HoldOffset.x = 0;
	leftGun->HoldOffset.y = -20;
	leftGun->HoldOffset.z = -10;

	leftGun->HoldRot.x = .3;
	leftGun->HoldRot.y = 0;
	leftGun->HoldRot.z = 0;

}



/************************ UPDATE RYGAR ATTACHMENTS ********************************/

void UpdateRygarAttachments(ObjNode *enemy)
{	
ObjNode 		*rightGun, *leftGun, *hat;
OGLMatrix4x4	m,mst,rm,m2;
int				i;

			/* GET ATTACHMENTS */
			
	rightGun = enemy->ChainNode;
	leftGun = rightGun->ChainNode;
	hat = leftGun->ChainNode;


			/********************/
			/* UPDATE RIGHT GUN */
			/********************/
		
	if (rightGun->GunLocation == GUN_LOCATION_HOLSTER)							// attached to hip or hand?
		FindJointFullMatrix(enemy, RYGAR_JOINT_RIGHTHIP, &m);					
	else
		FindJointFullMatrix(enemy, RYGAR_JOINT_RIGHTHAND, &m);					
		
	CalcGunMatrixFromJointMatrix(rightGun, &m, &rightGun->BaseTransformMatrix);		
	SetObjectTransformMatrix(rightGun);

	rightGun->Coord.x = rightGun->BaseTransformMatrix.value[M03];				// extract coord from matrix
	rightGun->Coord.y = rightGun->BaseTransformMatrix.value[M13];
	rightGun->Coord.z = rightGun->BaseTransformMatrix.value[M23];


			/*******************/
			/* UPDATE LEFT GUN */
			/*******************/
		
	if (leftGun->GunLocation == GUN_LOCATION_HOLSTER)							// attached to hip or hand?
		FindJointFullMatrix(enemy, RYGAR_JOINT_LEFTHIP, &m);					
	else
		FindJointFullMatrix(enemy, RYGAR_JOINT_LEFTHAND, &m);					
		
	CalcGunMatrixFromJointMatrix(leftGun, &m, &leftGun->BaseTransformMatrix);		
	SetObjectTransformMatrix(leftGun);

	leftGun->Coord.x = leftGun->BaseTransformMatrix.value[M03];				// extract coord from matrix
	leftGun->Coord.y = leftGun->BaseTransformMatrix.value[M13];
	leftGun->Coord.z = leftGun->BaseTransformMatrix.value[M23];


			/**************/
			/* UPDATE HAT */
			/**************/
		
	OGLMatrix4x4_SetTranslate(&mst, hat->HoldOffset.x, hat->HoldOffset.y, hat->HoldOffset.z);
	OGLMatrix4x4_SetRotate_XYZ(&rm, hat->HoldRot.x, hat->HoldRot.y, hat->HoldRot.z);
	OGLMatrix4x4_Multiply(&rm, &mst, &m2);
			
	FindJointFullMatrix(enemy, RYGAR_JOINT_HEAD, &m);					
	
	OGLMatrix4x4_Multiply(&m2, &m, &hat->BaseTransformMatrix);
	SetObjectTransformMatrix(hat);

	hat->Coord.x = hat->BaseTransformMatrix.value[M03];					// extract coord from matrix
	hat->Coord.y = hat->BaseTransformMatrix.value[M13];
	hat->Coord.z = hat->BaseTransformMatrix.value[M23];



			/******************/
			/* UPDATE SPARKLE */
			/******************/
			
	i = enemy->Sparkles[0];												// get sparkle index
	if (i != -1)
	{
		OGLMatrix4x4		m;
		const OGLPoint3D	off = {-16,-16,-3};
		const OGLVector3D	v = {-1,.1,0};
		
		FindJointFullMatrix(enemy, RYGAR_JOINT_LEFTELBOW,&m);
		OGLPoint3D_Transform(&off, &m, &gSparkles[i].where);
		OGLVector3D_Transform(&v, &m, &gSparkles[i].aim);
		
		enemy->SparkleWobble += gFramesPerSecondFrac * 15.0f;
		gSparkles[i].color.r =
		gSparkles[i].color.g =
		gSparkles[i].color.b = .5f + (sin(enemy->SparkleWobble) * .5f);
		
	}	
}


















