#pragma once
#include "Engine/Math/Vec2.hpp"

//-----------------------------------------------------------------------------------------------
// Forward declarations of classes/struct we intend to reference (but not use) below
//
struct FloatRange;
struct Vec2;
struct Vec3;
struct Vec4;
struct IntVec2;
struct AABB2;
struct AABB3;
struct LineSegment2;
struct Capsule2;
struct Triangle2;
struct OBB2;
struct OBB3;
struct Plane3;
struct Mat44;

//-----------------------------------------------------------------------------------------------
enum class BillboardType
{
	NONE = -1,
	WORLD_UP_FACING,
	WORLD_UP_OPPOSING,
	FULL_FACING,
	FULL_OPPOSING,
	COUNT
};

Mat44 GetBillboardTransform(
	BillboardType billboardType,
	Mat44 const& targetTransform,
	Vec3 const& billboardPosition,
	Vec2 const& billboardScale = Vec2(1.f, 1.f));

//-----------------------------------------------------------------------------------------------
// Clamp and lerp
//
float	GetClamped(float value, float minValue, float maxValue);
float	GetClampedZeroToOne(float value);
float	Interpolate(float start, float end, float fractionTowardsEnd);
Vec2	Interpolate(Vec2 start, Vec2 end, float fractionTowardsEnd);
Vec3	Interpolate(Vec3 start, Vec3 end, float fractionTowardsEnd);
float	GetFractionWithinRange(float value, float rangeStart, float rangeEnd);
float	RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd);
float	RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd);
int		RoundDownToInt(float value);

//-----------------------------------------------------------------------------------------------
// Angle utilities
//
float ConvertDegreesToRadians(float degrees);
float ConvertRadiansToDegrees(float radians);
float CosDegrees(float degrees);
float SinDegrees(float degrees);
float TanDegrees(float degrees);
float Atan2Degrees(float y, float x);
float CosRadians(float radians);
float SinRadians(float radians);
float Atan2Radians(float y, float x);
float AcosRadians(float value);
float GetShortestAngularDispDegrees(float startDegrees, float endDegrees);
float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees);
float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b); // not signed
float LinearSine(float value, float period, bool minusOneToOne = true);


//-----------------------------------------------------------------------------------------------
// Dot and Cross
//
float DotProduct2D(Vec2 const& a, Vec2 const& b);
float DotProduct3D(Vec3 const& a, Vec3 const& b);
float DotProduct4D(Vec4 const& a, Vec4 const& b);
float CrossProduct2D(Vec2 const& a, Vec2 const& b);
Vec3 CrossProduct3D(Vec3 const& a, Vec3 const& b);

//-----------------------------------------------------------------------------------------------
// Distance & projections utilities
//
float		GetDistance2D(Vec2 const& positionA, Vec2 const& positionB);
float		GetDistanceSquared2D(Vec2 const& positionA, Vec2 const& positionB);
float		GetDistance3D(Vec3 const& positionA, Vec3 const& positionB);
float		GetDistanceSquared3D(Vec3 const& positionA, Vec3 const& positionB);
float		GetDistanceXY3D(Vec3 const& positionA, Vec3 const& positionB);
float		GetDistanceXYSquared3D(Vec3 const& positionA, Vec3 const& positionB);
int			GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB);
float		GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);
Vec2 const	GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto);

void		CalculateTangentBitangent(Vec3& out_tangent, Vec3& out_bitangent, Vec3 const& pos0, Vec3 const& pos1, Vec3 const& pos2, Vec2 const& uv0, Vec2 const& uv1, Vec2 const& uv2);
//-----------------------------------------------------------------------------------------------
// Geometric query utilities
// 
bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius);
bool IsPointInsideAABB2D(Vec2 const& point, AABB2 const& box);
bool IsPointInsideCapsule2D(Vec2 const& point, Capsule2 const& capsule);
bool IsPointInsideCapsule2D(Vec2 const& point, Vec2 const& boneStart, Vec2 const& boneEnd, float radius);
bool IsPointInsideTriangle2D(Vec2 const& point, Vec2 const& triCCW0, Vec2 const& triCCW1, Vec2 const& triCCW2);
bool IsPointInsideTriangle2D(Vec2 const& point, Triangle2 const& triangle);
bool IsPointInsideOBB2D(Vec2 const& point, OBB2 const& orientedBox);
bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius);
bool IsPointInsideCylinderZ3D(Vec3 const& point, Vec2 const& centerXY, FloatRange const& minMaxZ, float radiusXY);
bool IsPointInsideSphere3D(Vec3 const& point, Vec3 sphereCenter, float sphereRadius);
bool IsPointInsideAABB3D(Vec3 const& point, AABB3 const& box);
bool IsPointInsideOBB3D(Vec3 const& point, OBB3 const& orientedBox);

bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB);
bool DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);
bool DoDiscAndAABBOverlap2D(Vec2 const& discCenter, float radius, AABB2 const& box);
bool DoAABBsOverlap2D(AABB2 const& first, AABB2 const& second);
bool DoAABBsOverlap3D(AABB3 const& first, AABB3 const& second);
bool DoSpheresOverlap3D(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB);
bool DoZCylindersOverlap3D(Vec2 cylinder1CenterXY, float cylinder1Radius, FloatRange cylinder1MinMaxZ, Vec2 cylinder2CenterXY, float cylinder2Radius, FloatRange cylinder2MinMaxZ);
bool DoSphereAndAABBOverlap3D(Vec3 sphereCenter, float sphereRadius, AABB3 box);
bool DoZCylinderAndAABBOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, AABB3 box);
bool DoZCylinderAndSphereOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, Vec3 sphereCenter, float sphereRadius);
bool DoSphereAndOBBOverlap3D(Vec3 const& sphereCenter, float sphereRadius, OBB3 const& orientedBox);
bool DoSphereAndPlaneOverlap3D(Vec3 const& sphereCenter, float sphereRadius, Plane3 const& plane);
bool DoAABBAndPlaneOverlap3D(AABB3 const& box, Plane3 const& plane);
bool DoOBBAndPlaneOverlap3D(OBB3 const& orientedBox, Plane3 const& plane);

Vec2 const GetNearestPointOnDisc2D(Vec2 const& referencePosition, Vec2 const& discCenter, float discRadius);
Vec2 const GetNearestPointOnAABB2D(Vec2 const& referencePos, AABB2 const& box);
Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, LineSegment2 const& infiniteLine);
Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, Vec2 const& pointOnLine, Vec2 const& anotherPointOnLine);
Vec2 const GetNearestPointOnLineSegment2D(Vec2 const& referencePos, LineSegment2 const& lineSegment);
Vec2 const GetNearestPointOnLineSegment2D(Vec2 const& referencePos, Vec2 const& lineSegStart, Vec2 const& lineSegEnd);
Vec2 const GetNearestPointOnCapsule2D(Vec2 const& referencePos, Capsule2 const& capsule);
Vec2 const GetNearestPointOnCapsule2D(Vec2 const& referencePos, Vec2 const& boneStart, Vec2 const& boneEnd, float radius);
Vec2 const GetNearestPointOnTriangle2D(Vec2 const& referencePos, Vec2 const& triCCW0, Vec2 const& triCCW1, Vec2 const& triCCW2);
Vec2 const GetNearestPointOnTriangle2D(Vec2 const& referencePos, Triangle2 const& triangle);
Vec2 const GetNearestPointOnOBB2D(Vec2 const& referencePos, OBB2 const& orientedBox);
Vec3 const GetNearestPointOnSphere3D(Vec3 const& referencePos, Vec3 sphereCenter, float sphereRadius);
Vec3 const GetNearestPointOnAABB3D(Vec3 const& referencePos, AABB3 const& box);
Vec3 const GetNearestPointOnCylinderZ3D(Vec3 const& referencePos, Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ);
Vec3 const GetNearestPointOnOBB3D(Vec3 const& referencePos, OBB3 const& orientedBox);

// return WerePushed
bool PushDiscOutOfFixedPoint2D(Vec2& mobileDiscCenter, float discRadius, Vec2 const& fixedPoint);
bool PushDiscOutOfFixedDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius);
bool PushDiscsOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius);
bool PushDiscOutOfFixedAABB2D(Vec2& mobileDisCenter, float discRadius, AABB2 const& fixedBox);

// Plane Classification: 0-Intersect, +1-completely Front, -1-completely on Back side of the plane
// Usually be used in bounding box/sphere
int ClassifyAABBAgainstPlane3D(AABB3 const& box, Plane3 const& plane);
int ClassifySphereAgainstPlane3D(Vec3 const& sphereCenter, float sphereRadius, Plane3 const& plane);


//-----------------------------------------------------------------------------------------------
// Transform utilities
//
void TransformPosition2D(Vec2& posToTransform, float uniformScale, float rotationDegrees, Vec2 const& translation);
void TransformPosition2D(Vec2& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation);
void TransformPositionXY3D(Vec3& positionToTransform, float scaleXY, float zRotationDegrees, Vec2 const& translationXY);
void TransformPositionXY3D(Vec3& positionToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translationXY);

//-----------------------------------------------------------------------------------------------
float NormalizeByte(unsigned char b);
unsigned char DenormalizeByte(float zeroToOne);


//-----------------------------------------------------------------------------------------------
//Mat44 const LookAtMat


//-----------------------------------------------------------------------------------------------
// Bezier standalone functions
//
float ComputeQuadraticBezier1D(float A, float B, float C, float t);
float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t);

float ComputeCubicBezier(float A, float B, float C, float D, float t);
float ComputeCubicBezierDerivative(float A, float B, float C, float D, float t);
float ComputeCubicBezierSecondDerivative(float A, float B, float C, float D, float t);

float ComputeCubicHermite(float startPos, float startVel, float endPos, float endVel, float t);
float ComputeCubicHermiteDerivative(float startPos, float startVel, float endPos, float endVel, float t);
float ComputeCubicHermiteSecondDerivative(float startPos, float startVel, float endPos, float endVel, float t);

Vec2 ComputeCubicBezier(Vec2 const& A, Vec2 const& B, Vec2 const& C, Vec2 const& D, float t);
Vec2 ComputeCubicBezierDerivative(Vec2 const& A, Vec2 const& B, Vec2 const& C, Vec2 const& D, float t);
Vec2 ComputeCubicBezierSecondDerivative(Vec2 const& A, Vec2 const& B, Vec2 const& C, Vec2 const& D, float t);

Vec2 ComputeCubicHermite(Vec2 const& startPos, Vec2 const& startVel, Vec2 const& endPos, Vec2 const& endVel, float t);
Vec2 ComputeCubicHermiteDerivative(Vec2 const& startPos, Vec2 const& startVel, Vec2 const& endPos, Vec2 const& endVel, float t);
Vec2 ComputeCubicHermiteSecondDerivative(Vec2 const& startPos, Vec2 const& startVel, Vec2 const& endPos, Vec2 const& endVel, float t);

Vec3 ComputeCubicBezier(Vec3 const& A, Vec3 const& B, Vec3 const& C, Vec3 const& D, float t);
Vec3 ComputeCubicBezierDerivative(Vec3 const& A, Vec3 const& B, Vec3 const& C, Vec3 const& D, float t);
Vec3 ComputeCubicBezierSecondDerivative(Vec3 const& A, Vec3 const& B, Vec3 const& C, Vec3 const& D, float t);

Vec3 ComputeCubicHermite(Vec3 const& startPos, Vec3 const& startVel, Vec3 const& endPos, Vec3 const& endVel, float t);
Vec3 ComputeCubicHermiteDerivative(Vec3 const& startPos, Vec3 const& startVel, Vec3 const& endPos, Vec3 const& endVel, float t);
Vec3 ComputeCubicHermiteSecondDerivative(Vec3 const& startPos, Vec3 const& startVel, Vec3 const& endPos, Vec3 const& endVel, float t);

//-----------------------------------------------------------------------------------------------
// Easing Functions
//
float SmoothLinear(float t);

float SmoothStart2(float t); // a.k.a. "EaseInQuadratic"
float SmoothStart3(float t); // a.k.a. "EaseInCubic"
float SmoothStart4(float t); // a.k.a. "EaseInQuartic"
float SmoothStart5(float t); // a.k.a. "EaseInQuintic"
float SmoothStart6(float t); // a.k.a. "EaseIn6thOrder"

float SmoothEnd2(float t); // a.k.a. "EaseOutQuadratic"
float SmoothEnd3(float t); // a.k.a. "EaseOutCubic"
float SmoothEnd4(float t); // a.k.a. "EaseOutQuartic"
float SmoothEnd5(float t); // a.k.a. "EaseOutQuintic"
float SmoothEnd6(float t); // a.k.a. "EaseOut6thOrder"

float SmoothStep3(float t); // a.k.a. "EaseInOutCubic"
float SmoothStep5(float t); // a.k.a. "EaseInOutQuintic"

float Hesitate3(float t);
float Hesitate5(float t);

float BounceClampBottom(float t);
float BounceClampTop(float t);
float BounceClampBottomTop(float t);

float BounceEndBezier5(float t);


//-----------------------------------------------------------------------------------------------
int Sign(float value);
