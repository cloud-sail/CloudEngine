#pragma once

class IntRange
{
public:
	int m_min = 0;
	int m_max = 0;

public:
	IntRange() = default;
	~IntRange() = default;

	explicit IntRange(int min, int max);

	bool IsOnRange(int value) const;
	bool IsOverlappingWith(IntRange const& other) const;

	void operator=(IntRange const& copyFrom);
	bool operator==(IntRange const& compare) const;
	bool operator!=(IntRange const& compare) const;

	static const IntRange ZERO;
	static const IntRange ONE;
	static const IntRange ZERO_TO_ONE;
};