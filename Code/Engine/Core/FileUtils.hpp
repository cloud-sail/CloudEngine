#pragma once
#include <vector>
#include <string>


int FileReadToBuffer(std::vector<uint8_t>& outBuffer, const std::string& fileName);
int FileReadToString(std::string& outString, const std::string& fileName);

bool FileExists(std::string const& filename);
int FileWriteFromBuffer(std::vector<uint8_t> const& inBuffer, const std::string& filename);

bool EnsureDirectoryExists(std::string const& dirPath);
