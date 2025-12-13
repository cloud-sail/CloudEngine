#pragma once
struct Vec4
{
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 0.f;

public:
	Vec4() {}
	Vec4(Vec4 const& copyFrom);
	explicit Vec4(float initialX, float initialY, float initialZ, float initialW);

	// Operators (const)
	bool		operator==(Vec4 const& compare) const;		// Vec4 == Vec4
	bool		operator!=(Vec4 const& compare) const;		// Vec4 != Vec4
	Vec4 const	operator+(Vec4 const& vecToAdd) const;		// Vec4 + Vec4
	Vec4 const	operator-(Vec4 const& vecToSubtract) const;	// Vec4 - Vec4
	Vec4 const	operator-() const;								// -Vec4, i.e. "unary negation"
	Vec4 const	operator*(float uniformScale) const;			// Vec4 * float
	Vec4 const	operator*(Vec4 const& vecToMultiply) const;	// Vec4 * Vec4
	Vec4 const	operator/(float inverseScale) const;			// Vec4 / float

	// Operators (self-mutating / non-const)
	void		operator+=(Vec4 const& vecToAdd);				// Vec4 += Vec4
	void		operator-=(Vec4 const& vecToSubtract);		// Vec4 -= Vec4
	void		operator*=(const float uniformScale);			// Vec4 *= float
	void		operator/=(const float uniformDivisor);		// Vec4 /= float
	void		operator=(Vec4 const& copyFrom);				// Vec4 = Vec4
};
