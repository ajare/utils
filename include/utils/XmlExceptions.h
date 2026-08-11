#pragma once

#include <string>
#include <stdexcept>

namespace utils
{
		
	class XmlException : public std::runtime_error
	{
	public:

		XmlException(std::string const& msg, std::string const& source)
			: std::runtime_error(source.empty() ? msg : source + ": " + msg)
		{
		}
	};

	class XmlPathException : public XmlException
	{
	public:

		XmlPathException(std::string const& path, std::string const& source)
			: XmlException("Path does not exist: " + path, source)
		{
		}
	};

} //  utils