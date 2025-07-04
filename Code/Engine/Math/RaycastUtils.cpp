#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/FloatRange.hpp"
#include <cmath>


//-----------------------------------------------------------------------------------------------
Ray2::Ray2(Vec2 const& startPos, Vec2 const& endPos)
	: m_startPos(startPos)
{
	Vec2 disp = endPos - startPos;
	m_fwdNormal = disp.GetNormalized();
	m_maxLength = disp.GetLength();
}

Ray2::Ray2(Vec2 const& startPos, Vec2 const& fwdNormal, float maxLength)
	: m_startPos(startPos)
	, m_fwdNormal(fwdNormal)
	, m_maxLength(maxLength)
{
}

//-----------------------------------------------------------------------------------------------
RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, Vec2 const& discCenter, float discRadius)
{
	RaycastResult2D raycastResult;
	raycastResult.m_ray.m_startPos = startPos;
	raycastResult.m_ray.m_fwdNormal = fwdNormal;
	raycastResult.m_ray.m_maxLength = maxDist;

	Vec2 startToCenter = discCenter - startPos;
	Vec2 iBasisNormal = fwdNormal;
	Vec2 jBasisNormal = iBasisNormal.GetRotated90Degrees();

	// Case 1: Altitude too large
	float startToCenterJProj = DotProduct2D(startToCenter, jBasisNormal);
	if (startToCenterJProj >= discRadius || startToCenterJProj <= -discRadius)
	{
		return raycastResult;
	}
	// Case 2: Too far from startPos and endPos
	float startToCenterIProj = DotProduct2D(startToCenter, iBasisNormal);
	if (startToCenterIProj >= discRadius + maxDist || startToCenterIProj <= -discRadius)
	{
		return raycastResult;
	}
	// Case 3: startPos inside Disc
	if (IsPointInsideDisc2D(startPos, discCenter, discRadius))
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = startPos;
		raycastResult.m_impactNormal = -fwdNormal;
		return raycastResult;
	}
	// Case 4: Miss at end
	float adjust = sqrtf(discRadius * discRadius - startToCenterJProj * startToCenterJProj);
	float impactDist = startToCenterIProj - adjust;
	if (impactDist >= maxDist || impactDist <= 0.f) // prevent zero-length raycast
	{
		return raycastResult;
	}
	// Case 5: Hit
	raycastResult.m_didImpact = true;
	raycastResult.m_impactDist = impactDist;
	raycastResult.m_impactPos = startPos + iBasisNormal * impactDist;
	raycastResult.m_impactNormal = (raycastResult.m_impactPos - discCenter).GetNormalized();
	return raycastResult;
}

RaycastResult2D RaycastVsLineSegment2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, LineSegment2 const& lineSegment)
{
	return RaycastVsLineSegment2D(startPos, fwdNormal, maxDist, lineSegment.m_start, lineSegment.m_end);
}

RaycastResult2D RaycastVsLineSegment2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, Vec2 const& lineSegStart, Vec2 const& lineSegEnd)
{
	RaycastResult2D raycastResult;
	raycastResult.m_ray.m_startPos = startPos;
	raycastResult.m_ray.m_fwdNormal = fwdNormal;
	raycastResult.m_ray.m_maxLength = maxDist;

	Vec2 iBasisNormal = fwdNormal;
	Vec2 jBasisNormal = iBasisNormal.GetRotated90Degrees();

	Vec2 startToSegStart = lineSegStart - startPos;
	Vec2 startToSegEnd = lineSegEnd - startPos;
	float startToSegStartJProj = DotProduct2D(startToSegStart, jBasisNormal);
	float startToSegEndJProj = DotProduct2D(startToSegEnd, jBasisNormal);

	// Case 1: Failure to Straddle
	if (startToSegStartJProj * startToSegEndJProj >= 0.f)
	{
		return raycastResult;
	}

	float startToSegStartIProj = DotProduct2D(startToSegStart, iBasisNormal);
	float startToSegEndIProj = DotProduct2D(startToSegEnd, iBasisNormal);

	// Case 2: After Ray End
	if (startToSegStartIProj >= maxDist && startToSegEndIProj >= maxDist)
	{
		return raycastResult;
	}

	// Case 3: Before Ray Start
	if (startToSegStartIProj <= 0.f && startToSegEndIProj <= 0.f)
	{
		return raycastResult;
	}

	float impactFraction = startToSegStartJProj / (startToSegStartJProj - startToSegEndJProj);
	float impactDist = startToSegStartIProj + impactFraction * (startToSegEndIProj - startToSegStartIProj);

	// Case 4: Miss
	if (impactDist <= 0.f || impactDist >= maxDist)
	{
		return raycastResult;
	}

	raycastResult.m_didImpact = true;
	raycastResult.m_impactDist = impactDist;
	raycastResult.m_impactPos = startPos + fwdNormal * impactDist;
	raycastResult.m_impactNormal = (lineSegEnd - lineSegStart).GetRotated90Degrees().GetNormalized();
	if (DotProduct2D(fwdNormal, raycastResult.m_impactNormal) > 0.f)
	{
		raycastResult.m_impactNormal *= -1.f;
	}

	return raycastResult;
}

RaycastResult2D RaycastVsAABB2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, AABB2 const& box)
{
	RaycastResult2D raycastResult;
	raycastResult.m_ray.m_startPos = startPos;
	raycastResult.m_ray.m_fwdNormal = fwdNormal;
	raycastResult.m_ray.m_maxLength = maxDist;

	float deltaX = fwdNormal.x * maxDist;
	float deltaY = fwdNormal.y * maxDist;

	float tX = -1.f;
	// x Axis
	if (startPos.x <= box.m_mins.x)
	{
		tX = box.m_mins.x - startPos.x;
		if (tX >= deltaX)
		{
			return raycastResult;
		}
		tX /= deltaX;
	}
	else if (startPos.x >= box.m_maxs.x)
	{
		tX = box.m_maxs.x - startPos.x;
		if (tX <= deltaX)
		{
			return raycastResult;
		}
		tX /= deltaX;
	}

	float tY = -1.f;
	// y Axis
	if (startPos.y <= box.m_mins.y)
	{
		tY = box.m_mins.y - startPos.y;
		if (tY >= deltaY)
		{
			return raycastResult;
		}
		tY /= deltaY;
	}
	else if (startPos.y >= box.m_maxs.y)
	{
		tY = box.m_maxs.y - startPos.y;
		if (tY <= deltaY)
		{
			return raycastResult;
		}
		tY /= deltaY;
	}

	if (tX < 0.f && tY < 0.f)
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = startPos;
		raycastResult.m_impactNormal = -fwdNormal;
		return raycastResult;
	}

	if (tX > tY)
	{
		float y = startPos.y + deltaY * tX;
		if (y <= box.m_mins.y || y >= box.m_maxs.y)
		{
			return raycastResult;
		}
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = tX * maxDist;
		raycastResult.m_impactNormal = Vec2((deltaX > 0.f)? -1.f : 1.f, 0.f);
		raycastResult.m_impactPos = startPos + fwdNormal * raycastResult.m_impactDist;
		return raycastResult;
	}
	else
	{
		float x = startPos.x + deltaX * tY;
		if (x <= box.m_mins.x || x >= box.m_maxs.x)
		{
			return raycastResult;
		}
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = tY * maxDist;
		raycastResult.m_impactNormal = Vec2(0.f, (deltaY > 0.f) ? -1.f : 1.f);
		raycastResult.m_impactPos = startPos + fwdNormal * raycastResult.m_impactDist;
		return raycastResult;
	}
}

RaycastResult3D RaycastVsAABB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, AABB3 box)
{
	RaycastResult3D raycastResult;
	raycastResult.m_rayStartPos = rayStart;
	raycastResult.m_rayFwdNormal = rayForwardNormal;
	raycastResult.m_rayLength = rayLength;

	float deltaX = rayForwardNormal.x * rayLength;
	float deltaY = rayForwardNormal.y * rayLength;
	float deltaZ = rayForwardNormal.z * rayLength;

	float xNormal = 0.f;
	float yNormal = 0.f;
	float zNormal = 0.f;

	float tX = -1.f;
	// x Axis
	if (rayStart.x <= box.m_mins.x)
	{
		float dX = box.m_mins.x - rayStart.x;
		if (dX >= deltaX)
		{
			return raycastResult;
		}
		tX = dX / deltaX;
		xNormal = -1.f;
	}
	else if (rayStart.x >= box.m_maxs.x)
	{
		float dX = box.m_maxs.x - rayStart.x;
		if (dX <= deltaX)
		{
			return raycastResult;
		}
		tX = dX / deltaX;
		xNormal = 1.f;
	}

	float tY = -1.f;
	// y Axis
	if (rayStart.y <= box.m_mins.y)
	{
		float dY = box.m_mins.y - rayStart.y;
		if (dY >= deltaY)
		{
			return raycastResult;
		}
		tY = dY / deltaY;
		yNormal = -1.f;
	}
	else if (rayStart.y >= box.m_maxs.y)
	{
		float dY = box.m_maxs.y - rayStart.y;
		if (dY <= deltaY)
		{
			return raycastResult;
		}
		tY = dY / deltaY;
		yNormal = 1.f;
	}

	float tZ = -1.f;
	// z Axis
	if (rayStart.z <= box.m_mins.z)
	{
		float dZ = box.m_mins.z - rayStart.z;
		if (dZ >= deltaZ)
		{
			return raycastResult;
		}
		tZ = dZ / deltaZ;
		zNormal = -1.f;
	}
	else if (rayStart.z >= box.m_maxs.z)
	{
		float dZ = box.m_maxs.z - rayStart.z;
		if (dZ <= deltaZ)
		{
			return raycastResult;
		}
		tZ = dZ / deltaZ;
		zNormal = 1.f;
	}

	// RayStart Inside Box
	if (tX < 0.f && tY < 0.f && tZ < 0.f)
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactNormal = -rayForwardNormal;
		return raycastResult;
	}

	// Maximum of time => enter/impact
	int maxTimeAxisDirection = 0; // x axis
	float tImpact = tX;
	if (tY > tImpact)
	{
		maxTimeAxisDirection = 1; // y axis
		tImpact = tY;
	}
	if (tZ > tImpact)
	{
		maxTimeAxisDirection = 2; // z axis
		tImpact = tZ;
	}

	if (maxTimeAxisDirection == 0) // yz plane
	{
		float y = rayStart.y + deltaY * tImpact;
		if (y <= box.m_mins.y || y >= box.m_maxs.y)
		{
			return raycastResult;
		}
		float z = rayStart.z + deltaZ * tImpact;
		if (z <= box.m_mins.z || z >= box.m_maxs.z)
		{
			return raycastResult;
		}
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = tImpact * rayLength;
		raycastResult.m_impactNormal = Vec3(xNormal, 0.f, 0.f);
		raycastResult.m_impactPos = rayStart + rayForwardNormal * raycastResult.m_impactDist;
	} 
	else if (maxTimeAxisDirection == 1) // xz plane
	{
		float x = rayStart.x + deltaX * tImpact;
		if (x <= box.m_mins.x || x >= box.m_maxs.x)
		{
			return raycastResult;
		}
		float z = rayStart.z + deltaZ * tImpact;
		if (z <= box.m_mins.z || z >= box.m_maxs.z)
		{
			return raycastResult;
		}
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = tImpact * rayLength;
		raycastResult.m_impactNormal = Vec3(0.f, yNormal, 0.f);
		raycastResult.m_impactPos = rayStart + rayForwardNormal * raycastResult.m_impactDist;
	}
	else if (maxTimeAxisDirection == 2) // xy plane
	{
		float x = rayStart.x + deltaX * tImpact;
		if (x <= box.m_mins.x || x >= box.m_maxs.x)
		{
			return raycastResult;
		}
		float y = rayStart.y + deltaY * tImpact;
		if (y <= box.m_mins.y || y >= box.m_maxs.y)
		{
			return raycastResult;
		}
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = tImpact * rayLength;
		raycastResult.m_impactNormal = Vec3(0.f, 0.f, zNormal);
		raycastResult.m_impactPos = rayStart + rayForwardNormal * raycastResult.m_impactDist;
	}

	return raycastResult;
}

RaycastResult3D RaycastVsSphere3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 sphereCenter, float sphereRadius)
{
	RaycastResult3D raycastResult;
	raycastResult.m_rayStartPos = rayStart;
	raycastResult.m_rayFwdNormal = rayForwardNormal;
	raycastResult.m_rayLength = rayLength;

	Vec3 startToCenter = sphereCenter - rayStart;
	 
	float startToCenterIProjSignedLength = DotProduct3D(rayForwardNormal, startToCenter);
	Vec3 startToCenterIProj = startToCenterIProjSignedLength * rayForwardNormal;
	Vec3 startToCenterJProj = startToCenter - startToCenterIProj;

	// Case 1: Altitude too large
	float startToCenterJProjLengthSquared = startToCenterJProj.GetLengthSquared();
	if (startToCenterJProjLengthSquared >= sphereRadius * sphereRadius)
	{
		return raycastResult;
	}

	// Case 2: Too far from startPos and endPos
	if (startToCenterIProjSignedLength >= sphereRadius + rayLength || startToCenterIProjSignedLength <= -sphereRadius)
	{
		return raycastResult;
	}

	// Case 3: startPos inside Sphere
	if (startToCenter.GetLengthSquared() < sphereRadius * sphereRadius)
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactNormal = -rayForwardNormal;
		return raycastResult;
	}

	// Case 4: Miss at end
	float adjust = sqrtf(sphereRadius * sphereRadius - startToCenterJProjLengthSquared);
	float impactDist = startToCenterIProjSignedLength - adjust;
	if (impactDist >= rayLength) // <= 0.f never happen!
	{
		return raycastResult;
	}

	// Case 5: Hit
	raycastResult.m_didImpact = true;
	raycastResult.m_impactDist = impactDist;
	raycastResult.m_impactPos = rayStart + rayForwardNormal * impactDist;
	raycastResult.m_impactNormal = (raycastResult.m_impactPos - sphereCenter).GetNormalized();
	return raycastResult;
}

RaycastResult3D RaycastVsCylinderZ3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec2 const& centerXY, FloatRange const& minMaxZ, float radiusXY)
{
	RaycastResult3D raycastResult;
	raycastResult.m_rayStartPos = rayStart;
	raycastResult.m_rayFwdNormal = rayForwardNormal;
	raycastResult.m_rayLength = rayLength;

	// Early Out 1: check if inside cylinder
	if (IsPointInsideCylinderZ3D(rayStart, centerXY, minMaxZ, radiusXY))
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactNormal = -rayForwardNormal;
		return raycastResult;
	}

	//-----------------------------------------------------------------------------------------------
	float deltaZ = rayForwardNormal.z* rayLength;
	float tTop = (minMaxZ.m_max - rayStart.z) / deltaZ;
	float tBottom = (minMaxZ.m_min - rayStart.z) / deltaZ;

	if (std::isnan(tTop) || std::isnan(tBottom))
	{
		return raycastResult;
	}

	float tMinZ = tBottom;
	float tMaxZ = tTop;

	if (tTop < tBottom)
	{
		tMinZ = tTop;
		tMaxZ = tBottom;
	}

	// Early Out 2: 0~1 not overlap with tZ range
	if (tMinZ >= 1.f || tMaxZ <= 0.f)
	{
		return raycastResult;
	}

	//-----------------------------------------------------------------------------------------------
	float tMinXY = -1.f;
	float tMaxXY = -1.f;
	Vec2 rayStart2D = Vec2(rayStart.x, rayStart.y);

	if (rayForwardNormal.x == 0.f && rayForwardNormal.y == 0.f)
	{
		if (!IsPointInsideDisc2D(rayStart2D, centerXY, radiusXY))
		{
			return raycastResult;
		}
		tMinXY = -2.f;
		tMaxXY = 2.f;
	}
	else
	{
		Vec2 rayFwdNormal2D = Vec2(rayForwardNormal.x, rayForwardNormal.y).GetNormalized();
		Vec2 iBasisNormal = rayFwdNormal2D;
		Vec2 jBasisNormal = rayFwdNormal2D.GetRotated90Degrees();
		Vec2 startToCenter = centerXY - rayStart2D;
		float startToCenterIProj = DotProduct2D(startToCenter, iBasisNormal);
		float startToCenterJProj = DotProduct2D(startToCenter, jBasisNormal);

		float adjustSquared = radiusXY * radiusXY - startToCenterJProj * startToCenterJProj;
		if (adjustSquared < 0.f)
		{
			return raycastResult;
		}

		float adjust = sqrtf(radiusXY * radiusXY - startToCenterJProj * startToCenterJProj);

		//float adjust = sqrtf(radiusXY * radiusXY - startToCenterJProj * startToCenterJProj);
		//if (std::isnan(adjust))
		//{
		//	return raycastResult;
		//}

		float maxLenXY = rayLength * (rayForwardNormal.x * rayFwdNormal2D.x + rayForwardNormal.y * rayFwdNormal2D.y); // rayLength * cosine
		tMinXY = (startToCenterIProj - adjust) / maxLenXY;
		tMaxXY = (startToCenterIProj + adjust) / maxLenXY;
	}


	// (0,1) (tMinZ, tMaxZ) (tMinXY, tMaxXY) get tEnter = Max(mins), tExit = Min(maxs) and tEnter < tExit, tEnter is the hit time.

	int hitType = 0; // 0 is Top/Bottom, 1 is Side, 2 is inside
	float tEnter = tMinZ;
	float tExit = tMaxZ;

	if (tMinXY > tEnter)
	{
		tEnter = tMinXY;
		hitType = 1;
	}
	if (0.f > tEnter)
	{
		tEnter = 0.f;
		hitType = 2;
	}

	if (tMaxXY < tExit)
	{
		tExit = tMaxXY;
	}
	if (1.f < tExit)
	{
		tExit = 1.f;
	}

	// Miss
	if (tEnter >= tExit)
	{
		return raycastResult;
	}

	// Hit
	if (hitType == 0)
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = tEnter * rayLength;
		raycastResult.m_impactNormal = Vec3(0.f, 0.f, (rayForwardNormal.z > 0.f)? -1.f: 1.f);
		raycastResult.m_impactPos = rayStart + rayForwardNormal * raycastResult.m_impactDist;
	}
	else if (hitType == 1)
	{
		raycastResult.m_didImpact = true;
		raycastResult.m_impactDist = tEnter * rayLength;
		raycastResult.m_impactPos = rayStart + rayForwardNormal * raycastResult.m_impactDist;
		Vec2 normalXY = Vec2(raycastResult.m_impactPos.x - centerXY.x, raycastResult.m_impactPos.y - centerXY.y).GetNormalized();
		raycastResult.m_impactNormal = Vec3(normalXY.x, normalXY.y, 0.f);
	}
	else
	{
		// Inside dist = 0
		raycastResult.m_didImpact = true;
		raycastResult.m_impactPos = rayStart;
		raycastResult.m_impactNormal = -rayForwardNormal;
	}
	return raycastResult;
}

RaycastResult3D RaycastVsPlane3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Plane3 const& plane)
{
	RaycastResult3D raycastResult;
	raycastResult.m_rayStartPos = rayStart;
	raycastResult.m_rayFwdNormal = rayForwardNormal;
	raycastResult.m_rayLength = rayLength;

	Vec3 rayEnd = rayStart + rayLength * rayForwardNormal;
	// Step 1 Check If straddling
	if (plane.GetSignedDistanceToPoint(rayStart) * plane.GetSignedDistanceToPoint(rayEnd) >= 0.f)
	{
		return raycastResult;
	}
	// Hit
	raycastResult.m_didImpact = true;
	raycastResult.m_impactDist = -plane.GetSignedDistanceToPoint(rayStart) / DotProduct3D(rayForwardNormal, plane.m_normal);
	raycastResult.m_impactPos = rayStart + rayForwardNormal * raycastResult.m_impactDist;
	raycastResult.m_impactNormal = plane.IsPointInFrontOf(rayStart) ? plane.m_normal : -plane.m_normal;

	return raycastResult;
}

RaycastResult3D RaycastVsOBB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, OBB3 const& orientedBox)
{
	RaycastResult3D raycastResult;
	raycastResult.m_rayStartPos = rayStart;
	raycastResult.m_rayFwdNormal = rayForwardNormal;
	raycastResult.m_rayLength = rayLength;

	AABB3 localBox = AABB3(-orientedBox.m_halfDimensions, orientedBox.m_halfDimensions);
	Mat44 localToWorld = orientedBox.GetLocalToWorldTransform();
	Mat44 worldToLocal = localToWorld.GetOrthonormalInverse();

	Vec3 localRayStart = worldToLocal.TransformPosition3D(rayStart);
	Vec3 localRayForwardNormal = worldToLocal.TransformVectorQuantity3D(rayForwardNormal);

	RaycastResult3D localRaycastResult = RaycastVsAABB3D(localRayStart, localRayForwardNormal, rayLength, localBox);

	if (!localRaycastResult.m_didImpact)
	{
		return raycastResult;
	}

	raycastResult.m_didImpact = true;
	raycastResult.m_impactDist = localRaycastResult.m_impactDist;
	raycastResult.m_impactPos = rayStart + rayForwardNormal * raycastResult.m_impactDist;
	raycastResult.m_impactNormal = localToWorld.TransformVectorQuantity3D(localRaycastResult.m_impactNormal);

	return raycastResult;
}

