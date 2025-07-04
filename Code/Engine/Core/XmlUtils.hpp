#pragma once
#include "Engine/Core/StringUtils.hpp"
#include "ThirdParty/TinyXML2/tinyxml2.h"
#include <string>

//-----------------------------------------------------------------------------------------------
struct Rgba8;
struct Vec2;
struct Vec3;
struct IntVec2;
struct EulerAngles;
struct FloatRange;

//-----------------------------------------------------------------------------------------------
// Generic (and shorter) nicknames for tinyxml2 types; allows us to pivot to a different
// library later if need be.
typedef tinyxml2::XMLDocument	XmlDocument;
typedef tinyxml2::XMLElement	XmlElement;
typedef tinyxml2::XMLAttribute	XmlAttribute;
typedef tinyxml2::XMLError		XmlResult;


int ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue);
char ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue);
bool ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue);
float ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue);
Rgba8 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue);
Vec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue);
IntVec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue);
Vec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue);
EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue);
FloatRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange const& defaultValue);
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue);
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue);
Strings ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues);


/*
Example TinyXML2

	XmlDocument doc;
	XmlResult result = doc.LoadFile("example.xml");
	if (result != tinyxml2::XML_SUCCESS)
	{
		//ERROR
	}

	XmlElement* root = doc.RootElement();
	if (root == nullptr)
	{
		//ERROR
	}

	char const* elementName = "Book";
	for (XmlElement* element = root->FirstChildElement(elementName); element != nullptr; element = element->NextSiblingElement(elementName))
	{
		// Attribute
		char const* attributeName = "id";
		char const* idCString = element->Attribute(attributeName);

		// QueryXXXAttribute
		int id = -1; // defaultValue
		element->QueryIntAttribute(attributeName, &id);

		// XXXAttribute
		int id = element->IntAttribute(attributeName, -1);
	}
*/

