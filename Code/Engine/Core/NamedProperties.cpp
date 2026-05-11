#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/EulerAngles.hpp"

NamedProperties::NamedProperties(std::map<std::string, std::string> const& initialMap)
{
	for (auto const& pair : initialMap)
	{
		SetValue(pair.first, pair.second);
	}
}

NamedProperties::NamedProperties(NamedProperties const& other)
{
	for (auto const& pair : other.m_keyValuePairs)
	{
		m_keyValuePairs[pair.first] = pair.second->Clone();
	}
}

NamedProperties& NamedProperties::operator=(NamedProperties const& other)
{
	if (this == &other) return *this;
	Clear();
	for (auto const& pair : other.m_keyValuePairs)
	{
		m_keyValuePairs[pair.first] = pair.second->Clone();
	}
	return *this;
}

NamedProperties::~NamedProperties()
{
	Clear();
}

void NamedProperties::SetValue(std::string const& keyName, char const* newValue)
{
	SetValue<std::string>(keyName, std::string(newValue));
}

std::string NamedProperties::GetValue(std::string const& keyName, char const* defaultValue) const
{
	return GetValue<std::string>(keyName, std::string(defaultValue));
}

void NamedProperties::PopulateFromXmlElementAttributes(XmlElement const& element)
{
	for (tinyxml2::XMLAttribute const* attr = element.FirstAttribute(); attr != nullptr; attr = attr->Next())
	{
		SetValue(attr->Name(), attr->Value());
	}
}

bool NamedProperties::HasKey(std::string const& keyName) const
{
	HashedCaseInsensitiveString hashedKey(keyName);
	return m_keyValuePairs.find(hashedKey) != m_keyValuePairs.end();
}

void NamedProperties::Clear()
{
	for (auto& pair : m_keyValuePairs)
	{
		delete pair.second;
		pair.second = nullptr;
	}
	m_keyValuePairs.clear();
}

std::map<std::string, std::string> NamedProperties::GetAllStringKeyValuePairs() const
{
	std::map<std::string, std::string> result;
	for (auto const& pair : m_keyValuePairs)
	{
		TypedProperty<std::string>* asString = dynamic_cast<TypedProperty<std::string>*>(pair.second);
		if (asString)
		{
			result[pair.first.GetOriginalString()] = asString->m_data;
		}
	}
	return result;
}

//===============================================================================================
// TryParseStringAs specializations
// Style matches NamedStrings: atoi, atof, SetFromText, no try-catch
//===============================================================================================

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<std::string>(std::string const& text, std::string& out_value)
{
	out_value = text;
	return true;
}

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<int>(std::string const& text, int& out_value)
{
	out_value = atoi(text.c_str());
	return true;
}

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<float>(std::string const& text, float& out_value)
{
	out_value = static_cast<float>(atof(text.c_str()));
	return true;
}

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<bool>(std::string const& text, bool& out_value)
{
	static const std::string TRUE_VALS[] = { "true", "True", "TRUE" };
	static const std::string FALSE_VALS[] = { "false", "False", "FALSE" };
	for (int i = 0; i < 3; ++i)
	{
		if (TRUE_VALS[i] == text)
		{
			out_value = true;
			return true;
		}
	}
	for (int i = 0; i < 3; ++i)
	{
		if (FALSE_VALS[i] == text)
		{
			out_value = false;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<Rgba8>(std::string const& text, Rgba8& out_value)
{
	out_value.SetFromText(text.c_str());
	return true;
}

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<Vec2>(std::string const& text, Vec2& out_value)
{
	out_value.SetFromText(text.c_str());
	return true;
}

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<Vec3>(std::string const& text, Vec3& out_value)
{
	out_value.SetFromText(text.c_str());
	return true;
}

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<IntVec2>(std::string const& text, IntVec2& out_value)
{
	out_value.SetFromText(text.c_str());
	return true;
}

//-----------------------------------------------------------------------------------------------
template<>
bool NamedProperties::TryParseStringAs<EulerAngles>(std::string const& text, EulerAngles& out_value)
{
	out_value.SetFromText(text.c_str());
	return true;
}
