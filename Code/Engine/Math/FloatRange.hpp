#pragma once
struct FloatRange
{
public:
	float m_min = 0.f;
	float m_max = 0.f;

public:
	FloatRange() = default;
	~FloatRange() = default;
	explicit FloatRange(float min, float max);


	// Mutators (non-const methods)
	void SetFromText(char const* text);
	void StretchToIncludeValue(float value);

	bool IsOnRange(float value) const;
	bool IsOverlappingWith(FloatRange const& other) const;

	void operator= (FloatRange const& copyFrom);
	bool operator== (FloatRange const& compare) const;
	bool operator!= (FloatRange const& compare) const;

	static const FloatRange ZERO;
	static const FloatRange ONE;
	static const FloatRange ZERO_TO_ONE;
};

