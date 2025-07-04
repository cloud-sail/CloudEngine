#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Quat.hpp"
#include <vector>
//-----------------------------------------------------------------------------------------------
// Notes:
// input key of a curve is ascending, 0, 1, 2 ... (just like the index of the point), except for reparamTable
// m_isLooped is not used in CurveXXX, just add a point which is the same as the start point

enum class CurveMode
{
	LINEAR,
	CURVE,
	COUNT,
};

//-----------------------------------------------------------------------------------------------
class CurvePointFloat
{
public:
	float m_inputKey = 0.f;
	float m_outputValue = 0.f;
	float m_arriveTangent = 0.f;
	float m_leaveTangent = 0.f;

	CurveMode m_mode = CurveMode::LINEAR;

public:
	CurvePointFloat() = default;
	CurvePointFloat(float inputKey, float outputValue, float arriveTangent = 0.f, float leaveTanget = 0.f, CurveMode mode = CurveMode::LINEAR);
};


class CurveFloat
{
public:
	std::vector<CurvePointFloat> m_points;


public:
	float	Eval(float inputKey, float defaultValue = 0.f) const;
	float	EvalDerivative(float inputKey, float defaultValue = 0.f) const;

	int		GetPointIndexForInputKey(float inputKey) const;
	int		GetPointIndexForOutputValueIfValueAccending(float outputValue) const;

	void	ReorgnizeInputKeys(); // not used
	void	Reset(int newCapacity = 8);

	bool	IsIndexValid(int index);
};

//-----------------------------------------------------------------------------------------------

class CurvePointVec2
{
public:
	float m_inputKey = 0.f;
	Vec2 m_outputValue;
	Vec2 m_arriveTangent;
	Vec2 m_leaveTangent;

	CurveMode m_mode = CurveMode::LINEAR;

public:
	CurvePointVec2() = default;
	CurvePointVec2(float inputKey, Vec2 outputValue, Vec2 arriveTangent = Vec2::ZERO, Vec2 leaveTanget = Vec2::ZERO, CurveMode mode = CurveMode::LINEAR);
};


class CurveVec2
{
public:
	std::vector<CurvePointVec2> m_points;

public:
	Vec2	Eval(float inputKey, Vec2 defaultValue = Vec2::ZERO) const;
	Vec2	EvalDerivative(float inputKey, Vec2 defaultValue = Vec2::ZERO) const;

	int		GetPointIndexForInputKey(float inputKey) const;

	void	ReorgnizeInputKeys(); // not used
	void	Reset(int newCapacity = 8);

	bool	IsIndexValid(int index);
};

//-----------------------------------------------------------------------------------------------

class CurvePointVec3
{
public:
	float m_inputKey = 0.f;
	Vec3 m_outputValue;
	Vec3 m_arriveTangent;
	Vec3 m_leaveTangent;

	CurveMode m_mode = CurveMode::LINEAR;

public:
	CurvePointVec3() = default;
	CurvePointVec3(float inputKey, Vec3 outputValue, Vec3 arriveTangent = Vec3::ZERO, Vec3 leaveTanget = Vec3::ZERO, CurveMode mode = CurveMode::LINEAR);
};


class CurveVec3
{
public:
	std::vector<CurvePointVec3> m_points;

public:
	Vec3	Eval(float inputKey, Vec3 defaultValue = Vec3::ZERO) const;
	Vec3	EvalDerivative(float inputKey, Vec3 defaultValue = Vec3::ZERO) const;

	int		GetPointIndexForInputKey(float inputKey) const;

	void	ReorgnizeInputKeys(); // not used
	void	Reset(int newCapacity = 8);

	bool	IsIndexValid(int index);
};

//-----------------------------------------------------------------------------------------------
class CurvePointQuat
{
public:
	float m_inputKey = 0.f;
	Quat m_outputValue;
	Quat m_arriveTangent = Quat::IDENTITY;
	Quat m_leaveTangent = Quat::IDENTITY;

	CurveMode m_mode = CurveMode::LINEAR;

public:
	CurvePointQuat() = default;
	CurvePointQuat(float inputKey, Quat outputValue, Quat arriveTangent = Quat::IDENTITY, Quat leaveTanget = Quat::IDENTITY, CurveMode mode = CurveMode::LINEAR);
};

class CurveQuat
{
public:
	std::vector<CurvePointQuat> m_points;

public:
	Quat	Eval(float inputKey, Quat defaultValue = Quat::IDENTITY) const;

	int		GetPointIndexForInputKey(float inputKey) const;

	void	AutoSetTangents();
	void	ReorgnizeInputKeys(); // not used
	void	Reset(int newCapacity = 8);

	bool	IsIndexValid(int index);
};


//-----------------------------------------------------------------------------------------------
// 2D spline
// - Position Data
// - Scale Data
// - ReparamTable it is linear!
//-----------------------------------------------------------------------------------------------
struct SplinePoint2D
{
	float m_inputKey = 0.f;
	Vec2 m_outputValue;
	Vec2 m_arriveTangent;
	Vec2 m_leaveTangent;
	CurveMode m_mode = CurveMode::LINEAR;

	SplinePoint2D() = default;
	explicit SplinePoint2D(float inputKey, Vec2 outputValue, Vec2 arriveTangent = Vec2::ZERO, Vec2 leaveTanget = Vec2::ZERO, CurveMode mode = CurveMode::LINEAR);
	
	static SplinePoint2D const MakeFromCubicBezier(float inputKey, Vec2 outputValue, Vec2 prevGuidePos, Vec2 nextGuidePos);
	static SplinePoint2D const MakeFromContinuousCubicBezierFromNextGuidePos(float inputKey, Vec2 outputValue, Vec2 nextGuidePos);
	static SplinePoint2D const MakeFromContinuousCubicBezierFromPrevGuidePos(float inputKey, Vec2 outputValue, Vec2 prevGuidePos);
	static SplinePoint2D const MakeFromHermite(float inputKey, Vec2 outputValue, Vec2 arriveTangent, Vec2 leaveTangent);
	static SplinePoint2D const MakeFromContinuousHermite(float inputKey, Vec2 outputValue, Vec2 leaveTangent); // or just Tangent?
};

class Spline2D
{
public:
	CurveVec2 m_position;
	CurveFloat m_reparamTable; // input: Distance output: t(input key)

	int m_reparamStepsPerSegment = 10;

public:
	// Mutator
	void SetSubdivisionsPerSegment(int numSubdivisions, bool needUpdate = true);
	void AddPoint(SplinePoint2D const& point, bool needUpdate = true);

	void ClearAllSplinePoints();
	void SetFromCatmullRomAlgorithm(std::vector<Vec2> const& points, bool needUpdate = true);

	// Accessors
	int GetNumberOfSplinePoints() const;
	int GetNumberOfSplineSegments() const;

	Vec2 GetPositionAtInputKey(float inputKey) const;

	Vec2 GetTangentAtInputKey(float inputKey) const;
	Vec2 GetDirectionAtInputKey(float inputKey) const;
	
	float GetDistanceAlongSplineAtInputKey(float inputKey) const;
	float GetInputKeyAtDistanceAlongSpline(float distance) const;
	float GetSplineLength() const;
	
	void GetPositionListWithSubdivisions(std::vector<Vec2>& positions, int numSubdivisions = 1) const;

private:
	Vec2 GetPositionFromSegment(int index, float parametricZeroToOne) const;
	float GetInputKeyFromSegment(int index, float parametricZeroToOne) const;

	void UpdateSpline();
};


//-----------------------------------------------------------------------------------------------
// 3D spline
// - Position Data
// - Rotation Data
// - Scale Data
// - ReparamTable it is linear!
//-----------------------------------------------------------------------------------------------
struct SplinePoint3D
{
	float m_inputKey = 0.f;
	Vec3 m_position;
	Vec3 m_arriveTangent;
	Vec3 m_leaveTangent;
	EulerAngles m_rotation;
	Vec3 m_scale = Vec3(1.f, 1.f, 1.f);


	CurveMode m_mode = CurveMode::LINEAR;

	SplinePoint3D() = default;
	explicit SplinePoint3D(float inputKey, Vec3 position, Vec3 arriveTangent = Vec3::ZERO, Vec3 leaveTanget = Vec3::ZERO, CurveMode mode = CurveMode::LINEAR, EulerAngles rotation = EulerAngles(), Vec3 scale = Vec3(1.f, 1.f, 1.f));

	//static SplinePoint2D const MakeFromCubicBezier(float inputKey, Vec2 outputValue, Vec2 prevGuidePos, Vec2 nextGuidePos);
	//static SplinePoint2D const MakeFromContinuousCubicBezierFromNextGuidePos(float inputKey, Vec2 outputValue, Vec2 nextGuidePos);
	//static SplinePoint2D const MakeFromContinuousCubicBezierFromPrevGuidePos(float inputKey, Vec2 outputValue, Vec2 prevGuidePos);
	//static SplinePoint2D const MakeFromHermite(float inputKey, Vec2 outputValue, Vec2 arriveTangent, Vec2 leaveTangent);
	static SplinePoint3D const MakeFromContinuousHermite(float inputKey, Vec3 outputValue, Vec3 leaveTangent); // or just Tangent?
};

class Spline3D
{
public:
	CurveVec3 m_position;
	CurveVec3 m_scale;
	CurveQuat m_rotation; // it is relative to the default Up vector

	CurveFloat m_reparamTable; // input: Distance output: t(input key)

	int m_reparamStepsPerSegment = 10;

	Vec3 m_defaultUpVector = Vec3(0.f, 0.f, 1.f); // for calculating transform along the spline

public:
	// Mutator
	void SetSubdivisionsPerSegment(int numSubdivisions, bool needUpdate = true);
	void AddPoint(SplinePoint3D const& point, bool needUpdate = true);
	void SetPositionAtSplinePoint(int pointIndex, Vec3 const& newPosition, bool needUpdate = true);
	void SetRotationAtSplinePoint(int pointIndex, EulerAngles const& newRotation, bool needUpdate = true);
	void SetScaleAtSplinePoint(int pointIndex, Vec3 const& newScale, bool needUpdate = true);

	void UpdateSpline();

	void ClearAllSplinePoints();
	void SetFromCatmullRomAlgorithm(std::vector<Vec3> const& points, bool needUpdate = true);


	// Accessors
	int GetNumberOfSplinePoints() const;
	int GetNumberOfSplineSegments() const;

	// Access by input key
	// Transform = Position + Quaternion + Scale
	Vec3 GetPositionAtInputKey(float inputKey) const;
	Vec3 GetTangentAtInputKey(float inputKey) const;
	Vec3 GetDirectionAtInputKey(float inputKey) const;
	Vec3 GetScaleAtInputKey(float inputKey) const;
	Quat GetQuaternionAtInputKey(float inputKey) const;
	Vec3 GetUpVectorAtInputKey(float inputKey) const; // rotate up vector by quat


	float GetDistanceAlongSplineAtInputKey(float inputKey) const;
	float GetInputKeyAtDistanceAlongSpline(float distance) const;
	float GetSplineLength() const;

	void GetPositionListWithSubdivisions(std::vector<Vec3>& positions, int numSubdivisions = 1) const;

private:
	Vec3 GetPositionFromSegment(int index, float parametricZeroToOne) const;
	float GetInputKeyFromSegment(int index, float parametricZeroToOne) const;

};

