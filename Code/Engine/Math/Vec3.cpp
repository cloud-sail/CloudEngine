#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

//-----------------------------------------------------------------------------------------------
const Vec3 Vec3::ZERO		= Vec3(0.f, 0.f, 0.f);
const Vec3 Vec3::FORWARD	= Vec3(1.f, 0.f, 0.f);
const Vec3 Vec3::BACKWARD	= Vec3(-1.f, 0.f, 0.f);
const Vec3 Vec3::LEFT		= Vec3(0.f, 1.f, 0.f);
const Vec3 Vec3::RIGHT		= Vec3(0.f, -1.f, 0.f);
const Vec3 Vec3::UP			= Vec3(0.f, 0.f, 1.f);
const Vec3 Vec3::DOWN		= Vec3(0.f, 0.f, -1.f);
const Vec3 Vec3::XAXIS			= Vec3(1.f, 0.f, 0.f);
const Vec3 Vec3::YAXIS			= Vec3(0.f, 1.f, 0.f);
const Vec3 Vec3::ZAXIS			= Vec3(0.f, 0.f, 1.f);


//-----------------------------------------------------------------------------------------------
Vec3::Vec3(Vec3 const& copy)
	: x(copy.x)
	, y(copy.y)
	, z(copy.z)
{
}

//-----------------------------------------------------------------------------------------------
Vec3::Vec3(float initialX, float initialY, float initialZ)
	: x(initialX)
	, y(initialY)
	, z(initialZ)
{
}

Vec3::Vec3(const Vec2& vec2, float initialZ /*= 0.f*/)
	: x(vec2.x)
	, y(vec2.y)
	, z(initialZ)
{
}

STATIC Vec3 const Vec3::MakeFromPolarRadians(float pitchRadians, float yawRadians, float length /*= 1.f*/)
{
	Vec3 result;
	result.x = CosRadians(pitchRadians) * CosRadians(yawRadians);
	result.y = CosRadians(pitchRadians) * SinRadians(yawRadians);
	result.z = -SinRadians(pitchRadians);
	return result * length;
}

STATIC Vec3 const Vec3::MakeFromPolarDegrees(float pitchDegrees, float yawDegrees, float length /*= 1.f*/)
{
	Vec3 result;
	result.x = CosDegrees(pitchDegrees) * CosDegrees(yawDegrees);
	result.y = CosDegrees(pitchDegrees) * SinDegrees(yawDegrees);
	result.z = -SinDegrees(pitchDegrees);
	return result * length;
}

float Vec3::GetLength() const
{
	return GetDistance3D(*this, Vec3(0.f, 0.f, 0.f));
}

float Vec3::GetLengthXY() const
{
	return GetDistanceXY3D(*this, Vec3(0.f, 0.f, 0.f));
}

float Vec3::GetLengthSquared() const
{
	return GetDistanceSquared3D(*this, Vec3(0.f, 0.f, 0.f));
}

float Vec3::GetLengthXYSquared() const
{
	return GetDistanceXYSquared3D(*this, Vec3(0.f, 0.f, 0.f));
}

float Vec3::GetAngleAboutZRadians() const
{
	return Atan2Radians(y, x);
}

float Vec3::GetAngleAboutZDegrees() const
{
	return Atan2Degrees(y, x);
}

Vec3 const Vec3::GetRotatedAboutZRadians(float deltaRadians) const
{
	// matrix method
	float cosDelta = CosRadians(deltaRadians);
	float sinDelta = SinRadians(deltaRadians);
	return Vec3(cosDelta * x - sinDelta * y, sinDelta * x + cosDelta * y, z);
	// polar cartesian method TODO

}

Vec3 const Vec3::GetRotatedAboutZDegrees(float deltaDegrees) const
{
	// matrix method
	float cosDelta = CosDegrees(deltaDegrees);
	float sinDelta = SinDegrees(deltaDegrees);
	return Vec3(cosDelta * x - sinDelta * y, sinDelta * x + cosDelta * y, z);
	// polar cartesian method TODO

}

Vec3 const Vec3::GetClamped(float maxLength) const
{
	float length = GetLength();
	if (length <= maxLength)
	{
		return *this;
	}
	float uniformScale = maxLength / length;
	return (*this) * uniformScale;
}

Vec3 const Vec3::GetNormalized() const
{
	float squaredLength = GetLengthSquared();
	if (squaredLength == 1.f)
	{
		return *this;
	}
	if (squaredLength == 0.f)
	{
		return ZERO;
	}
	float uniformScale = 1.f / sqrtf(squaredLength);
	return (*this) * uniformScale;
}

//-----------------------------------------------------------------------------------------------
void Vec3::ClampLength(float maxLength)
{
	Vec3 result = GetClamped(maxLength);
	x = result.x;
	y = result.y;
	z = result.z;
}

void Vec3::SetFromText(char const* text)
{
	Strings tokens = SplitStringOnDelimiter(text, ',');
	if (tokens.size() != 3)
	{
		ERROR_AND_DIE(Stringf("Wrong text format for Vec3! text: %s", text));
	}
	x = static_cast<float>(atof(tokens[0].c_str()));
	y = static_cast<float>(atof(tokens[1].c_str()));
	z = static_cast<float>(atof(tokens[2].c_str()));
}

//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator + (Vec3 const& vecToAdd) const
{
	return Vec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}


//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator-(Vec3 const& vecToSubtract) const
{
	return Vec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}


//------------------------------------------------------------------------------------------------
Vec3 const Vec3::operator-() const
{
	return Vec3(-x, -y, -z);
}


//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator*(float uniformScale) const
{
	return Vec3(x * uniformScale, y * uniformScale, z * uniformScale);
}


//------------------------------------------------------------------------------------------------
Vec3 const Vec3::operator*(Vec3 const& vecToMultiply) const
{
	return Vec3(x * vecToMultiply.x, y * vecToMultiply.y, z * vecToMultiply.z);
}


//-----------------------------------------------------------------------------------------------
Vec3 const Vec3::operator/(float inverseScale) const
{
	float uniformScale = 1.f / inverseScale;
	return Vec3(x * uniformScale, y * uniformScale, z * uniformScale);
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator+=(Vec3 const& vecToAdd)
{
	x += vecToAdd.x;
	y += vecToAdd.y;
	z += vecToAdd.z;
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator-=(Vec3 const& vecToSubtract)
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
	z -= vecToSubtract.z;
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator*=(const float uniformScale)
{
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator/=(const float uniformDivisor)
{
	float uniformScale = 1.f / uniformDivisor;
	x *= uniformScale;
	y *= uniformScale;
	z *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec3::operator=(Vec3 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}


//-----------------------------------------------------------------------------------------------
Vec3 const operator*(float uniformScale, Vec3 const& vecToScale)
{
	return Vec3(uniformScale * vecToScale.x, uniformScale * vecToScale.y, uniformScale * vecToScale.z);
}


//-----------------------------------------------------------------------------------------------
bool Vec3::operator==(Vec3 const& compare) const
{
	return x == compare.x && y == compare.y && z == compare.z;
}


//-----------------------------------------------------------------------------------------------
bool Vec3::operator!=(Vec3 const& compare) const
{
	return !(*this == compare);
}

