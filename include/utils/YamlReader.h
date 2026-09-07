#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Platform.h"

#include "StructuredData.h"

namespace utils
{

	// Library-neutral description of one schema document. Catalog keys are
	// absolute schema IDs; sources contain JSON Schema text held in memory.
	struct JsonSchemaDocument
	{
		std::string id;
		std::string source;
	};

	struct JsonSchemaValidationFailure
	{
		std::string instancePath;
		std::string message;
		int line = 0;
		int column = 0;
	};

	// Reads a YAML document into the same StructuredData shape utils::XmlReader
	// produces from XML: a document with a single root key, whose descendants
	// are either scalars or named children (repeated child names round-trip as
	// repeated entries, exactly as with XML elements).
	//
	// YAML mapping keys are unique, so a run of repeated element-equivalents has
	// to be written as a sequence. By default a sequence found under key K is
	// unpacked back into repeated entries named K (the "per-tag grouping"
	// scheme). If the caller knows a given wrapper key K stands in for a single
	// child tag (e.g. K="Buffer" wrapping items really named "Channel"), passing
	// wrapperItemNames[K]="Channel" makes the reader reconstruct that wrapper
	// node instead of flattening it. This table is entirely optional: leave it
	// empty to just get generic, schema-agnostic YAML<->StructuredData conversion.
	class UTILS_API YamlReader
	{
		void* mDocument;

		std::string mFilepath;

		std::map<std::string, std::string> mWrapperItemNames;

	private:

		YamlReader(void* doc, std::string const& filepath, std::map<std::string, std::string> const& wrapperItemNames);

	public:

		~YamlReader();

		static YamlReader* fromFile(std::string const& filepath, std::map<std::string, std::string> const& wrapperItemNames = {});

		static YamlReader* fromString(std::string const& text, std::map<std::string, std::string> const& wrapperItemNames = {});

		// Validates the parser-owned YAML tree directly. External references are
		// resolved exclusively from localSchemas; no filesystem or network
		// resolver is installed. The returned records contain no validator or
		// YAML implementation types.
		std::vector<JsonSchemaValidationFailure> validateJsonSchema(
			std::string const& rootSchema,
			std::vector<JsonSchemaDocument> const& localSchemas = {},
			std::size_t maximumFailures = 100) const;

		StructuredData readTree() const;
	};

} // utils
