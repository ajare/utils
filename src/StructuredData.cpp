#include <algorithm>
#include <stdexcept>

#include "StructuredData.h"

namespace utils
{

	using namespace std;

	StructuredData::StructuredData(string const& name)
		: mName(name)
		, mIsValue(false)
	{
	}

	StructuredData::StructuredData(string const& name, string const& value)
		: mName(name)
		, mIsValue(true)
		, mValue(value)
	{
	}

	string const& StructuredData::getName() const
	{
		return mName;
	}

	bool StructuredData::isValue() const
	{
		return mIsValue;
	}

	void StructuredData::setValue(string const& value)
	{
		mIsValue = true;
		mValue = value;
	}

	string const& StructuredData::getValue() const
	{
		return mValue;
	}

	void StructuredData::addEntry(string const& key, string const& value)
	{
		auto entry = make_pair(key, StructuredData(key, value));
		mEntries.push_back(entry);
	}

	void StructuredData::addEntry(string const& key, StructuredData const& value)
	{
		auto entry = make_pair(key, value);
		mEntries.push_back(entry);
	}

	StructuredData const& StructuredData::getEntry(string const& key) const
	{
		auto it = find_if(mEntries.begin(), mEntries.end(), [key](auto const& entry) { return entry.first == key; });

		if (it == mEntries.end())
		{
			string errMsg = "Could not find entry '" + key + "'.";
			throw runtime_error(errMsg);
		}

		return (*it).second;
	}

	StructuredData& StructuredData::getEntry(string const& key)
	{
		auto it=find_if(mEntries.begin(),mEntries.end(),[&](auto const& entry){return entry.first==key;});if(it==mEntries.end())throw runtime_error("Could not find entry '"+key+"'.");return it->second;
	}

	void StructuredData::setEntryValue(string const& key,string const& value)
	{
		auto it=find_if(mEntries.begin(),mEntries.end(),[&](auto const& entry){return entry.first==key;});if(it==mEntries.end())addEntry(key,value);else it->second.setValue(value);
	}

	bool StructuredData::hasEntry(string const& key) const
	{
		auto it = find_if(mEntries.begin(), mEntries.end(), [key](auto const& entry) { return entry.first == key; });

		return it != mEntries.end();
	}

	vector<StructuredData::Entry>::iterator StructuredData::begin(){return mEntries.begin();}
	vector<StructuredData::Entry>::iterator StructuredData::end(){return mEntries.end();}

	vector<StructuredData::Entry>::const_iterator StructuredData::begin() const
	{
		return mEntries.begin();
	}

	vector<StructuredData::Entry>::const_iterator StructuredData::end() const
	{
		return mEntries.end();
	}
}