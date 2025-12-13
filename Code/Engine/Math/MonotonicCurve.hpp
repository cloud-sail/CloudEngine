#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>

//-----------------------------------------------------------------------------------------------
// MonotonicCurve - Bidirectional X-Y lookup for monotonic relationships
// 
// Features:
// - X->Y forward evaluation
// - Y->X inverse evaluation (automatic direction detection)
// - Auto-clamping for out-of-range queries
// 
// Requirements:
// - Curve must be monotonic (Y always increases OR always decreases as X increases)
// - Adding points that violate monotonicity will trigger an error
//-----------------------------------------------------------------------------------------------
class MonotonicCurve
{
public:
	MonotonicCurve() = default;
	explicit MonotonicCurve(std::vector<Vec2> const& points);

	void AddPoint(float x, float y);
	void AddPoint(Vec2 const& point);
	void Clear();

	float EvaluateX(float x) const;  // X -> Y
	float EvaluateY(float y) const;  // Y -> X

	float GetMinX() const;
	float GetMaxX() const;
	float GetMinY() const;
	float GetMaxY() const;

	bool IsEmpty() const { return m_points.empty(); }
	int GetPointCount() const { return static_cast<int>(m_points.size()); }
	Vec2 GetPoint(int index) const;

private:
	void ValidateMonotonicity() const;  // Check all points maintain monotonicity

private:
	std::vector<Vec2> m_points; // Sorted by X
};