#pragma once

#include <string>
#include <vector>

#include "Platform.h"
#include "StringUtils.h"

namespace utils
{

	class UTILS_API XmlWriteNode
	{
		friend class XmlWriter;

	private:

		void* mDocument;

		void* mNode;

		std::vector<XmlWriteNode*> mChildren;

	private:

		XmlWriteNode(std::string const& name, void* parent, void* document);

	public:

		~XmlWriteNode();

		XmlWriteNode* createChild(std::string const& name);

		void setValue(std::string const& value);

		void setValue(int value);

		void setValue(unsigned int value);

		void setValue(float value);

		void setValue(bool value);

		void addAttribute(std::string const& name, std::string const& value);

		void addAttribute(std::string const& name, char const* value);

		void addAttribute(std::string const& name, int value);

		void addAttribute(std::string const& name, unsigned int value);

		void addAttribute(std::string const& name, float value);

		void addAttribute(std::string const& name, bool value);
	};

	class UTILS_API XmlWriter
	{
		void* mDocument;

		XmlWriteNode* mRootNode;

	public:

		explicit XmlWriter(std::string const& rootName);

		~XmlWriter();

		XmlWriteNode* getRootNode();

		void write(std::string const& filepath);
	};

} // utils