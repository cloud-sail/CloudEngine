#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

//-----------------------------------------------------------------------------------------------
struct FloatRange;
struct LineSegment2;
struct AABB2;
struct AABB3;
struct OBB3;
struct Plane3;

//-----------------------------------------------------------------------------------------------
struct Ray2
{
	Ray2() = default;
	Ray2(Vec2 const& startPos, Vec2 const& endPos);
	Ray2(Vec2 const& startPos, Vec2 const& fwdNormal, float maxLength);

	Vec2 m_startPos;
	Vec2 m_fwdNormal;
	float m_maxLength = 1.f;
};


//-----------------------------------------------------------------------------------------------
struct RaycastResult2D
{
	// Basic raycast result information 
	bool m_didImpact = false;
	float m_impactDist = 0.f;
	Vec2 m_impactPos;
	Vec2 m_impactNormal;

	// Original raycast information
	Ray2 m_ray;
};

//-----------------------------------------------------------------------------------------------

RaycastResult2D RaycastVsDisc2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, Vec2 const& discCenter, float discRadius);
RaycastResult2D RaycastVsLineSegment2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, LineSegment2 const& lineSegment); 
RaycastResult2D RaycastVsLineSegment2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, Vec2 const& lineSegStart, Vec2 const& lineSegEnd);
RaycastResult2D RaycastVsAABB2D(Vec2 const& startPos, Vec2 const& fwdNormal, float maxDist, AABB2 const& box);


//-----------------------------------------------------------------------------------------------
struct RaycastResult3D
{
	Vec3	m_rayStartPos;
	Vec3	m_rayFwdNormal;
	float	m_rayLength = 1.f;

	bool	m_didImpact = false;
	float	m_impactDist = 0.0f;
	Vec3	m_impactPos;
	Vec3	m_impactNormal;
};

//-----------------------------------------------------------------------------------------------
RaycastResult3D RaycastVsAABB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, AABB3 box);
RaycastResult3D RaycastVsSphere3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec3 sphereCenter, float sphereRadius);
RaycastResult3D RaycastVsCylinderZ3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Vec2 const& centerXY, FloatRange const& minMaxZ, float radiusXY);
RaycastResult3D RaycastVsPlane3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, Plane3 const& plane);
RaycastResult3D RaycastVsOBB3D(Vec3 rayStart, Vec3 rayForwardNormal, float rayLength, OBB3 const& orientedBox);

