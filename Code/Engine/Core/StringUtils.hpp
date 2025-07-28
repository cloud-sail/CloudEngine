#pragma once
//-----------------------------------------------------------------------------------------------
#include <string>
#include <vector>

//-----------------------------------------------------------------------------------------------
const std::string Stringf( char const* format, ... );
const std::string Stringf( int maxLength, char const* format, ... );
const std::wstring WStringf(const wchar_t* format, ...);

std::wstring ToWString(char const* c);
//-----------------------------------------------------------------------------------------------
typedef std::vector< std::string >		Strings;

Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterToSplitOn);
Strings SplitStringOnDelimiterAndDiscardEmpty(std::string const& originalString, char delimiterToSplitOn);

void TrimSpace(std::string& s);
void TrimSpaceInStrings(Strings& strings);
