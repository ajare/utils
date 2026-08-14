#pragma once

#include <string>
#include <map>

#include "Platform.h"

#include "StructuredData.h"

namespace utils
{

	// Writes a StructuredData tree out as YAML, as the inverse of YamlReader.
	// A node whose own name is a key in wrapperItemNames, and whose children
	// all share the single tag named by that entry, is collapsed to a bare
	// sequence (dropping the repeated child tag). Every other node with
	// repeated child tags is written as `Tag: [ ... ]` in place. Leave
	// wrapperItemNames empty for plain, schema-agnostic conversion.
	//
	// Scalars are written unquoted/plain wherever YAML allows it, matching how
	// values already round-trip as text through the rest of StructuredData.
	class UTILS_API YamlWriter
	{
		std::map<std::string, std::string> mWrapperItemNames;

	public:

		explicit YamlWriter(std::map<std::string, std::string> const& wrapperItemNames = {});

		void writeTree(StructuredData const& root, std::string const& filepath) const;
	};

} // utils
