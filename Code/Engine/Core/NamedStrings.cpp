#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <stdlib.h>

void NamedStrings::PopulateFromXmlElementAttributes(XmlElement const& element)
{
	for (tinyxml2::XMLAttribute const* attr = element.FirstAttribute(); attr != nullptr; attr = attr->Next())
	{
		SetValue(attr->Name(), attr->Value());
	}
}


void NamedStrings::SetValue(std::string const& keyName, std::string const& newValue)
{
	m_keyValuePairs[keyName] = newValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, std::string const& defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found == m_keyValuePairs.end())
	{
		return defaultValue;
	}
	return found->second;
}

bool NamedStrings::GetValue(std::string const& keyName, bool defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found == m_keyValuePairs.end())
	{
		return defaultValue;
	}

	std::string const& value = found->second;
	static const std::string TRUE_VALS[] = { "true", "True", "TRUE" };
	static const std::string FALSE_VALS[] = { "false", "False", "FALSE" };

	for (int i = 0; i < 3; ++i)
	{
		if (TRUE_VALS[i] == value)
		{
			return true;
		}
	}

	for (int i = 0; i < 3; ++i)
	{
		if (FALSE_VALS[i] == value)
		{
			return false;
		}
	}

	return defaultValue;
}

int NamedStrings::GetValue(std::string const& keyName, int defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found == m_keyValuePairs.end())
	{
		return defaultValue;
	}
	std::string const& value = found->second;
	return atoi(value.c_str());
}

float NamedStrings::GetValue(std::string const& keyName, float defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found == m_keyValuePairs.end())
	{
		return defaultValue;
	}
	std::string const& value = found->second;
	return static_cast<float>(atof(value.c_str()));
}

std::string NamedStrings::GetValue(std::string const& keyName, char const* defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found == m_keyValuePairs.end())
	{
		return defaultValue;
	}
	return found->second;
}

Rgba8 NamedStrings::GetValue(std::string const& keyName, Rgba8 const& defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found == m_keyValuePairs.end())
	{
		return defaultValue;
	}
	std::string const& value = found->second;
	Rgba8 result;
	result.SetFromText(value.c_str());
	return result;
}

Vec2 NamedStrings::GetValue(std::string const& keyName, Vec2 const& defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found == m_keyValuePairs.end())
	{
		return defaultValue;
	}
	std::string const& value = found->second;
	Vec2 result;
	result.SetFromText(value.c_str());
	return result;
}

IntVec2 NamedStrings::GetValue(std::string const& keyName, IntVec2 const& defaultValue) const
{
	auto found = m_keyValuePairs.find(keyName);
	if (found == m_keyValuePairs.end())
	{
		return defaultValue;
	}
	std::string const& value = found->second;
	IntVec2 result;
	result.SetFromText(value.c_str());
	return result;
}
