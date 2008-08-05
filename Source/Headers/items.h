//
// items.h
//



extern	void InitItemsManager(void);
void CreateCyclorama(void);

Boolean AddBuilding(TerrainItemEntryType *itemPtr, float  x, float z);
void DrawCyclorama(ObjNode *theNode, const OGLSetupOutputType *setupInfo);
Boolean AddHeadStone(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPlant(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddDuelRockWall(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddCoffin(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddBarrel(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddWoodCrate(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddHayBale(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPost(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddSceneryKangaCow(TerrainItemEntryType *itemPtr, float  x, float z);

void DefaultBulletHitCallback(ObjNode *bullet, ObjNode *hitObj, const OGLPoint3D *impactPt);

Boolean AddTable(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddChair(TerrainItemEntryType *itemPtr, float  x, float z);


		
void DefaultGotKickedCallback(ObjNode *player, ObjNode *kickedObj);
void MoveDefaultPickup(ObjNode *theNode);
void DefaultDropObject(ObjNode *player, ObjNode *held);

Boolean CheckDropThruFence(ObjNode *player, ObjNode *held);

Boolean AddDeadTree(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddRock(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddElectricFence(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddTumbleweed(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean PrimeTumbleweed(long splineNum, SplineItemType *itemPtr);


Boolean AddTeePee(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddSwampCabin(TerrainItemEntryType *itemPtr, float  x, float z);

Boolean AddSpearSkull(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPeso(TerrainItemEntryType *itemPtr, float  x, float z);

Boolean AddFreeLifePOW(TerrainItemEntryType *itemPtr, float  x, float z);






