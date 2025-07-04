#include "Engine/Math/Quat.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <math.h>

const Quat Quat::IDENTITY(0.f, 0.f, 0.f, 1.f);

Quat::Quat(float initialX, float initialY, float initialZ, float initialW)
	: x(initialX)
	, y(initialY)
	, z(initialZ)
	, w(initialW)
{

}


STATIC Quat const Quat::MakeFromAxisAngleDegrees(Vec3 const& axis, float angleDegrees)
{
	float halfAngleDegrees = 0.5f * angleDegrees;
	Quat result;
	result.w = CosDegrees(halfAngleDegrees);

	float sinHalfDegrees = SinDegrees(halfAngleDegrees);
	result.x = axis.x * sinHalfDegrees;
	result.y = axis.y * sinHalfDegrees;
	result.z = axis.z * sinHalfDegrees;
	return result;
}

STATIC Quat const Quat::MakeFromAxisAngleRadians(Vec3 const& axis, float angleRadians)
{
	float halfAngleRadians = 0.5f * angleRadians;
	Quat result;
	result.w = CosRadians(halfAngleRadians);

	float sinHalfRadians = SinRadians(halfAngleRadians);
	result.x = axis.x * sinHalfRadians;
	result.y = axis.y * sinHalfRadians;
	result.z = axis.z * sinHalfRadians;
	return result;
}

STATIC Quat const Quat::MakeFromEulerAngles(EulerAngles const& eulerRotation)
{
	float halfYaw	= eulerRotation.m_yawDegrees * 0.5f;
	float halfPitch = eulerRotation.m_pitchDegrees * 0.5f;
	float halfRoll	= eulerRotation.m_rollDegrees * 0.5f;

	float SY = SinDegrees(halfYaw);
	float CY = CosDegrees(halfYaw);
	float SP = SinDegrees(halfPitch);
	float CP = CosDegrees(halfPitch);
	float SR = SinDegrees(halfRoll);
	float CR = CosDegrees(halfRoll);

	Quat result;
	result.w = CY * CP * CR + SY * SP * SR;
	result.x = CY * CP * SR - SY * SP * CR;
	result.y = CY * SP * CR + SY * CP * SR;
	result.z = SY * CP * CR - CY * SP * SR;
	
	return result;
}

STATIC Quat const Quat::Lerp(Quat const& q0, Quat const& q1, float t, bool useShortestPath /*= true*/)
{
	float sign = 1.f;
	if (useShortestPath)
	{
		float dotProduct = q0 | q1;
		if (dotProduct < 0.f)
		{
			sign = -1.f;
		}
	}
	float k0 = 1.f - t;
	float k1 = t * sign;

	return k0 * q0 + k1 * q1;
}

STATIC Quat const Quat::Nlerp(Quat const& q0, Quat const& q1, float t, bool useShortestPath /*= true*/)
{
	return Lerp(q0, q1, t, useShortestPath).GetNormalized();
}

STATIC Quat const Quat::Slerp_NotNormalized(Quat const& q0, Quat const& q1, float t)
{
	// not normalized
	// choose the shortest route
	float cosOmega = q0 | q1;
	float sign = (cosOmega >= 0.f) ? 1.f: -1.f; // If cosOmega < 0, q1 *= -1, cosOmega *= -1;
	cosOmega *= sign;

	// Default is Linear interpolation, for small sin(omega)
	float k0 = 1.f - t;
	float k1 = t * sign;

	if (cosOmega < 0.9999f)
	{
		float omega = AcosRadians(cosOmega);
		float oneOverSinOmega = 1.f / SinRadians(omega);
		k0 = SinRadians(k0 * omega) * oneOverSinOmega;
		k1 = SinRadians(k1 * omega) * oneOverSinOmega;
	}

	return k0 * q0 + k1 * q1;
}

STATIC Quat const Quat::SlerpFullPath_NotNormalized(Quat const& q0, Quat const& q1, float t)
{
	float cosOmega = q0 | q1;

	// Default is Linear interpolation, for small sin(omega)
	float k0 = 1.f - t;
	float k1 = t;

	float omega = AcosRadians(cosOmega);
	if (omega > 0.0001f)
	{
		float oneOverSinOmega = 1.f / SinRadians(omega);
		k0 = SinRadians(k0 * omega) * oneOverSinOmega;
		k1 = SinRadians(k1 * omega) * oneOverSinOmega;
	}

	return k0 * q0 + k1 * q1;
}

STATIC Quat const Quat::Slerp(Quat const& q0, Quat const& q1, float t)
{
	return Slerp_NotNormalized(q0, q1, t).GetNormalized();
}


STATIC Quat const Quat::SlerpFullPath(Quat const& q0, Quat const& q1, float t)
{
	return SlerpFullPath_NotNormalized(q0, q1, t).GetNormalized();
}

STATIC Quat const Quat::Squad(Quat const& q0, Quat const& tan0, Quat const& q1, Quat const& tan1, float t)
{
	Quat const qA = Slerp_NotNormalized(q0, q1, t);
	Quat const qB = SlerpFullPath_NotNormalized(tan0, tan1, t);
	
	return SlerpFullPath(qA, qB, 2.f * t * (1.f - t));
}

STATIC Quat const Quat::GetTangent(Quat const& prevQ, Quat const& currQ, Quat const& nextQ)
{
	// Do not know why, just for squad algorithm
	Quat invCurrQ = currQ.GetInverse();
	Quat part1 = (invCurrQ * prevQ).Log();
	Quat part2 = (invCurrQ * nextQ).Log();

	Quat preExp = (part1 + part2) * -0.5f;

	return currQ * preExp.Exp();
}

STATIC Quat const Quat::FindQuatBetweenVectors(Vec3 const& A, Vec3 const& B)
{
	// https://www.euclideanspace.com/maths/algebra/vectors/angleBetween/index.htm
	// Normal way is to calculate axis angle [cos(theta/2) n*sin(theta/2)] calculate theta by acos(dotproduct), n by cross product
	float normAB = sqrtf(A.GetLengthSquared() * B.GetLengthSquared());

	float w = normAB + DotProduct3D(A, B);
	Quat result;

	if (w >= 1e-6f * normAB)
	{
		Vec3 XYZ = CrossProduct3D(A, B);
		result = Quat(XYZ.x, XYZ.y, XYZ.z, w);
	}
	else
	{
		// Opposite Direction
		w = 0.f;
		float x = fabsf(A.x);
		float y = fabsf(A.y);
		float z = fabsf(A.z);

		Vec3 basis = (x > y && x > z) ? Vec3::LEFT : Vec3::FORWARD;

		Vec3 XYZ = CrossProduct3D(A, basis);
		result = Quat(XYZ.x, XYZ.y, XYZ.z, w);
	}

	result.Normalize();
	return result;
}

Quat const Quat::GetInverse() const
{
	return Quat(-x, -y, -z, w);
}

Quat const Quat::GetNormalized() const
{
	float squaredMagnitude = GetLengthSquared();
	constexpr float SMALL_NUMBER = 0.00000001f;
	if (squaredMagnitude >= SMALL_NUMBER)
	{
		float uniformScale = 1.f / sqrtf(squaredMagnitude); // TODO Fast Sqrt Inv?
		return (*this) * uniformScale;
	}
	else
	{
		return Quat::IDENTITY;
	}
}

float Quat::GetLength() const
{
	return sqrtf(x * x + y * y + z * z + w * w);
}

float Quat::GetLengthSquared() const
{
	return x * x + y * y + z * z + w * w;
}

void Quat::GetAsAxisAngleRadians(Vec3& axis, float& angleRadians) const
{
	axis = GetRotationAxis();
	angleRadians = GetAngleRadians();
}

float Quat::GetAngleRadians() const
{
	return 2.f * AcosRadians(w);
}

Vec3 Quat::GetRotationAxis() const
{
	return Vec3(x, y, z).GetNormalized();
}

Quat Quat::Exp() const
{
	// Be used after Log()
	// exp(p) = exp([0, An]) = [cosA n*sinA]
	constexpr float SMALL_NUMBER = 0.00000001f;

	float alpha = sqrtf(x * x + y * y + z * z);
	float sinAlpha = SinRadians(alpha);

	Quat result;

	result.w = CosRadians(alpha);

	if (fabsf(sinAlpha) >= SMALL_NUMBER)
	{
		float uniformScale = sinAlpha / alpha;
		result.x = x * uniformScale;
		result.y = y * uniformScale;
		result.z = z * uniformScale;
	}
	else
	{
		result.x = x;
		result.y = y;
		result.z = z;
	}
	return result;
}

Quat Quat::Log() const
{
	// p = log(q) = log([cosA, n*sinA]) = [0, An]
	constexpr float SMALL_NUMBER = 0.00000001f;
	Quat result;

	result.w = 0.f;
	//
	if (fabsf(w) < 0.9999f)
	{
		float alpha = AcosRadians(w);
		float sinAlpha = SinRadians(alpha);

		if (fabsf(sinAlpha) >= SMALL_NUMBER)
		{
			float uniformScale = alpha / sinAlpha;
			result.x = x * uniformScale;
			result.y = y * uniformScale;
			result.z = z * uniformScale;

			return result;
		}
	}
	result.x = x;
	result.y = y;
	result.z = z;

	return result;
}

void Quat::Invert()
{
	x = -x;
	y = -y;
	z = -z;
}

void Quat::Normalize()
{
	float squaredMagnitude = GetLengthSquared();
	constexpr float SMALL_NUMBER = 0.00000001f;
	if (squaredMagnitude >= SMALL_NUMBER)
	{
		float uniformScale = 1.f / sqrtf(squaredMagnitude); // TODO Fast Sqrt Inv?
		x *= uniformScale;
		y *= uniformScale;
		z *= uniformScale;
		w *= uniformScale;
	}
	else
	{
		*this = Quat::IDENTITY;
	}
}

Vec3 const Quat::RotateVector(Vec3 const& vec) const
{
	// Simple version
	//Quat result = (*this) * Quat(vec.x, vec.y, vec.z, 0.f) * GetInverse();
	//return Vec3(result.x, result.y, result.z);

	// Mathematics for 3D Game Programming and Computer Graphics - CH. 4.6.2
	// q = [s v]
	// q*P*q^-1 = (s2 - v2)P + 2s* vxP + 2(v.Dot(P)) v
	// For this way, not need to be a unity quat
	float s = w;
	Vec3 v = Vec3(x, y, z);
	return (s * s - v.GetLengthSquared()) * vec
		+ 2 * s * CrossProduct3D(v, vec)
		+ 2 * DotProduct3D(v, vec) * v;
}

Quat const Quat::operator+(Quat const& rhs) const
{
	return Quat(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

Quat const Quat::operator-(Quat const& rhs) const
{
	return Quat(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

Quat const Quat::operator*(float rhs) const
{
	return Quat(x * rhs, y * rhs, z * rhs, w * rhs);
}

void Quat::operator*=(float rhs)
{
	x *= rhs;
	y *= rhs;
	z *= rhs;
	w *= rhs;
}

Quat const Quat::operator*(Quat const& rhs) const
{
	// Quaternion Multiplication
	// lhs * rhs
	Quat result;

	Vec3 leftV = Vec3(x, y, z);
	Vec3 rightV = Vec3(rhs.x, rhs.y, rhs.z);

	result.w = w * rhs.w - DotProduct3D(leftV, rightV);

	Vec3 newV = w * rightV + rhs.w * leftV + CrossProduct3D(leftV, rightV);

	result.x = newV.x;
	result.y = newV.y;
	result.z = newV.z;

	return result;
}

float Quat::operator|(Quat const& rhs) const
{
	return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

void Quat::operator-=(Quat const& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;
}

void Quat::operator+=(Quat const& rhs)
{
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;
}

Quat const operator*(float lhs, Quat const& rhs)
{
	return rhs * lhs;
}