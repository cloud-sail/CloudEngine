#pragma once


struct Vec3;
struct Mat44;
struct EulerAngles;

struct Quat
{
public:
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float w = 1.f;

public:
	static const Quat IDENTITY;

public:
	~Quat() = default;
	Quat() = default;
	explicit Quat(float initialX, float initialY, float initialZ, float initialW);

	// Static methods (e.g. creation functions, like a generator/factory)
	static Quat const MakeFromAxisAngleDegrees(Vec3 const& axis, float angleDegrees);
	static Quat const MakeFromAxisAngleRadians(Vec3 const& axis, float angleRadians);
	static Quat const MakeFromEulerAngles(EulerAngles const& eulerRotation);

	static Quat const Lerp(Quat const& q0, Quat const& q1, float t, bool useShortestPath = true); // not normalized
	static Quat const Nlerp(Quat const& q0, Quat const& q1, float t, bool useShortestPath = true); // normalized and shortest path

	static Quat const Slerp_NotNormalized(Quat const& q0, Quat const& q1, float t);
	static Quat const SlerpFullPath_NotNormalized(Quat const& q0, Quat const& q1, float t);
	static Quat const Slerp(Quat const& q0, Quat const& q1, float t);
	static Quat const SlerpFullPath(Quat const& q0, Quat const& q1, float t);

	// Essential Mathematics for Games and Interactive Application
	// squad(p,a,b,q,t) = slerp(slerp(p,q,t),slerp(a,b,t),2(1-t)t)
	// It performs a Bezier interpolation from p to q, using a and b as additional control points(or control orientations, to be more precise).
	static Quat const Squad(Quat const& q0, Quat const& tan0, Quat const& q1,Quat const& tan1, float t);

	static Quat const GetTangent(Quat const& prevQ, Quat const& currQ, Quat const& nextQ);

	// FindQuat Between Vector A and B.
	static Quat const FindQuatBetweenVectors(Vec3 const& fromVec, Vec3 const& toVec);

	// Accessors
	Quat const GetInverse() const;
	Quat const GetNormalized() const;
	float GetLength() const;
	float GetLengthSquared() const;

	// must be unit quaternion!
	void GetAsAxisAngleRadians(Vec3& axis, float& angleRadians) const;
	float GetAngleRadians() const;
	Vec3 GetRotationAxis() const;
	
	// unit quaternion! From unreal engine
	Quat Exp() const;
	Quat Log() const;

	// Mutators
	void Invert();
	void Normalize();

	// RotateVector UnrotateVector
	Vec3 const RotateVector(Vec3 const& vec) const;

	// Operators
	Quat const operator+(Quat const& rhs) const;
	Quat const operator-(Quat const& rhs) const;
	Quat const operator*(float rhs) const;
	Quat const operator*(Quat const& rhs) const;
	float operator|(Quat const& rhs) const;



	void operator+=(Quat const& rhs);
	void operator-=(Quat const& rhs);
	void operator*=(float rhs);

	// Standalone "friend" functions that are conceptually, but not actually, part of Quat::
	friend Quat const operator*(float lhs, Quat const& rhs);	// float * Quat
};

