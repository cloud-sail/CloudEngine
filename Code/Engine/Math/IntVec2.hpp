#pragma once
struct IntVec2
{
public:
	int x = 0;
	int y = 0;

public:
	// Construction/Destruction
	~IntVec2() = default;
	IntVec2() = default;
	IntVec2(IntVec2 const& copyFrom);
	explicit IntVec2(int initialX, int initialY);

	// Accessors (const methods)
	float	GetLength() const;
	int		GetTaxicabLength() const;
	int		GetLengthSquared() const;
	float	GetOrientationRadians() const;
	float	GetOrientationDegrees() const;
	IntVec2 const GetRotated90Degrees() const;
	IntVec2 const GetRotatedMinus90Degrees() const;

	// Mutators (non const methods)
	void SetFromText(char const* text);
	void Rotate90Degrees();
	void RotateMinus90Degrees();

	IntVec2 const operator+(IntVec2 const& vecToAdd) const;
	IntVec2 const operator-(IntVec2 const& vecToSubtract) const;

	// Operator (self-mutating / non-const)
	void operator=(IntVec2 const& copyFrom);

	// Operators (const)
	bool operator==(IntVec2 const& compare) const;
	bool operator!=(IntVec2 const& compare) const;
public:
	static const IntVec2 ZERO;
};

