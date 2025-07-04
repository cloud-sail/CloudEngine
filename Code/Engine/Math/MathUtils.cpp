#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Plane3.hpp"
#include <math.h>


Mat44 GetBillboardTransform(BillboardType billboardType, Mat44 const& targetTransform, Vec3 const& billboardPosition, Vec2 const& billboardScale /*= Vec2(1.f, 1.f)*/)
{
    Mat44 result;
    // Calculate rotation
    switch (billboardType)
    {
    case BillboardType::WORLD_UP_FACING:
        result = Mat44::MakeFromZX(Vec3::ZAXIS, targetTransform.GetTranslation3D() - billboardPosition);
        break;
    case BillboardType::WORLD_UP_OPPOSING:
        result = Mat44::MakeFromZX(Vec3::ZAXIS, -targetTransform.GetIBasis3D());
        break;
    case BillboardType::FULL_FACING:
        result = Mat44::MakeFromX(targetTransform.GetTranslation3D() - billboardPosition);
		break;
    case BillboardType::FULL_OPPOSING:
        result = Mat44(-targetTransform.GetIBasis3D(), -targetTransform.GetJBasis3D(), targetTransform.GetKBasis3D(), Vec3::ZERO);
		break;
    default:
        result = Mat44();
        break;
    }
    // scale and position
    result.SetIJKT3D(result.GetIBasis3D(), billboardScale.x * result.GetJBasis3D(), billboardScale.y * result.GetKBasis3D(), billboardPosition);

    return result;
}

float GetClamped(float value, float minValue, float maxValue)
{
	if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

float GetClampedZeroToOne(float value)
{
	if (value < 0.f) return 0.f;
	if (value > 1.f) return 1.f;
	return value;
}

float Interpolate(float start, float end, float fractionTowardsEnd)
{
    return start + (end - start) * fractionTowardsEnd;
}

Vec2 Interpolate(Vec2 start, Vec2 end, float fractionTowardsEnd)
{
    return start + (end - start) * fractionTowardsEnd;
}

Vec3 Interpolate(Vec3 start, Vec3 end, float fractionTowardsEnd)
{
	return start + (end - start) * fractionTowardsEnd;
}

float GetFractionWithinRange(float value, float rangeStart, float rangeEnd)
{
    if (rangeEnd == rangeStart)
    {
        return 0.f;
    }
    float fraction = (value - rangeStart) / (rangeEnd - rangeStart);
    return fraction;
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
    return Interpolate(outStart, outEnd, GetFractionWithinRange(inValue, inStart, inEnd));
}

float RangeMapClamped(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	return Interpolate(outStart, outEnd, GetClampedZeroToOne(GetFractionWithinRange(inValue, inStart, inEnd)));
}

int RoundDownToInt(float value)
{
    return static_cast<int>(floorf(value));
}

float ConvertDegreesToRadians(float degrees)
{
    return degrees * (3.1415926535897932384626433832795f / 180.f);
}

float ConvertRadiansToDegrees(float radians)
{
    return radians * (180.f / 3.1415926535897932384626433832795f);
}

float CosDegrees(float degrees)
{
    return cosf(ConvertDegreesToRadians(degrees));
}

float SinDegrees(float degrees)
{
    return sinf(ConvertDegreesToRadians(degrees));
}

float TanDegrees(float degrees)
{
    return tanf(ConvertDegreesToRadians(degrees));
}

float Atan2Degrees(float y, float x)
{
    return ConvertRadiansToDegrees(atan2f(y, x));
}

float CosRadians(float radians)
{
    return cosf(radians);
}

float SinRadians(float radians)
{
    return sinf(radians);
}

float Atan2Radians(float y, float x)
{
    return atan2f(y, x);
}

float AcosRadians(float value)
{
    return acosf( (value < -1.f) ? -1.f : ((value < 1.f) ? value : 1.f) );
}

float GetShortestAngularDispDegrees(float startDegrees, float endDegrees)
{
    float dispDegrees = endDegrees - startDegrees;
    while (dispDegrees > 180.f)
    {
        dispDegrees -= 360.f;
    }
	while (dispDegrees < -180.f)
	{
		dispDegrees += 360.f;
	}
    return dispDegrees;
}

float GetTurnedTowardDegrees(float currentDegrees, float goalDegrees, float maxDeltaDegrees)
{
    // assert maxDeltaDegrees >= 0.f
    float shortestDispDegrees = GetShortestAngularDispDegrees(currentDegrees, goalDegrees);
    if (shortestDispDegrees >= -maxDeltaDegrees && shortestDispDegrees <= maxDeltaDegrees)
    {
        return goalDegrees;
    }
    else
    {
        float direction = (shortestDispDegrees > 0.f) ? 1.f : -1.f;
        return currentDegrees + direction * maxDeltaDegrees;
    }
}

float GetAngleDegreesBetweenVectors2D(Vec2 const& a, Vec2 const& b)
{
    
    float dotProduct = DotProduct2D(a, b);
    float lengthA = a.GetLength();
    float lengthB = b.GetLength();
    if (lengthA == 0.f || lengthB == 0.f) return 0.f;
    float cosValue = dotProduct / (lengthA * lengthB);
    cosValue = GetClamped(cosValue, -1.f, 1.f);
    return ConvertRadiansToDegrees(acosf(cosValue));
}

float LinearSine(float value, float period, bool minusOneToOne /*= true*/)
{
    float t = (value / period);
    t = t - floorf(t);

    float result = 0.f;
    if (t < 0.25f)
    {
        result = 4.f * t;
    }
    else if (t < 0.75f)
    {
        result = 2.f - 4.f * t;
    }
    else
    {
        result = 4.f * t - 4.f;
    }

    if (!minusOneToOne)
    {
        result = (result + 1.f) * 0.5f;
    }

    return result;
}

float DotProduct2D(Vec2 const& a, Vec2 const& b)
{
    return (a.x * b.x) + (a.y * b.y);
}

float DotProduct3D(Vec3 const& a, Vec3 const& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

float DotProduct4D(Vec4 const& a, Vec4 const& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

float CrossProduct2D(Vec2 const& a, Vec2 const& b)
{
    return a.x * b.y - b.x * a.y;
}

Vec3 CrossProduct3D(Vec3 const& a, Vec3 const& b)
{
    return Vec3(a.y * b.z - b.y * a.z,
                -a.x * b.z + b.x * a.z,
                a.x * b.y - b.x * a.y);
}

float GetDistance2D(Vec2 const& positionA, Vec2 const& positionB)
{
    return sqrtf((positionA.x - positionB.x)* (positionA.x - positionB.x) +
                 (positionA.y - positionB.y)* (positionA.y - positionB.y));
}

float GetDistanceSquared2D(Vec2 const& positionA, Vec2 const& positionB)
{
    return (positionA.x - positionB.x) * (positionA.x - positionB.x) +
           (positionA.y - positionB.y) * (positionA.y - positionB.y);
}

float GetDistance3D(Vec3 const& positionA, Vec3 const& positionB)
{
    return sqrtf((positionA.x - positionB.x) * (positionA.x - positionB.x) +
                 (positionA.y - positionB.y) * (positionA.y - positionB.y) +
                 (positionA.z - positionB.z) * (positionA.z - positionB.z));
}

float GetDistanceSquared3D(Vec3 const& positionA, Vec3 const& positionB)
{
    return (positionA.x - positionB.x) * (positionA.x - positionB.x) +
           (positionA.y - positionB.y) * (positionA.y - positionB.y) +
           (positionA.z - positionB.z) * (positionA.z - positionB.z);
}

float GetDistanceXY3D(Vec3 const& positionA, Vec3 const& positionB)
{
    return sqrtf((positionA.x - positionB.x) * (positionA.x - positionB.x) +
                 (positionA.y - positionB.y) * (positionA.y - positionB.y));
}

float GetDistanceXYSquared3D(Vec3 const& positionA, Vec3 const& positionB)
{
    return (positionA.x - positionB.x) * (positionA.x - positionB.x) +
           (positionA.y - positionB.y) * (positionA.y - positionB.y);
}

int GetTaxicabDistance2D(IntVec2 const& pointA, IntVec2 const& pointB)
{
    return (pointB - pointA).GetTaxicabLength();
}

float GetProjectedLength2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto)
{
    return DotProduct2D(vectorToProject,vectorToProjectOnto) / vectorToProjectOnto.GetLength();
}

Vec2 const GetProjectedOnto2D(Vec2 const& vectorToProject, Vec2 const& vectorToProjectOnto)
{
    return DotProduct2D(vectorToProject, vectorToProjectOnto) * vectorToProjectOnto / vectorToProjectOnto.GetLengthSquared();
}

void CalculateTangentBitangent(Vec3& out_tangent, Vec3& out_bitangent, Vec3 const& pos0, Vec3 const& pos1, Vec3 const& pos2, Vec2 const& uv0, Vec2 const& uv1, Vec2 const& uv2)
{
    Vec3 e0 = pos1 - pos0;
    Vec3 e1 = pos2 - pos0;

	Vec2 deltaUV0 = uv1 - uv0;
	Vec2 deltaUV1 = uv2 - uv0;

	float du0 = uv1.x - uv0.x;
	float du1 = uv2.x - uv0.x;
	float dv0 = uv1.y - uv0.y;
	float dv1 = uv2.y - uv0.y;

    float r = 1.f / (du0 * dv1 - du1 * dv0);
    out_tangent = (e0 * dv1 - e1 * dv0) * r;
    out_bitangent = (e1 * du0 - e0 * du1) * r;
}

bool IsPointInsideDisc2D(Vec2 const& point, Vec2 const& discCenter, float discRadius)
{
    return GetDistanceSquared2D(point, discCenter) < discRadius * discRadius;
}

bool IsPointInsideAABB2D(Vec2 const& point, AABB2 const& box)
{
    return box.IsPointInside(point);
}

bool IsPointInsideCapsule2D(Vec2 const& point, Capsule2 const& capsule)
{
	Vec2 nearestPointOnBone = GetNearestPointOnLineSegment2D(point, capsule.m_bone);
	Vec2 nearestPointToRef = point - nearestPointOnBone;
    if (nearestPointToRef.GetLengthSquared() < capsule.m_radius * capsule.m_radius)
    {
        return true;
    }
    return false;
}

bool IsPointInsideCapsule2D(Vec2 const& point, Vec2 const& boneStart, Vec2 const& boneEnd, float radius)
{
    return IsPointInsideCapsule2D(point, Capsule2(boneStart, boneEnd, radius));
}

bool IsPointInsideTriangle2D(Vec2 const& point, Vec2 const& triCCW0, Vec2 const& triCCW1, Vec2 const& triCCW2)
{
	Vec2 normalOfCCW01 = (triCCW1 - triCCW0).GetRotated90Degrees();
	Vec2 normalOfCCW12 = (triCCW2 - triCCW1).GetRotated90Degrees();
	Vec2 normalOfCCW20 = (triCCW0 - triCCW2).GetRotated90Degrees();

	bool isLeftOfCCW01 = (DotProduct2D(point - triCCW0, normalOfCCW01) > 0.f);
	bool isLeftOfCCW12 = (DotProduct2D(point - triCCW1, normalOfCCW12) > 0.f);
	bool isLeftOfCCW20 = (DotProduct2D(point - triCCW2, normalOfCCW20) > 0.f);
    // CCW is all positive, CW is all negative
    return isLeftOfCCW01 && isLeftOfCCW12 && isLeftOfCCW20;
}

bool IsPointInsideTriangle2D(Vec2 const& point, Triangle2 const& triangle)
{
    return IsPointInsideTriangle2D(point, triangle.m_pointsCounterClockwise[0], triangle.m_pointsCounterClockwise[1], triangle.m_pointsCounterClockwise[2]);
}

bool IsPointInsideOBB2D(Vec2 const& point, OBB2 const& orientedBox)
{
    Vec2 displacement = point - orientedBox.m_center;
    float iProj = DotProduct2D(displacement, orientedBox.m_iBasisNormal);
    float jProj = DotProduct2D(displacement, orientedBox.m_iBasisNormal.GetRotated90Degrees());
    return (iProj > -orientedBox.m_halfDimensions.x &&
            iProj < orientedBox.m_halfDimensions.x &&
		    jProj > -orientedBox.m_halfDimensions.y &&
		    jProj < orientedBox.m_halfDimensions.y);
}

bool IsPointInsideOrientedSector2D(Vec2 const& point, Vec2 const& sectorTip, float sectorForwardDegrees, float sectorApertureDegrees, float sectorRadius)
{
    Vec2 disp = point - sectorTip;
    float lengthSquared = disp.GetLengthSquared();
    if (lengthSquared >= sectorRadius * sectorRadius || lengthSquared == 0.f) return false;

	float rotation = GetAngleDegreesBetweenVectors2D(disp, Vec2::MakeFromPolarDegrees(sectorForwardDegrees));
	return rotation < sectorApertureDegrees * 0.5f;
}

bool IsPointInsideDirectedSector2D(Vec2 const& point, Vec2 const& sectorTip, Vec2 const& sectorForwardNormal, float sectorApertureDegrees, float sectorRadius)
{
    Vec2 disp = point - sectorTip;
	float lengthSquared = disp.GetLengthSquared();
	if (lengthSquared >= sectorRadius * sectorRadius || lengthSquared == 0.f) return false;
    float rotation = GetAngleDegreesBetweenVectors2D(disp, sectorForwardNormal);
    return rotation < sectorApertureDegrees * 0.5f;
}

bool IsPointInsideCylinderZ3D(Vec3 const& point, Vec2 const& centerXY, FloatRange const& minMaxZ, float radiusXY)
{
    if (point.z >= minMaxZ.m_max || point.z <= minMaxZ.m_min)
    {
        return false;
    }
    if (GetDistanceSquared2D(Vec2(point.x, point.y), centerXY) >= radiusXY * radiusXY)
    {
        return false;
    }
    return true;
}

bool IsPointInsideSphere3D(Vec3 const& point, Vec3 sphereCenter, float sphereRadius)
{
    return GetDistanceSquared3D(point, sphereCenter) < sphereRadius * sphereRadius;
}

bool IsPointInsideAABB3D(Vec3 const& point, AABB3 const& box)
{
    return box.IsPointInside(point);
}

bool IsPointInsideOBB3D(Vec3 const& point, OBB3 const& orientedBox)
{
	Vec3 localPos = orientedBox.GetLocalPosForWorldPos(point);
	AABB3 localOBB = AABB3(-orientedBox.m_halfDimensions, orientedBox.m_halfDimensions);
	return localOBB.IsPointInside(localPos);
}

bool DoDiscsOverlap(Vec2 const& centerA, float radiusA, Vec2 const& centerB, float radiusB)
{
    float gap = radiusA + radiusB;
    return GetDistanceSquared2D(centerA, centerB) < gap * gap;
}

bool DoSpheresOverlap(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB)
{
    float gap = radiusA + radiusB;
    return GetDistanceSquared3D(centerA, centerB) < gap * gap;
}

bool DoDiscAndAABBOverlap2D(Vec2 const& discCenter, float radius, AABB2 const& box)
{
    Vec2 nearestPoint = box.GetNearestPoint(discCenter);
    return GetDistanceSquared2D(discCenter, nearestPoint) < radius * radius;
}

bool DoAABBsOverlap2D(AABB2 const& first, AABB2 const& second)
{
	if (first.m_mins.x >= second.m_maxs.x) return false;
	if (first.m_maxs.x <= second.m_mins.x) return false;
	if (first.m_mins.y >= second.m_maxs.y) return false;
	if (first.m_maxs.y <= second.m_mins.y) return false;

	return true;
}

bool DoAABBsOverlap3D(AABB3 const& first, AABB3 const& second)
{
	if (first.m_mins.x >= second.m_maxs.x) return false;
	if (first.m_maxs.x <= second.m_mins.x) return false;
	if (first.m_mins.y >= second.m_maxs.y) return false;
	if (first.m_maxs.y <= second.m_mins.y) return false;
	if (first.m_mins.z >= second.m_maxs.z) return false;
	if (first.m_maxs.z <= second.m_mins.z) return false;
    
    return true;
}

bool DoSpheresOverlap3D(Vec3 const& centerA, float radiusA, Vec3 const& centerB, float radiusB)
{
    float gap = radiusA + radiusB;
    return GetDistanceSquared3D(centerA, centerB) < gap * gap;
}

bool DoZCylindersOverlap3D(Vec2 cylinder1CenterXY, float cylinder1Radius, FloatRange cylinder1MinMaxZ, Vec2 cylinder2CenterXY, float cylinder2Radius, FloatRange cylinder2MinMaxZ)
{
	// Horizontally Overlapped
	if (!DoDiscsOverlap(cylinder1CenterXY, cylinder1Radius, cylinder2CenterXY, cylinder2Radius))
	{
		return false;
	}

	// Vertically Overlapped
	if (!cylinder1MinMaxZ.IsOverlappingWith(cylinder2MinMaxZ))
	{
		return false;
	}

    return true;
}

bool DoSphereAndAABBOverlap3D(Vec3 sphereCenter, float sphereRadius, AABB3 box)
{
    Vec3 nearestPoint = box.GetNearestPoint(sphereCenter);
    return IsPointInsideSphere3D(nearestPoint, sphereCenter, sphereRadius);
}

bool DoZCylinderAndAABBOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, AABB3 box)
{
    // Horizontally Overlapped
    if (!DoDiscAndAABBOverlap2D(cylinderCenterXY, cylinderRadius, AABB2(box.m_mins.x, box.m_mins.y, box.m_maxs.x, box.m_maxs.y)))
    {
        return false;
    }

    // Vertically Overlapped
    if (!cylinderMinMaxZ.IsOverlappingWith(FloatRange(box.m_mins.z, box.m_maxs.z)))
    {
        return false;
    }

    return true;
}

bool DoZCylinderAndSphereOverlap3D(Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ, Vec3 sphereCenter, float sphereRadius)
{
    Vec3 nearestPoint = GetNearestPointOnCylinderZ3D(sphereCenter, cylinderCenterXY, cylinderRadius, cylinderMinMaxZ);
    return IsPointInsideSphere3D(nearestPoint, sphereCenter, sphereRadius);
}

bool DoSphereAndOBBOverlap3D(Vec3 const& sphereCenter, float sphereRadius, OBB3 const& orientedBox)
{
    Vec3 nearestPoint = orientedBox.GetNearestPoint(sphereCenter);
    return IsPointInsideSphere3D(nearestPoint, sphereCenter, sphereRadius);
}

bool DoSphereAndPlaneOverlap3D(Vec3 const& sphereCenter, float sphereRadius, Plane3 const& plane)
{
    float signedDistance = plane.GetSignedDistanceToPoint(sphereCenter);
    return (signedDistance * signedDistance < sphereRadius * sphereRadius);
}

bool DoAABBAndPlaneOverlap3D(AABB3 const& box, Plane3 const& plane)
{
    Vec3 cornerPoints[8];
    box.GetCornerPoints(cornerPoints);

    bool hasPointFront = false;
    bool hasPointBehind = false;

    for (int i = 0; i < 8; ++i)
    {
		float distance = plane.GetSignedDistanceToPoint(cornerPoints[i]);
		if (distance > 0.f) {
            hasPointFront = true;
		}
		else if (distance < 0.f) {
            hasPointBehind = true;
		}
		if (hasPointFront && hasPointBehind) {
			return true;
		}
    }

    return false;
}

bool DoOBBAndPlaneOverlap3D(OBB3 const& orientedBox, Plane3 const& plane)
{
	Vec3 cornerPoints[8];
    orientedBox.GetCornerPoints(cornerPoints);

	bool hasPointFront = false;
	bool hasPointBehind = false;

	for (int i = 0; i < 8; ++i)
	{
		float distance = plane.GetSignedDistanceToPoint(cornerPoints[i]);
		if (distance > 0.f) {
			hasPointFront = true;
		}
		else if (distance < 0.f) {
			hasPointBehind = true;
		}
		if (hasPointFront && hasPointBehind) {
			return true;
		}
	}

	return false;
}



Vec2 const GetNearestPointOnDisc2D(Vec2 const& referencePosition, Vec2 const& discCenter, float discRadius)
{
    Vec2 centerToPos = (referencePosition - discCenter);
    float distanceSquared = centerToPos.GetLengthSquared();
    if (distanceSquared <= discRadius * discRadius)
    {
        return referencePosition;
    }
    centerToPos.SetLength(discRadius);
    return discCenter + centerToPos;
}

Vec2 const GetNearestPointOnAABB2D(Vec2 const& referencePos, AABB2 const& box)
{
    return box.GetNearestPoint(referencePos);
}

Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, LineSegment2 const& infiniteLine)
{
    Vec2 startToRef = referencePos - infiniteLine.m_start;
    Vec2 startToEnd = infiniteLine.m_end - infiniteLine.m_start;
    return infiniteLine.m_start + GetProjectedOnto2D(startToRef, startToEnd);
}

Vec2 const GetNearestPointOnInfiniteLine2D(Vec2 const& referencePos, Vec2 const& pointOnLine, Vec2 const& anotherPointOnLine)
{
    return GetNearestPointOnInfiniteLine2D(referencePos, LineSegment2(pointOnLine, anotherPointOnLine));
}

Vec2 const GetNearestPointOnLineSegment2D(Vec2 const& referencePos, LineSegment2 const& lineSegment)
{
	Vec2 startToRef = referencePos - lineSegment.m_start;
	Vec2 startToEnd = lineSegment.m_end - lineSegment.m_start;
    if (DotProduct2D(startToRef, startToEnd) <= 0.f)
    {
        return lineSegment.m_start;
    }
    Vec2 endToRef = referencePos - lineSegment.m_end;
    if (DotProduct2D(endToRef, startToEnd) >= 0.f)
    {
        return lineSegment.m_end;
    }
    return GetNearestPointOnInfiniteLine2D(referencePos, lineSegment);
}

Vec2 const GetNearestPointOnLineSegment2D(Vec2 const& referencePos, Vec2 const& lineSegStart, Vec2 const& lineSegEnd)
{
    return GetNearestPointOnLineSegment2D(referencePos, LineSegment2(lineSegStart, lineSegEnd));
}

Vec2 const GetNearestPointOnCapsule2D(Vec2 const& referencePos, Capsule2 const& capsule)
{
    Vec2 nearestPointOnBone = GetNearestPointOnLineSegment2D(referencePos, capsule.m_bone);
    Vec2 nearestPointToRef = referencePos - nearestPointOnBone;
    if (nearestPointToRef.GetLengthSquared() <= capsule.m_radius * capsule.m_radius)
    {
        return referencePos;
    }
    return nearestPointOnBone + nearestPointToRef.GetClamped(capsule.m_radius);
}

Vec2 const GetNearestPointOnCapsule2D(Vec2 const& referencePos, Vec2 const& boneStart, Vec2 const& boneEnd, float radius)
{
    return GetNearestPointOnCapsule2D(referencePos, Capsule2(boneStart, boneEnd, radius));
}

Vec2 const GetNearestPointOnTriangle2D(Vec2 const& referencePos, Vec2 const& triCCW0, Vec2 const& triCCW1, Vec2 const& triCCW2)
{
 //   if (IsPointInsideTriangle2D(referencePos, triCCW0, triCCW1, triCCW2))
 //   {
 //       return referencePos;
 //   }

	//Vec2 point0 = GetNearestPointOnLineSegment2D(referencePos, triCCW0, triCCW1);
	//Vec2 point1 = GetNearestPointOnLineSegment2D(referencePos, triCCW1, triCCW2);
	//Vec2 point2 = GetNearestPointOnLineSegment2D(referencePos, triCCW2, triCCW0);

	//float squaredDistance0 = (referencePos - point0).GetLengthSquared();
	//float squaredDistance1 = (referencePos - point1).GetLengthSquared();
	//float squaredDistance2 = (referencePos - point2).GetLengthSquared();

	//if (squaredDistance0 <= squaredDistance1 && squaredDistance0 <= squaredDistance2)
	//{
	//	return point0;
	//}
	//else if (squaredDistance1 <= squaredDistance0 && squaredDistance1 <= squaredDistance2)
	//{
	//	return point1;
	//}
	//else
	//{
	//	return point2;
	//}

    //-----------------------------------------------------------------------------------------------
    // Voronoi Method
	Vec2 CCW0ToRef = referencePos - triCCW0;
	Vec2 CCW1ToRef = referencePos - triCCW1;
	Vec2 CCW2ToRef = referencePos - triCCW2;
	Vec2 CCW01 = triCCW1 - triCCW0;
	Vec2 CCW12 = triCCW2 - triCCW1;
	Vec2 CCW20 = triCCW0 - triCCW2;
	// Voronoi triCCW0
	if (DotProduct2D(CCW0ToRef, CCW01) <= 0.f && DotProduct2D(CCW0ToRef, CCW20) >= 0.f)
	{
		return triCCW0;
	}
	// Voronoi triCCW1
	if (DotProduct2D(CCW1ToRef, CCW12) <= 0.f && DotProduct2D(CCW1ToRef, CCW01) >= 0.f)
	{
		return triCCW1;
	}
	// Voronoi triCCW2
	if (DotProduct2D(CCW2ToRef, CCW20) <= 0.f && DotProduct2D(CCW2ToRef, CCW12) >= 0.f)
	{
		return triCCW2;
	}
	// Voronoi CCW01
	Vec2 normalOfCCW01 = CCW01.GetRotated90Degrees();
	if (DotProduct2D(CCW0ToRef, normalOfCCW01) <= 0.f && DotProduct2D(CCW0ToRef, CCW01) >= 0.f && DotProduct2D(CCW1ToRef, CCW01) <= 0.f)
	{
		return GetNearestPointOnInfiniteLine2D(referencePos, triCCW0, triCCW1);
	}
	// Voronoi CCW12
	Vec2 normalOfCCW12 = CCW12.GetRotated90Degrees();
	if (DotProduct2D(CCW1ToRef, normalOfCCW12) <= 0.f && DotProduct2D(CCW1ToRef, CCW12) >= 0.f && DotProduct2D(CCW2ToRef, CCW12) <= 0.f)
	{
		return GetNearestPointOnInfiniteLine2D(referencePos, triCCW1, triCCW2);
	}
	// Voronoi CCW12
	Vec2 normalOfCCW20 = CCW20.GetRotated90Degrees();
	if (DotProduct2D(CCW2ToRef, normalOfCCW20) <= 0.f && DotProduct2D(CCW2ToRef, CCW20) >= 0.f && DotProduct2D(CCW0ToRef, CCW20) <= 0.f)
	{
		return GetNearestPointOnInfiniteLine2D(referencePos, triCCW2, triCCW0);
	}
	// Voronoi On/Inside the triangle
    return referencePos;
}

Vec2 const GetNearestPointOnTriangle2D(Vec2 const& referencePos, Triangle2 const& triangle)
{
    return GetNearestPointOnTriangle2D(referencePos, triangle.m_pointsCounterClockwise[0], triangle.m_pointsCounterClockwise[1], triangle.m_pointsCounterClockwise[2]);
}

Vec2 const GetNearestPointOnOBB2D(Vec2 const& referencePos, OBB2 const& orientedBox)
{
	Vec2 displacement = referencePos - orientedBox.m_center;
	float iProj = DotProduct2D(displacement, orientedBox.m_iBasisNormal);
	float jProj = DotProduct2D(displacement, orientedBox.m_iBasisNormal.GetRotated90Degrees());
    float nearestI = GetClamped(iProj, -orientedBox.m_halfDimensions.x, orientedBox.m_halfDimensions.x);
    float nearestJ = GetClamped(jProj, -orientedBox.m_halfDimensions.y, orientedBox.m_halfDimensions.y);
    return orientedBox.m_center + nearestI * orientedBox.m_iBasisNormal + nearestJ * orientedBox.m_iBasisNormal.GetRotated90Degrees();
}

Vec3 const GetNearestPointOnSphere3D(Vec3 const& referencePos, Vec3 sphereCenter, float sphereRadius)
{
    Vec3 centerToPos = (referencePos - sphereCenter);
    float squaredDistance = centerToPos.GetLengthSquared();
    if (squaredDistance <= sphereRadius * sphereRadius)
    {
        return referencePos;
    }

	Vec3 nearestPointOffset = centerToPos * (sphereRadius / sqrtf(squaredDistance));
	return sphereCenter + nearestPointOffset;
}

Vec3 const GetNearestPointOnAABB3D(Vec3 const& referencePos, AABB3 const& box)
{
    return box.GetNearestPoint(referencePos);
}

Vec3 const GetNearestPointOnCylinderZ3D(Vec3 const& referencePos, Vec2 cylinderCenterXY, float cylinderRadius, FloatRange cylinderMinMaxZ)
{
    Vec2 nearestPointXY = GetNearestPointOnDisc2D(Vec2(referencePos.x, referencePos.y), cylinderCenterXY, cylinderRadius);
    float nearestPointZ = GetClamped(referencePos.z, cylinderMinMaxZ.m_min, cylinderMinMaxZ.m_max);
    return Vec3(nearestPointXY.x, nearestPointXY.y, nearestPointZ);
}

Vec3 const GetNearestPointOnOBB3D(Vec3 const& referencePos, OBB3 const& orientedBox)
{
    return orientedBox.GetNearestPoint(referencePos);
}

bool PushDiscOutOfFixedPoint2D(Vec2& mobileDiscCenter, float discRadius, Vec2 const& fixedPoint)
{
    Vec2 pointToCenter = mobileDiscCenter - fixedPoint;
    float distanceSquared = pointToCenter.GetLengthSquared();
    if (distanceSquared >= discRadius * discRadius || distanceSquared == 0.f)
    {
        return false;
    }
    pointToCenter.SetLength(discRadius);
    mobileDiscCenter = fixedPoint + pointToCenter;
    return true;
}

bool PushDiscOutOfFixedDisc2D(Vec2& mobileDiscCenter, float mobileDiscRadius, Vec2 const& fixedDiscCenter, float fixedDiscRadius)
{
    Vec2 fixedToMobile = mobileDiscCenter - fixedDiscCenter;
	float distanceSquared = fixedToMobile.GetLengthSquared();
    float sumOfRadii = mobileDiscRadius + fixedDiscRadius;
	if (distanceSquared >= sumOfRadii * sumOfRadii || distanceSquared == 0.f)
	{
		return false;
	}
    fixedToMobile.SetLength(sumOfRadii);
    mobileDiscCenter = fixedDiscCenter + fixedToMobile;
    return true;
}

bool PushDiscsOutOfEachOther2D(Vec2& aCenter, float aRadius, Vec2& bCenter, float bRadius)
{
    Vec2 AToB = bCenter - aCenter;
    float distanceSquared = AToB.GetLengthSquared();
    float sumOfRadii = aRadius + bRadius;
    if (distanceSquared >= sumOfRadii * sumOfRadii || distanceSquared == 0.f)
    {
        return false;
    }
    float overlapDepth = sumOfRadii - AToB.GetLength();
    Vec2 correctionForB = AToB.GetNormalized() * (overlapDepth * 0.5f);

    aCenter -= correctionForB;
    bCenter += correctionForB;

    return true;
}

bool PushDiscOutOfFixedAABB2D(Vec2& mobileDiscCenter, float discRadius, AABB2 const& fixedBox)
{
    Vec2 nearestPoint = fixedBox.GetNearestPoint(mobileDiscCenter);
    if (GetDistanceSquared2D(nearestPoint, mobileDiscCenter) >= discRadius * discRadius)
    {
        return false;
    }
    // if disc center is in/on the bounding box, do not push
    return PushDiscOutOfFixedPoint2D(mobileDiscCenter, discRadius, nearestPoint);
}

int ClassifyAABBAgainstPlane3D(AABB3 const& box, Plane3 const& plane)
{
    float minDistance = 0;
    float maxDistance = 0;

    if (plane.m_normal.x > 0.f)
    {
        minDistance += plane.m_normal.x * box.m_mins.x;
        maxDistance += plane.m_normal.x * box.m_maxs.x;
    }
    else
    {
		minDistance += plane.m_normal.x * box.m_maxs.x;
		maxDistance += plane.m_normal.x * box.m_mins.x;
    }

	if (plane.m_normal.y > 0.f)
	{
		minDistance += plane.m_normal.y * box.m_mins.y;
		maxDistance += plane.m_normal.y * box.m_maxs.y;
	}
	else
	{
		minDistance += plane.m_normal.y * box.m_maxs.y;
		maxDistance += plane.m_normal.y * box.m_mins.y;
	}

	if (plane.m_normal.z > 0.f)
	{
		minDistance += plane.m_normal.z * box.m_mins.z;
		maxDistance += plane.m_normal.z * box.m_maxs.z;
	}
	else
	{
		minDistance += plane.m_normal.z * box.m_maxs.z;
		maxDistance += plane.m_normal.z * box.m_mins.z;
	}

    // completely in front of the plane
    if (minDistance >= plane.m_distance)
    {
        return 1;
    }

    // completely behind the plane
    if (maxDistance <= plane.m_distance)
    {
        return -1;
    }
    // Straddle
    return 0;
}



int ClassifySphereAgainstPlane3D(Vec3 const& sphereCenter, float sphereRadius, Plane3 const& plane)
{
    float distance = plane.GetSignedDistanceToPoint(sphereCenter);

    if (distance >= sphereRadius)
    {
        return 1;
    }

    if (distance <= -sphereRadius)
    {
        return -1;
    }

    return 0;
}

void TransformPosition2D(Vec2& posToTransform, float uniformScale, float rotationDegrees, Vec2 const& translation)
{
    posToTransform *= uniformScale;
    posToTransform.RotateDegrees(rotationDegrees);
    posToTransform += translation;

}

void TransformPosition2D(Vec2& posToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translation)
{
    posToTransform = posToTransform.x * iBasis + posToTransform.y * jBasis + translation;
}

void TransformPositionXY3D(Vec3& positionToTransform, float scaleXY, float zRotationDegrees, Vec2 const& translationXY)
{
    positionToTransform.x *= scaleXY;
    positionToTransform.y *= scaleXY;
    positionToTransform = positionToTransform.GetRotatedAboutZDegrees(zRotationDegrees);
    positionToTransform.x += translationXY.x;
    positionToTransform.y += translationXY.y;
}

void TransformPositionXY3D(Vec3& positionToTransform, Vec2 const& iBasis, Vec2 const& jBasis, Vec2 const& translationXY)
{
    Vec2 positionXY = Vec2(positionToTransform.x, positionToTransform.y);
    TransformPosition2D(positionXY, iBasis, jBasis, translationXY);
    positionToTransform.x = positionXY.x;
    positionToTransform.y = positionXY.y;
}

float NormalizeByte(unsigned char b)
{
	float floatZeroToOne = static_cast<float>(b) / 255.f;
	return floatZeroToOne;
}

unsigned char DenormalizeByte(float zeroToOne)
{
    int result = RoundDownToInt(zeroToOne * 256.f);
    if (result == 256) // or zeroToOne == 1.f
    {
        result = 255;
    }
	return  static_cast<unsigned char>(result);
	//return  static_cast<unsigned char>(zeroToOne * 255.f);
}

float ComputeQuadraticBezier1D(float A, float B, float C, float t)
{
	float AB = Interpolate(A, B, t);
	float BC = Interpolate(B, C, t);

	float ABC = Interpolate(AB, BC, t);

    return ABC;
}

float ComputeQuinticBezier1D(float A, float B, float C, float D, float E, float F, float t)
{
	// De Casteljau's Algorithm
	float AB = Interpolate(A, B, t);
	float BC = Interpolate(B, C, t);
	float CD = Interpolate(C, D, t);
	float DE = Interpolate(D, E, t);
	float EF = Interpolate(E, F, t);

	float ABC = Interpolate(AB, BC, t);
	float BCD = Interpolate(BC, CD, t);
	float CDE = Interpolate(CD, DE, t);
	float DEF = Interpolate(DE, EF, t);

	float ABCD = Interpolate(ABC, BCD, t);
	float BCDE = Interpolate(BCD, CDE, t);
	float CDEF = Interpolate(CDE, DEF, t);

	float ABCDE = Interpolate(ABCD, BCDE, t);
	float BCDEF = Interpolate(BCDE, CDEF, t);

	float ABCDEF = Interpolate(ABCDE, BCDEF, t);

	return ABCDEF;
}

float ComputeCubicBezier(float A, float B, float C, float D, float t)
{
    // De Casteljau's Algorithm
	//float AB = Interpolate(A, B, t);
	//float BC = Interpolate(B, C, t);
	//float CD = Interpolate(C, D, t);

 //   float ABC = Interpolate(AB, BC, t);
 //   float BCD = Interpolate(BC, CD, t);

 //   float ABCD = Interpolate(ABC, BCD, t);
 //   return ABCD;

    // Polynomial Form
    float a = -1.f * A + 3.f * B - 3.f * C + D;
    float b = 3.f * A - 6.f * B + 3.f * C;
    float c = -3.f * A + 3.f * B;
    float d = A;

    float t2 = t * t;
    float t3 = t2 * t;

    return (a * t3) + (b * t2) + (c * t) + d;
}

float ComputeCubicBezierDerivative(float A, float B, float C, float D, float t)
{
    float a = -3.f * A + 9.f * B - 9.f * C + 3.f * D;
    float b = 6.f * A - 12.f * B + 6.f * C;
    float c = -3.f * A + 3.f * B;

    float t2 = t * t;

    return (a * t2) + (b * t) + c;
}

float ComputeCubicBezierSecondDerivative(float A, float B, float C, float D, float t)
{
    float a = -6.f * A + 18.f * B - 18.f * C + 6.f * D;
    float b = 6.f * A - 12.f * B + 6.f * C;

    return (a * t) + b;
}

float ComputeCubicHermite(float startPos, float startVel, float endPos, float endVel, float t)
{
    float B = startPos + (startVel / 3.f);
    float C = endPos - (endVel / 3.f);
    return ComputeCubicBezier(startPos, B, C, endPos, t);
}


float ComputeCubicHermiteDerivative(float startPos, float startVel, float endPos, float endVel, float t)
{
	float B = startPos + (startVel / 3.f);
	float C = endPos - (endVel / 3.f);
	return ComputeCubicBezierDerivative(startPos, B, C, endPos, t);
}

float ComputeCubicHermiteSecondDerivative(float startPos, float startVel, float endPos, float endVel, float t)
{
	float B = startPos + (startVel / 3.f);
	float C = endPos - (endVel / 3.f);
	return ComputeCubicBezierSecondDerivative(startPos, B, C, endPos, t);
}

Vec2 ComputeCubicBezier(Vec2 const& A, Vec2 const& B, Vec2 const& C, Vec2 const& D, float t)
{
	Vec2 a = -1.f * A + 3.f * B - 3.f * C + D;
	Vec2 b = 3.f * A - 6.f * B + 3.f * C;
	Vec2 c = -3.f * A + 3.f * B;
	Vec2 d = A;

	float t2 = t * t;
	float t3 = t2 * t;

	return (a * t3) + (b * t2) + (c * t) + d;
}

Vec2 ComputeCubicBezierDerivative(Vec2 const& A, Vec2 const& B, Vec2 const& C, Vec2 const& D, float t)
{
	Vec2 a = -3.f * A + 9.f * B - 9.f * C + 3.f * D;
	Vec2 b = 6.f * A - 12.f * B + 6.f * C;
	Vec2 c = -3.f * A + 3.f * B;

	float t2 = t * t;

	return (a * t2) + (b * t) + c;
}

Vec2 ComputeCubicBezierSecondDerivative(Vec2 const& A, Vec2 const& B, Vec2 const& C, Vec2 const& D, float t)
{
	Vec2 a = -6.f * A + 18.f * B - 18.f * C + 6.f * D;
	Vec2 b = 6.f * A - 12.f * B + 6.f * C;

	return (a * t) + b;
}

Vec2 ComputeCubicHermite(Vec2 const& startPos, Vec2 const& startVel, Vec2 const& endPos, Vec2 const& endVel, float t)
{
	Vec2 B = startPos + (startVel / 3.f);
	Vec2 C = endPos - (endVel / 3.f);
	return ComputeCubicBezier(startPos, B, C, endPos, t);
}

Vec2 ComputeCubicHermiteDerivative(Vec2 const& startPos, Vec2 const& startVel, Vec2 const& endPos, Vec2 const& endVel, float t)
{
	Vec2 B = startPos + (startVel / 3.f);
	Vec2 C = endPos - (endVel / 3.f);
	return ComputeCubicBezierDerivative(startPos, B, C, endPos, t);
}

Vec2 ComputeCubicHermiteSecondDerivative(Vec2 const& startPos, Vec2 const& startVel, Vec2 const& endPos, Vec2 const& endVel, float t)
{
	Vec2 B = startPos + (startVel / 3.f);
	Vec2 C = endPos - (endVel / 3.f);
	return ComputeCubicBezierSecondDerivative(startPos, B, C, endPos, t);
}

Vec3 ComputeCubicBezier(Vec3 const& A, Vec3 const& B, Vec3 const& C, Vec3 const& D, float t)
{
	Vec3 a = -1.f * A + 3.f * B - 3.f * C + D;
	Vec3 b = 3.f * A - 6.f * B + 3.f * C;
	Vec3 c = -3.f * A + 3.f * B;
	Vec3 d = A;

	float t2 = t * t;
	float t3 = t2 * t;

	return (a * t3) + (b * t2) + (c * t) + d;
}

Vec3 ComputeCubicBezierDerivative(Vec3 const& A, Vec3 const& B, Vec3 const& C, Vec3 const& D, float t)
{
	Vec3 a = -3.f * A + 9.f * B - 9.f * C + 3.f * D;
	Vec3 b = 6.f * A - 12.f * B + 6.f * C;
	Vec3 c = -3.f * A + 3.f * B;

	float t2 = t * t;

	return (a * t2) + (b * t) + c;
}

Vec3 ComputeCubicBezierSecondDerivative(Vec3 const& A, Vec3 const& B, Vec3 const& C, Vec3 const& D, float t)
{
	Vec3 a = -6.f * A + 18.f * B - 18.f * C + 6.f * D;
	Vec3 b = 6.f * A - 12.f * B + 6.f * C;

	return (a * t) + b;
}

Vec3 ComputeCubicHermite(Vec3 const& startPos, Vec3 const& startVel, Vec3 const& endPos, Vec3 const& endVel, float t)
{
	Vec3 B = startPos + (startVel / 3.f);
	Vec3 C = endPos - (endVel / 3.f);
	return ComputeCubicBezier(startPos, B, C, endPos, t);
}

Vec3 ComputeCubicHermiteDerivative(Vec3 const& startPos, Vec3 const& startVel, Vec3 const& endPos, Vec3 const& endVel, float t)
{
	Vec3 B = startPos + (startVel / 3.f);
	Vec3 C = endPos - (endVel / 3.f);
	return ComputeCubicBezierDerivative(startPos, B, C, endPos, t);
}

Vec3 ComputeCubicHermiteSecondDerivative(Vec3 const& startPos, Vec3 const& startVel, Vec3 const& endPos, Vec3 const& endVel, float t)
{
	Vec3 B = startPos + (startVel / 3.f);
	Vec3 C = endPos - (endVel / 3.f);
	return ComputeCubicBezierSecondDerivative(startPos, B, C, endPos, t);
}

float SmoothLinear(float t)
{
	return t;
}

float SmoothStart2(float t)
{
	return t * t;
}

float SmoothStart3(float t)
{
	return t * t * t;
}

float SmoothStart4(float t)
{
	return t * t * t * t;
}

float SmoothStart5(float t)
{
	return t * t * t * t * t;
}

float SmoothStart6(float t)
{
    return t * t * t * t * t * t;
}

float SmoothEnd2(float t)
{
    float s = 1.f - t;
	return 1 - s * s;
}

float SmoothEnd3(float t)
{
	float s = 1.f - t;
	return 1 - s * s * s;
}

float SmoothEnd4(float t)
{
	float s = 1.f - t;
	return 1 - s * s * s * s;
}

float SmoothEnd5(float t)
{
	float s = 1.f - t;
	return 1 - s * s * s * s * s;
}

float SmoothEnd6(float t)
{
	float s = 1.f - t;
	return 1 - s * s * s * s * s * s;
}


float SmoothStep3(float t)
{
	//return (1.f - t) * SmoothStart2(t) + t * SmoothEnd2(t);
    return ComputeCubicBezier(0.f, 0.f, 1.f, 1.f, t);
}

float SmoothStep5(float t)
{
	//return (1.f - t) * SmoothStart4(t) + t * SmoothEnd4(t);
    return ComputeQuinticBezier1D(0.f, 0.f, 0.f, 1.f, 1.f, 1.f, t);
}

float Hesitate3(float t)
{
    return ComputeCubicBezier(0.f, 1.f, 0.f, 1.f, t);
}

float Hesitate5(float t)
{
    return ComputeQuinticBezier1D(0.f, 1.f, 0.f, 1.f, 0.f, 1.f, t);
}

float BounceClampBottom(float t)
{
    return fabsf(t);
}

float BounceClampTop(float t)
{
    return 1.f - fabsf(1.f - t);
}

float BounceClampBottomTop(float t)
{
    return BounceClampTop(BounceClampBottom(t));
}

float BounceEndBezier5(float t)
{
    return BounceClampTop(ComputeQuinticBezier1D(0.f, 0.f, 2.f, 1.25f, 0.75f, 1.f, t));
}

int Sign(float value)
{
	if (value > 0.0f)
	{
		return 1;
	}
	else if (value < 0.0f)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

