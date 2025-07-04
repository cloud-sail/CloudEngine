#include "Engine/Math/IntRange.hpp"

const IntRange IntRange::ZERO = IntRange();
const IntRange IntRange::ONE = IntRange(1, 1);
const IntRange IntRange::ZERO_TO_ONE = IntRange(0, 1);

IntRange::IntRange(int min, int max)
	: m_min(min)
	, m_max(max)
{
}

bool IntRange::IsOnRange(int value) const
{
	return value >= m_min && value <= m_max;
}

bool IntRange::IsOverlappingWith(const IntRange& other) const
{
	return !(other.m_min > m_max || other.m_max < m_min);
}

void IntRange::operator=(const IntRange& copyFrom)
{
	m_min = copyFrom.m_min;
	m_max = copyFrom.m_max;
}

bool IntRange::operator==(const IntRange& compare) const
{
	return m_min == compare.m_min && m_max == compare.m_max;
}

bool IntRange::operator!=(const IntRange& compare) const
{
	return m_min != compare.m_min || m_max != compare.m_max;
}