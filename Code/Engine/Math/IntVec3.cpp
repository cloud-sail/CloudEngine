#include "Engine/Math/IntVec3.hpp"
#include <cmath>

IntVec3::IntVec3(IntVec3 const& copyFrom)
	: x(copyFrom.x)
	, y(copyFrom.y)
	, z(copyFrom.z)
{

}

void IntVec3::operator=(IntVec3 const& copyFrom)
{
	x = copyFrom.x;
	y = copyFrom.y;
	z = copyFrom.z;
}

IntVec3::IntVec3(int initialX, int initialY, int initialZ)
	: x(initialX)
	, y(initialY)
	, z(initialZ)
{

}

float IntVec3::GetLength() const
{
	return sqrtf(static_cast<float>(x * x + y * y + z * z));
}

int IntVec3::GetLengthSquared() const
{
	return (x * x + y * y + z * z);
}

int IntVec3::GetTaxicabLength() const
{
	return abs(x) + abs(y) + abs(z);
}



