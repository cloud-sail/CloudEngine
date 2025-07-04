#include "Engine/Input/AnalogJoystick.hpp"
#include "Engine/Math/MathUtils.hpp"

Vec2 AnalogJoystick::GetPosition() const
{
	return m_correctedPosition;
}

float AnalogJoystick::GetMagnitude() const
{
	return m_correctedPosition.GetLength();
}

float AnalogJoystick::GetOrientationDegrees() const
{
	return m_correctedPosition.GetOrientationDegrees();
}

Vec2 AnalogJoystick::GetRawUncorrectedPosition() const
{
	return m_rawPosition;
}

float AnalogJoystick::GetInnerDeadZoneFraction() const
{
	return m_innerDeadzoneFraction;
}

float AnalogJoystick::GetOuterDeadZoneFraction() const
{
	return m_outerDeadzoneFraction;
}

void AnalogJoystick::Reset()
{
	// Unused Function
	// Now the deadZone is fixed
	m_rawPosition = Vec2();
	m_correctedPosition = Vec2();
	m_innerDeadzoneFraction = 0.00f;
	m_outerDeadzoneFraction = 1.00f;
}

void AnalogJoystick::SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold)
{
	m_innerDeadzoneFraction = normalizedInnerDeadzoneThreshold;
	m_outerDeadzoneFraction = normalizedOuterDeadzoneThreshold;
}

void AnalogJoystick::UpdatePosition(float rawNormalizedX, float rawNormalizedY)
{
	// rawNormalized : RangeMap(shortRawX, -32768.f, 32767.f, -1.f, 1.f)
	// raw float X, Y[-1.f, 1.f]
	m_rawPosition.x = rawNormalizedX;
	m_rawPosition.y = rawNormalizedY;
	// raw polar
	float rawRadius = m_rawPosition.GetLength();
	float rawDegrees = m_rawPosition.GetOrientationDegrees();
	// corrected polar R [0.f, 1.f]
	float correctedRadius = RangeMapClamped(rawRadius, m_innerDeadzoneFraction, m_outerDeadzoneFraction, 0.f, 1.f);
	float correctedDegrees = rawDegrees;
	m_correctedPosition.x = correctedRadius * CosDegrees(correctedDegrees);
	m_correctedPosition.y = correctedRadius * SinDegrees(correctedDegrees);
}

