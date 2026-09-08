#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <fstream>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/adapters/yaml_cpp_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

#include "YamlReader.h"
#include "YamlExceptions.h"

namespace utils
{
	using namespace std;

	namespace
	{
		StructuredData convertValue(YAML::Node const& value, string const& name, map<string, string> const& wrapperItemNames);

		nlohmann::json jsonValue(YAML::Node const& value)
		{
			if (value.IsNull())
				return nullptr;
			if (value.IsSequence())
			{
				auto result = nlohmann::json::array();
				for (auto const& item : value)
					result.push_back(jsonValue(item));
				return result;
			}
			if (value.IsMap())
			{
				auto result = nlohmann::json::object();
				for (auto const& entry : value)
					result[entry.first.as<string>()] = jsonValue(entry.second);
				return result;
			}

			string const scalar = value.as<string>();
			string const tag = value.Tag();
			// yaml-cpp marks quoted/block scalars with the non-specific string tag
			// ('!'). Plain scalars use '?', so resolve their JSON-compatible native
			// type while retaining every quoted value as a string.
			if (tag == "!" || tag == "tag:yaml.org,2002:str")
				return scalar;

			string lower = scalar;
			transform(lower.begin(), lower.end(), lower.begin(),
				[](unsigned char character) { return static_cast<char>(tolower(character)); });
			if (tag == "tag:yaml.org,2002:bool" || lower == "true" || lower == "false" ||
				lower == "yes" || lower == "no" || lower == "on" || lower == "off")
				return lower == "true" || lower == "yes" || lower == "on";

			static regex const integerPattern(R"(^[+-]?[0-9]+$)");
			static regex const numberPattern(
				R"(^[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$)");
			try
			{
				if (tag == "tag:yaml.org,2002:int" || regex_match(scalar, integerPattern))
					return stoll(scalar);
				if (tag == "tag:yaml.org,2002:float" || regex_match(scalar, numberPattern))
					return stod(scalar);
			}
			catch (exception const&)
			{
				// Out-of-range numbers stay strings and are rejected by numeric schemas.
			}
			return scalar;
		}

		void emitCanonical(YAML::Emitter& emitter, YAML::Node const& value)
		{
			if (value.IsNull())
			{
				emitter << YAML::Null;
				return;
			}
			if (value.IsSequence())
			{
				emitter << YAML::BeginSeq;
				for (auto const& item : value)
					emitCanonical(emitter, item);
				emitter << YAML::EndSeq;
				return;
			}
			if (value.IsMap())
			{
				emitter << YAML::BeginMap;
				for (auto const& entry : value)
				{
					emitter << YAML::Key << entry.first.as<string>() << YAML::Value;
					emitCanonical(emitter, entry.second);
				}
				emitter << YAML::EndMap;
				return;
			}

			string const scalar = value.as<string>();
			string const tag = value.Tag();
			if (tag == "!" || tag == "tag:yaml.org,2002:str")
			{
				emitter << YAML::DoubleQuoted << scalar;
				return;
			}

			string lower = scalar;
			transform(lower.begin(), lower.end(), lower.begin(),
				[](unsigned char character) { return static_cast<char>(tolower(character)); });
			if (tag == "tag:yaml.org,2002:bool" || lower == "true" || lower == "false" ||
				lower == "yes" || lower == "no" || lower == "on" || lower == "off")
			{
				emitter << (lower == "true" || lower == "yes" || lower == "on");
				return;
			}

			static regex const integerPattern(R"(^[+-]?[0-9]+$)");
			static regex const numberPattern(
				R"(^[+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$)");
			try
			{
				if (tag == "tag:yaml.org,2002:int" || regex_match(scalar, integerPattern))
				{
					emitter << stoll(scalar);
					return;
				}
				if (tag == "tag:yaml.org,2002:float" || regex_match(scalar, numberPattern))
				{
					string_view numeric = scalar;
					if (numeric.starts_with('+'))
						numeric.remove_prefix(1);
					double number = 0;
					auto const parsed = from_chars(numeric.data(), numeric.data() + numeric.size(),
						number, chars_format::general);
					if (parsed.ec != errc{} || parsed.ptr != numeric.data() + numeric.size() ||
						!isfinite(number))
						throw invalid_argument("Non-finite or malformed YAML number");

					array<char, 64> buffer{};
					auto const formatted = to_chars(buffer.data(), buffer.data() + buffer.size(),
						number, chars_format::general);
					if (formatted.ec != errc{})
						throw runtime_error("Could not format YAML number");
					string canonicalNumber(buffer.data(), formatted.ptr);
					if (canonicalNumber.find_first_of(".eE") == string::npos)
						canonicalNumber += ".0";
					// Passing the numeric token as an untagged scalar preserves its
					// floating-point type; passing a double lets yaml-cpp emit 1.0 as 1.
					emitter << canonicalNumber;
					return;
				}
			}
			catch (exception const&)
			{
				// Out-of-range values retain string semantics and fail numeric schemas.
			}
			emitter << YAML::DoubleQuoted << scalar;
		}

		vector<YAML::Node> loadSingleDocument(string const& text)
		{
			auto documents = YAML::LoadAll(text);
			if (documents.size() != 1)
				throw YamlException("YAML stream must contain exactly one document.", "");
			return documents;
		}

		string unescapeJsonPointerToken(string token)
		{
			for (size_t position = 0; (position = token.find('~', position)) != string::npos;)
			{
				if (position + 1 < token.size() && token[position + 1] == '0')
					token.replace(position, 2, "~");
				else if (position + 1 < token.size() && token[position + 1] == '1')
					token.replace(position, 2, "/");
				++position;
			}
			return token;
		}

		YAML::Node nodeAtJsonPointer(YAML::Node const& root, string const& pointer)
		{
			YAML::Node node = root;
			size_t begin = pointer.empty() ? string::npos : 1;
			while (begin != string::npos && begin <= pointer.size())
			{
				auto const end = pointer.find('/', begin);
				auto const token = unescapeJsonPointerToken(pointer.substr(begin, end - begin));
				if (node.IsMap())
				{
					auto const child = static_cast<YAML::Node const&>(node)[token];
					if (!child)
						return {};
					node.reset(child);
				}
				else if (node.IsSequence())
				{
					try
					{
						auto const child = static_cast<YAML::Node const&>(node)[static_cast<size_t>(stoull(token))];
						if (!child)
							return {};
						node.reset(child);
					}
					catch (exception const&)
					{
						return {};
					}
				}
				else
					return {};
				begin = end == string::npos ? string::npos : end + 1;
			}
			return node;
		}

		void appendChildren(YAML::Node const& mapNode, StructuredData& parent, map<string, string> const& wrapperItemNames)
		{
			for (auto const& kv : mapNode)
			{
				string key = kv.first.as<string>();
				YAML::Node value = kv.second;

				if (value.IsSequence())
				{
					auto wrapper = wrapperItemNames.find(key);
					if (wrapper != wrapperItemNames.end())
					{
						StructuredData node(key);
						for (auto const& item : value)
						{
							node.addEntry(wrapper->second, convertValue(item, wrapper->second, wrapperItemNames));
						}
						parent.addEntry(key, node);
					}
					else
					{
						for (auto const& item : value)
						{
							parent.addEntry(key, convertValue(item, key, wrapperItemNames));
						}
					}
				}
				else
				{
					parent.addEntry(key, convertValue(value, key, wrapperItemNames));
				}
			}
		}

		StructuredData convertValue(YAML::Node const& value, string const& name, map<string, string> const& wrapperItemNames)
		{
			if (value.IsScalar())
			{
				return StructuredData(name, value.as<string>());
			}

			if (value.IsNull())
			{
				return StructuredData(name, string());
			}

			// Sequences reaching here (not unpacked by a wrapper table entry) have
			// no way to recover a per-item tag name, so treat each item as an
			// anonymous occurrence of `name` nested one level deeper.
			StructuredData result(name);

			if (value.IsSequence())
			{
				for (auto const& item : value)
				{
					result.addEntry(name, convertValue(item, name, wrapperItemNames));
				}
				return result;
			}

			appendChildren(value, result, wrapperItemNames);
			return result;
		}
	}

	YamlReader::YamlReader(void* doc, string const& filepath,
		map<string, string> const& wrapperItemNames)
		: mDocument(doc)
		, mFilepath(filepath)
		, mWrapperItemNames(wrapperItemNames)
	{
	}

	YamlReader::~YamlReader()
	{
		delete static_cast<YAML::Node*>(mDocument);
	}

	YamlReader* YamlReader::fromFile(string const& filepath, map<string, string> const& wrapperItemNames)
	{
		try
		{
			ifstream input(filepath, ios::binary);
			if (!input)
				throw YamlException("Could not open YAML file.", filepath);
			ostringstream contents;
			contents << input.rdbuf();
			if (!input && !input.eof())
				throw YamlException("Could not read YAML file.", filepath);
			auto documents = loadSingleDocument(contents.str());
			return new YamlReader(new YAML::Node(std::move(documents.front())), filepath,
				wrapperItemNames);
		}
		catch (YamlException const&)
		{
			throw;
		}
		catch (YAML::Exception const& ex)
		{
			throw YamlException(string("Could not load YAML file: ") + ex.what(), filepath);
		}
	}

	YamlReader* YamlReader::fromString(string const& text, map<string, string> const& wrapperItemNames)
	{
		try
		{
			auto documents = loadSingleDocument(text);
			return new YamlReader(new YAML::Node(std::move(documents.front())), "",
				wrapperItemNames);
		}
		catch (YamlException const&)
		{
			throw;
		}
		catch (YAML::Exception const& ex)
		{
			throw YamlException(string("Could not parse YAML: ") + ex.what(), "");
		}
	}

	vector<JsonSchemaValidationFailure> YamlReader::validateJsonSchema(
		string const& rootSchema,
		vector<JsonSchemaDocument> const& localSchemas,
		size_t maximumFailures) const
	{
		vector<JsonSchemaValidationFailure> failures;
		if (maximumFailures == 0)
			return failures;

		try
		{
			YAML::Node schemaDocument = YAML::Load(rootSchema);
			valijson::adapters::YamlCppAdapter schemaAdapter(schemaDocument);
			valijson::Schema schema;
			valijson::SchemaParser parser(valijson::SchemaParser::kDraft7);

			auto fetchSchema = [&localSchemas](string const& uri) -> YAML::Node const*
			{
				auto const found = find_if(localSchemas.begin(), localSchemas.end(),
					[&uri](JsonSchemaDocument const& candidate) { return candidate.id == uri; });
				if (found == localSchemas.end())
					return nullptr;
				return new YAML::Node(YAML::Load(found->source));
			};
			auto freeSchema = [](YAML::Node const* schemaNode) { delete schemaNode; };
			parser.populateSchema(schemaAdapter, schema, fetchSchema, freeSchema);

			auto const& document = *static_cast<YAML::Node*>(mDocument);
			auto const typedDocument = jsonValue(document);
			valijson::adapters::NlohmannJsonAdapter documentAdapter(typedDocument);
			valijson::ValidationResults results;
			valijson::Validator validator(valijson::Validator::kStrongTypes);
			validator.validate(schema, documentAdapter, &results);

			for (auto const& error : results)
			{
				if (failures.size() == maximumFailures)
					break;
				auto const failedNode = nodeAtJsonPointer(document, error.jsonPointer);
				auto const mark = failedNode ? failedNode.Mark() : YAML::Mark::null_mark();
				failures.push_back({error.jsonPointer, error.description,
					mark.is_null() ? 0 : mark.line + 1,
					mark.is_null() ? 0 : mark.column + 1});
			}
		}
		catch (exception const& error)
		{
			// Schema parsing and unresolved-reference errors are represented using
			// the same library-neutral record rather than leaking Valijson errors.
			failures.push_back({"", string("Schema error: ") + error.what(), 0, 0});
		}

		return failures;
	}

	optional<string> YamlReader::scalarAtJsonPointer(string const& pointer) const
	{
		auto const& doc = *static_cast<YAML::Node*>(mDocument);
		auto const node = nodeAtJsonPointer(doc, pointer);
		if (!node || !node.IsScalar())
			return nullopt;
		try
		{
			return node.as<string>();
		}
		catch (YAML::Exception const&)
		{
			return nullopt;
		}
	}

	YamlDocumentInspection YamlReader::inspect(size_t maximumDepth,
		size_t maximumNodeCount) const
	{
		YamlDocumentInspection result;
		struct PendingNode
		{
			YAML::Node node;
			size_t depth;
		};
		vector<PendingNode> pending{{*static_cast<YAML::Node*>(mDocument), 1}};

		while (!pending.empty())
		{
			auto current = std::move(pending.back());
			pending.pop_back();
			++result.nodeCount;
			result.maximumDepth = max(result.maximumDepth, current.depth);
			if (current.depth > maximumDepth || result.nodeCount > maximumNodeCount)
			{
				result.limitExceeded = true;
				auto const mark = current.node.Mark();
				result.line = mark.is_null() ? 0 : mark.line + 1;
				result.column = mark.is_null() ? 0 : mark.column + 1;
				result.message = current.depth > maximumDepth
					? "YAML nesting depth exceeds the defensive limit of " + to_string(maximumDepth) + "."
					: "YAML node count exceeds the defensive limit of " + to_string(maximumNodeCount) + ".";
				return result;
			}

			if (current.node.IsSequence())
			{
				for (auto const& item : current.node)
					pending.push_back({item, current.depth + 1});
			}
			else if (current.node.IsMap())
			{
				set<string> keys;
				for (auto const& entry : current.node)
				{
					if (!entry.first.IsScalar())
					{
						result.structuralProblem = true;
						result.message = "YAML mapping keys must be scalar strings.";
					}
					else
					{
						auto const key = entry.first.as<string>();
						if (!keys.insert(key).second)
						{
							result.structuralProblem = true;
							result.message = "YAML mapping key '" + key + "' is duplicated.";
						}
					}
					if (result.structuralProblem)
					{
						auto const mark = entry.first.Mark();
						result.line = mark.is_null() ? 0 : mark.line + 1;
						result.column = mark.is_null() ? 0 : mark.column + 1;
						return result;
					}
					pending.push_back({entry.second, current.depth + 1});
					pending.push_back({entry.first, current.depth + 1});
				}
			}
		}
		return result;
	}

	string YamlReader::canonicalYaml() const
	{
		YAML::Emitter emitter;
		emitter.SetIndent(2);
		emitter.SetMapFormat(YAML::Block);
		emitter.SetSeqFormat(YAML::Block);
		emitCanonical(emitter, *static_cast<YAML::Node*>(mDocument));
		if (!emitter.good())
			throw YamlException(string("Could not serialize YAML: ") + emitter.GetLastError(), mFilepath);
		return string(emitter.c_str()) + '\n';
	}

	StructuredData YamlReader::readTree() const
	{
		auto const& doc = *static_cast<YAML::Node*>(mDocument);

		if (!doc.IsMap() || doc.size() != 1)
		{
			throw YamlException("YAML document must have exactly one top-level key naming the root element.", mFilepath);
		}

		auto it = doc.begin();
		string rootName = it->first.as<string>();

		return convertValue(it->second, rootName, mWrapperItemNames);
	}

} // utils
