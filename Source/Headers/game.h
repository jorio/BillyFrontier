#pragma once

#if _MSC_VER
#define _Static_assert static_assert
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL.h>
#include <SDL_opengl.h>

#if __APPLE__
#include <OpenGL/GLU.h>
#else
#include <GL/glu.h>
#endif

#include "Pomme.h"
#include "globals.h"
#include "structs.h"

#include "metaobjects.h"
#include "ogl_support.h"
#include "main.h"
#include "player.h"
#include "mobjtypes.h"
#include "objects.h"
#include "misc.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "sound2.h"
#include "sobjtypes.h"
#include "terrain.h"
#include "sprites.h"
#include "shards.h"
#include "sparkle.h"
#include "bg3d.h"
#include "effects.h"
#include "camera.h"
#include "collision.h"
#include "input.h"
#include "file.h"
#include "fences.h"
#include "splineitems.h"
#include "items.h"
#include "window.h"
#include "enemy.h"
#include "water.h"
#include "miscscreens.h"
#include "pick.h"
#include "3dmath.h"
#include "infobar.h"

extern BG3DFileContainer *gBG3DContainerList[MAX_BG3D_GROUPS];
extern Boolean gAllowAudioKeys;
extern Boolean gDisableAnimSounds;
extern Boolean gDisableHiccupTimer;
extern Boolean gDoneFaceOff;
extern Boolean gDrawLensFlare;
extern Boolean gDuelWon[NUM_LEVELS];
extern Boolean gFreezeCameraFromXZ;
extern Boolean gFreezeCameraFromY;
extern Boolean gGameOver;
extern Boolean gIsPicking;
extern Boolean gLevelCompleted;
extern Boolean gLevelWon[NUM_LEVELS];
extern Boolean gLostGame;
extern Boolean gMuteMusicFlag;
extern Boolean gMyState_Lighting;
extern Boolean gNeedToReloadNextAmmoClip;
extern Boolean gPlayNow;
extern Boolean gPlayerIsDead;
extern Boolean gPlayerToWinDuel;
extern Boolean gPlayingFromSavedGame;
extern Boolean gShowSaveMenu;
extern Boolean gShootoutCanProceedToNextStopPoint;
extern Boolean gSongPlayingFlag;
extern Boolean gWonGame;
extern Byte **gMapSplitMode;
extern Byte gDebugMode;
extern Byte gDuelKeySequenceMode;
extern Byte gDuelKeySequence[MAX_DUEL_KEY_SEQUENCE_LENGTH];
extern Byte gNumEnemiesThisStopPoint[/*MAX_STOP_POINTS*/];
extern Byte gShootoutMode;
extern CollisionRec gCollisionList[/*MAX_COLLISIONS*/];
extern FSSpec gDataSpec;
extern FenceDefType *gFenceList;
extern LineMarkerDefType gLineMarkerList[MAX_LINEMARKERS];
extern MOMaterialObject *gMostRecentMaterial;
extern MOMaterialObject *gSuperTileTextureObjects[MAX_SUPERTILE_TEXTURES];
extern MOVertexArrayData **gLocalTriMeshesOfSkelType;
extern MetaObjectPtr gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern NewObjectDefinitionType gNewObjectDefinition;
extern NewParticleGroupDefType gNewParticleGroupDef;
extern OGLBoundingBox gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern OGLBoundingBox gWaterBBox[/*MAX_WATER*/];
extern OGLColorRGB gGlobalColorFilter;
extern OGLColorRGBA gFillColor1;
extern OGLMatrix4x4 *gCurrentObjMatrix;
extern OGLMatrix4x4 gViewToFrustumMatrix;
extern OGLMatrix4x4 gWorldToFrustumMatrix;
extern OGLMatrix4x4 gWorldToViewMatrix;
extern OGLMatrix4x4 gWorldToWindowMatrix;
extern OGLPoint2D gCrosshairsCoord;
extern OGLPoint3D gCoord;
extern OGLSetupOutputType *gGameViewInfoPtr;
extern OGLVector3D gDelta;
extern OGLVector3D gRecentTerrainNormal;
extern OGLVector3D gWorldSunDirection;
extern ObjNode *gCurrentNode;
extern ObjNode *gCursor;
extern ObjNode *gCyc;
extern ObjNode *gDuelers[/*MAX_DUELERS*/];
extern ObjNode *gFirstNodePtr;
extern ParticleGroupType *gParticleGroups[MAX_PARTICLE_GROUPS];
extern PlayerInfoType gPlayerInfo;
extern PrefsType gGamePrefs;
extern SDL_GLContext gAGLContext;
extern SDL_Window *gSDLWindow;
extern SInt32 gScrollWheelDelta;
extern SparkleType gSparkles[MAX_SPARKLES];
extern SplineDefType **gSplineList;
extern SpriteType *gSpriteGroupList[MAX_SPRITE_GROUPS];
extern SuperTileItemIndexType **gSuperTileItemIndexGrid;
extern SuperTileMemoryType gSuperTileMemoryList[MAX_SUPERTILES];
extern SuperTileStatus **gSuperTileStatusGrid;
extern TerrainItemEntryType **gMasterItemList;
extern WaterDefType **gWaterListHandle;
extern WaterDefType *gWaterList;
extern char gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];
extern float **gMapYCoords;
extern float **gMapYCoordsOriginal;
extern float **gVertexShading;
extern float g2DLogicalHeight;
extern float g2DLogicalWidth;
extern float gAutoFadeEndDist;
extern float gAutoFadeRange_Frac;
extern float gAutoFadeStartDist;
extern float gCameraDistFromMe;
extern float gCameraLookAtYOff;
extern float gCurrentAspectRatio;
extern float gCurrentMaxSpeed;
extern float gFramesPerSecond;
extern float gFramesPerSecondFrac;
extern float gGlobalTransparency;
extern float gGravity;
extern float gLevelCompletedCoolDownTimer;
extern float gMapToUnitValue;
extern float gObjectGroupBSphereList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern float gPlayerBottomOff;
extern float gTargetMaxSpeed;
extern float gTargetPracticeTimer;
extern float gTerrainPolygonSize;
extern float gTerrainSuperTileUnitSize;
extern float gTerrainSuperTileUnitSizeFrac;
extern float gTimeSinceLastEnemyShot;
extern int gCurrentAntialiasingLevel;
extern int gCurrentArea;
extern int gCurrentMenuItem;
extern int gDuelKeyBufferIndex;
extern int gDuelKeySequenceLength;
extern int gDuelReflex;
extern int gGameWindowHeight;
extern int gGameWindowWidth;
extern int gNumEnemies;
extern int gNumLineMarkers;
extern int gNumObjectNodes;
extern int gNumObjectsInBG3DGroupList[MAX_BG3D_GROUPS];
extern int gNumPointers;
extern int gNumSparkles;
extern int gNumSuperTilesDeep;
extern int gNumSuperTilesWide;
extern int gNumUniqueSuperTiles;
extern int gNumWorldCalcsThisFrame;
extern int gPepperCount;
extern int gPolysThisFrame;
extern int gStopPointNum;
extern int gSuperTileActiveRange;
extern int gTerrainTileDepth;
extern int gTerrainTileWidth;
extern int gTerrainUnitWidth, gTerrainUnitDepth;
extern int gVRAMUsedThisFrame;
extern int32_t gNumSpritesInGroupList[MAX_SPRITE_GROUPS];
extern long gMouseDeltaX;
extern long gMouseDeltaY;
extern long gNumFences;
extern long gNumSplines;
extern long gNumWaterPatches;
extern long gPrefsFolderDirID;
extern short **gSuperTileTextureGrid;
extern short gCurrentSong;
extern short gMainAppRezFile;
extern short gNumActiveParticleGroups;
extern short gNumCollisions;
extern short gNumFencesDrawn;
extern short gNumSuperTilesDrawn;
extern short gNumTerrainItems;
extern short gNumWaterDrawn;
extern short gPrefsFolderVRefNum;
extern signed char gNumEnemyOfKind[NUM_ENEMY_KINDS];
extern uint32_t gAutoFadeStatusBits;
extern uint32_t gGameFrameNum;
extern uint32_t gGlobalMaterialFlags;
extern uint32_t gLoadedScore;
extern uint32_t gScore;

#if _DEBUG
#define IMPLEMENT_ME_SOFT() printf("IMPLEMENT ME: %s:%d\n", __func__, __LINE__)
#else
#define IMPLEMENT_ME_SOFT()
#endif
#define IMPLEMENT_ME() DoFatalAlert("IMPLEMENT ME: %s:%d", __func__, __LINE__)

#define GAME_ASSERT(condition) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, #condition); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, message); } while(0)
