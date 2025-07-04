#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/EngineCommon.hpp"

const FloatRange FloatRange::ZERO = FloatRange();
const FloatRange FloatRange::ONE = FloatRange(1.f, 1.f);
const FloatRange FloatRange::ZERO_TO_ONE = FloatRange(0.f, 1.f);

FloatRange::FloatRange(float min, float max)
	: m_min(min)
	, m_max(max)
{
}

void FloatRange::SetFromText(char const* text)
{
	Strings tokens = SplitStringOnDelimiter(text, '~');
	if (tokens.size() != 2)
	{
		ERROR_AND_DIE(Stringf("Wrong text format for FloatRange! text: %s", text));
	}
	m_min = static_cast<float>(atof(tokens[0].c_str()));
	m_max = static_cast<float>(atof(tokens[1].c_str()));
}

void FloatRange::StretchToIncludeValue(float value)
{
	if (value < m_min)
	{
		m_min = value;
	}
	if (value > m_max)
	{
		m_max = value;
	}
}

bool FloatRange::IsOnRange(float value) const
{
	return value >= m_min && value <= m_max;
}

bool FloatRange::IsOverlappingWith(FloatRange const& other) const
{
	return !(other.m_min >= m_max || other.m_max <= m_min);
}

void FloatRange::operator=(FloatRange const& copyFrom)
{
	m_min = copyFrom.m_min;
	m_max = copyFrom.m_max;
}

bool FloatRange::operator==(FloatRange const& compare) const
{
	return m_min == compare.m_min && m_max == compare.m_max;
}

bool FloatRange::operator!=(FloatRange const& compare) const
{
	return m_min != compare.m_min || m_max != compare.m_max;
}
