#pragma once
#include "Engine/Core/HashedCaseInsensitiveString.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include <map>
#include <string>

struct Vec2;
struct Vec3;
struct IntVec2;
struct Rgba8;
struct EulerAngles;

//-----------------------------------------------------------------------------------------------
// Typed-erased base class fir storing any value type
//-----------------------------------------------------------------------------------------------
class TypedPropertyBase
{
	friend class NamedProperties;
public:
	virtual ~TypedPropertyBase() {}
	virtual TypedPropertyBase* Clone() const = 0;
};

//-----------------------------------------------------------------------------------------------
// Concrete typed property holding a value
//-----------------------------------------------------------------------------------------------
template<typename T>
class TypedProperty : public TypedPropertyBase
{
	friend class NamedProperties;
public:
	TypedProperty(T const& value) : m_data(value) {}

	virtual TypedPropertyBase* Clone() const override
	{
		return new TypedProperty<T>(m_data);
	}

	T m_data;
};


//-----------------------------------------------------------------------------------------------
// 
class NamedProperties
{
public:
	NamedProperties() = default;
	explicit NamedProperties(std::map<std::string, std::string> const& initialMap);
	~NamedProperties();

	NamedProperties(NamedProperties const& other);
	NamedProperties& operator=(NamedProperties const& other);

	template<typename T>
	void SetValue(std::string const& keyName, T const& newValue);

	void SetValue(std::string const& keyName, char const* newValue);

	template<typename T>
	T GetValue(std::string const& keyName, T const& defaultValue) const;

	std::string GetValue(std::string const& keyName, char const* defaultValue) const;

	void PopulateFromXmlElementAttributes(XmlElement const& element);

	bool HasKey(std::string const& keyName) const;
	void Clear();

	std::map<std::string, std::string> GetAllStringKeyValuePairs() const;

private:
	std::map<HashedCaseInsensitiveString, TypedPropertyBase*> m_keyValuePairs;

	template<typename T>
	static bool TryParseStringAs(std::string const& text, T& out_value);
};

//-----------------------------------------------------------------------------------------------
// Generic fallback: unknown type, can't parse
//-----------------------------------------------------------------------------------------------
template<typename T>
bool NamedProperties::TryParseStringAs(std::string const& text, T& out_value)
{
	(void)text;
	(void)out_value;
	return false;
}

template<> bool NamedProperties::TryParseStringAs<std::string>(std::string const&, std::string&);
template<> bool NamedProperties::TryParseStringAs<int>(std::string const&, int&);
template<> bool NamedProperties::TryParseStringAs<float>(std::string const&, float&);
template<> bool NamedProperties::TryParseStringAs<bool>(std::string const&, bool&);
template<> bool NamedProperties::TryParseStringAs<Rgba8>(std::string const&, Rgba8&);
template<> bool NamedProperties::TryParseStringAs<Vec2>(std::string const&, Vec2&);
template<> bool NamedProperties::TryParseStringAs<Vec3>(std::string const&, Vec3&);
template<> bool NamedProperties::TryParseStringAs<IntVec2>(std::string const&, IntVec2&);
template<> bool NamedProperties::TryParseStringAs<EulerAngles>(std::string const&, EulerAngles&);


template<typename T>
void NamedProperties::SetValue(std::string const& keyName, T const& newValue)
{
	HashedCaseInsensitiveString hashedKey(keyName);
	auto found = m_keyValuePairs.find(hashedKey);
	if (found == m_keyValuePairs.end())
	{
		m_keyValuePairs[hashedKey] = new TypedProperty<T>(newValue);
	}
	else
	{
		TypedProperty<T>* asMatchingType = dynamic_cast<TypedProperty<T>*>(found->second);
		if (asMatchingType)
		{
			asMatchingType->m_data = newValue;
		}
		else
		{
			delete found->second;
			found->second = new TypedProperty<T>(newValue);
		}
	}
}

template<typename T>
T NamedProperties::GetValue(std::string const& keyName, T const& defaultValue) const
{
	HashedCaseInsensitiveString hashedKey(keyName);
	auto found = m_keyValuePairs.find(hashedKey);
	if (found != m_keyValuePairs.end())
	{
		// First try exact type match
		TypedProperty<T>* asMatchingType = dynamic_cast<TypedProperty<T>*>(found->second);
		if (asMatchingType)
		{
			return asMatchingType->m_data;
		}

		// Backward compatibility: if stored as std::string, try to parse as T
		TypedProperty<std::string>* asString = dynamic_cast<TypedProperty<std::string>*>(found->second);
		if (asString)
		{
			T parsedValue;
			if (TryParseStringAs<T>(asString->m_data, parsedValue))
			{
				return parsedValue;
			}
		}
	}

	return defaultValue;
}


