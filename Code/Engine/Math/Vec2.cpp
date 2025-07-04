#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <cstdlib>

//-----------------------------------------------------------------------------------------------
Vec2::Vec2( Vec2 const& copy )
	: x(copy.x)
	, y(copy.y)
{
}


//-----------------------------------------------------------------------------------------------
Vec2::Vec2( float initialX, float initialY )
	: x(initialX)
	, y(initialY)
{
}

Vec2::Vec2(IntVec2 const& vec)
	: x(static_cast<float>(vec.x))
	, y(static_cast<float>(vec.y))
{
}

const Vec2 Vec2::RIGHT(1.f, 0.f);
const Vec2 Vec2::ONE(1.f, 1.f);
const Vec2 Vec2::ZERO(0.f, 0.f);

//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::MakeFromPolarRadians(float orientationRadians, float length)
{
	float orientationDegrees = ConvertRadiansToDegrees(orientationRadians);
	return Vec2(length * CosDegrees(orientationDegrees), length * SinDegrees(orientationDegrees));
}

Vec2 const Vec2::MakeFromPolarDegrees(float orientationDegrees, float length)
{
	return Vec2(length * CosDegrees(orientationDegrees), length * SinDegrees(orientationDegrees));
}

float Vec2::GetLength() const
{
	return GetDistance2D(*this, Vec2(0.f, 0.f));
}

float Vec2::GetLengthSquared() const
{
	return GetDistanceSquared2D(*this, Vec2(0.f, 0.f));
}

float Vec2::GetOrientationRadians() const
{
	return Atan2Radians(y, x);
}

float Vec2::GetOrientationDegrees() const
{
	return Atan2Degrees(y, x);
}

Vec2 const Vec2::GetRotated90Degrees() const
{
	return Vec2(-y, x);
}

Vec2 const Vec2::GetRotatedMinus90Degrees() const
{
	return Vec2(y, -x);
}

Vec2 const Vec2::GetRotatedRadians(float deltaRadians) const
{
	// matrix method TESTED
	float cosDelta = CosRadians(deltaRadians);
	float sinDelta = SinRadians(deltaRadians);
	return Vec2(cosDelta * x - sinDelta * y, sinDelta * x + cosDelta * y);
	// 
	// polar Cartesian method TESTED
	//return Vec2::MakeFromPolarRadians(GetOrientationRadians() + deltaRadians, GetLength());

}

Vec2 const Vec2::GetRotatedDegrees(float deltaDegrees) const
{
	// matrix method TESTED
	float cosDelta = CosDegrees(deltaDegrees);
	float sinDelta = SinDegrees(deltaDegrees);
	return Vec2(cosDelta * x - sinDelta * y, sinDelta * x + cosDelta * y);
	// 
	// polar Cartesian method TESTED
	//return Vec2::MakeFromPolarDegrees(GetOrientationDegrees() + deltaDegrees, GetLength());

}

Vec2 const Vec2::GetClamped(float maxLength) const
{
	float length = GetLength();
	if (length <= maxLength)
	{
		return *this;
	}
	float uniformScale = maxLength / length;
	return (*this) * uniformScale;
}

Vec2 const Vec2::GetNormalized() const
{
	float length = GetLength();
	if (length == 0.f)
	{
		return ZERO;
	}
	float uniformScale = 1.f / length;
	return (*this) * uniformScale;
}

Vec2 const Vec2::GetReflected(Vec2 const& unitNormalOfSurfaceToReflectoffof) const
{
	//Vec2 normalComponent = GetProjectedOnto2D(*this, normalOfSurfaceToReflectoffof); // make normal vector normal before use
	Vec2 result = (*this) - 2 * DotProduct2D(unitNormalOfSurfaceToReflectoffof, *this) * unitNormalOfSurfaceToReflectoffof;
	return result;
}

void Vec2::SetFromText(char const* text)
{
	Strings tokens = SplitStringOnDelimiter(text, ',');
	if (tokens.size() != 2)
	{
		ERROR_AND_DIE(Stringf("Wrong text format for Vec2! text: %s", text));
	}
	x = static_cast<float>(atof(tokens[0].c_str()));
	y = static_cast<float>(atof(tokens[1].c_str()));
}

void Vec2::SetOrientationRadians(float newOrientationRadians)
{
	float length = GetLength();
	x = length * CosRadians(newOrientationRadians);
	y = length * SinRadians(newOrientationRadians);
}

void Vec2::SetOrientationDegrees(float newOrientationDegrees)
{
	float length = GetLength();
	x = length * CosDegrees(newOrientationDegrees);
	y = length * SinDegrees(newOrientationDegrees);
}

void Vec2::SetPolarRadians(float newOrientationRadians, float newLength)
{
	x = newLength * CosRadians(newOrientationRadians);
	y = newLength * SinRadians(newOrientationRadians);
}

void Vec2::SetPolarDegrees(float newOrientationDegrees, float newLength)
{
	x = newLength * CosDegrees(newOrientationDegrees);
	y = newLength * SinDegrees(newOrientationDegrees);
}

void Vec2::Rotate90Degrees()
{
	Vec2 result = GetRotated90Degrees();
	x = result.x;
	y = result.y;
}

void Vec2::RotateMinus90Degrees()
{
	Vec2 result = GetRotatedMinus90Degrees();
	x = result.x;
	y = result.y;
}

void Vec2::RotateRadians(float deltaRadians)
{
	Vec2 result = GetRotatedRadians(deltaRadians);
	x = result.x;
	y = result.y;
}

void Vec2::RotateDegrees(float deltaDegrees)
{
	Vec2 result = GetRotatedDegrees(deltaDegrees);
	x = result.x;
	y = result.y;
}

void Vec2::SetLength(float newLength)
{
	float uniformScale = newLength / GetLength();
	x *= uniformScale;
	y *= uniformScale;
}

void Vec2::ClampLength(float maxLength)
{
	Vec2 result = GetClamped(maxLength);
	x = result.x;
	y = result.y;
}

void Vec2::Normalize()
{
	Vec2 result = GetNormalized();
	x = result.x;
	y = result.y;
}

float Vec2::NormalizeAndGetPreviousLength()
{
	float prevLength = GetLength();
	Normalize();
	return prevLength;
}

void Vec2::Reflect(Vec2 const& unitNormalOfSurfaceToReflectoffof)
{
	Vec2 result = (*this) - 2 * DotProduct2D(unitNormalOfSurfaceToReflectoffof, *this) * unitNormalOfSurfaceToReflectoffof;
	x = result.x;
	y = result.y;
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator + ( Vec2 const& vecToAdd ) const
{
	return Vec2(x + vecToAdd.x, y + vecToAdd.y);
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator-( Vec2 const& vecToSubtract ) const
{
	return Vec2(x - vecToSubtract.x, y - vecToSubtract.y);
}


//------------------------------------------------------------------------------------------------
Vec2 const Vec2::operator-() const
{
	return Vec2(-x, -y);
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator*( float uniformScale ) const
{
	return Vec2(x * uniformScale, y * uniformScale);
}


//------------------------------------------------------------------------------------------------
Vec2 const Vec2::operator*( Vec2 const& vecToMultiply ) const
{
	return Vec2(x * vecToMultiply.x, y * vecToMultiply.y);
}


//-----------------------------------------------------------------------------------------------
Vec2 const Vec2::operator/( float inverseScale ) const
{
	// multiplication is faster than division
	float uniformScale = 1.f / inverseScale;
	return Vec2(x * uniformScale, y * uniformScale);
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator+=( Vec2 const& vecToAdd )
{
	x += vecToAdd.x;
	y += vecToAdd.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator-=( Vec2 const& vecToSubtract )
{
	x -= vecToSubtract.x;
	y -= vecToSubtract.y;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator*=( const float uniformScale )
{
	x *= uniformScale;
	y *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator/=( const float uniformDivisor )
{
	float uniformScale = 1.f / uniformDivisor;
	x *= uniformScale;
	y *= uniformScale;
}


//-----------------------------------------------------------------------------------------------
void Vec2::operator=( Vec2 const& copyFrom )
{
	x = copyFrom.x;
	y = copyFrom.y;
}


//-----------------------------------------------------------------------------------------------
Vec2 const operator*( float uniformScale, Vec2 const& vecToScale )
{
	return Vec2(uniformScale * vecToScale.x, uniformScale * vecToScale.y);
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator==( Vec2 const& compare ) const
{
	return x == compare.x && y == compare.y;
}


//-----------------------------------------------------------------------------------------------
bool Vec2::operator!=( Vec2 const& compare ) const
{
	return !(*this == compare);
}

