#include <algorithm>
#include <sstream>
#include <numeric>
#include <regex>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "FileSystem.h"
#include "StringUtils.h"

namespace utils
{

	using namespace std;
	using namespace boost::filesystem;

	//
	// FileInfo
	//
	FileSystem::FileInfo::FileInfo(string const& filepath)
		: mFilepath(standardisePath(filepath))
	{
	}

	string const& FileSystem::FileInfo::getFilePath() const
	{
		return mFilepath;
	}

	string FileSystem::FileInfo::getPath() const
	{
		return standardisePath(path(mFilepath).relative_path().string());
	}

	string FileSystem::FileInfo::getFileName() const
	{
		return path(mFilepath).filename().string();
	}

	string FileSystem::FileInfo::getFileNameWithoutExtension() const
	{
		return path(mFilepath).stem().string();
	}

	string FileSystem::FileInfo::getExtension() const
	{
		return path(mFilepath).extension().string();
	}
	
	//
	// DirectoryInfo
	//
	FileSystem::DirectoryInfo::DirectoryInfo(string const& path)
		: mPath(standardisePath(path))
	{
	}

	string const& FileSystem::DirectoryInfo::getDirectoryPath() const
	{
		return mPath;
	}

	FileSystem::FileInfo FileSystem::DirectoryInfo::createFile(string const& filename)
	{
		string filepath = FileSystem::concatPaths(mPath, filename);
		return FileSystem::createFile(filepath);
	}

	FileSystem::DirectoryInfo FileSystem::DirectoryInfo::createSubDirectory(string const& subdir)
	{
		string dirpath = FileSystem::concatPaths(mPath, FileSystem::standardisePath(subdir));
		return FileSystem::createDirectory(dirpath);
	}

	//
	// FileSystem
	//
	void FileSystem::standardisePath(string& path)
	{
		replace(path.begin(), path.end(), '\\', '/');

		// Replace duplicate '/'s
		struct both_slashes
		{
			bool operator()(char a, char b) const
			{
				return a == '/' && b == '/';
			}
		};

		path.erase(unique(path.begin(), path.end(), both_slashes()), path.end());
	}

	string FileSystem::standardisePath(string const& path)
	{
		string ret = path;

		standardisePath(ret);
		return ret;
	}

	bool FileSystem::matchesFilePattern(string const& input, string const& pattern)
	{
		string fixedPattern = pattern;

		boost::replace_all(fixedPattern, ".", "\\.");
		boost::replace_all(fixedPattern, "*", ".*");
		boost::replace_all(fixedPattern, "?", ".");

		regex re = regex(fixedPattern, regex_constants::icase);
		return regex_search(input, re);
	}

	FileSystem::FileInfo FileSystem::createFile(string const& filepath)
	{
		boost::filesystem::ofstream fp;
			
		const string standardisedPath = standardisePath(filepath);
		fp.open(path(standardisedPath), ios_base::trunc);

		if (!fp.is_open())
		{
			throw FileException("Could not create file '" + standardisedPath + "'.");
		}
			
		fp.close();
		return FileInfo(standardisePath(standardisedPath));
	}

	FileSystem::DirectoryInfo FileSystem::createDirectory(string const& dirpath)
	{
		const string standardisedPath = standardisePath(dirpath);
		if (!directoryExists(standardisedPath))
		{
			if (!create_directory(path(standardisedPath)))
			{
				throw FileException("Could not create directory '" + standardisedPath + "'.");
			}
		}

		return DirectoryInfo(standardisedPath);
	}

	void FileSystem::deleteFile(string const& filepath)
	{
		const string standardisedPath = standardisePath(filepath);
		if (!boost::filesystem::remove(path(standardisedPath)))
		{
			throw FileException("Could not delete '" + standardisedPath + "'.");
		}
	}

	void FileSystem::deleteFile(FileInfo const& fi)
	{
		deleteFile(fi.getFilePath());
	}

	void FileSystem::deleteDirectory(string const& dirpath)
	{
		const string standardisedPath = standardisePath(dirpath);
		if (!boost::filesystem::remove_all(path(standardisedPath)))
		{
			throw FileException("Could not delete '" + standardisedPath + "'.");
		}
	}

	void FileSystem::deleteDirectory(DirectoryInfo const& di)
	{
		deleteDirectory(di.getDirectoryPath());
	}

	FileSystem::FileInfo FileSystem::getFile(string const& filepath)
	{
		const string standardisedPath = standardisePath(filepath);

		path p(standardisedPath);

		if (!exists(p))
		{
			throw FileException("File '" + standardisedPath + "' not found.");
		}
		else
		{
			if (!is_regular_file(p))
			{
				throw FileException("'" + standardisedPath + "' is not a file.");
			}

			return FileInfo(standardisedPath);
		}
	}

	FileSystem::DirectoryInfo FileSystem::getDirectory(string const& dirpath)
	{
		const string standardisedPath = standardisePath(dirpath);
		path p(standardisedPath);

		if (!exists(p))
		{
			throw FileException("Directory '" + standardisedPath + "' not found.");
		}
		else
		{
			if (!is_directory(p))
			{
				throw FileException("'" + standardisedPath + "' is not a directory.");
			}

			return DirectoryInfo(standardisedPath);
		}
	}

	FileSystem::DirectoryInfo FileSystem::getCurrentDirectory()
	{
		return DirectoryInfo(standardisePath(current_path().string()));
	}

	vector<FileSystem::FileInfo> FileSystem::getFilesInDirectory(string const& dir, string const& pattern, bool subdirs)
	{
		vector<FileInfo> files;

		path dirPath(dir);
		if (!is_directory(dirPath))
		{
			throw FileException("Path '" + dir + "' is not a directory.");
		}

		// Split patterns
		auto patterns = StringUtils::split(pattern, "|");

		if (subdirs)
		{
			recursive_directory_iterator it(dirPath), end;
			while (it != end)
			{
				path entryPath = it->path();

				if (is_regular_file(entryPath))
				{
					// Ignore anything that doesn't match pattern
					string filename = entryPath.filename().string();

					for (auto const& p: patterns)
					{
						if (matchesFilePattern(filename, p))
						{
							files.push_back(FileInfo(standardisePath(entryPath.string())));
						}
						break;
					}
				}

				++it;
			}
		}
		else
		{
			directory_iterator it(dirPath), end;
			while (it != end)
			{
				path entryPath = it->path();

				if (is_regular_file(entryPath))
				{
					// Ignore anything that doesn't match pattern
					string filename = entryPath.filename().string();
					if (matchesFilePattern(filename, pattern))
					{
						files.push_back(FileInfo(standardisePath(entryPath.string())));
					}
				}

				++it;
			}
		}

		return files;
	}

	vector<FileSystem::FileInfo> FileSystem::getFilesInDirectory(DirectoryInfo const& di, string const& pattern, bool subdirs)
	{
		return getFilesInDirectory(di.getDirectoryPath(), pattern, subdirs);
	}

	bool FileSystem::fileExists(string const& filepath)
	{
		return exists(path(filepath));
	}

	bool FileSystem::fileExists(FileInfo const& fi)
	{
		return fileExists(fi.getFilePath());
	}

	bool FileSystem::directoryExists(string const& dirpath)
	{
		return exists(path(dirpath));
	}

	bool FileSystem::directoryExists(DirectoryInfo const& di)
	{
		return directoryExists(di.getDirectoryPath());
	}

	string FileSystem::concatPaths(string const& path1, string const& path2)
	{
		return standardisePath((path(path1) / path(path2)).string());
	}

	string FileSystem::concatPaths(vector<string> const& paths)
	{
		return standardisePath(accumulate(paths.begin(), paths.end(), string{}, [] (string const& a, string const& b)
		{
			return (a.empty() ? path(a) : path(a) / path(b)).string();
		}));
	}

} // utils
