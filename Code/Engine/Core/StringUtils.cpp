#include "Engine/Core/StringUtils.hpp"
#include <stdarg.h>


//-----------------------------------------------------------------------------------------------
constexpr int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;


//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... )
{
	char textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ STRINGF_STACK_LOCAL_TEMP_LENGTH - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	return std::string( textLiteral );
}


//-----------------------------------------------------------------------------------------------
const std::string Stringf( int maxLength, char const* format, ... )
{
	char textLiteralSmall[ STRINGF_STACK_LOCAL_TEMP_LENGTH ];
	char* textLiteral = textLiteralSmall;
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		textLiteral = new char[ maxLength ];

	va_list variableArgumentList;
	va_start( variableArgumentList, format );
	vsnprintf_s( textLiteral, maxLength, _TRUNCATE, format, variableArgumentList );	
	va_end( variableArgumentList );
	textLiteral[ maxLength - 1 ] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

	std::string returnValue( textLiteral );
	if( maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH )
		delete[] textLiteral;

	return returnValue;
}

Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterToSplitOn)
{
	Strings result;
	size_t start = 0;
	size_t found = originalString.find(delimiterToSplitOn);
	while (found != std::string::npos)
	{
		result.emplace_back(originalString.substr(start, found - start));
		start = found + 1; // add length of delimiter
		found = originalString.find(delimiterToSplitOn, start);
	}
	result.emplace_back(originalString.substr(start));

	return result;
}

Strings SplitStringOnDelimiterAndDiscardEmpty(std::string const& originalString, char delimiterToSplitOn)
{
	Strings result;
	size_t start = 0;
	size_t found = originalString.find(delimiterToSplitOn);
	while (found != std::string::npos)
	{
		std::string substring = originalString.substr(start, found - start);
		if (!substring.empty())
		{
			result.push_back(substring); // std::move? emplace_back?
		}
		start = found + 1; // add length of delimiter
		found = originalString.find(delimiterToSplitOn, start);
	}
	std::string lastSubstring = originalString.substr(start);
	if (!lastSubstring.empty())
	{
		result.push_back(lastSubstring);
	}

	return result;
}

void TrimSpace(std::string& s)
{
	const std::string whiteSpaces = " \f\n\r\t\v";
	size_t start = s.find_first_not_of(whiteSpaces);

	if (start == std::string::npos)
	{
		s.erase();
		return;
	}

	size_t end = s.find_last_not_of(whiteSpaces);
	s = s.substr(start, end - start + 1);
}

void TrimSpaceInStrings(Strings& strings)
{
	for (int i = 0; i < (int)strings.size(); ++i)
	{
		TrimSpace(strings[i]);
	}
}






