#pragma once

#include <string>
#include <vector>
#include <exception>

#include "Platform.h"

namespace utils
{

	class UTILS_API FileSystem
	{
	public:

		//
		// Generic exception class
		//
		class FileException : public std::exception
		{
		public:

			explicit FileException(std::string const& message)
				: exception(message.c_str())
			{
			}
		};

		//
		// Class for holding info about a file
		//
		class FileInfo
		{
			std::string mFilepath;

		public:

			explicit FileInfo(std::string const& filepath);

			std::string const& getFilePath() const;

			std::string getPath() const;

			std::string getFileName() const;

			std::string getFileNameWithoutExtension() const;

			std::string getExtension() const;
		};

		//
		// Class for holding info about a directory
		//
		class DirectoryInfo
		{
			std::string mPath;

		public:

			explicit DirectoryInfo(std::string const& path);

			std::string const& getDirectoryPath() const;

			FileInfo createFile(std::string const& filename);

			DirectoryInfo createSubDirectory(std::string const& subdir);
		};

	public:

		static bool matchesFilePattern(std::string const& input, std::string const& pattern);

		static void standardisePath(std::string& path);

		static std::string standardisePath(std::string const& path);

		static FileInfo createFile(std::string const& filepath);

		static DirectoryInfo createDirectory(std::string const& dirpath);

		static void deleteFile(std::string const& filepath);

		static void deleteFile(FileInfo const& fi);

		static void deleteDirectory(std::string const& dirpath);

		static void deleteDirectory(DirectoryInfo const& di);

		static FileInfo getFile(std::string const& filepath);

		static DirectoryInfo getDirectory(std::string const& dirpath);

		static DirectoryInfo getCurrentDirectory();

		static std::vector<FileInfo> getFilesInDirectory(std::string const& dir, std::string const& pattern, bool subdirs);

		static std::vector<FileInfo> getFilesInDirectory(DirectoryInfo const& di, std::string const& pattern, bool subdirs);

		static bool fileExists(std::string const& filepath);

		static bool fileExists(FileInfo const& fi);

		static bool directoryExists(std::string const& dirpath);

		static bool directoryExists(DirectoryInfo const& di);

		static std::string concatPaths(std::string const& path1, std::string const& path2);

		static std::string concatPaths(std::vector<std::string> const& paths);
	};

} // utils
