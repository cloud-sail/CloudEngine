#include "Engine/Math/Spline.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/EngineCommon.hpp"

CurvePointFloat::CurvePointFloat(float inputKey, float outputValue, float arriveTangent /*= 0.f*/, float leaveTanget /*= 0.f*/, CurveMode mode /*= CurveMode::LINEAR*/)
	: m_inputKey(inputKey)
	, m_outputValue(outputValue)
	, m_arriveTangent(arriveTangent)
	, m_leaveTangent(leaveTanget)
	, m_mode(mode)
{
}

CurvePointVec2::CurvePointVec2(float inputKey, Vec2 outputValue, Vec2 arriveTangent /*= Vec2::ZERO*/, Vec2 leaveTanget /*= Vec2::ZERO*/, CurveMode mode /*= CurveMode::LINEAR*/)
	: m_inputKey(inputKey)
	, m_outputValue(outputValue)
	, m_arriveTangent(arriveTangent)
	, m_leaveTangent(leaveTanget)
	, m_mode(mode)
{

}

CurvePointVec3::CurvePointVec3(float inputKey, Vec3 outputValue, Vec3 arriveTangent /*= Vec3::ZERO*/, Vec3 leaveTanget /*= Vec3::ZERO*/, CurveMode mode /*= CurveMode::LINEAR*/)
	: m_inputKey(inputKey)
	, m_outputValue(outputValue)
	, m_arriveTangent(arriveTangent)
	, m_leaveTangent(leaveTanget)
	, m_mode(mode)
{

}


CurvePointQuat::CurvePointQuat(float inputKey, Quat outputValue, Quat arriveTangent /*= Quat::IDENTITY*/, Quat leaveTanget /*= Quat::IDENTITY*/, CurveMode mode /*= CurveMode::LINEAR*/)
	: m_inputKey(inputKey)
	, m_outputValue(outputValue)
	, m_arriveTangent(arriveTangent)
	, m_leaveTangent(leaveTanget)
	, m_mode(mode)
{

}



int CurveFloat::GetPointIndexForInputKey(float inputKey) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	GUARANTEE_OR_DIE(numPoints > 0, "No point is in curve");

	if (inputKey < m_points[0].m_inputKey)
	{
		return -1;
	}

	if (inputKey >= m_points[lastIndex].m_inputKey)
	{
		return lastIndex;
	}

	int left = 0;
	int right = numPoints;

	while (right - left > 1)
	{
		int mid = (left + right) / 2;
		if (m_points[mid].m_inputKey <= inputKey)
		{
			left = mid;
		}
		else
		{
			right = mid;
		}
	}

	return left;
}

int CurveFloat::GetPointIndexForOutputValueIfValueAccending(float outputValue) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	GUARANTEE_OR_DIE(numPoints > 0, "No point is in curve");

	if (outputValue < m_points[0].m_outputValue)
	{
		return -1;
	}

	if (outputValue >= m_points[lastIndex].m_outputValue)
	{
		return lastIndex;
	}

	int left = 0;
	int right = numPoints;

	while (right - left > 1)
	{
		int mid = (left + right) / 2;
		if (m_points[mid].m_outputValue <= outputValue)
		{
			left = mid;
		}
		else
		{
			right = mid;
		}
	}

	return left;
}

void CurveFloat::ReorgnizeInputKeys()
{
	int numPoints = (int)m_points.size();

	for (int i = 0; i < numPoints; ++i)
	{
		m_points[i].m_inputKey = (float)i;
	}
}

void CurveFloat::Reset(int newCapacity)
{
	m_points.clear();
	m_points.reserve(newCapacity);
}

bool CurveFloat::IsIndexValid(int index)
{
	int size = (int)m_points.size();
	return index >= 0 && index < size;
}

float CurveFloat::Eval(float inputKey, float defaultValue /*= 0.f*/) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return defaultValue;
	}

	int index = GetPointIndexForInputKey(inputKey);

	if (index < 0)
	{
		return m_points[0].m_outputValue;
	}

	if (index == lastIndex)
	{
		return m_points[lastIndex].m_outputValue;
	}

	CurvePointFloat prevPoint = m_points[index];
	CurvePointFloat nextPoint = m_points[index + 1];

	// Important: 
	// float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;
	// leave / arrive tangent need to "*= diff" when evaluate cubic curve

	float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;

	if (diff > 0.f)
	{
		float t = (inputKey - prevPoint.m_inputKey) / diff;
		if (prevPoint.m_mode == CurveMode::LINEAR)
		{
			return Interpolate(prevPoint.m_outputValue, nextPoint.m_outputValue, t);
		}
		else
		{
			return ComputeCubicHermite(prevPoint.m_outputValue, prevPoint.m_leaveTangent * diff, nextPoint.m_outputValue, nextPoint.m_arriveTangent * diff, t);
		}
	}
	else
	{
		return prevPoint.m_outputValue;
	}
}

float CurveFloat::EvalDerivative(float inputKey, float defaultValue /*= 0.f*/) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return defaultValue;
	}

	int index = GetPointIndexForInputKey(inputKey);

	if (index < 0)
	{
		return m_points[0].m_leaveTangent;
	}

	if (index == lastIndex)
	{
		return m_points[lastIndex].m_arriveTangent;
	}

	CurvePointFloat prevPoint = m_points[index];
	CurvePointFloat nextPoint = m_points[index + 1];

	float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;

	if (diff > 0.f)
	{
		if (prevPoint.m_mode == CurveMode::LINEAR)
		{
			return (nextPoint.m_outputValue - prevPoint.m_outputValue) / diff;
		}
		else
		{
			float t = (inputKey - prevPoint.m_inputKey) / diff;
			return ComputeCubicHermiteDerivative(prevPoint.m_outputValue, prevPoint.m_leaveTangent * diff, nextPoint.m_outputValue, nextPoint.m_arriveTangent * diff, t);
		}
	}
	else
	{
		return 0.f;
	}
}

Vec2 CurveVec2::Eval(float inputKey, Vec2 defaultValue /*= Vec2::ZERO*/) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return defaultValue;
	}

	int index = GetPointIndexForInputKey(inputKey);

	if (index < 0)
	{
		return m_points[0].m_outputValue;
	}

	if (index == lastIndex)
	{
		return m_points[lastIndex].m_outputValue;
	}

	CurvePointVec2 prevPoint = m_points[index];
	CurvePointVec2 nextPoint = m_points[index + 1];

	// Important: 
	// float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;
	// leave / arrive tangent need to "*= diff" when evaluate cubic curve

	float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;

	if (diff > 0.f)
	{
		float t = (inputKey - prevPoint.m_inputKey) / diff;
		if (prevPoint.m_mode == CurveMode::LINEAR)
		{

			return Interpolate(prevPoint.m_outputValue, nextPoint.m_outputValue, t);
		}
		else
		{
			return ComputeCubicHermite(prevPoint.m_outputValue, prevPoint.m_leaveTangent * diff, nextPoint.m_outputValue, nextPoint.m_arriveTangent * diff, t);
		}
	}
	else
	{
		return prevPoint.m_outputValue;
	}
}

Vec2 CurveVec2::EvalDerivative(float inputKey, Vec2 defaultValue /*= = Vec2::ZERO*/) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return defaultValue;
	}

	int index = GetPointIndexForInputKey(inputKey);

	if (index < 0)
	{
		return m_points[0].m_leaveTangent;
	}

	if (index == lastIndex)
	{
		return m_points[lastIndex].m_arriveTangent;
	}

	CurvePointVec2 prevPoint = m_points[index];
	CurvePointVec2 nextPoint = m_points[index + 1];

	float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;

	if (diff > 0.f)
	{
		if (prevPoint.m_mode == CurveMode::LINEAR)
		{
			return (nextPoint.m_outputValue - prevPoint.m_outputValue) / diff;
		}
		else
		{
			float t = (inputKey - prevPoint.m_inputKey) / diff;
			return ComputeCubicHermiteDerivative(prevPoint.m_outputValue, prevPoint.m_leaveTangent * diff, nextPoint.m_outputValue, nextPoint.m_arriveTangent * diff, t);
		}
	}
	else
	{
		return Vec2::ZERO;
	}
}

int CurveVec2::GetPointIndexForInputKey(float inputKey) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	GUARANTEE_OR_DIE(numPoints > 0, "No point is in curve");

	if (inputKey < m_points[0].m_inputKey)
	{
		return -1;
	}

	if (inputKey >= m_points[lastIndex].m_inputKey)
	{
		return lastIndex;
	}

	int left = 0;
	int right = numPoints;

	while (right - left > 1)
	{
		int mid = (left + right) / 2;
		if (m_points[mid].m_inputKey <= inputKey)
		{
			left = mid;
		}
		else
		{
			right = mid;
		}
	}

	return left;
}

void CurveVec2::ReorgnizeInputKeys()
{
	int numPoints = (int)m_points.size();

	for (int i = 0; i < numPoints; ++i)
	{
		m_points[i].m_inputKey = (float)i;
	}
}

void CurveVec2::Reset(int newCapacity)
{
	m_points.clear();
	m_points.reserve(newCapacity);
}

bool CurveVec2::IsIndexValid(int index)
{
	int size = (int)m_points.size();
	return index >= 0 && index < size;
}

Vec3 CurveVec3::Eval(float inputKey, Vec3 defaultValue /*= Vec3::ZERO*/) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return defaultValue;
	}

	int index = GetPointIndexForInputKey(inputKey);

	if (index < 0)
	{
		return m_points[0].m_outputValue;
	}

	if (index == lastIndex)
	{
		return m_points[lastIndex].m_outputValue;
	}

	CurvePointVec3 prevPoint = m_points[index];
	CurvePointVec3 nextPoint = m_points[index + 1];

	// Important: 
	// float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;
	// leave / arrive tangent need to "*= diff" when evaluate cubic curve

	float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;

	if (diff > 0.f)
	{
		float t = (inputKey - prevPoint.m_inputKey) / diff;
		if (prevPoint.m_mode == CurveMode::LINEAR)
		{

			return Interpolate(prevPoint.m_outputValue, nextPoint.m_outputValue, t);
		}
		else
		{
			return ComputeCubicHermite(prevPoint.m_outputValue, prevPoint.m_leaveTangent * diff, nextPoint.m_outputValue, nextPoint.m_arriveTangent * diff, t);
		}
	}
	else
	{
		return prevPoint.m_outputValue;
	}
}

Vec3 CurveVec3::EvalDerivative(float inputKey, Vec3 defaultValue /*= = Vec3::ZERO*/) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return defaultValue;
	}

	int index = GetPointIndexForInputKey(inputKey);

	if (index < 0)
	{
		return m_points[0].m_leaveTangent;
	}

	if (index == lastIndex)
	{
		return m_points[lastIndex].m_arriveTangent;
	}

	CurvePointVec3 prevPoint = m_points[index];
	CurvePointVec3 nextPoint = m_points[index + 1];

	float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;

	if (diff > 0.f)
	{
		if (prevPoint.m_mode == CurveMode::LINEAR)
		{
			return (nextPoint.m_outputValue - prevPoint.m_outputValue) / diff;
		}
		else
		{
			float t = (inputKey - prevPoint.m_inputKey) / diff;
			return ComputeCubicHermiteDerivative(prevPoint.m_outputValue, prevPoint.m_leaveTangent * diff, nextPoint.m_outputValue, nextPoint.m_arriveTangent * diff, t);
		}
	}
	else
	{
		return Vec3::ZERO;
	}
}

int CurveVec3::GetPointIndexForInputKey(float inputKey) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	GUARANTEE_OR_DIE(numPoints > 0, "No point is in curve");

	if (inputKey < m_points[0].m_inputKey)
	{
		return -1;
	}

	if (inputKey >= m_points[lastIndex].m_inputKey)
	{
		return lastIndex;
	}

	int left = 0;
	int right = numPoints;

	while (right - left > 1)
	{
		int mid = (left + right) / 2;
		if (m_points[mid].m_inputKey <= inputKey)
		{
			left = mid;
		}
		else
		{
			right = mid;
		}
	}

	return left;
}

void CurveVec3::ReorgnizeInputKeys()
{
	int numPoints = (int)m_points.size();

	for (int i = 0; i < numPoints; ++i)
	{
		m_points[i].m_inputKey = (float)i;
	}
}

void CurveVec3::Reset(int newCapacity)
{
	m_points.clear();
	m_points.reserve(newCapacity);
}


bool CurveVec3::IsIndexValid(int index)
{
	int size = (int)m_points.size();
	return index >= 0 && index < size;
}

Quat CurveQuat::Eval(float inputKey, Quat defaultValue /*= Quat::IDENTITY*/) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return defaultValue;
	}

	int index = GetPointIndexForInputKey(inputKey);

	if (index < 0)
	{
		return m_points[0].m_outputValue;
	}

	if (index == lastIndex)
	{
		return m_points[lastIndex].m_outputValue;
	}

	CurvePointQuat prevPoint = m_points[index];
	CurvePointQuat nextPoint = m_points[index + 1];

	// Important: 
	// float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;
	// leave / arrive tangent need to "*= diff" when evaluate cubic curve

	float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;

	if (diff > 0.f)
	{
		float t = (inputKey - prevPoint.m_inputKey) / diff;
		if (prevPoint.m_mode == CurveMode::LINEAR)
		{
			return Quat::Slerp(prevPoint.m_outputValue, nextPoint.m_outputValue, t); // consider the computation cost?
		}
		else
		{
			return Quat::Squad(prevPoint.m_outputValue, prevPoint.m_leaveTangent * diff, nextPoint.m_outputValue, nextPoint.m_arriveTangent * diff, t);
		}
	}
	else
	{
		return prevPoint.m_outputValue;
	}
}

int CurveQuat::GetPointIndexForInputKey(float inputKey) const
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	GUARANTEE_OR_DIE(numPoints > 0, "No point is in curve");

	if (inputKey < m_points[0].m_inputKey)
	{
		return -1;
	}

	if (inputKey >= m_points[lastIndex].m_inputKey)
	{
		return lastIndex;
	}

	int left = 0;
	int right = numPoints;

	while (right - left > 1)
	{
		int mid = (left + right) / 2;
		if (m_points[mid].m_inputKey <= inputKey)
		{
			left = mid;
		}
		else
		{
			right = mid;
		}
	}

	return left;
}

void CurveQuat::AutoSetTangents()
{
	int numPoints = (int)m_points.size();
	int lastIndex = numPoints - 1;

	for (int currIndex = 0; currIndex < numPoints; currIndex++)
	{
		int prevIndex = (currIndex == 0) ? 0 : currIndex - 1;
		int nextIndex = (currIndex == lastIndex) ? lastIndex : currIndex + 1;

		CurvePointQuat& currPoint = m_points[currIndex];
		CurvePointQuat const& prevPoint = m_points[prevIndex];
		CurvePointQuat const& nextPoint = m_points[nextIndex];

		float diff = nextPoint.m_inputKey - prevPoint.m_inputKey;
		if (diff <= 0.0001f)
		{
			diff = 0.0001f;
		}

		Quat tangent = Quat::GetTangent(prevPoint.m_outputValue, currPoint.m_outputValue, nextPoint.m_outputValue);
		tangent *= (1.f / diff);
		currPoint.m_arriveTangent = tangent;
		currPoint.m_leaveTangent = tangent;
	}
}

void CurveQuat::ReorgnizeInputKeys()
{
	int numPoints = (int)m_points.size();

	for (int i = 0; i < numPoints; ++i)
	{
		m_points[i].m_inputKey = (float)i;
	}
}

void CurveQuat::Reset(int newCapacity /*= 8*/)
{
	m_points.clear();
	m_points.reserve(newCapacity);
}



bool CurveQuat::IsIndexValid(int index)
{
	int size = (int)m_points.size();
	return index >= 0 && index < size;
}

void Spline2D::SetSubdivisionsPerSegment(int numSubdivisions, bool needUpdate /*= true*/)
{
	m_reparamStepsPerSegment = numSubdivisions;
	if (needUpdate)
	{
		UpdateSpline();
	}
}

void Spline2D::AddPoint(SplinePoint2D const& point, bool needUpdate /*= true*/)
{
	m_position.m_points.push_back(CurvePointVec2(point.m_inputKey, point.m_outputValue, point.m_arriveTangent, point.m_leaveTangent, point.m_mode));

	if (needUpdate)
	{
		UpdateSpline();
	}
}

void Spline2D::ClearAllSplinePoints()
{
	m_position.Reset();
	m_reparamTable.Reset();
}


int Spline2D::GetNumberOfSplinePoints() const
{
	return static_cast<int>(m_position.m_points.size());
}

int Spline2D::GetNumberOfSplineSegments() const
{
	int numPoints = (int)m_position.m_points.size();
	return (numPoints >= 1) ? (numPoints - 1) : 0;
}

Vec2 Spline2D::GetPositionAtInputKey(float inputKey) const
{
	return m_position.Eval(inputKey);
}

Vec2 Spline2D::GetTangentAtInputKey(float inputKey) const
{
	return m_position.EvalDerivative(inputKey);
}

Vec2 Spline2D::GetDirectionAtInputKey(float inputKey) const
{
	return m_position.EvalDerivative(inputKey).GetNormalized();
}

float Spline2D::GetDistanceAlongSplineAtInputKey(float inputKey) const
{
	// inputKey is the output value for m_reparamTable
	int numPoints = (int)m_reparamTable.m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return 0.f;
	}

	int index = m_reparamTable.GetPointIndexForOutputValueIfValueAccending(inputKey);

	if (index < 0)
	{
		return  m_reparamTable.m_points[0].m_inputKey;
	}

	if (index == lastIndex)
	{
		return GetSplineLength();
		//return m_reparamTable.m_points[lastIndex].m_inputKey; // Get Spline Length
	}

	CurvePointFloat prevPoint = m_reparamTable.m_points[index];
	CurvePointFloat nextPoint = m_reparamTable.m_points[index + 1];

	if (prevPoint.m_inputKey == nextPoint.m_inputKey || prevPoint.m_outputValue == nextPoint.m_outputValue)
	{
		return prevPoint.m_inputKey;
	}

	return RangeMap(inputKey, prevPoint.m_outputValue, nextPoint.m_outputValue, prevPoint.m_inputKey, nextPoint.m_inputKey);
}

float Spline2D::GetInputKeyAtDistanceAlongSpline(float distance) const
{
	return m_reparamTable.Eval(distance);
}

float Spline2D::GetSplineLength() const
{
	int lastIndex = static_cast<int>(m_reparamTable.m_points.size() - 1);
	if (lastIndex >= 0)
	{
		return m_reparamTable.m_points[lastIndex].m_inputKey;
	}
	return 0.f;
}

void Spline2D::GetPositionListWithSubdivisions(std::vector<Vec2>& positions, int numSubdivisions /*= 1*/) const
{
	GUARANTEE_OR_DIE(numSubdivisions > 0, "invalid subdivision number.");

	int numPoints = (int)m_position.m_points.size();
	if (numPoints == 0)
	{
		return;
	}

	int numSegments = numPoints - 1;

	positions.clear();
	positions.reserve(numSegments * numSubdivisions + 1);

	positions.push_back(m_position.m_points[0].m_outputValue);

	for (int segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
	{
		for (int step = 1; step <= numSubdivisions; ++step)
		{
			float parametricZeroToOne = static_cast<float>(step) / static_cast<float>(numSubdivisions);
			Vec2 curPoint = GetPositionFromSegment(segmentIndex, parametricZeroToOne); // faster? not to binary search the index.

			positions.push_back(curPoint);
		}

	}
}

Vec2 Spline2D::GetPositionFromSegment(int index, float parametricZeroToOne) const
{
	// Compared to Eval, it skips the binary search
	CurvePointVec2 const& startPoint = m_position.m_points[index];
	CurvePointVec2 const& endPoint = m_position.m_points[index + 1];

	float diff = endPoint.m_inputKey - startPoint.m_inputKey;

	if (diff > 0.f)
	{
		if (startPoint.m_mode == CurveMode::LINEAR)
		{
			return Interpolate(startPoint.m_outputValue, endPoint.m_outputValue, parametricZeroToOne);
		}
		else
		{
			return ComputeCubicHermite(startPoint.m_outputValue, startPoint.m_leaveTangent * diff, endPoint.m_outputValue, endPoint.m_arriveTangent * diff, parametricZeroToOne);
		}
	}
	else
	{
		return startPoint.m_outputValue;
	}
}

float Spline2D::GetInputKeyFromSegment(int index, float parametricZeroToOne) const
{
	CurvePointVec2 const& startPoint = m_position.m_points[index];
	CurvePointVec2 const& endPoint = m_position.m_points[index + 1];

	return Interpolate(startPoint.m_inputKey, endPoint.m_inputKey, parametricZeroToOne);
}

void Spline2D::UpdateSpline()
{
	int numPoints = (int)m_position.m_points.size();

	GUARANTEE_OR_DIE(m_reparamStepsPerSegment > 0, "Invalid subdivisions number!");

	int numSegments = (numPoints >= 1) ? (numPoints - 1) : 0;

	m_reparamTable.Reset(numSegments * m_reparamStepsPerSegment + 1);
	float accumulatedLength = 0.f;
	Vec2 prevPoint;

	// First Point
	if (numPoints > 0)
	{
		prevPoint = m_position.m_points[0].m_outputValue;
		m_reparamTable.m_points.push_back(CurvePointFloat(accumulatedLength, m_position.m_points[0].m_inputKey, 0.f, 0.f, CurveMode::LINEAR));
	}
	else
	{
		m_reparamTable.m_points.push_back(CurvePointFloat(accumulatedLength, 0.f, 0.f, 0.f, CurveMode::LINEAR));
		return;
	}

	for (int segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
	{
		for (int step = 1; step <= m_reparamStepsPerSegment; ++step)
		{
			float parametricZeroToOne = static_cast<float>(step) / static_cast<float>(m_reparamStepsPerSegment);
			Vec2 curPoint = GetPositionFromSegment(segmentIndex, parametricZeroToOne);
			float dist = (curPoint - prevPoint).GetLength();
			float curInputKey = GetInputKeyFromSegment(segmentIndex, parametricZeroToOne);


			accumulatedLength += dist;
			prevPoint = curPoint;

			m_reparamTable.m_points.push_back(CurvePointFloat(accumulatedLength, curInputKey, 0.f, 0.f, CurveMode::LINEAR));

		}

		// last one
		//Vec2 lastPoint = m_position.m_points[segmentIndex + 1].m_outputValue;
		//float lastDist = (lastPoint - prevPoint).GetLength();
		//accumulatedLength += lastDist;
		//prevPoint = lastPoint;
		//m_reparamTable.m_points.push_back(CurvePointFloat(accumulatedLength, m_position.m_points[segmentIndex + 1].m_inputKey, 0.f, 0.f, CurveMode::LINEAR));

	}

}

void Spline2D::SetFromCatmullRomAlgorithm(std::vector<Vec2> const& points, bool needUpdate /*= true*/)
{

	ClearAllSplinePoints();

	int numPoints = (int)points.size();

	if (numPoints <= 0)
	{
		return;
	}
	else if (numPoints == 1)
	{
		AddPoint(SplinePoint2D::MakeFromContinuousHermite(0.f, points[0], Vec2::ZERO), false);
	}
	else
	{
		AddPoint(SplinePoint2D::MakeFromContinuousHermite(0.f, points[0], Vec2::ZERO), false);
		
		for (int index = 1; index <= numPoints - 2; ++index)
		{
			Vec2 tangent = (points[index + 1] - points[index - 1]) * 0.5f;
			AddPoint(SplinePoint2D::MakeFromContinuousHermite((float)index, points[index], tangent), false);
		}

		AddPoint(SplinePoint2D::MakeFromContinuousHermite((float)(numPoints - 1), points[numPoints - 1], Vec2::ZERO), false);
	}

	if (needUpdate)
	{
		UpdateSpline();
	}
}

SplinePoint2D::SplinePoint2D(float inputKey, Vec2 outputValue, Vec2 arriveTangent /*= Vec2::ZERO*/, Vec2 leaveTanget /*= Vec2::ZERO*/, CurveMode mode /*= CurveMode::LINEAR*/)
	: m_inputKey(inputKey)
	, m_outputValue(outputValue)
	, m_arriveTangent(arriveTangent)
	, m_leaveTangent(leaveTanget)
	, m_mode(mode)
{

}

STATIC SplinePoint2D const SplinePoint2D::MakeFromCubicBezier(float inputKey, Vec2 outputValue, Vec2 prevGuidePos, Vec2 nextGuidePos)
{
	return SplinePoint2D(inputKey, outputValue, 3 * (outputValue - prevGuidePos), 3 * (nextGuidePos - outputValue), CurveMode::CURVE);
}

STATIC SplinePoint2D const SplinePoint2D::MakeFromContinuousCubicBezierFromNextGuidePos(float inputKey, Vec2 outputValue, Vec2 nextGuidePos)
{
	Vec2 leaveTangent = 3 * (nextGuidePos - outputValue);
	return SplinePoint2D(inputKey, outputValue, leaveTangent, leaveTangent, CurveMode::CURVE);
}

STATIC SplinePoint2D const SplinePoint2D::MakeFromContinuousCubicBezierFromPrevGuidePos(float inputKey, Vec2 outputValue, Vec2 prevGuidePos)
{
	Vec2 arriveTangent = 3 * (outputValue - prevGuidePos);
	return SplinePoint2D(inputKey, outputValue, arriveTangent, arriveTangent, CurveMode::CURVE);
}

STATIC SplinePoint2D const SplinePoint2D::MakeFromHermite(float inputKey, Vec2 outputValue, Vec2 arriveTangent, Vec2 leaveTangent)
{
	return SplinePoint2D(inputKey, outputValue, arriveTangent, leaveTangent, CurveMode::CURVE);
}

STATIC SplinePoint2D const SplinePoint2D::MakeFromContinuousHermite(float inputKey, Vec2 outputValue, Vec2 leaveTangent)
{
	return SplinePoint2D(inputKey, outputValue, leaveTangent, leaveTangent, CurveMode::CURVE);
}


//-----------------------------------------------------------------------------------------------
SplinePoint3D::SplinePoint3D(float inputKey, Vec3 position, Vec3 arriveTangent /*= Vec3::ZERO*/, Vec3 leaveTanget /*= Vec3::ZERO*/, CurveMode mode /*= CurveMode::LINEAR*/, EulerAngles rotation /*= EulerAngles()*/, Vec3 scale)
	: m_inputKey(inputKey)
	, m_position(position)
	, m_arriveTangent(arriveTangent)
	, m_leaveTangent(leaveTanget)
	, m_mode(mode)
	, m_rotation(rotation)
	, m_scale(scale)
{

}

STATIC SplinePoint3D const SplinePoint3D::MakeFromContinuousHermite(float inputKey, Vec3 outputValue, Vec3 leaveTangent)
{
	return SplinePoint3D(inputKey, outputValue, leaveTangent, leaveTangent, CurveMode::CURVE);
}

void Spline3D::SetSubdivisionsPerSegment(int numSubdivisions, bool needUpdate /*= true*/)
{
	m_reparamStepsPerSegment = numSubdivisions;
	if (needUpdate)
	{
		UpdateSpline();
	}
}

void Spline3D::AddPoint(SplinePoint3D const& point, bool needUpdate /*= true*/)
{
	m_position.m_points.push_back(CurvePointVec3(point.m_inputKey, point.m_position, point.m_arriveTangent, point.m_leaveTangent, point.m_mode));
	m_scale.m_points.push_back(CurvePointVec3(point.m_inputKey, point.m_scale, Vec3::ZERO, Vec3::ZERO, CurveMode::CURVE)); // SmoothStep3/EaseInOutCubic
	m_rotation.m_points.push_back(CurvePointQuat(point.m_inputKey, Quat::MakeFromEulerAngles(point.m_rotation), Quat::IDENTITY, Quat::IDENTITY, CurveMode::CURVE)); // Linear Slerp, Cubic Squad

	if (needUpdate)
	{
		UpdateSpline();
	}
}

void Spline3D::SetPositionAtSplinePoint(int pointIndex, Vec3 const& newPosition, bool needUpdate /*= true*/)
{
	bool isValid = m_position.IsIndexValid(pointIndex);
	GUARANTEE_RECOVERABLE(isValid, "SetPositionAtSplinePoint: Index out of range");
	if (!isValid) return;

	m_position.m_points[pointIndex].m_outputValue = newPosition;

	if (needUpdate)
	{
		UpdateSpline();
	}
}

void Spline3D::SetRotationAtSplinePoint(int pointIndex, EulerAngles const& newRotation, bool needUpdate /*= true*/)
{
	bool isValid = m_rotation.IsIndexValid(pointIndex);
	GUARANTEE_RECOVERABLE(isValid, "SetRotationAtSplinePoint: Index out of range");
	if (!isValid) return;

	Quat const newQuat = Quat::MakeFromEulerAngles(newRotation);

	Vec3 upVector = newQuat.RotateVector(Vec3::UP);
	// Update Rotation
	Quat const q = Quat::FindQuatBetweenVectors(m_defaultUpVector, upVector);
	m_rotation.m_points[pointIndex].m_outputValue = q;

	Vec3 forwardVector = newQuat.RotateVector(Vec3::FORWARD);
	m_position.m_points[pointIndex].m_arriveTangent = m_position.m_points[pointIndex].m_arriveTangent.GetLength() * forwardVector;
	m_position.m_points[pointIndex].m_leaveTangent = m_position.m_points[pointIndex].m_leaveTangent.GetLength() * forwardVector;

	if (needUpdate)
	{
		UpdateSpline();
	}
}

void Spline3D::SetScaleAtSplinePoint(int pointIndex, Vec3 const& newScale, bool needUpdate /*= true*/)
{
	bool isValid = m_scale.IsIndexValid(pointIndex);
	GUARANTEE_RECOVERABLE(isValid, "SetScaleAtSplinePoint: Index out of range");
	if (!isValid) return;

	m_scale.m_points[pointIndex].m_outputValue = newScale;

	if (needUpdate)
	{
		UpdateSpline();
	}
}

void Spline3D::ClearAllSplinePoints()
{
	m_position.Reset();
	m_scale.Reset();
	m_rotation.Reset();
	m_reparamTable.Reset();
}

void Spline3D::SetFromCatmullRomAlgorithm(std::vector<Vec3> const& points, bool needUpdate /*= true*/)
{
	ClearAllSplinePoints();

	int numPoints = (int)points.size();

	if (numPoints <= 0)
	{
		return;
	}
	else if (numPoints == 1)
	{
		AddPoint(SplinePoint3D::MakeFromContinuousHermite(0.f, points[0], Vec3::ZERO), false);
	}
	else
	{
		AddPoint(SplinePoint3D::MakeFromContinuousHermite(0.f, points[0], Vec3::ZERO), false);

		for (int index = 1; index <= numPoints - 2; ++index)
		{
			Vec3 tangent = (points[index + 1] - points[index - 1]) * 0.5f;
			AddPoint(SplinePoint3D::MakeFromContinuousHermite((float)index, points[index], tangent), false);
		}

		AddPoint(SplinePoint3D::MakeFromContinuousHermite((float)(numPoints - 1), points[numPoints - 1], Vec3::ZERO), false);
	}

	if (needUpdate)
	{
		UpdateSpline();
	}
}

int Spline3D::GetNumberOfSplinePoints() const
{
	return static_cast<int>(m_position.m_points.size());
}

int Spline3D::GetNumberOfSplineSegments() const
{
	int numPoints = (int)m_position.m_points.size();
	return (numPoints >= 1) ? (numPoints - 1) : 0;
}

Vec3 Spline3D::GetPositionAtInputKey(float inputKey) const
{
	return m_position.Eval(inputKey);
}

Vec3 Spline3D::GetTangentAtInputKey(float inputKey) const
{
	return m_position.EvalDerivative(inputKey);
}

Vec3 Spline3D::GetDirectionAtInputKey(float inputKey) const
{
	return m_position.EvalDerivative(inputKey).GetNormalized();
}

Vec3 Spline3D::GetScaleAtInputKey(float inputKey) const
{
	return m_scale.Eval(inputKey, Vec3(1.f, 1.f, 1.f));
}

Quat Spline3D::GetQuaternionAtInputKey(float inputKey) const
{
	Quat q = m_rotation.Eval(inputKey);
	q.Normalize();

	Vec3 forward = GetDirectionAtInputKey(inputKey);
	Vec3 up = q.RotateVector(m_defaultUpVector);

	return Mat44::MakeFromXZ(forward, up).GetQuat();
}

Vec3 Spline3D::GetUpVectorAtInputKey(float inputKey) const
{
	Quat q = m_rotation.Eval(inputKey);
	q.Normalize();

	Vec3 forward = GetDirectionAtInputKey(inputKey);
	Vec3 up = q.RotateVector(m_defaultUpVector);

	return Mat44::MakeFromXZ(forward, up).GetKBasis3D();
}

float Spline3D::GetDistanceAlongSplineAtInputKey(float inputKey) const
{
	// inputKey is the output value for m_reparamTable
	int numPoints = (int)m_reparamTable.m_points.size();
	int lastIndex = numPoints - 1;

	if (numPoints == 0)
	{
		return 0.f;
	}

	int index = m_reparamTable.GetPointIndexForOutputValueIfValueAccending(inputKey);

	if (index < 0)
	{
		return  m_reparamTable.m_points[0].m_inputKey;
	}

	if (index == lastIndex)
	{
		return GetSplineLength();
		//return m_reparamTable.m_points[lastIndex].m_inputKey; // Get Spline Length
	}

	CurvePointFloat prevPoint = m_reparamTable.m_points[index];
	CurvePointFloat nextPoint = m_reparamTable.m_points[index + 1];

	if (prevPoint.m_inputKey == nextPoint.m_inputKey || prevPoint.m_outputValue == nextPoint.m_outputValue)
	{
		return prevPoint.m_inputKey;
	}

	return RangeMap(inputKey, prevPoint.m_outputValue, nextPoint.m_outputValue, prevPoint.m_inputKey, nextPoint.m_inputKey);
}

float Spline3D::GetInputKeyAtDistanceAlongSpline(float distance) const
{
	return m_reparamTable.Eval(distance);
}

float Spline3D::GetSplineLength() const
{
	int lastIndex = static_cast<int>(m_reparamTable.m_points.size() - 1);
	if (lastIndex >= 0)
	{
		return m_reparamTable.m_points[lastIndex].m_inputKey;
	}
	return 0.f;
}

void Spline3D::GetPositionListWithSubdivisions(std::vector<Vec3>& positions, int numSubdivisions /*= 1*/) const
{
	GUARANTEE_OR_DIE(numSubdivisions > 0, "invalid subdivision number.");

	int numPoints = (int)m_position.m_points.size();
	if (numPoints == 0)
	{
		return;
	}

	int numSegments = numPoints - 1;

	positions.clear();
	positions.reserve(numSegments * numSubdivisions + 1);

	positions.push_back(m_position.m_points[0].m_outputValue);

	for (int segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
	{
		for (int step = 1; step <= numSubdivisions; ++step)
		{
			float parametricZeroToOne = static_cast<float>(step) / static_cast<float>(numSubdivisions);
			Vec3 curPoint = GetPositionFromSegment(segmentIndex, parametricZeroToOne); // faster? not to binary search the index.

			positions.push_back(curPoint);
		}

	}
}

Vec3 Spline3D::GetPositionFromSegment(int index, float parametricZeroToOne) const
{
	// Compared to Eval, it skips the binary search
	CurvePointVec3 const& startPoint = m_position.m_points[index];
	CurvePointVec3 const& endPoint = m_position.m_points[index + 1];

	float diff = endPoint.m_inputKey - startPoint.m_inputKey;

	if (diff > 0.f)
	{
		if (startPoint.m_mode == CurveMode::LINEAR)
		{
			return Interpolate(startPoint.m_outputValue, endPoint.m_outputValue, parametricZeroToOne);
		}
		else
		{
			return ComputeCubicHermite(startPoint.m_outputValue, startPoint.m_leaveTangent * diff, endPoint.m_outputValue, endPoint.m_arriveTangent * diff, parametricZeroToOne);
		}
	}
	else
	{
		return startPoint.m_outputValue;
	}
}

float Spline3D::GetInputKeyFromSegment(int index, float parametricZeroToOne) const
{
	CurvePointVec3 const& startPoint = m_position.m_points[index];
	CurvePointVec3 const& endPoint = m_position.m_points[index + 1];

	return Interpolate(startPoint.m_inputKey, endPoint.m_inputKey, parametricZeroToOne);
}

void Spline3D::UpdateSpline()
{
	// #ToDo: rotation?

	m_rotation.AutoSetTangents();

	//-----------------------------------------------------------------------------------------------
	// Recalculate the reparam table
	int numPoints = (int)m_position.m_points.size();

	GUARANTEE_OR_DIE(m_reparamStepsPerSegment > 0, "Invalid subdivisions number!");

	int numSegments = (numPoints >= 1) ? (numPoints - 1) : 0;

	m_reparamTable.Reset(numSegments * m_reparamStepsPerSegment + 1);
	float accumulatedLength = 0.f;
	Vec3 prevPoint;

	// First Point
	if (numPoints > 0)
	{
		prevPoint = m_position.m_points[0].m_outputValue;
		m_reparamTable.m_points.push_back(CurvePointFloat(accumulatedLength, m_position.m_points[0].m_inputKey, 0.f, 0.f, CurveMode::LINEAR));
	}
	else
	{
		m_reparamTable.m_points.push_back(CurvePointFloat(accumulatedLength, 0.f, 0.f, 0.f, CurveMode::LINEAR));
		return;
	}

	for (int segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
	{
		for (int step = 1; step <= m_reparamStepsPerSegment; ++step)
		{
			float parametricZeroToOne = static_cast<float>(step) / static_cast<float>(m_reparamStepsPerSegment);
			Vec3 curPoint = GetPositionFromSegment(segmentIndex, parametricZeroToOne);
			float dist = (curPoint - prevPoint).GetLength();
			float curInputKey = GetInputKeyFromSegment(segmentIndex, parametricZeroToOne);


			accumulatedLength += dist;
			prevPoint = curPoint;

			m_reparamTable.m_points.push_back(CurvePointFloat(accumulatedLength, curInputKey, 0.f, 0.f, CurveMode::LINEAR));
		}
	}

}
