#pragma once
#include "Engine/Core/XmlUtils.hpp"
#include <map>
#include <string>


//-----------------------------------------------------------------------------------------------
struct Rgba8;
struct Vec2;
struct IntVec2;

//-----------------------------------------------------------------------------------------------

class NamedStrings
{
public:
	NamedStrings() = default;
	explicit NamedStrings(std::map<std::string, std::string>const& initialMap)
		: m_keyValuePairs(initialMap)
	{
	}

	void			PopulateFromXmlElementAttributes( XmlElement const& element);
	void			SetValue(std::string const& keyName, std::string const& newValue);
	std::string		GetValue(std::string const& keyName, std::string const& defaultValue) const;
	bool			GetValue(std::string const& keyName, bool defaultValue) const;
	int				GetValue(std::string const& keyName, int defaultValue) const;
	float			GetValue(std::string const& keyName, float defaultValue) const;
	std::string		GetValue(std::string const& keyName, char const* defaultValue) const;
	Rgba8			GetValue(std::string const& keyName, Rgba8 const& defaultValue) const;
	Vec2			GetValue(std::string const& keyName, Vec2 const& defaultValue) const;
	IntVec2			GetValue(std::string const& keyName, IntVec2 const& defaultValue) const;


public:
	std::map<std::string, std::string> m_keyValuePairs;
};

