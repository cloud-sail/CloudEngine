#pragma once

//-----------------------------------------------------------------------------------------------
struct Vec2;

//-----------------------------------------------------------------------------------------------
struct Vec3
{
public: // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

public:
	static const Vec3 ZERO;
	static const Vec3 FORWARD;
	static const Vec3 BACKWARD;
	static const Vec3 LEFT;
	static const Vec3 RIGHT;
	static const Vec3 UP;
	static const Vec3 DOWN;
	static const Vec3 XAXIS;
	static const Vec3 YAXIS;
	static const Vec3 ZAXIS;

public:
	// Construction/Destruction
	~Vec3() {}												// destructor (do nothing)
	Vec3() = default;										// default constructor (do nothing)
	Vec3(Vec3 const& copyFrom);								// copy constructor (from another vec3)
	explicit Vec3(const Vec2& vec2, float initialZ = 0.f);
	explicit Vec3(float initialX, float initialY, float initialZ);			// explicit constructor (from x, y, z)

	// Static methods (e.g. creation functions, like a generator/factory)
	static Vec3 const MakeFromPolarRadians(float pitchRadians, float yawRadians, float length = 1.f);
	static Vec3 const MakeFromPolarDegrees(float pitchDegrees, float yawDegrees, float length = 1.f);

	// Accessors (const methods)
	float		GetLength() const;
	float		GetLengthXY() const;
	float		GetLengthSquared() const;
	float		GetLengthXYSquared() const;
	float		GetAngleAboutZRadians() const;
	float		GetAngleAboutZDegrees() const;
	Vec3 const	GetRotatedAboutZRadians(float deltaRadians) const;
	Vec3 const	GetRotatedAboutZDegrees(float deltaDegrees) const;
	Vec3 const	GetClamped(float maxLength) const;
	Vec3 const	GetNormalized() const;

	// Mutators
	void		ClampLength(float maxLength);
	void		SetFromText(char const* text);

	// Operators (const)
	bool		operator==(Vec3 const& compare) const;		// vec3 == vec3
	bool		operator!=(Vec3 const& compare) const;		// vec3 != vec3
	Vec3 const	operator+(Vec3 const& vecToAdd) const;		// vec3 + vec3
	Vec3 const	operator-(Vec3 const& vecToSubtract) const;	// vec3 - vec3
	Vec3 const	operator-() const;							// -vec3, i.e. "unary negation"
	Vec3 const	operator*(float uniformScale) const;		// vec3 * float
	Vec3 const	operator*(Vec3 const& vecToMultiply) const;	// vec3 * vec3
	Vec3 const	operator/(float inverseScale) const;		// vec3 / float

	// Operators (self-mutating / non-const)
	void		operator+=(Vec3 const& vecToAdd);			// vec3 += vec3
	void		operator-=(Vec3 const& vecToSubtract);		// vec3 -= vec3
	void		operator*=(const float uniformScale);		// vec3 *= float
	void		operator/=(const float uniformDivisor);		// vec3 /= float
	void		operator=(Vec3 const& copyFrom);			// vec3 = vec3

	// Standalone "friend" functions that are conceptually, but not actually, part of Vec3::
	friend Vec3 const operator*(float uniformScale, Vec3 const& vecToScale);	// float * vec3
};


