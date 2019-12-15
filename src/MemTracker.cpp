#include "MemTracker.h"
#include "StringUtils.h"

namespace utils
{
	using namespace std;

	map<uint64_t, MemTracker::Entry> MemTracker::msEntries;

	unsigned int MemTracker::msId = 0;

} // utils