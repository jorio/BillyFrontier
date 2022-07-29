//
// camera.h
//

// Externals
#include "game.h"

void InitCamera_Terrain(void);
void InitCamera_Duel(void);
void UpdateCamera_Duel(void);

void UpdateCamera_Terrain(void);
void ResetCameraSettings(void);
void DrawLensFlare(OGLSetupOutputType *setupInfo);

void UpdateCamera_Shootout(void);

Boolean PrimeStampedeCamera(long splineNum, SplineItemType *itemPtr);

void InitCamera_TargetPractice(void);


void PrepAnaglyphCameras(void);
void RestoreCamerasFromAnaglyph(void);
void CalcAnaglyphCameraOffset(short pass);
