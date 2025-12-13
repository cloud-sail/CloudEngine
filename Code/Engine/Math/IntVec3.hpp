#pragma once
struct IntVec3
{
public:
	int x = 0;
	int y = 0;
	int z = 0;

public:
	~IntVec3() = default;	// destructor
	IntVec3() = default;	// constructor
	IntVec3(IntVec3 const& copyFrom); // copy constructor
	void operator=(IntVec3 const& copyFrom); // copy assignment operator
	explicit IntVec3(int initialX, int initialY, int initialZ);


	// Accessors (const methods)
	float	GetLength() const;
	int		GetLengthSquared() const;
	int		GetTaxicabLength() const;


	IntVec3 const operator+(IntVec3 const& vecToAdd) const;
	IntVec3 const operator-(IntVec3 const& vecToSubtract) const;


	// Operators (const)
	bool operator==(IntVec3 const& compare) const;
	bool operator!=(IntVec3 const& compare) const;

	IntVec3 const operator<<(int shift) const;
	IntVec3 const operator>>(int shift) const;

	IntVec3 const operator*(int uniformScale) const;
};


inline IntVec3 const IntVec3::operator<<(int shift) const
{
	return IntVec3(x << shift, y << shift, z << shift);
}

inline IntVec3 const IntVec3::operator>>(int shift) const
{
	return IntVec3(x >> shift, y >> shift, z >> shift);
}

inline IntVec3 const IntVec3::operator+(IntVec3 const& vecToAdd) const
{
	return IntVec3(x + vecToAdd.x, y + vecToAdd.y, z + vecToAdd.z);
}

inline IntVec3 const IntVec3::operator-(IntVec3 const& vecToSubtract) const
{
	return IntVec3(x - vecToSubtract.x, y - vecToSubtract.y, z - vecToSubtract.z);
}

inline bool IntVec3::operator!=(IntVec3 const& compare) const
{
	return !(*this == compare);
}

inline bool IntVec3::operator==(IntVec3 const& compare) const
{
	return x == compare.x && y == compare.y && z == compare.z;
}

inline  IntVec3 const IntVec3::operator*(int uniformScale) const
{
	return IntVec3(x * uniformScale, y * uniformScale, z * uniformScale);
}
