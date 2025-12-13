#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include <stdio.h>
#include <filesystem>
#include <fstream>

int FileReadToBuffer(std::vector<uint8_t>& outBuffer, const std::string& fileName)
{
	errno_t err;
	FILE* fp;
	err = fopen_s(&fp, fileName.c_str(), "rb");
	if (err != 0)
	{
		ERROR_RECOVERABLE("Could not open file.");
		return -1;
	}

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		ERROR_RECOVERABLE("Fseek failed");
		fclose(fp);
		return -1;
	}

	long fileSize = ftell(fp);
	outBuffer.resize(fileSize);

	rewind(fp);
	size_t result = fread(outBuffer.data(), sizeof(uint8_t), outBuffer.size(), fp);

	if (result != outBuffer.size())
	{
		if (feof(fp))
		{
			ERROR_RECOVERABLE("Unexpected end of file");
		}
		else if (ferror(fp))
		{
			ERROR_RECOVERABLE("Error reading file");
		}
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return static_cast<int>(result);
}

int FileReadToString(std::string& outString, const std::string& fileName)
{
	std::vector<uint8_t> buffer;
	int result = FileReadToBuffer(buffer, fileName);

	//buffer.push_back('\0');
	outString = std::string(buffer.begin(), buffer.end());

	return result;
}

bool FileExists(std::string const& filename)
{
	return std::filesystem::exists(filename);
}

int FileWriteFromBuffer(std::vector<uint8_t> const& inBuffer, const std::string& filename)
{
	std::ofstream ofs(filename, std::ios::binary | std::ios::out | std::ios::trunc);
	if (!ofs.is_open())
	{
		ERROR_RECOVERABLE("Could not open file.");
		return -1;
	}

	if (!inBuffer.empty())
	{
		ofs.write(reinterpret_cast<const char*>(inBuffer.data()), static_cast<std::streamsize>(inBuffer.size()));
		if (!ofs.good())
		{
			ERROR_RECOVERABLE("Could not write file.");
			return -2;
		}
	}
	ofs.close();
	return static_cast<int>(inBuffer.size());
}

bool EnsureDirectoryExists(std::string const& dirPath)
{
	namespace fs = std::filesystem;
	std::error_code ec;
	if (dirPath.empty())
		return false;
	fs::path p = fs::u8path(dirPath);
	if (fs::exists(p, ec))
		return fs::is_directory(p, ec);
	return fs::create_directories(p, ec);
}

