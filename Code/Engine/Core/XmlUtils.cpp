#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"

int ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue)
{
    return element.IntAttribute(attributeName, defaultValue);
}

char ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue)
{
    char const* result = element.Attribute(attributeName);
    if (result == nullptr)
    {
        return defaultValue;
    }
    return *result;
}

bool ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue)
{
	// static const char* TRUE_VALS[] = { "true", "True", "TRUE", 0 };
	// static const char* FALSE_VALS[] = { "false", "False", "FALSE", 0 };
    return element.BoolAttribute(attributeName, defaultValue);
}

float ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue)
{
    return element.FloatAttribute(attributeName, defaultValue);
}

Rgba8 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue)
{
    const char* text = element.Attribute(attributeName);
    if (text == nullptr)
    {
        return defaultValue;
    }
    Rgba8 result;
    result.SetFromText(text);
    return result;
}

Vec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}
	Vec2 result;
	result.SetFromText(text);
	return result;
}

IntVec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}
	IntVec2 result;
	result.SetFromText(text);
	return result;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}
    return std::string(text);
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return std::string(defaultValue);
	}
	return std::string(text);
}

Strings ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValues;
	}
	Strings result = SplitStringOnDelimiter(text, ',');
    return result;
}

Vec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}
	Vec3 result;
	result.SetFromText(text);
	return result;
}

EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}
	EulerAngles result;
	result.SetFromText(text);
	return result;
}

FloatRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange const& defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}
	FloatRange result;
	result.SetFromText(text);
	return result;
}

