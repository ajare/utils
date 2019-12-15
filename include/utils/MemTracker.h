#pragma once

#include <string>
#include <map>

#include "Platform.h"

namespace utils
{

	class MemTracker
	{
		struct Entry
		{
			uint64_t address;
			unsigned int id;

			std::string file, function;
			int line;
			bool released;
		};

		static unsigned int msId;

		static std::map<uint64_t, Entry> msEntries;

	public:

		template<typename T>
		static T* track(T* mem, char const* file, int line, char const* function)
		{
			Entry entry;
			entry.address = reinterpret_cast<uint64_t>(mem);
			entry.id = msId++;
			entry.file = file;
			entry.line = line;
			entry.function = function;
			entry.released = false;

			msEntries[entry.address] = entry;

			return mem;
		}

		template<typename T>
		static void untrack(T* mem)
		{
			uint64_t address = reinterpret_cast<uint64_t>(mem);
			auto& it = msEntries.find(address);
			if (it == msEntries.end())
			{
				throw std::exception("Memory was not tracked!");
			}

			it->second.released = true;
		}

		static void dump(std::string const& file)
		{
			FILE* fp;
			fopen_s(&fp, file.c_str(), "wt");

			for (auto it: msEntries)
			{
				auto const& entry = it.second;
				fprintf(fp, "%d: 0x%I64x %s:%d %s%s\n", 
					entry.id, 
					entry.address, 
					entry.file.c_str(), 
					entry.line, 
					entry.function.c_str(), 
					entry.released ? " [RELEASED]" : "");
			}

			fclose(fp);
		}

	};

} // utils