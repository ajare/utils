#include <format>

#include "XmlWriter.h"
#include "StringUtils.h"
#include "tinyxml2.h"

namespace utils
{
	using namespace std;

	XmlWriteNode::XmlWriteNode(string const& name, void* parent, void* document)
		: mNode(nullptr)
		, mDocument(document)
	{
		mNode = static_cast<tinyxml2::XMLDocument*>(mDocument)->NewElement(name.c_str());
		auto child = static_cast<tinyxml2::XMLElement*>(mNode);

		if (parent)
		{
			auto p = static_cast<tinyxml2::XMLElement*>(static_cast<XmlWriteNode*>(parent)->mNode);
			p->InsertEndChild(child);
		}
		else
		{
			auto p = static_cast<tinyxml2::XMLDocument*>(mDocument);
			p->InsertEndChild(child);
		}
	}

	XmlWriteNode::~XmlWriteNode()
	{
		for (auto node: mChildren)
		{
			delete node;
		}
	}

	XmlWriteNode* XmlWriteNode::createChild(string const& name)
	{
		auto node = new XmlWriteNode(name, this, mDocument);
		mChildren.push_back(node);
		return node;
	}

	void XmlWriteNode::setValue(string const& value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetText(value.c_str());
	}

	void XmlWriteNode::setValue(int value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetText(std::format("{}", value).c_str());
	}

	void XmlWriteNode::setValue(unsigned int value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetText(std::format("{}", value).c_str());
	}

	void XmlWriteNode::setValue(float value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetText(std::format("{}", value).c_str());
	}

	void XmlWriteNode::setValue(bool value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetText(std::format("{}", value).c_str());
	}

	void XmlWriteNode::addAttribute(string const& name, string const& value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetAttribute(name.c_str(), value.c_str());
	}

	void XmlWriteNode::addAttribute(std::string const& name, char const* value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetAttribute(name.c_str(), value);
	}

	void XmlWriteNode::addAttribute(string const& name, int value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetAttribute(name.c_str(), std::format("{}", value).c_str());
	}

	void XmlWriteNode::addAttribute(string const& name, unsigned int value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetAttribute(name.c_str(), std::format("{}", value).c_str());
	}

	void XmlWriteNode::addAttribute(string const& name, float value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetAttribute(name.c_str(), std::format("{}", value).c_str());
	}

	void XmlWriteNode::addAttribute(string const& name, bool value)
	{
		static_cast<tinyxml2::XMLElement*>(mNode)->SetAttribute(name.c_str(), std::format("{}", value).c_str());
	}

	XmlWriter::XmlWriter(string const& rootName)
	{
		mDocument = new tinyxml2::XMLDocument();
		auto doc = static_cast<tinyxml2::XMLDocument*>(mDocument);

		auto decl = doc->NewDeclaration();
		doc->LinkEndChild(decl);

		mRootNode = new XmlWriteNode(rootName, nullptr, mDocument);
	}

	XmlWriter::~XmlWriter()
	{
		auto doc = static_cast<tinyxml2::XMLDocument*>(mDocument);
		delete doc;

		delete mRootNode;
	}

	XmlWriteNode* XmlWriter::getRootNode()
	{
		return mRootNode;
	}

	void XmlWriter::write(std::string const& filepath)
	{
		auto doc = static_cast<tinyxml2::XMLDocument*>(mDocument);
		doc->SaveFile(filepath.c_str());
	}

	namespace
	{
		void appendChildren(StructuredData const& node, XmlWriteNode* element)
		{
			if (node.isValue())
			{
				element->setValue(node.getValue());
				return;
			}

			for (auto const& entry : node)
			{
				appendChildren(entry.second, element->createChild(entry.first));
			}
		}
	}

	void XmlWriter::writeTree(StructuredData const& root, string const& filepath)
	{
		XmlWriter writer(root.getName());
		appendChildren(root, writer.getRootNode());
		writer.write(filepath);
	}

} // utils