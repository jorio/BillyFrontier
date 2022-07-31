//
// pick.h
//

#pragma once

Boolean OGLPoint3D_InsideTriangle3D(const OGLPoint3D *point3D, const OGLPoint3D *trianglePoints, const OGLVector3D	*triangleNormal);

Boolean OGL_DoesLineSegmentIntersectSphere(OGLPoint3D *p1, OGLPoint3D *p2, OGLVector3D *segVector, OGLPoint3D *sphereCenter, float sphereRadius, OGLPoint3D *intersectPt);

ObjNode *OGL_DoRayCollision(OGLRay *ray, OGLPoint3D *worldHitCoord, u_long statusFilter, u_long cTypes);

void OGL_GetWorldRayAtScreenPoint(OGLPoint2D *screenCoord, OGLRay *ray);
Boolean	OGL_RayIntersectsTriangle(OGLPoint3D *trianglePoints, OGLRay *ray, OGLPoint3D *intersectPt);

ObjNode *OGL_DoLineSegmentCollision(OGLPoint3D *p1, OGLPoint3D *p2, OGLPoint3D *worldHitCoord, OGLVector3D *worldHitFaceNormal, u_long cTypes);

Boolean OGL_DoesRayIntersectTrianglePlane(const OGLPoint3D	triWorldPoints[], OGLRay *ray, OGLPlaneEquation	*planeEquation);
