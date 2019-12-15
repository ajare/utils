#include "XmlReader.h"
#include "FileSystem.h"
#include "StringUtils.h"
#include "tinyxml2.h"

namespace utils
{
	using namespace std;

	XmlNode::XmlNode(void* node, string const& xpath, string const& name, string const& filepath)
		: mNode(node)
		, mXpath(xpath)
		, mName(name)
		, mFilepath(filepath)
	{
	}

	XmlNode::~XmlNode()
	{
		for (auto node: mChildren)
		{
			delete node;
		}
	}

	bool XmlNode::next()
	{
		mNode = static_cast<tinyxml2::XMLNode*>(mNode)->NextSiblingElement(mName.c_str());
		return mNode != nullptr;
	}

	string const& XmlNode::getPath() const
	{
		return mXpath;
	}

	string XmlNode::getValue() const
	{
		const char* text = static_cast<tinyxml2::XMLElement*>(mNode)->GetText();
		if (text)
		{
			return string(text);
		}
		else
		{
			return string();
		}
	}

	XmlNode* XmlNode::getChild(string const& child) const
	{
		string xpath = mXpath + "/" + child;

		auto childNode = static_cast<tinyxml2::XMLNode*>(mNode)->FirstChildElement(child.c_str());
		if (!childNode)
		{
			string errMsg = mFilepath == "" ? "" : mFilepath + ": ";
			errMsg += "invalid path: " + xpath;

			throw exception(errMsg.c_str());
		}

		auto node = new XmlNode(childNode, xpath, child, mFilepath);
		mChildren.push_back(node);

		return node;
	}

	XmlNode* XmlNode::getOptionalChild(string const& child) const
	{
		string xpath = mXpath + "/" + child;

		auto childNode = static_cast<tinyxml2::XMLNode*>(mNode)->FirstChildElement(child.c_str());
		if (!childNode)
		{
			return nullptr;
		}

		auto node = new XmlNode(childNode, xpath, child, mFilepath);
		mChildren.push_back(node);

		return node;
	}

	string XmlNode::getAttribute(string const& attrib) const
	{
		char const* attr = static_cast<tinyxml2::XMLElement*>(mNode)->Attribute(attrib.c_str());
		if (attr)
		{
			return string(attr);
		}
		else
		{
			string errMsg = mFilepath == "" ? "" : mFilepath + ": ";
			errMsg += "could not find required attribute '" + attrib + "' at " + mXpath;

			throw exception(errMsg.c_str());
		}
	}

	bool XmlNode::getOptionalAttribute(string const& attrib, string& value) const
	{
		char const* attr = static_cast<tinyxml2::XMLElement*>(mNode)->Attribute(attrib.c_str());
		if (attr)
		{
			value = string(attr);
			return true;
		}
		else
		{
			return false;
		}
	}

	string XmlNode::getAsText() const
	{
		tinyxml2::XMLPrinter printer;
		static_cast<tinyxml2::XMLElement*>(mNode)->Accept(&printer);

		return printer.CStr();
	}

	XmlReader::XmlReader(void* doc, string const& filepath)
		: mDocument(static_cast<tinyxml2::XMLDocument*>(doc))
		, mFilepath(filepath)
	{
		mRootNode = static_cast<tinyxml2::XMLDocument*>(mDocument)->RootElement();
		mRootNodeName = static_cast<tinyxml2::XMLElement*>(mRootNode)->Name();
	}

	XmlReader::~XmlReader()
	{
		delete (tinyxml2::XMLDocument*)mDocument;

		for (auto node: mNodes)
		{
			delete node;
		}
	}

	XmlReader* XmlReader::fromFile(string const& filepath)
	{
		tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument();
		tinyxml2::XMLError err = doc->LoadFile(filepath.c_str());

		if (err != tinyxml2::XMLError::XML_SUCCESS)
		{
			string errMsg = getErrorMessage(err, true, filepath);
			throw exception(errMsg.c_str());
		}


		return new XmlReader(doc, filepath);
	}

	XmlReader* XmlReader::fromString(string const& text)
	{
		tinyxml2::XMLDocument* doc = new tinyxml2::XMLDocument();
		tinyxml2::XMLError err = doc->Parse(text.c_str());

		if (err != tinyxml2::XMLError::XML_SUCCESS)
		{
			string errMsg = getErrorMessage(err, false, "");
			throw exception(errMsg.c_str());
		}

		return new XmlReader(doc, "");
	}

	string XmlReader::getErrorMessage(int errorCode, bool loadNotParse, string const& filepath)
	{
		string errMsg = "";

		switch (errorCode)
		{
		case tinyxml2::XMLError::XML_ERROR_FILE_NOT_FOUND:
		case tinyxml2::XMLError::XML_ERROR_FILE_COULD_NOT_BE_OPENED:
		case tinyxml2::XMLError::XML_ERROR_FILE_READ_ERROR:
			errMsg = "Could not load '" + filepath + "'.  The file could either not be found, or could not be opened.";
			break;
		case tinyxml2::XMLError::XML_ERROR_ELEMENT_MISMATCH:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Element mismatch found.";
			}
			else
			{
				errMsg = "Could not parse input. Element mismatch found.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_PARSING_ELEMENT:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Element could not be parsed.";
			}
			else
			{
				errMsg = "Could not parse input. Element could not be parsed.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_PARSING_ATTRIBUTE:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Attribute could not be parsed.";
			}
			else
			{
				errMsg = "Could not parse input. Attribute could not be parsed.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_IDENTIFYING_TAG:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Tag could not be identified.";
			}
			else
			{
				errMsg = "Could not parse input. Tag could not be identified.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_PARSING_TEXT:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Text could not be parsed.";
			}
			else
			{
				errMsg = "Could not parse input. Text could not be parsed.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_PARSING_CDATA:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  CDATA could not be parsed.";
			}
			else
			{
				errMsg = "Could not parse input. CDATA could not be parsed.";
			}
			break;

		case tinyxml2::XMLError::XML_ERROR_PARSING_COMMENT:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Comment could not be parsed.";
			}
			else
			{
				errMsg = "Could not parse input. Comment could not be parsed.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_PARSING_DECLARATION:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Declaration could not be parsed.";
			}
			else
			{
				errMsg = "Could not parse input. Declaration could not be parsed.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_PARSING_UNKNOWN:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Unknown object found.";
			}
			else
			{
				errMsg = "Could not parse input. Unknown object found.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_EMPTY_DOCUMENT:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Empty document.";
			}
			else
			{
				errMsg = "Could not parse input. Empty document.";
			}
			break;
		case tinyxml2::XMLError::XML_ERROR_MISMATCHED_ELEMENT:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Element mismatch found.";
			}
			else
			{
				errMsg = "Could not parse input. Element mismatch found.";
			}
			break;

		case tinyxml2::XMLError::XML_SUCCESS:
			break;

		case tinyxml2::XMLError::XML_ERROR_PARSING:
		default:
			if (loadNotParse)
			{
				errMsg = "Could not load '" + filepath + "'.  Error(s) parsing XML.";
			}
			else
			{
				errMsg = "Could not parse input.";
			}
			break;
		}

		return errMsg;
	}

	XmlNode* XmlReader::getNode(string const& path)
	{
		string xpath = FileSystem::standardisePath(path);
		vector<string> nodes = StringUtils::split(xpath, "/");
		
		tinyxml2::XMLNode* cur = static_cast<tinyxml2::XMLNode*>(mRootNode);

		// Check name
		if (nodes.front() != mRootNodeName)
		{
			string errMsg = mFilepath == "" ? "" : mFilepath + ": ";
			errMsg += "invalid path: " + xpath;

			throw exception(xpath.c_str());
		}

		auto it = nodes.begin(); ++it;
		while (it != nodes.end())
		{
			auto node = *it;
			cur = static_cast<tinyxml2::XMLNode*>(cur->FirstChildElement(node.c_str()));
			if (!cur)
			{
				string errMsg = mFilepath == "" ? "" : mFilepath + ": ";
				errMsg += "invalid path: " + xpath;

				throw exception(xpath.c_str());
			}

			++it;
		}

		auto node = new XmlNode(cur, path, nodes.back(), mFilepath);
		mNodes.push_back(node);

		return node;
	}

} // utils