#pragma once
#include <string>


class HashedCaseInsensitiveString
{
public:
	HashedCaseInsensitiveString() = default;
	HashedCaseInsensitiveString(HashedCaseInsensitiveString const& copyFrom) = default;
	HashedCaseInsensitiveString(char const* text);
	HashedCaseInsensitiveString(std::string const& text);

	static unsigned int CalHashForText(char const* text);
	static unsigned int CalHashForText(std::string const& text);

	// < == != == HashedCaseInsensitiveString
	// == != string

	unsigned int GetHash() const { return m_lowerCaseHash; }
	std::string const& GetOriginalString() const { return m_caseIntactText; }
	char const* c_str() const { return m_caseIntactText.c_str(); }

	void operator=(HashedCaseInsensitiveString const& assignFrom);
	void operator=(char const* text);
	void operator=(std::string const& text);

	bool operator==(HashedCaseInsensitiveString const& compare) const;
	bool operator!=(HashedCaseInsensitiveString const& compare) const;
	bool operator<(HashedCaseInsensitiveString const& compare) const;

	bool operator==(char const* text) const;
	bool operator!=(char const* text) const;
	bool operator==(std::string const& text) const;
	bool operator!=(std::string const& text) const;



private:
	std::string		m_caseIntactText;
	unsigned int	m_lowerCaseHash = 0;
};
