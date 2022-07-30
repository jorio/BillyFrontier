//
// splineitems.h
//

// Externals
#include "game.h"

//=====================================================

void PrimeSplines(void);
void GetCoordOnSplineFromIndex(SplineDefType *splinePtr, double findex, float *x, float *z);
void GetCoordOnSpline(SplineDefType *splinePtr, double placement, float *x, float *z);
void GetCoordOnSpline2(SplineDefType *splinePtr, double placement, double offset, float *x, float *z, Boolean allowWrap);
void GetNextCoordOnSpline(SplineDefType *splinePtr, double placement, float *x, float *z);
Boolean IsSplineItemOnActiveTerrain(ObjNode *theNode);
void AddToSplineObjectList(ObjNode *theNode, Boolean setAim);
void MoveSplineObjects(void);
Boolean RemoveFromSplineObjectList(ObjNode *theNode);
void EmptySplineObjectList(void);
Boolean IncreaseSplineIndex(ObjNode *theNode, double speed, Boolean noWrapping);
void IncreaseSplineIndexZigZag(ObjNode *theNode, double speed);
void DetachObjectFromSpline(ObjNode* theNode, movecall_t moveCall);
void SetSplineAim(ObjNode *theNode);
void GetObjectCoordOnSpline(ObjNode *theNode);
void GetObjectCoordOnSpline2(ObjNode *theNode, float *x, float *z);















