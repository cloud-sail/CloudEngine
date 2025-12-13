#pragma once
#include "Engine/Math/Plane3.hpp"

//https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
struct Frustum
{
public:
	Plane3 m_topFace;
	Plane3 m_bottomFace;
	
	Plane3 m_leftFace;
	Plane3 m_rightFace;

	Plane3 m_farFace;
	Plane3 m_nearFace;
};

