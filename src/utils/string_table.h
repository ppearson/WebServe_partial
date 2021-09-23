/*
 WebServe
 Copyright 2019 Peter Pearson.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include <vector>
#include <map>

#include "hash.h"

// String Interning implementation, designed for efficient (mainly from a memory perspective) usage
// of many different string values which are immutable after initial creation, of which many may be duplicates.

class StringTable;


// TODO: arguably it's not great having both classes in here in terms of including, but it makes things easier for the
//       moment.
class StringInstance
{
public:
	StringInstance() : m_pStringValue(nullptr),
		m_hashValue(0)
	{

	}

	StringInstance(const char* pStringValue, HashValue hashValue) : m_pStringValue(pStringValue),
		m_hashValue(hashValue)
	{

	}

	bool operator==(const StringInstance& rhs)
	{
		if (m_pStringValue == rhs.m_pStringValue && m_hashValue == rhs.m_hashValue)
			return true;

		return false;
	}

	const char* getRawString() const
	{
		if (!m_pStringValue || m_hashValue == 0)
			return "";

		return m_pStringValue;
	}

	std::string getString() const
	{
		if (!m_pStringValue || m_hashValue == 0)
			return "";

		return std::string(m_pStringValue);
	}

	HashValue getHashValue() const
	{
		return m_hashValue;
	}

	bool isEmpty() const
	{
		return !m_pStringValue && m_hashValue == 0;
	}


protected:
	const char*				m_pStringValue;

	HashValue				m_hashValue;
};

// this is effectively just a slab allocator that owns the contents of the strings...
// Note: it's not thread-safe, and it's only designed to be used to construct strings in a batch
//       process up-front, and then use them later without further addition of strings.
class StringTable
{
public:
	StringTable();
	~StringTable();

	void init(size_t blockSize = 32768);

	StringInstance createString(const std::string& stringValue);

	void freeMem();

protected:
	const char* allocString(const std::string& stringValue);

protected:
	void* alloc(size_t size);

protected:
	std::vector<char*>	m_usedBlocks;

	std::map<HashValue, const char*>	m_aStrings;

	char*				m_pCurrentBlock;

	size_t				m_blockSize;
	size_t				m_currentBlockPos;
};

#endif // STRING_TABLE_H
