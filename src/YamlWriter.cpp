#include <fstream>

#include <yaml-cpp/yaml.h>

#include "YamlWriter.h"
#include "YamlExceptions.h"

namespace utils
{
	using namespace std;

	namespace
	{
		YAML::Node convertValue(StructuredData const& node, map<string, string> const& wrapperItemNames)
		{
			if (node.isValue())
			{
				return YAML::Node(node.getValue());
			}

			// Group children by tag, preserving the order each tag first appears in.
			vector<string> order;
			map<string, vector<StructuredData const*>> groups;

			for (auto const& entry : node)
			{
				if (groups.find(entry.first) == groups.end())
				{
					order.push_back(entry.first);
				}
				groups[entry.first].push_back(&entry.second);
			}

			// Collapse: this node's own name is a known wrapper for exactly the
			// single child tag it contains, so drop the wrapping tag entirely.
			if (order.size() == 1)
			{
				auto wrapper = wrapperItemNames.find(node.getName());
				if (wrapper != wrapperItemNames.end() && wrapper->second == order.front())
				{
					YAML::Node sequence(YAML::NodeType::Sequence);
					for (auto const* item : groups[order.front()])
					{
						sequence.push_back(convertValue(*item, wrapperItemNames));
					}
					return sequence;
				}
			}

			YAML::Node result(YAML::NodeType::Map);
			for (auto const& key : order)
			{
				auto const& items = groups[key];
				if (items.size() > 1)
				{
					YAML::Node sequence(YAML::NodeType::Sequence);
					for (auto const* item : items)
					{
						sequence.push_back(convertValue(*item, wrapperItemNames));
					}
					result[key] = sequence;
				}
				else
				{
					result[key] = convertValue(*items.front(), wrapperItemNames);
				}
			}

			return result;
		}
	}

	YamlWriter::YamlWriter(map<string, string> const& wrapperItemNames)
		: mWrapperItemNames(wrapperItemNames)
	{
	}

	void YamlWriter::writeTree(StructuredData const& root, string const& filepath) const
	{
		YAML::Node document(YAML::NodeType::Map);
		document[root.getName()] = convertValue(root, mWrapperItemNames);

		ofstream out(filepath, ios::binary | ios::trunc);
		if (!out)
		{
			throw YamlException("Could not write YAML file.", filepath);
		}

		out << document;
		if (!out)
		{
			throw YamlException("Could not finish writing YAML file.", filepath);
		}
	}

} // utils
