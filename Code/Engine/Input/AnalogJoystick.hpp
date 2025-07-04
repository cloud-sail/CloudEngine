#pragma once
#include "Engine/Math/Vec2.hpp"


class AnalogJoystick
{
public:
	Vec2	GetPosition() const;
	float	GetMagnitude() const;
	float	GetOrientationDegrees() const;

	Vec2	GetRawUncorrectedPosition() const;
	float	GetInnerDeadZoneFraction() const;
	float	GetOuterDeadZoneFraction() const;

	// For use by XboxController, et al.
	void	Reset();
	void	SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold);
	void	UpdatePosition(float rawNormalizedX, float rawNormalizedY);

protected:
	Vec2 m_rawPosition; // Unreliable; does not rest at zero (or consistently snap tp rest position)
	Vec2 m_correctedPosition; // Deadzone-corrected position
	float m_innerDeadzoneFraction = 0.3f; // if R < this (0-1), R = 0; "input range start" for corrective range map
	float m_outerDeadzoneFraction = 0.95f; // if R < this (0-1), R = 0; "input range end" for corrective range map
};

