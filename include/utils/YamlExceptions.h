#pragma once

#include <string>
#include <stdexcept>

namespace utils
{

	class YamlException : public std::runtime_error
	{
	public:

		YamlException(std::string const& msg, std::string const& source)
			: std::runtime_error(source.empty() ? msg : source + ": " + msg)
		{
		}
	};

} //  utils
