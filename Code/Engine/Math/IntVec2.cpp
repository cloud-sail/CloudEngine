#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <math.h>



//-----------------------------------------------------------------------------------------------
const IntVec2 IntVec2::ZERO = IntVec2();

//-----------------------------------------------------------------------------------------------
IntVec2::IntVec2(IntVec2 const& copyFrom)
	: x(copyFrom.x)
	, y(copyFrom.y)
{
}

IntVec2::IntVec2(int initialX, int initialY)
	: x(initialX)
	, y(initialY)
{
}

float IntVec2::GetLength() const
{
	return sqrtf(static_cast<float>(x * x + y * y));
}

int IntVec2::GetTaxicabLength() const
{
	return abs(x) + abs(y);
}

int IntVec2::GetLengthSquared() const
{
	return (x * x + y * y);
}

float IntVec2::GetOrientationRadians() const
{
	return Atan2Radians(static_cast<float>(y), static_cast<float>(x));
}

float IntVec2::GetOrientationDegrees() const
{
	return Atan2Degrees(static_cast<float>(y), static_cast<float>(x));
}

IntVec2 const IntVec2::GetRotated90Degrees() const
{
	return IntVec2(-y, x);
}

IntVec2 const IntVec2::GetRotatedMinus90Degrees() const
{
	return IntVec2(y, -x);
}

void IntVec2::SetFromText(char const* text)
{
	Strings tokens = SplitStringOnDelimiter(text, ',');
	if (tokens.size() != 2)
	{
		ERROR_AND_DIE(Stringf("Wrong text format for IntVec2! text: %s", text));
	}
	x = atoi(tokens[0].c_str());
	y = atoi(tokens[1].c_str());
}

void IntVec2::Rotate90Degrees()
{
	IntVec2 result = GetRotated90Degrees();
	x = result.x;
	y = result.y;
}

void IntVec2::RotateMinus90Degrees()
{
	IntVec2 result = GetRotatedMinus90Degrees();
	x = result.x;
	y = result.y;
}

IntVec2 const IntVec2::operator+(IntVec2 const& vecToAdd) const
{
	return IntVec2(x + vecToAdd.x, y + vecToAdd.y);;
}

IntVec2 const IntVec2::operator-(IntVec2 const& vecToSubtract) const
{
	return IntVec2(x - vecToSubtract.x, y - vecToSubtract.y);
}

void IntVec2::operator=(IntVec2 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
}

bool IntVec2::operator==(IntVec2 const& compare) const
{
	return x == compare.x && y == compare.y;
}

bool IntVec2::operator!=(IntVec2 const& compare) const
{
	return !(*this == compare);
}
