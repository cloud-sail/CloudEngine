#include "Engine/Core/HashedCaseInsensitiveString.hpp"

HashedCaseInsensitiveString::HashedCaseInsensitiveString(char const* text)
	: m_caseIntactText(text)
	, m_lowerCaseHash(CalHashForText(text))
{

}

HashedCaseInsensitiveString::HashedCaseInsensitiveString(std::string const& text)
	: m_caseIntactText(text)
	, m_lowerCaseHash(CalHashForText(text))
{

}

unsigned int HashedCaseInsensitiveString::CalHashForText(char const* text)
{
	unsigned int hash = 0;

	for (char const* scan = text; *scan != '\0'; ++scan)
	{
		hash *= 31;

		hash += static_cast<unsigned int>(std::tolower(*scan));
	}

	return hash;
}

unsigned int HashedCaseInsensitiveString::CalHashForText(std::string const& text)
{
	return CalHashForText(text.c_str());
}

//-----------------------------------------------------------------------------------------------
void HashedCaseInsensitiveString::operator=(HashedCaseInsensitiveString const& assignFrom)
{
	m_caseIntactText = assignFrom.m_caseIntactText;
	m_lowerCaseHash = assignFrom.m_lowerCaseHash;
}

void HashedCaseInsensitiveString::operator=(char const* text)
{
	m_caseIntactText = text;
	m_lowerCaseHash = CalHashForText(text);
}

void HashedCaseInsensitiveString::operator=(std::string const& text)
{
	m_caseIntactText = text;
	m_lowerCaseHash = CalHashForText(text);
}

//-----------------------------------------------------------------------------------------------
bool HashedCaseInsensitiveString::operator!=(HashedCaseInsensitiveString const& compare) const
{
	return !(*this == compare);
}

bool HashedCaseInsensitiveString::operator==(HashedCaseInsensitiveString const& compare) const
{
	if (m_lowerCaseHash != compare.m_lowerCaseHash)
		return false;
	return _stricmp(m_caseIntactText.c_str(), compare.m_caseIntactText.c_str()) == 0;
}

bool HashedCaseInsensitiveString::operator<(HashedCaseInsensitiveString const& compare) const
{
	if (m_lowerCaseHash != compare.m_lowerCaseHash)
		return m_lowerCaseHash < compare.m_lowerCaseHash;
	return _stricmp(m_caseIntactText.c_str(), compare.m_caseIntactText.c_str()) < 0;
}

bool HashedCaseInsensitiveString::operator!=(std::string const& text) const
{
	return !(*this == text);
}

bool HashedCaseInsensitiveString::operator==(std::string const& text) const
{
	return *this == HashedCaseInsensitiveString(text);
}

bool HashedCaseInsensitiveString::operator!=(char const* text) const
{
	return !(*this == text);
}

bool HashedCaseInsensitiveString::operator==(char const* text) const
{
	return *this == HashedCaseInsensitiveString(text);
}

