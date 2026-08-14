#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

#include "YamlReader.h"
#include "YamlExceptions.h"

namespace utils
{
	using namespace std;

	namespace
	{
		StructuredData convertValue(YAML::Node const& value, string const& name, map<string, string> const& wrapperItemNames);

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

	YamlReader::YamlReader(void* doc, string const& filepath, map<string, string> const& wrapperItemNames)
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
		YAML::Node* doc = nullptr;

		try
		{
			doc = new YAML::Node(YAML::LoadFile(filepath));
		}
		catch (YAML::Exception const& ex)
		{
			throw YamlException(string("Could not load YAML file: ") + ex.what(), filepath);
		}

		return new YamlReader(doc, filepath, wrapperItemNames);
	}

	YamlReader* YamlReader::fromString(string const& text, map<string, string> const& wrapperItemNames)
	{
		YAML::Node* doc = nullptr;

		try
		{
			doc = new YAML::Node(YAML::Load(text));
		}
		catch (YAML::Exception const& ex)
		{
			throw YamlException(string("Could not parse YAML: ") + ex.what(), "");
		}

		return new YamlReader(doc, "", wrapperItemNames);
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
